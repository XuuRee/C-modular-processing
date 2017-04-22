#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

char* readLine(FILE *file)
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

bool unusedLine(const char *line)
{
    for (unsigned int i = 0; i < strlen(line); i++) {
        if (!isspace(line[i])) {
            if (line[i] == ';') {
                return true;
            } else {
                return false;
            }
        }
    }

    return true;
}

bool firstSquareBracket(const char character)
{
    return (character == '[') ? true : false;
}

bool checkFirstSquareBracket(const char *line, unsigned int *start)
{
    unsigned int length = strlen(line);

    for (unsigned int i = 0; i < length; i++) {
        if (!isspace(line[i])) {
            if (firstSquareBracket(line[i])) {
                *start = i;
                return true;
            } else {
                break;
            }
        }
    }

    return false;
}

bool lastSquareBracket(const char character)
{
    return (character == ']') ? true : false;
}

bool checkSecondSquareBracket(const char *line, unsigned int *end)
{
    unsigned int length = strlen(line);

    for (unsigned int j = length - 1; j > 0; j--) {
        if (!isspace(line[j])) {
            if (lastSquareBracket(line[j])) {
                *end = j;
                return true;
            } else {
                break;
            }
        }
    }

    return false;
}

bool ispunctuation(const char character)
{
    switch(character) {
    case '-': return true;
    case '_': return true;
    case ':': return true;
    default: return false;
    }
}

bool checkSection(const char *line)
{
    unsigned int start = 0, end = 0;

    if (!checkFirstSquareBracket(line, &start)) {
        return false;
    }

    if (!checkSecondSquareBracket(line, &end)) {
        return false;
    }

    start += 1;

    for (unsigned int i = start; i < end; i++) {
        if (!isspace(line[i])) {
            if (!(!isalpha(line[i]) || !ispunctuation(line[i]))) {
                if (!isdigit(line[i])) {
                    return false;
                }
            }
        }
   }

    return true;
}

char *strdup(const char *src)
{
    char *str;
    char *p;
    int len = 0;

    while (src[len]) {
        len++;
    }

    str = malloc(len + 1);
    p = str;

    while (*src) {
        *p++ = *src++;
    }

    *p = '\0';

    return str;
}

bool checkKeyValue(const char *line)
{
    char *copy = strdup(line);

    if (!copy) {
        return false;
    }

    char *token = strtok(copy, " \t\n\v\f\r=");

    if (!token) {
        free(copy);
        return false;
    }

    for (unsigned int i = 0; token[i] != '\0'; i++) {
        if (!(isdigit(token[i]) || isalpha(token[i]))) {
            free(copy);
            return false;
        }
    }

    free(copy);
    return true;
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
        // TODO
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
