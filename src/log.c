#include "log.h"

void LOG(enum log_level level, const char* fmt, ...)
{
	if (level < LOGLEVEL)
		return;

	switch (level) {
	case INFO:
		printf("INFO > ");
		break;
	case DEBUG:
		printf("DEBUG > ");
		break;
	case ERROR:
		printf("ERROR > ");
		break;
	case WARN:
		printf("WARN > ");
		break;
	default:
		printf("LOG > ");
	}
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");
	fflush(stdout);
}
