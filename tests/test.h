#ifndef TEST_H
#define TEST_H

#include <stdio.h>

extern int testFailures;

#define CHECK(cond)                                                                                \
    do                                                                                             \
    {                                                                                              \
        if (!(cond))                                                                               \
        {                                                                                          \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                 \
            testFailures++;                                                                        \
        }                                                                                          \
    } while (0)

#define CHECK_EQ(a, b)                                                                             \
    do                                                                                             \
    {                                                                                              \
        long long checkA = (long long)(a);                                                         \
        long long checkB = (long long)(b);                                                         \
        if (checkA != checkB)                                                                      \
        {                                                                                          \
            printf("FAIL %s:%d: %s == %s (%lld != %lld)\n", __FILE__, __LINE__, #a, #b, checkA,    \
                   checkB);                                                                        \
            testFailures++;                                                                        \
        }                                                                                          \
    } while (0)

void testMotion();
void testBleCodec();

#endif
