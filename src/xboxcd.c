/*
 * Xbox 360 ATAPI diagnostic baseline.
 *
 * The register layout/packet mechanism follows the Free60 XeLL ATAPI
 * implementation. This baseline intentionally uses bounded polling and
 * isolates the transport so a real DMA implementation can replace it later.
 */

#include "xboxcd.h"
#include "xboxcd_log.h"

#include <stdint.h>
#include <string.h>

#define ATAPI_BASE 0x80000200ea001200ULL

#define REG_DATA       0
#define REG_ERROR      1
#define REG_FEATURES   1
#define REG_BCNTL      4
#define REG_BCNTH      5
#define REG_DRIVESEL   6
#define REG_COMMAND    7
#define REG_STATUS     7

#define ATA_SR_ERR 0x01
#define ATA_SR_DRQ 0x08
#define ATA_SR_BSY 0x80
#define ATA_SR_RDY 0x40

#define ATA_CMD_PACKET 0xA0

static inline void out8(unsigned reg, uint8_t v)
{
    *(volatile uint8_t *)(ATAPI_BASE + reg) = v;
}

static inline void out32(unsigned reg, uint32_t v)
{
    *(volatile uint32_t *)(ATAPI_BASE + reg) = v;
}

static inline uint8_t in8(unsigned reg)
{
    return *(volatile uint8_t *)(ATAPI_BASE + reg);
}

static inline uint32_t in32(unsigned reg)
{
    return *(volatile uint32_t *)(ATAPI_BASE + reg);
}

static int wait_ready(unsigned long loops)
{
    while (loops--) {
        uint8_t s = in8(REG_STATUS);

        if (s == 0xFF)
            return XBOXCD_IO_ERROR;

        if (!(s & ATA_SR_BSY) && (s & ATA_SR_RDY))
            return XBOXCD_OK;
    }

    return XBOXCD_TIMEOUT;
}

static int wait_drq(unsigned long loops)
{
    while (loops--) {
        uint8_t s = in8(REG_STATUS);

        if (s & ATA_SR_ERR)
            return XBOXCD_IO_ERROR;

        if (!(s & ATA_SR_BSY) && (s & ATA_SR_DRQ))
            return XBOXCD_OK;
    }

    return XBOXCD_TIMEOUT;
}

/*
 * Monta um uint32_t a partir de 4 bytes consecutivos de um buffer, sem
 * depender de um load desalinhado do hardware nem da endianness do host.
 * O CDB é transmitido byte a byte na ordem cdb[0..3], cdb[4..7], cdb[8..11],
 * então construímos a word explicitamente em vez de fazer
 * "*(uint32_t*)(cdb+N)", que:
 *   (a) viola o alinhamento natural de 4 bytes exigido por lwz/stw em
 *       PowerPC quando cdb é um array de uint8_t (alinhamento 1) na pilha;
 *   (b) é comportamento indefinido em C (violação de aliasing estrito).
 */
static inline uint32_t bytes_to_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  |  (uint32_t)p[3];
}

/*
 * PIO PACKET transport.
 *
 * data_len must be even. The caller supplies a CDB of 12 bytes.
 *
 * NOTA IMPORTANTE (não verificada em hardware real):
 * A ordem de bytes exata esperada pelo registrador de dados ATAPI do
 * controlador do Xbox 360 (isto é, se o primeiro byte do CDB deve cair no
 * byte mais significativo ou menos significativo da word de 32 bits escrita
 * em REG_DATA) segue aqui a mesma convenção adotada no baseline PIO do XeLL
 * antigo (big-endian, MSB primeiro). Isso NÃO foi validado neste ambiente
 * porque não há hardware disponível para teste. Se o log mostrar
 * TEST UNIT READY/READ TOC falhando de forma consistente com CHECK
 * CONDITION/ATA_SR_ERR já na primeira transação, este é o primeiro lugar a
 * inspecionar - pode ser necessário inverter para bytes_to_le32-equivalente.
 */
static int atapi_packet(const uint8_t cdb[12],
                        uint8_t *data,
                        unsigned data_len,
                        uint8_t *status_out,
                        uint8_t *error_out)
{
    unsigned words = data_len / 2;
    int rc;

    out8(REG_DRIVESEL, 0xA0);
    out8(REG_BCNTL, (uint8_t)(data_len & 0xFF));
    out8(REG_BCNTH, (uint8_t)(data_len >> 8));
    out8(REG_FEATURES, 0);
    out8(REG_COMMAND, ATA_CMD_PACKET);

    rc = wait_ready(5000000UL);
    if (rc)
        goto done;

    if (!(in8(REG_STATUS) & ATA_SR_DRQ)) {
        rc = XBOXCD_IO_ERROR;
        goto done;
    }

    out32(REG_DATA, bytes_to_be32(cdb + 0));
    out32(REG_DATA, bytes_to_be32(cdb + 4));
    out32(REG_DATA, bytes_to_be32(cdb + 8));

    while (words) {
        uint32_t v;

        rc = wait_drq(5000000UL);
        if (rc)
            goto done;

        v = in32(REG_DATA);

        if (words >= 2) {
            /* Grava byte a byte: 'data' pode não estar 4-byte aligned. */
            data[0] = (uint8_t)(v >> 24);
            data[1] = (uint8_t)(v >> 16);
            data[2] = (uint8_t)(v >> 8);
            data[3] = (uint8_t)v;
            data += 4;
            words -= 2;
        } else {
            /*
             * Equivalente exato ao "*(uint16_t*)data = (uint16_t)v" original:
             * usa os 16 bits BAIXOS de v (não os altos), gravados em ordem
             * big-endian (byte mais significativo primeiro), que é como o
             * PowerPC gravaria um uint16_t nativamente. Só a forma de
             * escrita mudou (byte a byte), não o valor final.
             */
            data[0] = (uint8_t)((v & 0xFFFFu) >> 8);
            data[1] = (uint8_t)(v & 0xFFu);
            words--;
        }
    }

    rc = (in8(REG_STATUS) & ATA_SR_ERR) ? XBOXCD_IO_ERROR : XBOXCD_OK;

done:
    if (status_out) *status_out = in8(REG_STATUS);
    if (error_out)  *error_out = in8(REG_ERROR);

    return rc;
}

