#include "xboxcd_log.h"
#include <stdio.h>
#include <stdarg.h>

static FILE *g_log;

void log_init(void)
{
    g_log = fopen("xboxcd.log", "wb");
    if (!g_log)
        g_log = fopen("/xboxcd.log", "wb");

    if (g_log)
        fprintf(g_log, "=== XBOX360 PHYSICAL CD TEST ===\n");

    printf("=== XBOX360 PHYSICAL CD TEST ===\n");
}

void log_close(void)
{
    if (g_log) {
        fflush(g_log);
        fclose(g_log);
        g_log = 0;
    }
}

void log_line(const char *fmt, ...)
{
    char buf[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    printf("%s\n", buf);

    if (g_log) {
        fprintf(g_log, "%s\n", buf);
        fflush(g_log);
    }
}

void log_hex(const char *label, const unsigned char *p, unsigned n)
{
    unsigned i;
    printf("%s:", label);
    if (g_log) fprintf(g_log, "%s:", label);

    for (i = 0; i < n; ++i) {
        printf("%02X", p[i]);
        if (g_log) fprintf(g_log, "%02X", p[i]);
    }

    printf("\n");
    if (g_log) {
        fprintf(g_log, "\n");
        fflush(g_log);
    }
}
