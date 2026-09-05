#include "xboxcd.h"
#include "xboxcd_log.h"

#include <stdint.h>
#include <string.h>

/*
 * CORREÇÃO: estas inicializações estavam ausentes desde a versão original
 * do projeto (não foram adicionadas por mim nem pelo DeepSeek — já
 * faltavam no ZIP inicial). Sem elas:
 *   - xenos_init()+console_init(): nenhum printf/log_line aparece na tela
 *     (a tela continua mostrando o último texto desenhado pelo XeLL/loader,
 *     parecendo "travada" mesmo que o programa esteja rodando).
 *   - xenon_ata_init()+xenon_atapi_init(): o controlador ATA/ATAPI não é
 *     inicializado (poder/clock/reset) antes do nosso código de baixo nível
 *     tentar escrever direto nos registradores MMIO — acessar um
 *     controlador não inicializado é uma causa provável de travamento.
 * Todas as quatro funções fazem parte da própria libxenon.a (já linkada
 * via -lxenon no Makefile), confirmado olhando o Makefile de build da
 * libxenon (LIBOBJS inclui console.o, ata.o, xenos.o).
 *
 * NÃO incluí fatInitDefault()/<libfat/fat.h> aqui de propósito: essa é uma
 * biblioteca SEPARADA da libxenon.a, e não há confirmação de que ela foi
 * construída no seu ambiente (o build-xenon-toolchain tem um passo "libs"
 * distinto de "libxenon" que talvez não tenha rodado). Se o log continuar
 * não sendo gravado depois desta correção, é o próximo passo a investigar
 * — mas não queria trocar "trava no console" por "erro de link" agora.
 */
#include <xenos/xenos.h>
#include <console/console.h>
#include <diskio/ata.h>

static void probe(uint32_t lba)
{
    static uint8_t sector[2352] __attribute__((aligned(32)));
    xboxcd_result_t r;
    static const uint8_t sync[12] =
        {0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00};

    memset(&r, 0, sizeof(r));

    if (xboxcd_read_raw2352(lba, sector, &r) == XBOXCD_OK) {
        log_line("[VERIFY] LBA=%u sync=%s mode=%u",
                 lba,
                 memcmp(sector, sync, 12) == 0 ? "OK" : "MISMATCH",
                 sector[15]);
    }
}

int main(void)
{
    xboxcd_result_t r;
    static uint8_t sense[24] __attribute__((aligned(4)));
    static uint8_t toc[256] __attribute__((aligned(4)));
    int rc;

    /*
     * Ordem baseada no exemplo oficial da libxenon (file/browser):
     * vídeo/console primeiro (senão nada aparece na tela), depois
     * ATA/ATAPI (senão o controlador não está pronto pro nosso código
     * de baixo nível).
     */
    xenos_init(VIDEO_MODE_AUTO);
    console_init();

    xenon_ata_init();
    xenon_atapi_init();

    log_init();

    log_line("[INFO] MEDIA=REAL_PC_ENGINE_CD");
    log_line("[INFO] FULL_CACHE=DISABLED");
    log_line("[INFO] SMALL_CACHE=DISABLED");
    log_line("[INFO] READ_AHEAD=DISABLED");

    rc = xboxcd_init();
    log_line("[STEP] init=%d", rc);

    rc = xboxcd_test_unit_ready(&r);
    if (rc) {
        log_line("[STOP] TEST UNIT READY failed");
        xboxcd_request_sense(sense, &r);
        log_close();
        return 0;
    }

    rc = xboxcd_read_toc(toc, sizeof(toc), &r);
    if (rc) {
        log_line("[STOP] READ TOC failed");
        xboxcd_request_sense(sense, &r);
        log_close();
        return 0;
    }

    /*
     * Baseline probes. After the first log, the exact PCE TOC will determine
     * the next set of sector/track tests.
     */
    probe(0);
    probe(16);
    probe(32);
    probe(1000);

    log_line("[SUMMARY] baseline diagnostic finished");
    log_line("[SUMMARY] upload xboxcd.log");

    log_close();
    return 0;
}