int xboxcd_init(void)
{
    log_line("[INIT] ATAPI_BASE=0x%016llX",
             (unsigned long long)ATAPI_BASE);
    log_line("[INIT] transport=PIO_BASELINE");
    log_line("[INIT] cache=OFF");
    log_line("[INIT] read_ahead=OFF");
    return XBOXCD_OK;
}

int xboxcd_test_unit_ready(xboxcd_result_t *r)
{
    uint8_t cdb[12] = {0};
    uint8_t status = 0, error = 0;
    int rc;

    cdb[0] = 0x00;

    rc = atapi_packet(cdb, 0, 0, &status, &error);

    if (r) {
        r->status = status;
        r->error = error;
        r->result = rc;
    }

    log_line("[TUR] rc=%d status=0x%02X error=0x%02X",
             rc, status, error);

    return rc;
}

int xboxcd_request_sense(uint8_t sense[24], xboxcd_result_t *r)
{
    uint8_t cdb[12] = {0};
    uint8_t status = 0, error = 0;
    int rc;

    cdb[0] = 0x03;
    cdb[4] = 24;

    memset(sense, 0, 24);
    rc = atapi_packet(cdb, sense, 24, &status, &error);

    if (r) {
        r->status = status;
        r->error = error;
        r->result = rc;
    }

    log_line("[SENSE] rc=%d status=0x%02X error=0x%02X",
             rc, status, error);

    if (!rc) {
        log_line("[SENSE] key=0x%02X asc=0x%02X ascq=0x%02X",
                 sense[2] & 0x0F, sense[12], sense[13]);
        log_hex("[SENSE] raw=", sense, 24);
    }

    return rc;
}

int xboxcd_read_toc(uint8_t *toc, unsigned len, xboxcd_result_t *r)
{
    uint8_t cdb[12] = {0};
    uint8_t status = 0, error = 0;
    int rc;

    cdb[0] = 0x43;
    cdb[6] = 0x01;
    cdb[7] = (uint8_t)(len >> 8);
    cdb[8] = (uint8_t)len;

    memset(toc, 0, len);
    rc = atapi_packet(cdb, toc, len, &status, &error);

    if (r) {
        r->status = status;
        r->error = error;
        r->result = rc;
    }

    log_line("[TOC] rc=%d status=0x%02X error=0x%02X",
             rc, status, error);

    if (!rc)
        log_hex("[TOC] raw=", toc, len > 64 ? 64 : len);

    return rc;
}

int xboxcd_read_raw2352(uint32_t lba, uint8_t *out, xboxcd_result_t *r)
{
    uint8_t cdb[12] = {0};
    uint8_t status = 0, error = 0;
    int rc;

    /*
     * MMC READ CD (BE), one logical sector.
     * This command is retained as the RAW2352 validation target.
     */
    cdb[0] = 0xBE;
    cdb[2] = (uint8_t)(lba >> 24);
    cdb[3] = (uint8_t)(lba >> 16);
    cdb[4] = (uint8_t)(lba >> 8);
    cdb[5] = (uint8_t)lba;
    cdb[8] = 1;

    /*
     * Main Channel Selection (CDB byte 9) para RAW completo de 2352 bytes:
     *   bit7 Sync=1, bits6:5 Header Code=11 (Header+Sub-header),
     *   bit4 User Data=1, bit3 EDC/ECC=1, bits2:1 C2=00, bit0 reservado=0
     *   => 0b11111000 = 0xF8
     * Este valor é o mesmo usado por libburn (mmc_read_cd, req=0xF8) e por
     * outras implementações MMC maduras para "raw 2352" genérico, então foi
     * mantido (não é um valor inventado nem apenas copiado sem checagem).
     * Ressalva ainda válida: em mídia sem sub-header (CD-ROM Mode 1, que é
     * o caso mais provável de um disco PC Engine CD/TurboGrafx-CD), alguns
     * drives podem devolver os 8 bytes de sub-header zerados em vez de dar
     * CHECK CONDITION; outros podem ser mais estritos. Isso só pode ser
     * confirmado com o drive físico do Xbox 360 e fica registrado no log
     * (status/error/sense) de cada READ CD para análise.
     */
    cdb[9] = 0xF8;

    memset(out, 0, 2352);
    rc = atapi_packet(cdb, out, 2352, &status, &error);

    if (r) {
        r->status = status;
        r->error = error;
        r->result = rc;
    }

    log_line("[READ2352] lba=%u rc=%d status=0x%02X error=0x%02X",
             lba, rc, status, error);

    if (!rc)
        log_hex("[READ2352] first64=", out, 64);

    return rc;
}
