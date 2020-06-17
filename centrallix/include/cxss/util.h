#ifndef CXSS_UTIL_H
#define CXSS_UTIL_H

#include <stddef.h>

char *cxssGetTimestamp(void);
char *cxssStrdup(const char *str);
char *cxssBlobdup(const char *buf, size_t bufsize);

#endif /* CXSS_UTIL_H */
