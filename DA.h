#ifndef DA_H
#define DA_H

/*
    Dynamic arrays
*/

#define da_append(da, e) \
do { \
    if ((da).capacity == 0) { \
        (da).capacity = 256; \
        (da).items = malloc((da).capacity * sizeof(*(da).items)); \
    } \
    if ((da).count >= (da).capacity) { \
        if ((da).capacity == 0) (da).capacity = 256; \
        else (da).capacity *= 2; \
        (da).items = realloc((da).items, (da).capacity * sizeof(*(da).items)); \
    } \
    (da).items[(da).count++] = (e); \
} while (0)

#define da_appendP(da, e) \
do { \
    if ((da)->capacity == 0) { \
        (da)->capacity = 64; \
        (da)->items = malloc((da)->capacity * sizeof(*(da)->items)); \
    } \
    if ((da)->count >= (da)->capacity) { \
        if ((da)->capacity == 0) (da)->capacity = 64; \
        else (da)->capacity *= 2; \
        (da)->items = realloc((da)->items, (da)->capacity * sizeof(*(da)->items)); \
    } \
    (da)->items[(da)->count++] = (e); \
} while (0)

#define da_init(type) malloc(64 * sizeof(type))

#define da_free(da) \
do { \
    (da).count = 0; \
    (da).capacity = 0; \
    free((da).items); \
    (da).items = NULL; \
} while (0)

typedef struct {
    int *items;
    int count;
    int capacity;
} numbers;

typedef struct {
    float *items;
    int count;
    int capacity;
} numbersf;

#endif