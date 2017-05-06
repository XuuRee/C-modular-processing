#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "log.h"
#include "config.h"
#include "module-cache.h"
#include "module-toupper.h"
#include "module-tolower.h"
#include "module-decorate.h"
#include "module-magic.h"


void process(const char *queryText, struct module *pre, int preSize, struct module *post, int postSize)
{
    struct query query;
    memset(&query, 0, sizeof(struct query));
    query.query = queryText;
    query.queryCleanup = NULL;
    query.response = "";
    query.responseCleanup = NULL;

    LOG(LInfo, "query: %s", queryText);

    for (int m = 0; m < preSize; ++m) {
        LOG(LDebug, "Running module %s", pre[m].name);
        pre[m].process(&pre[m], &query);

        switch (query.responseCode) {
        case RCSuccess:
            LOG(LInfo, "Response success");
        case RCDone:
            LOG(LInfo, "Response done");
        case RCError:
            LOG(LError, "Error");
        default:
            LOG(LError, "Error");
        }
    }

    LOG(LDebug, "responseCode: %i", query.responseCode);
    if (query.responseCode == RCSuccess) {
        for (int m = 0; m < postSize; ++m) {
            LOG(LDebug, "Postprocessing by %s", post[m].name);
            if (pre[m].postProcess) {
                post[m].postProcess(&post[m], &query);
                switch (query.responseCode) {
                case RCSuccess:
                    LOG(LInfo, "Response success");
                    continue;
                case RCDone:
                    LOG(LInfo, "Response done");
                    break;
                case RCError:
                    LOG(LError, "Error");
                    break;
                default:
                    LOG(LError, "Error");
                    break;
                }
            }
        }
    }

    LOG(LInfo, "response: %s", query.response);
    char *status = NULL;

    if (query.responseCode == RCSuccess) {
        status = "SUCCES";
    } else if (query.responseCode == RCDone) {
        status = "DONE";
    } else if (query.responseCode == RCError) {
        status = "ERROR";
    } else {
        status = "UNKNOWN";
    }

    printf("query: %s\nresponse: %s\nstatus: %s\n", query.query, status, query.response);

    if (query.responseCleanup) {
        query.responseCleanup(&query);
    }
    if (query.queryCleanup) {
        query.queryCleanup(&query);
    }
}


void setLogSetting(const struct config *cfg)
{
    const char *logFile;
    if (!configValue(cfg, "log", "File", CfgString, &logFile)) {
        if (setLogFile(logFile)) {
            LOG(LWarn, "Invalid value for File: '%s'", logFile);
        }
    }

    const char *logLevel;
    if (!configValue(cfg, "log", "Level", CfgString, &logLevel)) {
        if (!strcmp(logLevel, "D") || !strcmp(logLevel, "d")) {
            setLogLevel(LDebug);
        } else if (!strcmp(logLevel, "I") || !strcmp(logLevel, "i")) {
            setLogLevel(LInfo);
        } else if (!strcmp(logLevel, "W") || !strcmp(logLevel, "w")) {
            setLogLevel(LWarn);
        } else if (!strcmp(logLevel, "E") || !strcmp(logLevel, "e")) {
            setLogLevel(LError);
        } else if (!strcmp(logLevel, "F") || !strcmp(logLevel, "f")) {
            setLogLevel(LFatal);
        } else if (!strcmp(logLevel, "N") || !strcmp(logLevel, "n")) {
            setLogLevel(LNoLog);
        } else {
            LOG(LWarn, "Invalid value for Mask: '%s'", logLevel);
        }
    }
}


bool processModule(const char *module)
{
    if (strcmp(module, "cache") == 0) {
        return true;
    } else if (strcmp(module, "magic") == 0) {
        return true;
    } else if (strcmp(module, "toupper") == 0) {
        return true;
    } else if (strcmp(module, "tolower") == 0) {
        return true;
    } else if (strcmp(module, "decorate") == 0) {
        return true;
    } else {
        return false;
    }
}


bool isModuleStored(char **sequence, const char *last, unsigned int index)
{
    for (unsigned int i = 0; i < index; i++) {
        if (strcmp(sequence[i], last) == 0) {
            return true;
        }
    }
    return false;
}


bool processOrderModules(char *data, int *size)
{
    char *copy = (char*)calloc(strlen(data) + 1, sizeof(char));
    char **buffer = (char**)calloc(5, sizeof(char*));

    if (!copy || !buffer) {
        return false;
    }

    strcpy(copy, data);

    unsigned int index = 0;
    char *token = strtok(copy, " \t\n\v\f\r");

    while (token != NULL) {
        if (!processModule(token)) {
            free(copy);
            free(buffer);
            return false;
        }
        if (!isModuleStored(buffer, token, index)) {
            buffer[index] = token;
            index++;
        }
        token = strtok(NULL, " \t\n\v\f\r");
    }

    *size = index;
    memset(data, 0, strlen(data));

    for (unsigned int i = 0; i < index; i++) {
        strcat(data, buffer[i]);
        if (i + 1 != index) {
            strcat(data, " ");
        }
    }

    free(copy);
    free(buffer);
    return true;
}


bool checkPostProcessFunctions(char *data, struct module *modules, int modulesCount)
{
    char *copy = (char*)calloc(strlen(data) + 1, sizeof(char));

    if (!copy) {
        return false;
    }

    strcpy(copy, data);

    char *token = strtok(copy, " \t\n\v\f\r");

    while (token != NULL) {
        for (int i = 0; i < modulesCount; i++) {
            if (strcmp(modules[i].name, token) == 0) {
                if (modules[i].postProcess == NULL) {
                    free(copy);
                    return false;
                } else {
                    break;
                }
            }
        }
        token = strtok(NULL, " \t\n\v\f\r");
    }

    free(copy);
    return true;
}


