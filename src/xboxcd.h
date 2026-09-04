#ifndef XBOXCD_H
#define XBOXCD_H

#include <stdint.h>

#define XBOXCD_OK        0
#define XBOXCD_NO_DISC  -10
#define XBOXCD_TIMEOUT  -11
#define XBOXCD_IO_ERROR -12

typedef struct {
    uint8_t status;
    uint8_t error;
    int result;
} xboxcd_result_t;

int xboxcd_init(void);
int xboxcd_test_unit_ready(xboxcd_result_t *r);
int xboxcd_request_sense(uint8_t sense[24], xboxcd_result_t *r);
int xboxcd_read_toc(uint8_t *toc, unsigned len, xboxcd_result_t *r);
int xboxcd_read_raw2352(uint32_t lba, uint8_t *out, xboxcd_result_t *r);

#endif
