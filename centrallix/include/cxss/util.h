#ifndef CXSS_UTIL_H
#define CXSS_UTIL_H

#include <stddef.h>

char *get_timestamp(void);
char *cxss_strdup(const char *str);
char *cxss_blobdup(const char *buf, size_t bufsize);

#endif /* CXSS_UTIL_H */
