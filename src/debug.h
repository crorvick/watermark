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

#ifndef DEBUG_H_included
#define DEBUG_H_included

void set_verbosity(int v);
int get_verbosity();
int bump_verbosity();

#define QUIET 0
#define ERROR 1
#define WARN  2
#define INFO  3
#define DEBUG 4

void logmsg(int level, const char *fmt, ...);

#define error(fmt, args...) logmsg(ERROR, "error: " fmt, args);
#define warn(fmt, args...)  logmsg(WARN,  "warn: "  fmt, args);
#define info(fmt, args...)  logmsg(INFO,  "info: "  fmt, args);
#define debug(fmt, args...) logmsg(DEBUG, "debug: " fmt, args);

#endif  /* DEBUG_H_included */
