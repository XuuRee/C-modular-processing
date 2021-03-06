#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "log.h"
#include "module-cache.h"
#include "config.h"

enum {
    defaultTimeout = 2,
    defaultBucketCount = 32
};

struct cacheItem {
    char *key;
    char *response;
    size_t responseLength;
    enum responseCode responseCode;
    time_t timeOfDeath;
    struct cacheItem *next;
};

struct bucket {
    struct cacheItem *first;
    struct cacheItem *last;
};

struct cache {
    struct bucket *buckets;
    size_t bucketCount;
    int timeout;
};

MODULE_PRIVATE
void responseCleanup(struct query *query)
{
    free(query->response);
    query->response = NULL;
}

MODULE_PRIVATE
int loadConfig(struct module *module, const struct config *cfg, const char *section)
{
    struct cache *cache = (struct cache *)module->privateData;
    if (!cache) {
        LOG(LError, "Corrupted module");
        return -1;
    }

    int rv;
    if ((rv = configValue(cfg, section, "Timeout", CfgInteger, &cache->timeout))) {
        LOG(LWarn, "Could not read value Timeout, using default = %d", defaultTimeout);
    }

    int bucketCount;
    if ((rv = configValue(cfg, section, "BucketCount", CfgInteger, &bucketCount))) {
        LOG(LWarn, "Could not read value BucketCount, using default = %d", defaultBucketCount);
        bucketCount = defaultBucketCount;
    }

    if (bucketCount < 0) {
        return 3;
    }
    if ((size_t)bucketCount != cache->bucketCount || !cache->buckets) {
        cache->bucketCount = bucketCount;
        void *buckets = realloc(cache->buckets, cache->bucketCount * sizeof(struct bucket));
        if (!buckets) {
            LOG(LFatal, "Allocation failed (%zu bytes)", cache->bucketCount * sizeof(struct bucket));
            return -1;
        }
        cache->buckets = (struct bucket *)buckets;
    }

    memset(cache->buckets, 0, cache->bucketCount * sizeof(struct bucket));
    return 0;
}

MODULE_PRIVATE
size_t hash(const char *key)
{
    const size_t base = 31;
    size_t h = 0;
    for (size_t coef = 1; *key; ++key) {
        h += *key * coef;
        coef *= base;
    }
    return h;
}

MODULE_PRIVATE
struct cacheItem *find(struct cache *cache, const char *key)
{
    size_t bucketId = hash(key) % cache->bucketCount;
    time_t now = time(NULL);

    struct cacheItem **item = &cache->buckets[bucketId].first; 
    while (*item) {
        if ((*item)->timeOfDeath < now) {
            struct cacheItem *toBeFreed = *item;
            *item = (*item)->next;
            free(toBeFreed->key);
            free(toBeFreed->response);
            free(toBeFreed);
            continue;
        }
        if (strcmp(key, (*item)->key) == 0) {
            return *item;
        }
        item = &(*item)->next;
    }
    if (!cache->buckets[bucketId].first) {
        cache->buckets[bucketId].last = NULL;
    }
    return NULL;
}

MODULE_PRIVATE
void process(struct module *module, struct query *query)
{
    struct cache *cache = (struct cache *)module->privateData;
    if (!cache) {
        query->responseCode = RCError;
        return;
    }

    struct cacheItem *item = find(cache, query->query);
    if (!item) {
        query->responseCode = RCSuccess;
        return;
    }

    if (query->responseCleanup)
        query->responseCleanup(query);

    query->response = (char *)malloc(item->responseLength + 1);
    if (!query->response) {
        LOG(LFatal, "Allocation failed (%zu bytes)", item->responseLength + 1);
        query->responseCode = RCError;
        return;
    }
    strcpy(query->response, item->response);
    query->responseCode = RCDone;
    query->responseCleanup = responseCleanup;
}

MODULE_PRIVATE
void postProcess(struct module *module, struct query *query)
{
    struct cache *cache = (struct cache *)module->privateData;
    if (!cache) {
        query->responseCode = RCError;
        return;
    }
    struct cacheItem *item = find(cache, query->query);

    if (!item) {
        size_t bucketId = hash(query->query) % cache->bucketCount;
        time_t now = time(NULL);

        struct cacheItem *newItem = (struct cacheItem *)malloc(sizeof(struct cacheItem));
        if (!newItem) {
            LOG(LFatal, "Allocation failed (%zu bytes)", sizeof(struct cacheItem));
            return;
        }
        size_t queryLength = strlen(query->query);
        newItem->key = (char *)malloc(queryLength + 1);
        if (!newItem->key) {
            LOG(LFatal, "Allocation failed (%zu bytes)", queryLength + 1);
            free(newItem);
            return;
        }
        size_t responseLength = strlen(query->response);
        newItem->response = (char *)malloc(responseLength + 1);
        if (!newItem->response) {
            LOG(LFatal, "Allocation failed (%zu bytes)", responseLength + 1);
            free(newItem->key);
            free(newItem);
            return;
        }
        strcpy(newItem->key, query->query);
        if (query->response) {
            strcpy(newItem->response, query->response);
        } else {
            newItem->response[0] = '\0';
        }
        newItem->responseCode = query->responseCode;
        newItem->responseLength = responseLength;
        newItem->timeOfDeath = now + cache->timeout;
        newItem->next = cache->buckets[bucketId].first;

        cache->buckets[bucketId].first = newItem;
        if (!cache->buckets[bucketId].last)
            cache->buckets[bucketId].last = newItem;
    }
}


MODULE_PRIVATE
void cleanup(struct module *module)
{
    if (!module) {
        return;
    }
    struct cache *cache = (struct cache *)module->privateData;
    if (!cache) {
        return;
    }

    for (size_t i = 0; i != cache->bucketCount; ++i) {
        while (cache->buckets[i].first) {
            struct cacheItem *item = cache->buckets[i].first;
            cache->buckets[i].first = item->next;

            free(item->key);
            free(item->response);
            free(item);
        }
    }
    free(cache->buckets);
    free(cache);
}

void moduleCache(struct module *module)
{
    module->privateData = NULL;
    module->name = "cache";
    module->loadConfig = loadConfig;
    module->process = process;
    module->postProcess = postProcess;
    module->cleanup = cleanup;

    struct cache *cache = (struct cache *)malloc(sizeof(struct cache));
    if (!cache) {
        LOG(LFatal, "Allocation failed (%zu bytes)", sizeof(struct cache));
        return;
    }

    cache->timeout = 2;
    cache->bucketCount = 32;
    cache->buckets = (struct bucket *)malloc(sizeof(struct bucket) * cache->bucketCount);
    if (!cache->buckets) {
        LOG(LFatal, "Allocation failed (%zu bytes)", sizeof(struct bucket) * cache->bucketCount);
    }
    memset(cache->buckets, 0, cache->bucketCount * sizeof(struct bucket));
    module->privateData = cache;
}
