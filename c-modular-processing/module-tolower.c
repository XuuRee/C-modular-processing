#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "log.h"
#include "module-tolower.h"

MODULE_PRIVATE
void responseCleanup(struct query *query)
{
    free(query->response);
    query->response = NULL;
}

MODULE_PRIVATE
void process(struct module *module, struct query *query)
{
    (void)module;
    if (query->responseCleanup)
        query->responseCleanup(query);

    size_t length = strlen(query->query);
    query->response = (char *)malloc(length + 1);
    if (!query->response) {
        LOG(LFatal, "Allocation failed (%zu bytes)", length + 1);
        query->responseCode = RCError;
        return;
    }

    query->responseCleanup = responseCleanup;

    char *out = query->response;
    for (const char *in = query->query; *in; ++in, ++out) {
        *out = tolower(*in);
    }
    *out = '\0';
    query->responseCode = RCSuccess;
}

void moduleToLower(struct module *module)
{
    module->privateData = NULL;
    module->name = "tolower";
    module->loadConfig = NULL;
    module->process = process;
    module->postProcess = NULL;
    module->cleanup = NULL;
}
