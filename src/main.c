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

#include "config.h"

#include <cairo.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

struct line_of_text
{
	char text[BUFSIZ];

	double size;

	cairo_font_extents_t fe;
	cairo_text_extents_t te;

	struct line_of_text *next;
};

void
format_line(cairo_t *cr, struct line_of_text *line, const char *prefix)
{
	cairo_surface_t *surface = cairo_get_target(cr);
	double image_width = cairo_image_surface_get_width(surface);
	double v;
	char *p;
	cairo_text_extents_t te;


	switch (*prefix) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		v = strtol(prefix, &p, 10);
		if (*p != '\0') {
			fprintf(stderr, "error: invalid format: %s\n", prefix);
			exit(1);
		}
		line->size = v;
		break;

	case 'w':
		v = strtol(prefix+1, &p, 10);
		if (*p == '%') {
			v = (v / 100) * image_width;
			++p;
		}
		if (*p != '\0') {
			fprintf(stderr, "error: invalid format: %s\n", prefix);
			exit(1);
		}
		cairo_set_font_size(cr, 1000);
		cairo_text_extents(cr, line->text, &te);
		line->size = 1000 * (v / te.width);
		break;

	case '\0':
		line->size = 12;
		break;

	default:
		fprintf(stderr, "error: invalid format: %s\n", prefix);
		exit(1);
		break;
	}

	/* apply the desired size and fill in our font and text extents */
	cairo_set_font_size(cr, line->size);
	cairo_font_extents(cr, &line->fe);
	cairo_text_extents(cr, line->text, &line->te);
}

struct line_of_text *
get_lines(cairo_t *cr, FILE *fp)
{
	struct line_of_text *ret = NULL;
	struct line_of_text **next = &ret;

	while (1) {
		char buf[BUFSIZ];
		char *prefix = "";
		char *text;
		size_t i, len;
		struct line_of_text *line = NULL;

		if (fgets(buf, sizeof (buf), fp) == NULL)
			break;

		if (*buf == '#')
			continue;

		len = strlen(buf);
		while (len > 0 && isspace(buf[len - 1]))
			buf[--len] = '\0';

		text = strchr(buf, ':');
		if (text != NULL) {
			prefix = buf;
			*(text++) = '\0';
		} else
			text = buf;

		while (*text != '\0' && isspace(*text))
			text++;

		line = malloc(sizeof (struct line_of_text));
		memset(line, 0, sizeof (struct line_of_text));

		strncpy(line->text, text, sizeof (buf));
		format_line(cr, line, prefix);

		*next = line;
		next = &line->next;
	}

	return ret;
}

static cairo_status_t
write_to_stdio_filp(void *closure, const unsigned char *data, unsigned length)
{

	return fwrite(data, 1, length, (FILE *) closure) == length
		? CAIRO_STATUS_SUCCESS
		: CAIRO_STATUS_WRITE_ERROR;
}

void usage(FILE *o, const char *arg0)
{
	const char *name = strrchr(arg0, '/');
	if (name++ == NULL)
		name = arg0;

	fprintf(o,
"Usage: %s [OPTION]... WIDTHxHEIGHT [FILE]\n"
"Generate bitmask from input text to be used as a watermark.\n"
"\n"
"Options:\n"
"  -o, --output FILE             write output to FILE instead of stdout\n"
"  -h, --help                    display this message and exit\n"
"  -v, --version                 display version information and exit\n"
"\n"
"Report bugs to: " PACKAGE_BUGREPORT "\n"
		, name);
}

int main(int argc, char *argv[])
{
	const char *input_file = "-";
	const char *output_file = "-";
	FILE *in = stdin;
	FILE *out = stdout;
	const char *geometry;
	struct line_of_text *line, *lines;
	struct line_of_text *last;
	long image_width, image_height;
	char *p;

	while (1) {
		static const struct option long_opts[] = {
			{ "output",       required_argument, NULL, 'o' },
			{ "help",         no_argument,       NULL, 'h' },
			{ "version",      no_argument,       NULL, 'V' },
			{ 0, 0, 0, 0 }
		};

		int c, opt_idx = 0;

		c = getopt_long(argc, argv, "o:hV",
			long_opts, &opt_idx);

		if (c == -1)
			break;

		switch (c) {
		case 'o':
			output_file = optarg;
			break;

		case 'V':
			puts(PACKAGE_STRING);
			exit(0);
			break;

		case 'h':
			usage(stdout, argv[0]);
			exit(0);
			break;

		case '?':
			usage(stderr, argv[0]);
			exit(1);
			break;

		default:
			abort();
			break;
		}
	}

	switch (argc - optind) {
	default:
		usage(stderr, argv[0]);
		exit(1);
		break;

	case 2:
		input_file = argv[optind + 1];
	case 1:
		geometry = argv[optind + 0];
		break;
	}

	image_width = strtol(geometry, &p, 10);
	if (*p != 'x') {
		fprintf(stderr, "error: invalid geometry: %s\n", geometry);
		exit(1);
	}
	image_height = strtol(p+1, &p, 10);
	if (*p != '\0') {
		fprintf(stderr, "error: invalid geometry: %s\n", geometry);
		exit(1);
	}

	if (strcmp(input_file, "-") != 0) {
		in = fopen(input_file, "r");
		if (out == NULL) {
			fprintf(stderr, "error: cannot open input file (%s): %s\n",
				strerror(errno), input_file);
			exit(1);
		}
	}

	if (strcmp(output_file, "-") != 0) {
		out = fopen(output_file, "w");
		if (out == NULL) {
			fprintf(stderr, "error: cannot open output file (%s): %s\n",
				strerror(errno), output_file);
			exit(1);
		}
	}

	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_font_options_t *fo;

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, image_width, image_height);
	cr = cairo_create(surface);

	/* use the standart Postscript typewriter font */
	cairo_select_font_face(cr, "Courier",
		CAIRO_FONT_SLANT_NORMAL,
		CAIRO_FONT_WEIGHT_BOLD);

	/* final output is a 1-bit TIFF; disable font aliasing */
	fo = cairo_font_options_create();
	cairo_get_font_options(cr, fo);
	cairo_font_options_set_antialias(fo, CAIRO_ANTIALIAS_NONE);
	cairo_set_font_options(cr, fo);
	cairo_font_options_destroy(fo);

	lines = get_lines(cr, in);

	if (lines != NULL) {
		double text_height = 0.0;
		double x, y;

		/* calculate total height of text */
		for (line = lines; line != NULL; line = line->next) {
			text_height += line->fe.ascent + line->fe.descent;
			last = line;
		}

		/* trim off whitespace from top and bottom */
		text_height -= lines->fe.ascent;
		text_height -= last->fe.descent;
		text_height -= lines->te.y_bearing;
		text_height += last->te.height + last->te.y_bearing;

		y = (image_height - text_height) / 2;

		/* nullify the first ascent and use y-bearing instead */
		y -= lines->fe.ascent + lines->te.y_bearing;

		/* center each line at the desired size and render it */
		cairo_set_source_rgb(cr, 0, 0, 0);
		for (line = lines; line != NULL; line = line->next) {
			x = (image_width - line->te.width) / 2;
			x -= line->te.x_bearing;
			y += line->fe.ascent;

			cairo_move_to(cr, x, y);
			cairo_set_font_size(cr, line->size);
			cairo_show_text(cr, line->text);

			y += line->fe.descent;
		}

		cairo_surface_write_to_png_stream(surface, &write_to_stdio_filp, out);
	}
}
