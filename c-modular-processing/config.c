#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

char *readLine(FILE *file)
{
    char *buffer = NULL;
    int   size   = 0;
    int   incr   = 63;

    do {
        char *tmp = realloc(buffer, size + incr + 1);

        if (!tmp) {
            free(buffer);
            return NULL;
        }

        buffer = tmp;
        if (!fgets(buffer + size, incr + 1, file)) {
            free(buffer);
            return NULL;
        }

        size += incr;
    } while (!strchr(buffer + (size - incr), '\n'));

    return buffer;
}

bool checkConfigFileUnusedLine(char *line)
{
    char *token = NULL;
    token = strtok(line, " \t\n\v\f\r");

    if (strcmp(token, ";") == 0) {
        return true;
    }

    return false;
}

int configRead(struct config *cfg, const char *name)
{
    assert(name != NULL);

    FILE* configFile = fopen(name, "r");

    if (configFile == NULL) {
        return 1;
    }

    char *buffer = NULL;

    do {
        buffer = readLine(configFile);

        if (checkConfigFileUnusedLine(buffer)) {
            continue;
        }

    }
    while (buffer != NULL);

    fclose(configFile);
    return 0;
}


int configValue(const struct config *cfg,
                const char *section,
                const char *key,
                enum configValueType type,
                void *value)
{
    /// TODO: implement
    return 2;
}


void configClean(struct config *cfg)
{
    /// TODO: implement
}
