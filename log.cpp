#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "log.h"

Log::Log()
{
    fp = NULL;
    level = 1;
}

Log::~Log()
{
	if (fp) fclose(fp);
}

void Log::Init()
{
 	char szLog[1024];
#ifdef WIN32
    strcpy(szLog, "lemonlauncher.log");
#else	
    sprintf(szLog, "%s/.lemonlauncher/lemonlauncher.log", getenv("HOME"));
#endif
	fp = fopen(szLog, "wt");
	if (fp == NULL) fprintf(stderr, "warning: can't create log file\n");

	level = 4;
}

void Log::SetLevel(int l)
{
	level = l;

	Log4("Log: log level set to %d\n", level);
}

int Log::Log1(const char* format, ...)
{
	if (1 > level || fp == NULL) return 0;

	va_list arglist;
	va_start(arglist, format);
	int res = vfprintf(fp, format, arglist);
	va_end(arglist);

	fflush(fp);
	return res;
}
int Log::Log2(const char* format, ...)
{
	if (2 > level || fp == NULL) return 0;

	va_list arglist;
	va_start(arglist, format);
	int res = vfprintf(fp, format, arglist);
	va_end(arglist);

	fflush(fp);
	return res;
}
int Log::Log3(const char* format, ...)
{
	if (3 > level || fp == NULL) return 0;

	va_list arglist;
	va_start(arglist, format);
	int res = vfprintf(fp, format, arglist);
	va_end(arglist);

	fflush(fp);
	return res;
}

int Log::Log4(const char* format, ...)
{
	if (4 > level || fp == NULL) return 0;

	va_list arglist;
	va_start(arglist, format);
	int res = vfprintf(fp, format, arglist);
	va_end(arglist);

	fflush(fp);
	return res;
}
