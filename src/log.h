/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 for more details (a copy is included
 * in the LICENSE file that accompanied this code).
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Copyright (c) 2017, Thomas Stibor <thomas@stibor.net>
 */

#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <linux/limits.h>

#define UNUSED(x) (void)(x)

#define NRM  "\x1B[0m"
#define RED  "\x1B[31m"
#define GRN  "\x1B[32m"
#define YEL  "\x1B[33m"
#define BLU  "\x1B[34m"
#define MAG  "\x1B[35m"
#define CYN  "\x1B[36m"
#define WHT  "\x1B[37m"
#define RESET "\033[0m"

/* Bottom three bits reserved for api_message_level. */
#define API_MSG_MASK		0x00000007
#define API_MSG_NO_ERRNO	0x00000010

enum api_message_level {
	API_MSG_OFF    = 0,
	API_MSG_FATAL  = 1,
	API_MSG_ERROR  = 2,
	API_MSG_WARN   = 3,
	API_MSG_NORMAL = 4,
	API_MSG_INFO   = 5,
	API_MSG_DEBUG  = 6,
	API_MSG_MAX
};

static unsigned int api_msg_level = API_MSG_MAX;

int api_msg_get_level(void)
{
	return api_msg_level;
}

void api_msg_set_level(int level)
{
        /* ensure level is in the good range */
        if (level < API_MSG_OFF)
                api_msg_level = API_MSG_OFF;
        else if (level > API_MSG_MAX)
                api_msg_level = API_MSG_MAX;
        else
                api_msg_level = level;
}

double ct_now(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec + 0.000001 * tv.tv_usec;
}

void __log(enum api_message_level level, int err, const char *fmt, va_list ap)
{
	vfprintf(stderr, fmt, ap);
	if (level & API_MSG_NO_ERRNO || !err)
		fprintf(stderr, "\n");
	else
		fprintf(stderr, ": %s (%d)\n", strerror(err), err);
}

void _log(enum api_message_level level, int err, const char *fmt, ...)
{
        if ((level & API_MSG_MASK) > api_msg_level)
                return;

	int tmp_errno = errno;
	va_list args;
	va_start(args, fmt);
	__log(level, abs(err), fmt, args);
        va_end(args);
	errno = tmp_errno;
}

#define C_ERROR(_rc, _format, ...)				\
	_log(API_MSG_ERROR, _rc,				\
	     RED "[ERROR] " RESET "%f [%ld] %s:%d "_format,	\
	     ct_now(), syscall(SYS_gettid), __FILE__, __LINE__,	\
	     ## __VA_ARGS__)

#define C_WARN(_format, ...)					\
	_log(API_MSG_WARN | API_MSG_NO_ERRNO, 0,		\
	     RED "[WARN] " RESET "%f [%ld] %s:%d "_format,	\
	     ct_now(), syscall(SYS_gettid), __FILE__, __LINE__,	\
	     ## __VA_ARGS__)

#define C_MESSAGE(_format, ...)					\
	_log(API_MSG_NORMAL | API_MSG_NO_ERRNO, 0,		\
	     MAG "[MESSAGE] " RESET "%f [%ld] %s:%d "_format,	\
	     ct_now(), syscall(SYS_gettid), __FILE__, __LINE__,	\
	     ## __VA_ARGS__)

#define C_INFO(_format, ...)					\
	_log(API_MSG_INFO | API_MSG_NO_ERRNO, 0,		\
	     YEL "[INFO] " RESET "%f [%ld] %s:%d "_format,	\
	     ct_now(), syscall(SYS_gettid), __FILE__, __LINE__,	\
	     ## __VA_ARGS__)

#define C_DEBUG(_format, ...)					\
	_log(API_MSG_DEBUG | API_MSG_NO_ERRNO, 0,		\
	     BLU "[DEBUG] " RESET "%f [%ld] %s:%d "_format,	\
	     ct_now(), syscall(SYS_gettid), __FILE__, __LINE__,	\
	     ## __VA_ARGS__)

#endif /* LOG_H */
