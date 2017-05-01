#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>


void listInit(struct config *cfg)
{
    assert(cfg != NULL);

    cfg->end = NULL;
    cfg->head = NULL;
}


void memoryInit(char **values, unsigned int index)
{
    for (unsigned int i = index; i < index + 10; i++) {
        values[i] = NULL;
    }
}


bool reallocate2D(char ***values, unsigned int *memory)
{
    *values = (char**)realloc(*values, (*memory + *memory) * sizeof(char*));

    if (!*values) {
        return false;
    }

    memoryInit(*values, *memory);
    *memory += *memory;
    return true;
}


bool configPush(struct config *cfg, char *name)
{
    assert(cfg != NULL);
    assert(name != NULL);

    struct section *sec = malloc(sizeof *sec);
    if (!sec) return false;

    sec->name = (char*)malloc(strlen(name) + 1);
    if (!sec->name) return false;

    strcpy(sec->name, name);

    sec->keys = calloc(10, sizeof(char*));
    if (!sec->keys) return false;

    sec->values = calloc(10, sizeof(char*));
    if (!sec->values) return false;

    memoryInit(sec->keys, 0);
    memoryInit(sec->values, 0);

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


unsigned int startIndex(char *value)
{
    unsigned int start = 0;

    for (unsigned int i = 0; i < strlen(value); i++) {
        if (!isspace(value[i])) {
            if (value[i] != '=') {
                start = i;
                break;
            }
        }
    }

    return start;
}


unsigned int endIndex(char *value)
{
    unsigned int end = strlen(value);

    if (end == 1) {
        return 1;
    }

    for (unsigned int i = end - 1; i > 0; i--) {
        if (!isspace(value[i])) {
            end = i + 1;
            break;
        }
    }

    return end;
}


bool configAddValue(struct section *sec,
                    char *buffer,
                    unsigned int index,
                    unsigned int *memory)
{
    assert(sec != NULL);

    if (index >= *memory) {
        sec->keys = (char**)realloc(sec->keys, (*memory + *memory) * sizeof(char*));
        sec->values = (char**)realloc(sec->values, (*memory + *memory) * sizeof(char*));

        if (!sec->keys || !sec->values) {
            return false;
        }

        memoryInit(sec->keys, *memory);
        memoryInit(sec->values, *memory);
        *memory += *memory;
    }

    char *key = strtok(buffer, " \t\n\v\f\r=");
    char *value = strtok(NULL, "\n");

    if (key && !value) {
        value = "";
    } else {
        value = value + startIndex(value);;
        unsigned int end = endIndex(value);
        value[end] = '\0';
    }

    sec->keys[index] = (char*)malloc(strlen(key) + 1);
    sec->values[index] = (char*)malloc(strlen(value) + 1);

    if (!sec->keys[index] || !sec->values[index]) {
        return false;
    }

    strcpy(sec->keys[index], key);
    strcpy(sec->values[index], value);
    return true;
}


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


/**
 * Check if the first character of line is ';'
 *
 * @param   character const char
 * @return  true if characters are equal,
 *          false otherwise
 */
bool commentLine(const char character)
{
    return (character == ';') ? true : false;
}


/**
 * Check if given line of the file is useless.
 * That means if there is commentary, empty line
 * or whitespaces.
 *
 * @param   line given line from file (buffer)
 * @return  true if line is useless, false otherwise
 */
bool unusedLine(const char *line)
{
    if (commentLine(line[0])) {
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


/**
 * Check if given char is allowed character
 * (one of the: dash, underscore or colon)
 *
 * @param   character given char from line
 * @return  true if character is allowed,
 *          false otherwise
 */
bool isPunctuation(const char character)
{
    switch(character) {
    case '-': return true;
    case '_': return true;
    case ':': return true;
    default: return false;
    }
}


/**
 * Check if given section fulfill criteria
 * from task.
 *
 * @param   line given line of the file (buffer)
 * @return  true if line is fine, false otherwise
 */
bool checkSection(const char *line)
{
    unsigned int end = 0;
    unsigned int start = 0;

    if (!checkFirstSquareBracket(line[0])) {
        return false;
    }

    if (!checkSecondSquareBracket(line, &end)) {
        return false;
    }

    if (strlen(line) == 3) {
        return false;
    }

    for (unsigned int i = 1; i < end; i++) {
        if (!isspace(line[i])) {
            start = i;
            break;
        }
    }

    for (unsigned int i = end - 1; i > 0; i--) {
        if (!isspace(line[i])) {
            end = i;
            break;
        }
    }

    for (unsigned int i = start; i < end + 1; i++) {
        if (!(isalpha(line[i]) || isPunctuation(line[i]))) {
            if (!isdigit(line[i])) {
                return false;
            }
        }
   }

    return true;
}


bool checkKeyValue(const char *line)
{
    unsigned int equal = 0;
    unsigned int start = 0, end = 0;

    for (unsigned int i = 0; i < strlen(line); i++) {
        if (line[i] == '=') {
            equal = i;
            break;
        }
    }

    if (!equal) {
        return false;
    }

    for (unsigned int i = 0; i < equal; i++) {
        if (!isspace(line[i])) {
            start = i;
            break;
        }
    }

    for (unsigned int i = equal - 1; i > start; i--) {
        if (!isspace(line[i])) {
            end = i;
            break;
        }
    }

    for (unsigned int i = start; i < end + 1; i++) {
        if (!(isdigit(line[i]) || isalpha(line[i]))) {
            return false;
        }
    }

    return true;
}


void cleanValues(char **values)
{
    for (unsigned int i = 0; values[i] != NULL; i++) {
        free(values[i]);
    }

    free(values);
}


bool searchDuplicitySections(char **sections, char *last, unsigned int index)
{
    char *tokenLast = strtok(last, " \t\n\v\f\r[]");

    for(unsigned int i = 0; i < index; i++) {
        char *tokenElement = strtok(sections[i], " \t\n\v\f\r[]");

        if (strcmp(tokenElement, tokenLast) == 0) {
            return true;
        }
    }

    return false;
}


bool searchDuplicityKeys(char **keys, char *last, unsigned int index)
{
    char *tokenLast = strtok(last, " \t\n\v\f\r=");

    for(unsigned int i = 0; i < index; i++) {
        char *tokenElement = strtok(keys[i], " \t\n\v\f\r=");

        if (strcmp(tokenElement, tokenLast) == 0) {
            return true;
        }
    }

    return false;
}


bool validFile(FILE *file)
{
    char **sections = (char**)calloc(10, sizeof(char*));
    char **keys = (char**)calloc(10, sizeof(char*));

    if (!sections || !keys) {
        return false;
    }

    memoryInit(sections, 0);

    unsigned int memorySections = 10, memoryKeys = 10;
    unsigned int indexSections = 0, indexKeys = 0;

    bool firstSectionIndicator = false;
    bool newSectionIndicator = false;

    char *buffer = readLine(file);

    while (buffer != NULL) {
        if (!unusedLine(buffer)) {
            // Check if section is valid
            if (checkSection(buffer)) {
                firstSectionIndicator = true;
                newSectionIndicator = true;
                if (indexSections >= memorySections) {
                    if (!reallocate2D(&sections, &memorySections)) {
                        cleanValues(sections);
                        cleanValues(keys);
                        free(buffer);
                        return false;
                    }
                }
                sections[indexSections] = (char*)calloc(strlen(buffer) + 1, sizeof(char));
                if (!sections[indexSections]) {
                    cleanValues(sections);
                    cleanValues(keys);
                    free(buffer);
                    return false;
                }
                strcpy(sections[indexSections], buffer);
                if (searchDuplicitySections(sections, buffer, indexSections)) {
                    cleanValues(sections);
                    cleanValues(keys);
                    free(buffer);
                    return false;
                }
                indexSections++;
            }
            // Check if keys and values are valid
            else if (checkKeyValue(buffer)) {
                if (!firstSectionIndicator) {
                    cleanValues(sections);
                    cleanValues(keys);
                    free(buffer);
                    return false;
                }
                if (newSectionIndicator) {
                    newSectionIndicator = false;
                    memoryInit(keys, 0);
                    memoryKeys = 10;
                    indexKeys = 0;
                }
                if (indexKeys >= memoryKeys) {
                    if (!reallocate2D(&keys, &memoryKeys)) {
                        cleanValues(sections);
                        cleanValues(keys);
                        free(buffer);
                        return false;
                    }
                }
                keys[indexKeys] = (char*)calloc(strlen(buffer) + 1, sizeof(char));
                if (!keys[indexKeys]) {
                    cleanValues(sections);
                    cleanValues(keys);
                    free(buffer);
                    return false;
                }
                strcpy(keys[indexKeys], buffer);
                if (searchDuplicityKeys(keys, buffer, indexKeys)) {
                    cleanValues(sections);
                    cleanValues(keys);
                    free(buffer);
                    return false;
                }
                indexKeys++;
            }
            else {
                cleanValues(sections);
                cleanValues(keys);
                free(buffer);
                return false;
            }
        }

        free(buffer);
        buffer = readLine(file);
    }

    cleanValues(sections);
    cleanValues(keys);
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

    listInit(cfg);

    if (!validFile(configFile)) {
        fclose(configFile);
        return 2;
    }

    fseek(configFile, 0, SEEK_SET);

    unsigned int index = 0;
    unsigned int memory = 10;

    char *buffer = readLine(configFile);

    while (buffer != NULL) {
        if (!unusedLine(buffer)) {
            if (checkFirstSquareBracket(buffer[0])) {
                char *name = strtok(buffer, " \t\n\v\f\r[]");
                if (!configPush(cfg, name)) {
                    free(buffer);
                    fclose(configFile);
                    return 1;
                }
                index = 0;
                memory = 10;
            } else {
                if (!configAddValue(cfg->end, buffer, index, &memory)) {
                    free(buffer);
                    fclose(configFile);
                    return 1;
                }
                index++;
            }
        }
        free(buffer);
        buffer = readLine(configFile);
    }

    fclose(configFile);
    return 0;
}


bool findKey(char **keys, const char *seek, unsigned int *index)
{
    while (*keys) {
        if (strcmp(*keys, seek) == 0) {
            return true;
        }
        keys++;
        (*index)++;
    }

    return false;
}


int configValue(const struct config *cfg,
                const char *section,
                const char *key,
                enum configValueType type,
                void *value)
{
    assert(cfg != NULL);
    assert(key != NULL);
    assert(section != NULL);

    struct section *current = cfg->head;

    unsigned int index = 0;
    bool isSection = false;

    while (current != NULL && isSection != true) {
        if (strcmp(current->name, section) == 0) {
            isSection = true;
            if (findKey(current->keys, key, &index)) {
                char *valueFromCfg = current->values[index], *buffer;
                int resultInteger = -1, *result = &resultInteger;
                switch(type) {
                case CfgString:
                    *((const char**)value) = valueFromCfg;
                    return 0;

                case CfgInteger:
                    resultInteger = strtol(valueFromCfg, &buffer, 10);
                    if (valueFromCfg == buffer) {
                        return 3;
                    }
                    if (*result == 1 || *result == 0) {
                        return 3;
                    }
                    *((int*)value) = *result;
                    return 0;

                case CfgBool:
                    resultInteger = strtol(valueFromCfg, &buffer, 10);
                    if (valueFromCfg == buffer) {
                        return 3;
                    }
                    if (*result != 1 && *result != 0) {
                        return 3;
                    }
                    *((int*)value) = *result;
                    return 0;
                default:
                    return 4;
                }
            }
        }

        current = current->next;
    }

    if (!isSection) {
        return 1;
    }

    return 2;
}


void configClean(struct config *cfg)
{
    if (cfg->head && cfg->end) {
        struct section *next = NULL;
        struct section *current = cfg->head;

        while (current != NULL) {
            next = current->next;
            free(current->name);
            cleanValues(current->keys);
            cleanValues(current->values);
            free(current);
            current = next;
        }

        cfg->head = NULL;
        cfg->end = NULL;
    }
}
