#ifndef XBOXCD_LOG_H
#define XBOXCD_LOG_H

void log_init(void);
void log_close(void);
void log_line(const char *fmt, ...);
void log_hex(const char *label, const unsigned char *p, unsigned n);

#endif
