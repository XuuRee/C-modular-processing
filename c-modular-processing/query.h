#ifndef QUERY_H
#define QUERY_H

#include <stdint.h>

#include "functions.h"

enum responseCode {
    RCError,
    RCDone,
    RCSuccess
};

struct query {
    const char *query;
    queryCleanupFn queryCleanup;

    char *response;
    enum responseCode responseCode;
    queryCleanupFn responseCleanup;
};

void initQuery(struct query *);

#endif
