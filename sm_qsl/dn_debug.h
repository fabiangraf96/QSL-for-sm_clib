/*
Copyright (c) 2016, Dust Networks. All rights reserved.

Debug macros (based on http://c.learncodethehardway.org/book/ex20.html ).

\license See attached DN_LICENSE.txt.
*/

#ifndef DN_DEBUG_H
#define DN_DEBUG_H

#include <stdio.h>
#include <errno.h>
#include <string.h>

/* Comment out this define to include debug messages */
#define NDEBUG

/* Comment out this define to include log messages */
//#define NLOG

#ifdef NDEBUG
#define debug(M, ...)	do {}while(0)
#else
#define debug(M, ...)	fprintf(stderr, "DEBUG %s:%d: " M "\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#ifdef NLOG
#define log_err(M, ...)		do {}while(0)
#define log_warn(M, ...)	do {}while(0)
#define log_info(M, ...)	do {}while(0)
#else
#define log_err(M, ...)		fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\r\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)
#define log_warn(M, ...)	fprintf(stderr, "[WARN] (%s:%d: errno: %s) " M "\r\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)
#define log_info(M, ...)	fprintf(stderr, "[INFO] (%s:%d) " M "\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#endif
