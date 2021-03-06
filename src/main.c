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
#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo.h>
#include <poppler.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
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

#define IMAGE_DPI 600

static cairo_t *
popplermain(const char *pdf_file)
{
    PopplerDocument *document;
    PopplerPage *page;
    double width, height;
    GError *error;
    gchar *absolute, *uri;
    int page_num, num_pages;
    cairo_surface_t *surface;
    cairo_t *cr;
    cairo_status_t status;

    /* printf("popplermain\n"); */

    page_num = 1;
    /*g_type_init (); */
    error = NULL;

    if (g_path_is_absolute(pdf_file)) {
        absolute = g_strdup (pdf_file);
    } else {
        gchar *dir = g_get_current_dir ();
        absolute = g_build_filename (dir, pdf_file, (gchar *) 0);
        free (dir);
    }

    uri = g_filename_to_uri (absolute, NULL, &error);
    free (absolute);
    if (uri == NULL) {
        printf("%s\n", error->message);
        return NULL;
    }

    document = poppler_document_new_from_file (uri, NULL, &error);
    if (document == NULL) {
        printf("%s\n", error->message);
        return NULL;
    }

    num_pages = poppler_document_get_n_pages (document);
    if (page_num < 1 || page_num > num_pages) {
        printf("page must be between 1 and %d\n", num_pages);
        return NULL;
    }

    page = poppler_document_get_page (document, page_num - 1);
    if (page == NULL) {
        printf("poppler fail: page not found\n");
        return NULL;
    }

    poppler_page_get_size (page, &width, &height);

    /* For correct rendering of PDF, the PDF is first rendered to a
     * transparent image (all alpha = 0). */
    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                          IMAGE_DPI*width/72.0,
                                          IMAGE_DPI*height/72.0);
    cr = cairo_create (surface);
    cairo_save (cr);
    cairo_scale (cr, IMAGE_DPI/72.0, IMAGE_DPI/72.0);
    cairo_save (cr);
    poppler_page_render (page, cr);
    cairo_restore (cr);
    g_object_unref (page);

    /* Then the image is painted on top of a white "page". Instead of
     * creating a second image, painting it white, then painting the
     * PDF image over it we can use the CAIRO_OPERATOR_DEST_OVER
     * operator to achieve the same effect with the one image. */
    cairo_set_operator (cr, CAIRO_OPERATOR_DEST_OVER);
    cairo_set_source_rgb (cr, 1, 1, 1);
    cairo_paint (cr);

    status = cairo_status(cr);
    if (status)
        printf("%s\n", cairo_status_to_string (status));

    /*cairo_destroy (cr);  */
    cairo_surface_destroy (surface);

    g_object_unref (document);
    cairo_restore (cr);

    return cr;
}


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
			error("invalid format: %s\n", prefix);
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
			error("invalid format: %s\n", prefix);
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
		error("invalid format: %s\n", prefix);
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
		size_t len;
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

static cairo_t *
create_cairo_context(const char *path)
{
	cairo_t *cr = NULL;
	cairo_surface_t *surface = NULL;
	GdkPixbuf *pixbuf = NULL;
	long w = 0, h = 0;
	char *p;

	w = strtol(path, &p, 10);
	if (*p == 'x')
		h = strtol(p+1, &p, 10);

	if (w <= 0 || h <= 0 || *p != '\0') {
		GdkPixbuf *tmp;
		GError *error = NULL;

		if ( (tmp = gdk_pixbuf_new_from_file(path, &error)) == NULL)
			goto fail;

		pixbuf = gdk_pixbuf_apply_embedded_orientation(tmp);

		g_object_unref(G_OBJECT(tmp));

		if (pixbuf != NULL) {
			w = gdk_pixbuf_get_width(pixbuf);
			h = gdk_pixbuf_get_height(pixbuf);
		}
	}

	if ( (surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h)) == NULL)
		goto fail;

	if ( (cr = cairo_create(surface)) == NULL)
		goto fail;

	if (pixbuf != NULL) {
		gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
	} else {
		cairo_set_source_rgb(cr, 0,0,0);
	}

	cairo_paint(cr);