void initModule(char *data, struct module *modules, struct module *orig, int size)
{
    char *token = strtok(data, " \t\n\v\f\r");

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < 5; j++) {
            if (strcmp(orig[j].name, token) == 0) {
                modules[i] = orig[j];
                break;
            }
        }
        token = strtok(NULL, " \t\n\v\f\r");
    }
}


int loadConfig(const char *configFile,
               struct module *modules,
               char **sequenceModulesPre,
               char **sequenceModulesPost,
               int modulesCount)
{
    struct config cfg;
    int rv = configRead(&cfg, configFile);
    switch (rv) {
    case 0:
        break;
    case 1:
        LOG(LError, "Config file '%s' cannot be opened", configFile);
        break;
    case 2:
        LOG(LError, "Config file '%s' is corrupted", configFile);
        break;
    }

    setLogSetting(&cfg);

    const char *prov = NULL;
    if(configValue(&cfg, "run", "Process", CfgString, &prov)) {
        LOG(LError, "Key 'Process' is not in section");
        return 1;
    }

    char *process = (char*)calloc(strlen(prov) + 1, sizeof(char));

    if (!process) {
        return 1;
    }

    strcpy(process, prov);
    *sequenceModulesPre = process;

    const char *post = NULL;
    char *postProcess = NULL;
    configValue(&cfg, "run", "PostProcess", CfgString, &post);

    if (post) {
        LOG(LInfo, "Key 'PostProcess' is located in section");
        postProcess = (char*)calloc(strlen(post) + 1, sizeof(char));
        if (!postProcess) {
            free(process);
            return 1;
        }
        strcpy(postProcess, post);
        *sequenceModulesPost = postProcess;
    }

    char section[265] = "module::";
    char *moduleName = section + strlen(section);
    for (int m = 0; m < modulesCount; ++m) {
        if (!modules[m].loadConfig) {
            continue;
        }
        strcpy(moduleName, modules[m].name);
        LOG(LDebug, "Loading config of section '%s'", section);
        if ((rv = modules[m].loadConfig(&modules[m], &cfg, section))) {
            LOG(LWarn, "Config loading failed (module: '%s', rv: %i)", modules[m].name, rv);
        }
    }

    configClean(&cfg);
    return 0;
}


void processFile(const char *file,
                 struct module *pre,
                 int preSize,
                 struct module *post,
                 int postSize)
{
    LOG(LDebug, "Opening file '%s'", file);
    FILE *input = fopen(file, "r");
    if (!input) {
        LOG(LError, "Cannot open file '%s'", file);
        return;
    }
    char line[64 + 1] = {0};

    for (int i = 1; fgets(line, 64, input); ++i) {
        
        for (char *end = line + strlen(line) - 1; isspace(*end); --end) {
            *end = '\0';
        }

        LOG(LDebug, "line: '%s'", line);
        process(line, pre, preSize, post, postSize);
    }
    fclose(input);
}


int main(int argc, char **argv)
{
    if (argc < 3) {
        LOG(LError, "Cannot run without config and input file name");
        return 6;
    }

    const char *configFile = argv[1];
    const char *inputFile = argv[2];

    int modulesCount = 5;

    struct module modules[5];
    moduleCache(&modules[0]);
    moduleToUpper(&modules[1]);
    moduleDecorate(&modules[2]);
    moduleToLower(&modules[3]);
    moduleMagic(&modules[4]);

    char *seqModulesPre = NULL;
    char *seqModulesPost = NULL;

    int rv;
    if ((rv = loadConfig(configFile, modules, &seqModulesPre, &seqModulesPost, modulesCount))) {
        if (rv == 1)
            LOG(LWarn, "config file %s is missing", configFile);
        else
            return rv;
    }

    int sizeProcess = 0;
    if (!processOrderModules(seqModulesPre, &sizeProcess)) {
        LOG(LError, "Function 'processOrderModules' end with 0 code");
        free(seqModulesPre);
        free(seqModulesPost);
        return 1;
    }

    int sizePostProcess = 0;
    if (!processOrderModules(seqModulesPost, &sizePostProcess)) {
        LOG(LError, "Function 'processOrderModules' end with 0 code");
        free(seqModulesPre);
        free(seqModulesPost);
        return 1;
    }

    if (!checkPostProcessFunctions(seqModulesPost, modules, modulesCount)) {
        LOG(LError, "Function 'checkPostProcessFunctions' end with 0 code");
        free(seqModulesPre);
        free(seqModulesPost);
        return 1;
    }

    LOG(LInfo, "Start");

    struct module selectedModulesPre[sizeProcess];
    initModule(seqModulesPre, selectedModulesPre, modules, sizeProcess);
    struct module selectedModulesPost[sizePostProcess];
    initModule(seqModulesPost, selectedModulesPost, modules, sizePostProcess);

    processFile(inputFile, selectedModulesPre, sizeProcess, selectedModulesPost, sizePostProcess);

    for (int m = 0; m < modulesCount; ++m) {
        if (modules[m].cleanup) {
            modules[m].cleanup(&modules[m]);
        }
    }

    free(seqModulesPre);
    free(seqModulesPost);

    LOG(LInfo, "Finished");
    return 0;
}
