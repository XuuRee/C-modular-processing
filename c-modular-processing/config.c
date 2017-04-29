#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>


void configInit(struct config *cfg)
{
    assert(cfg != NULL);

    cfg->end = NULL;
    cfg->head = NULL;
}


void configInitMemory(char **values, unsigned int index)
{
    for (unsigned int i = index; i < index + 10; i++) {
        values[i] = NULL;
    }
}


bool configPush(struct config *cfg, char *name)
{
    assert(cfg != NULL);
    assert(name != NULL);

    struct section *sec = malloc(sizeof *sec);

    if (!sec) {
        return false;
    }

    sec->name = (char *)malloc(strlen(name) + 1);

    if (!sec->name) {
        return false;
    }

    sec->values = calloc(10, sizeof(char *));
    configInitMemory(sec->values, 0);

    if (!sec->values) {
        return false;
    }

    strcpy(sec->name, name);

    if (!cfg->head) {
        cfg->end = sec;
        cfg->head = sec;
        sec->prev = NULL;
        sec->next = NULL;
    } else {
        sec->next = NULL;
        sec->prev = cfg->end;
        cfg->end->next = sec;
        cfg->end = sec;
    }

    return true;
}


bool configAddValue(struct section *sec,
                    char *value,
                    unsigned int index,
                    unsigned int *memory)
{
    assert(sec != NULL);
    assert(value != NULL);

    if (index >= *memory) {
        sec->values = (char**)realloc(sec->values, (*memory + *memory) * sizeof(char *));

        if (!sec->values) {
            return false;
        }

        configInitMemory(sec->values, *memory);
        *memory += *memory;
    }

    sec->values[index] = (char *)malloc(strlen(value) + 1);

    if (!sec->values[index]) {
        return false;
    }

    strcpy(sec->values[index], value);
    return true;
}


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


bool commentLine(const char character) {
    return (character == ';') ? true : false;
}


bool unusedLine(const char *line)
{
    char character = line[0];

    if (commentLine(character)) {
        return true;
    }

    for (unsigned int i = 0; i < strlen(line); i++) {
        if (!isspace(line[i])) {
            return false;
        }
    }

    return true;
}


bool checkFirstSquareBracket(const char character)
{
    return (character == '[') ? true : false;
}


bool checkSecondSquareBracket(const char *line, unsigned int *end)
{
    unsigned int length = strlen(line);

    for (unsigned int j = length - 1; j > 0; j--) {
        if (!isspace(line[j])) {
            if (line[j] == ']') {
                *end = j;
                return true;
            } else {
                break;
            }
        }
    }

    return false;
}


bool isPunctuation(const char character)
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
    unsigned int end = 0;
    char character = line[0];

    if (!checkFirstSquareBracket(character)) {
        return false;
    }

    if (!checkSecondSquareBracket(line, &end)) {
        return false;
    }

    for (unsigned int i = 1; i < end; i++) {
        if (!isspace(line[i])) {
            if (!(isalpha(line[i]) || isPunctuation(line[i]))) {
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


bool validFile(FILE *file)
{
    bool sectionIndicator = false;
    char *buffer = readLine(file);

    while (buffer != NULL) {
        if (!unusedLine(buffer)) {
            if (checkSection(buffer)) {
                sectionIndicator = true;
            } else if (checkKeyValue(buffer)) {
                if (!sectionIndicator) {
                    free(buffer);
                    return false;
                }
            } else {
                free(buffer);
                return false;
            }
        }
        free(buffer);
        buffer = readLine(file);
    }

    return true;
}


int configRead(struct config *cfg, const char *name)
{
    assert(name != NULL);
    assert(cfg != NULL);

    FILE* configFile = fopen(name, "r");

    if (!configFile) {
        return 1;
    }

    configInit(cfg);

    if (!validFile(configFile)) {
        fclose(configFile);
        return 2;
    }

    fseek(configFile, 0, SEEK_SET);

    unsigned int index = 0;
    unsigned int memory = 10;

    char *buffer = readLine(configFile);

    while (buffer != NULL) {
        if (checkSection(buffer)) {
            if (!configPush(cfg, buffer)) {
                fclose(configFile);
                return 1;
            }
            memory = 10;
            index = 0;
        }

        if (checkKeyValue(buffer)) {
            if (!configAddValue(cfg->end, buffer, index, &memory)) {
                fclose(configFile);
                return 1;
            }
            index++;
        }

        free(buffer);
        buffer = readLine(configFile);
    }

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


void configCleanValues(char **values)
{
    for (unsigned int i = 0; values[i] != NULL; i++) {
        free(values[i]);
    }

    free(values);
}


void configClean(struct config *cfg)
{
    if (cfg->head && cfg->end) {
        struct section *next = NULL;
        struct section *current = cfg->head;

        while (current != NULL) {
            next = current->next;
            free(current->name);
            configCleanValues(current->values);
            free(current);
            current = next;
        }

        cfg->head = NULL;
        cfg->end = NULL;
    }
}
