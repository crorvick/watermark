/* The MIT License (MIT)
 *
 * Copyright (c) 2014 Chris Rorvick
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "debug.h"

#include <stdarg.h>

static FILE *log_stream = NULL;
static int verbosity = WARN;

void set_verbosity(int v)
{
	verbosity = v;
}

int get_verbosity()
{
	return verbosity;
}

int bump_verbosity()
{
	return ++verbosity;
}

FILE *set_log_stream(FILE *log)
{
	FILE *tmp = log_stream;
	log_stream = log;

	return tmp;
}

static FILE *get_log_stream()
{
	if (log_stream == NULL)
		log_stream = stderr;

	return log_stream;
}

void logmsg(int level, const char *fmt, ...)
{
	va_list ap;

	if (level > verbosity)
		return;

	va_start(ap, fmt);
	vfprintf(get_log_stream(), fmt, ap);
	va_end(ap);
}