fail:
	if (surface != NULL)
		cairo_surface_destroy(surface);
	if (pixbuf != NULL)
		g_object_unref(G_OBJECT(pixbuf));

	if (cr == NULL)  /* maybe its a PDF */
		cr = popplermain(path);

	return cr;
}

static gboolean
write_to_stdio_filp(const gchar *buf, gsize count, GError **error, gpointer data)
{
	return (fwrite(buf, 1, count, (FILE *) data) == count);
}

static void
write_image(cairo_t *cr, FILE *fp, const char *type, int rotate)
{
	cairo_surface_t *surface = cairo_get_target(cr);
	int w = cairo_image_surface_get_width(surface);
	int h = cairo_image_surface_get_height(surface);
	GdkPixbuf *pixbuf, *tmp;
	GError *error = NULL;

	unsigned char *src_data, *dst_data;
	int src_stride, dst_stride, row, col;

	if ( (pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, 1, 8, w, h)) == NULL)
		return;

	cairo_surface_flush(surface);

	src_data = cairo_image_surface_get_data(surface);
	dst_data = gdk_pixbuf_get_pixels(pixbuf);
	src_stride = cairo_image_surface_get_stride(surface);
	dst_stride = gdk_pixbuf_get_rowstride(pixbuf);

	for (row = 0; row < h; row++) {
		guint32 *src = (guint32 *) src_data;
		for (col = 0; col < w; col++) {
			dst_data[4 * col + 0] = (src[col] >> 16) & 0xff;
			dst_data[4 * col + 1] = (src[col] >>  8) & 0xff;
			dst_data[4 * col + 2] = (src[col] >>  0) & 0xff;
			dst_data[4 * col + 3] = (src[col] >> 24) & 0xff;
		}

		src_data += src_stride;
		dst_data += dst_stride;
	}

	if (rotate != 0) {
		tmp = gdk_pixbuf_rotate_simple(pixbuf, rotate);
		g_object_unref(G_OBJECT(pixbuf));
		pixbuf = tmp;
	}

	gdk_pixbuf_save_to_callback(pixbuf, &write_to_stdio_filp, fp, type, &error, NULL);
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
"  -t, --type FORMAT             use FORMAT when writing output file\n"
"  -r, --rotate DEGREES          rotate the watermark text by DEGREES\n"
"  -r, --orient ORIENTATION      write final image with ORIENTATION\n"
"  -l, --log FILE                send log output to FILE\n"
"  -v, --verbose                 increase verbosity\n"
"  -q, --quiet                   silence all log messages\n"
"  -h, --help                    display this message and exit\n"
"  -V, --version                 display version information and exit\n"
"\n"
"Report bugs to: " PACKAGE_BUGREPORT "\n"
		, name);
}

