#ifndef _ERRORS_H
#define _ERRORS_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define assert(A, M, ...)                             \
    if (!(A)) {                                       \
        fprintf(stderr, "[ERROR] " M, ##__VA_ARGS__); \
        if (errno != 0) {                             \
            fprintf(stderr, "%s", strerror(errno));   \
        }                                             \
        fprintf(stderr, "\n");                        \
        exit(1);                                      \
    }
#endif /* ifndef _ERRORS_H */
