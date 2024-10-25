#ifndef NES_TOOLS_LOG_H
#define NES_TOOLS_LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define LOGLEVEL 0

enum log_level
{
	DEBUG = 0,
	ERROR,
	WARN,
	INFO,
};

void LOG(enum log_level level, const char* fmt, ...);

#endif // NES_TOOLS_LOG_H
