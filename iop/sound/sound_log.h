#include <stdio.h>

#ifndef SOUND_LOG_H
#define SOUND_LOG_H

#define STRINGIFY_IMPL(X) #X
#define STRINGIFY(X) STRINGIFY_IMPL(X)

#define LOG_LVL 1

#if  defined(LOG_LVL)
#define LOGGER(fmt, ...) \
        (printf("SOUND IRX : %s " STRINGIFY(__LINE__) " : " fmt "\n", __FILE__,  ##__VA_ARGS__))
#else
#define LOGGER(fmt, ...)
#endif

#endif