int main(int argc, char *argv[])
{
	const char *input_file = "-";
	const char *output_file = "-";
	const char *log_file = "-";
	FILE *in = stdin;
	FILE *out = stdout;
	const char *source_image;
	const char *output_type = "png";
	const char *rotate_spec = "none";
	const char *orient_spec = "none";
	struct line_of_text *line, *lines;
	struct line_of_text *last;
	long image_width, image_height;
	double aspect_ratio;
	double rotate = 0.0;
	int rotate_image = 0;

	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_font_options_t *fo;
        cairo_status_t status;

	while (1) {
		static const struct option long_opts[] = {
			{ "output",       required_argument, NULL, 'o' },
			{ "type",         required_argument, NULL, 't' },
			{ "rotate",       required_argument, NULL, 'r' },
			{ "orient",       required_argument, NULL, 'O' },
			{ "log",          required_argument, NULL, 'l' },
			{ "verbose",      required_argument, NULL, 'v' },
			{ "quiet",        required_argument, NULL, 'q' },
			{ "help",         no_argument,       NULL, 'h' },
			{ "version",      no_argument,       NULL, 'V' },
			{ 0, 0, 0, 0 }
		};

		int c, opt_idx = 0;

		c = getopt_long(argc, argv, "o:t:r:O:l:vqhV",
			long_opts, &opt_idx);

		if (c == -1)
			break;

		switch (c) {
		case 'o':
			output_file = optarg;
			break;

		case 't':
			output_type = optarg;
			break;

		case 'r':
			rotate_spec = optarg;
			break;

		case 'O':
			orient_spec = optarg;
			break;

		case 'l':
			log_file = optarg;
			break;

		case 'v':
			bump_verbosity();
			break;

		case 'q':
			set_verbosity(0);
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
		source_image = argv[optind + 0];
		break;
	}

	if (strcmp(input_file, "-") != 0) {
		in = fopen(input_file, "r");
		if (out == NULL) {
			error("cannot open input file (%s): %s\n",
				strerror(errno), input_file);
			exit(1);
		}
	}

	if (strcmp(output_file, "-") != 0) {
		out = fopen(output_file, "w");
		if (out == NULL) {
			error("cannot open output file (%s): %s\n",
				strerror(errno), output_file);
			exit(1);
		}
	}

	if (strcmp(log_file, "-") != 0) {
		FILE *log = fopen(log_file, "a");
		if (log == NULL) {
			error("cannot open log file (%s): %s\n",
				strerror(errno), log_file);
			exit(1);
		}
		set_log_stream(log);
	}

	cr = create_cairo_context(source_image);
	if (cr == NULL) {
		error("invalid geometry specification: %s\n", source_image);
		exit(1);
	}

	image_width = cairo_image_surface_get_width(cairo_get_target(cr));
	image_height = cairo_image_surface_get_height(cairo_get_target(cr));
	aspect_ratio = (double) image_width / image_height;

	if (strcmp(rotate_spec, "ldiag") == 0) {
		rotate = -atan(1/aspect_ratio);
	} else if (strcmp(rotate_spec, "rdiag") == 0) {
		rotate = atan(1/aspect_ratio);
	} else if (strcmp(rotate_spec, "none") != 0) {
		char *p;
		long degrees = strtol(rotate_spec, &p, 10);
		info("degrees => %d\n", degrees);
		if (*p == '\0')
			rotate = degrees * M_PI / 180.0;
		else
			warn("unknown rotation: %s\n", rotate_spec);
	}

	if (strcmp(orient_spec, "portrait") == 0) {
		rotate_image = (aspect_ratio > 1.05 ? 90 : 0);
	} else if (strcmp(orient_spec, "landscape") == 0) {
		rotate_image = (aspect_ratio < 0.95 ? 90 : 0);
	} else if (strcmp(orient_spec, "none") != 0) {
		warn("unknown orientation: %s\n", orient_spec);
	}

	/* use the standard Postscript typewriter font */
	cairo_select_font_face(cr, "Courier",
		CAIRO_FONT_SLANT_NORMAL,
		CAIRO_FONT_WEIGHT_BOLD);

	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.1);

	/* final output is a 1-bit TIFF; disable font aliasing */
	fo = cairo_font_options_create();
	cairo_get_font_options(cr, fo);
	cairo_font_options_set_antialias(fo, CAIRO_ANTIALIAS_NONE);
	cairo_set_font_options(cr, fo);
	cairo_font_options_destroy(fo);

	lines = get_lines(cr, in);

	cairo_translate(cr, image_width / 2.0, image_height / 2.0);
	cairo_rotate(cr, rotate);
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

		y = -text_height / 2.0;

		/* nullify the first ascent and use y-bearing instead */
		y -= lines->fe.ascent + lines->te.y_bearing;

		/* center each line at the desired size and render it */
		for (line = lines; line != NULL; line = line->next) {
			x = -line->te.width / 2.0;
			x -= line->te.x_bearing;
			y += line->fe.ascent;

			cairo_move_to(cr, x, y);
			cairo_set_font_size(cr, line->size);
			cairo_show_text(cr, line->text);
			/* printf("line: %s\n", line->text); */
		        status = cairo_status(cr);
		        if (status)
		    		printf("%s\n", cairo_status_to_string (status));

			y += line->fe.descent;
		}

		write_image(cr, out, output_type, rotate_image);
	}

	return EXIT_SUCCESS;
}
