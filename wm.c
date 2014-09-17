#include <cairo/cairo.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

	fprintf(o, "usage: %s WIDTH HEIGHT [ INPUT ]\n", name);
}

int main(int argc, char *argv[])
{
	FILE *in = stdin;
	struct line_of_text *line, *lines;
	struct line_of_text *last;
	long image_width, image_height;
	char *p;

	while (1) {
		static const struct option long_opts[] = {
			{ 0, 0, 0, 0 }
		};

		int c, opt_idx = 0;

		c = getopt_long(argc, argv, "",
			long_opts, &opt_idx);

		if (c == -1)
			break;

		switch (c) {
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

	case 3:
		in = fopen(argv[optind + 2], "r");
		if (in == NULL) {
			perror("fopen");
			exit(1);
		}
		/* fall through */
	case 2:
		image_width = strtol(argv[optind + 0], &p, 0);
		if (*p != '\0') {
			fprintf(stderr, "error: invalid width: %s\n",
				argv[optind + 0]);
			exit(1);
		}
		image_height = strtol(argv[optind + 1], &p, 0);
		if (*p != '\0') {
			fprintf(stderr, "error: invalid height: %s\n",
				argv[optind + 1]);
			exit(1);
		}
		break;
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

		cairo_surface_write_to_png_stream(surface, &write_to_stdio_filp, stdout);
	}
}
