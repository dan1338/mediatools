#pragma once

#ifdef __ANDROID__

#ifndef TAG
#error "No android log TAG defined"
#endif

#include <android/log.h>

#define ERROR(...) do {__android_log_print(ANDROID_LOG_ERROR,    TAG, __VA_ARGS__); abort();} while(0)
#define WARN(...) __android_log_print(ANDROID_LOG_WARN,     TAG, __VA_ARGS__)
#define INFO(...) __android_log_print(ANDROID_LOG_INFO,     TAG, __VA_ARGS__)
#define DEBUG(...) __android_log_print(ANDROID_LOG_DEBUG,    TAG, __VA_ARGS__)

#else

#define ERROR(...) do {printf(__VA_ARGS__), abort();} while(0)
#define WARN(...) printf(__VA_ARGS__)
#define INFO(...) printf(__VA_ARGS__)
#define DEBUG(...) printf(__VA_ARGS__)

#endif