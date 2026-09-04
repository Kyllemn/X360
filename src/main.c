#include "xboxcd.h"
#include "xboxcd_log.h"

#include <stdint.h>
#include <string.h>

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
