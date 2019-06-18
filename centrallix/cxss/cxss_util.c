#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cxss/util.h"

/** @brief Duplicate a string
 *
 *  This is just a strdup wrapper function to nicely 
 *  handle cases in which the SQL query returns NULL.
 *
 *  @param str   String to dup.
 *  @return      Dupped string
 */
char *
cxss_strdup(const char *str)
{
    if (!str)
        return NULL;
    return strdup(str);
}

/** @brief Duplicate an array of bytes
 *
 *  This function returns a pointer to a dynamic array 
 *  containing a copy of the dupped data.
 *
 *  @param blob         Blob to dup
 *  @param len          Length of blob
 *  @return             Dupped blob
 */
char *
cxss_blobdup(const char *blob, size_t len)
{
    char *copy;

    /* Check for NULL */
    if (!blob)
        return NULL;

    copy = malloc(sizeof(char) * len);
    if (!copy) {
        mssError(0, "CXSS", "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    memcpy(copy, blob, len);
    
    return copy;
}

/** @brief Get current timestamp
 *
 *  This function returns the current timestamp stored in a 
 *  static buffer. Hence, the buffer does not need to be freed.
 *
 *  @return     timestamp
 */    
char *
cxssGetTimestamp(void)
{
    time_t current_time;
    char *timestamp;

    /* Generate timestamp from current time */
    current_time = time(NULL);
    if (current_time == (time_t)-1) {
        mssError(0, "CXSS", "Failed to retrieve system time\n");
        exit(EXIT_FAILURE);
    }
    timestamp = ctime(&current_time);
    if (!timestamp) {
        mssError(0, "CXSS", "Failed to generate timestamp\n");
        exit(EXIT_FAILURE);
    }

    /* Remove pesky newline */
    char *ch;
    for (ch = timestamp; *ch != '\0'; ch++) {
        if (*ch == '\n') {
            *ch = '\0';
            break;
        }
    }
    return timestamp;
}

