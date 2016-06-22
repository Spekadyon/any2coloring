/*
 * Copyright 2016 - Geoffrey Brun <geoffrey@spekadyon.org>
 *
 * This file is part of any2coloring.
 *
 * any2coloring is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * any2coloring is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * any2coloring. If not, see <http://www.gnu.org/licenses/>.
 */

// strdup(), getopt()
#define _POSIX_C_SOURCE 200809L
// asprintf
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <libgen.h>
#include <regex.h>
#include <search.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <tiffio.h>

#define TMPNAME	".any2coloring"

struct color_s {
	uint8_t R;
	uint8_t G;
	uint8_t B;
};

struct palette_s {
	struct color_s color;
	uint32_t hex;
	char *tag;
};

struct pixel_s {
	struct color_s color;
	bool checked;
};

struct picture_s {
	struct pixel_s *pixels;
	size_t width;
	size_t height;
};

struct point_s {
	int x;
	int y;
};

struct pattern_s {
	GArray *points;
	struct color_s color;
};


typedef struct color_s color;
typedef struct pixel_s pixel;
typedef struct picture_s picture;
typedef struct point_s point;
typedef struct pattern_s pattern;
typedef struct palette_s palette;


/*
 * Prototypes
 */

void help_die(char *str);
bool pixel_compare(pixel A, pixel B);
bool pixel_isblack(pixel A);
bool pixel_onedge(const picture *pic, point position);
void pixel_set_checked(picture *pic, point pt, bool flag);
point point_rotate_left(point pt);
point point_rotate_right(point pt);
point point_next(point pt, point direction);
bool point_compare(point A, point B);
pixel picture_get_pixel(const picture *pic, point pt);
picture *picture_allocate(size_t width, size_t height);
void picture_free(picture *pic);
const char *color2str(color col);
void strchomp(char *str);
void regex_err_print(int err, regex_t *regex);


/*
 * Help
 */

void help_die(char *str)
{
	fprintf(stderr, "Usage\n");
	fprintf(stderr, "\t%s [options] -i image.png -p palette.csv [-o output.svg]\n", basename(str));
	fprintf(stderr, "\n");
	fprintf(stderr, "Description\n");
	fprintf(stderr, "\tTransform any image supported by gmic into a coloring picture.\n");
	fprintf(stderr, "\tColors are indexed and numeroted using palette.csv.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Options\n");
	fprintf(stderr, "\t-f\t\toverwrite output file\n");
	fprintf(stderr, "\t-h\t\tprint this message\n");
	fprintf(stderr, "\t-i <arg>\tinput image file (mandatory)\n");
	fprintf(stderr, "\t-o <arg>\toutput file\n");
	fprintf(stderr, "\t-p <arg>\tpalette (colors, mandatory)\n");
	fprintf(stderr, "\t-t <double>\ttext size in the svg output (defaults to 5.5)\n");
	fprintf(stderr, "\t-w int\t\toutput image size (width or height)\n");
	exit(EXIT_FAILURE);
}

/*
 * Palette-related functions
 */

bool color_compare(color A, color B)
{
	if (
	    A.R == B.R &&
	    A.G == B.G &&
	    A.B == B.B
	   ) {
		return true;
	} else {
		return false;
	}
}


const char *palette_color2tag(const GArray *pal, color clr)
{
	for (size_t i = 0; i < pal->len; i += 1) {
		palette *elt;

		elt = &g_array_index(pal, struct palette_s, i);

		if (color_compare(elt->color, clr)) {
			return elt->tag;
		}
	}
	return NULL;
}

void palette_array_free(GArray *palette_array)
{
	for (size_t i = 0; i < palette_array->len; i += 1)
		free(g_array_index(palette_array, struct palette_s, i).tag);
	g_array_free(palette_array, TRUE);
}

/*
 * Reading palette
 * Must be a tab separated-value files, with hex color in the first
 * color and unique numbers in the second one
 */

GArray *palette_read(const char *str)
{
	GArray *color_palette = NULL;
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	int err;

	regex_t regex_compiled;
	regmatch_t regex_match[3];

	// Regex compilation
	err = regcomp(&regex_compiled, "^([0-9A-Fa-f]{6})\\s(\\w+)$", REG_EXTENDED);
	if (err) {
		regex_err_print(err, &regex_compiled);
		goto palette_read_regex;
	}

	// Palette open()
	fp = fopen(str, "r");
	if (fp == NULL) {
		fprintf(stderr, "Unable to open palette file %s: %s\n", str, strerror(errno));
		goto palette_read_open;
	}

	color_palette = g_array_new(FALSE, FALSE, sizeof(palette));

	if (color_palette == NULL) {
		fprintf(stderr, "Unable to allocate memory for the palette\n");
		goto palette_read_array_new;
	}

	// Palette read()
	while ( (read = getline(&line, &len, fp)) != -1 ) {
		unsigned int color;
		char *tag;
		char *endptr;
		/*ssize_t substr_len;*/

		strchomp(line);

		err = regexec(&regex_compiled, line, 3, regex_match, 0);
		if (err) {
			// err == REG_NOMATCH, cf. man page
			fprintf(stderr, "palette_read(): invalid line: %s\n", line);
			continue;
		}

		errno = 0;
		color = strtoul(line+regex_match[1].rm_so, &endptr, 16);
		if (errno) {
			fprintf(stderr, "Unable to extract color from line %s: %s\n", line, strerror(errno));
			continue;
		}
		if (endptr == line+regex_match[1].rm_so) {
			fprintf(stderr, "Unable to extract color from line %s\n", line);
			continue;
		}

		tag = strndup(line+regex_match[2].rm_so, regex_match[2].rm_eo - regex_match[2].rm_so);
		if (tag == NULL) {
			fprintf(stderr, "Unable to allocate memory for string: %s\n", strerror(errno));
			goto palette_read_str_malloc;
		}

		struct color_s col;
		col = (struct color_s) {
			.R = (color >> 16) & 0xFF,
			.G = (color >> 8 ) & 0xFF,
			.B = (color)       & 0xFF
		};
		palette tmp = {.color = col, .hex = color, .tag = tag };
		g_array_append_val(color_palette, tmp);
	}


	if (line)
		free(line);

	if (color_palette->len == 0) {
		fprintf(stderr, "Unable to read valid palette data from %s\n", str);
		g_array_free(color_palette, TRUE);
		color_palette = NULL;
	}

	fclose(fp);
	regfree(&regex_compiled);

	return color_palette;

palette_read_str_malloc:
	palette_array_free(color_palette);
palette_read_array_new:
	fclose(fp);
palette_read_open:
	regfree(&regex_compiled);
palette_read_regex:
	return NULL;
}

int palette_gen(const GArray *color_palette, int fd)
{
	FILE *fp;
	int err = 0;

	if ( (fp = fdopen(fd, "w")) == NULL ) {
		fprintf(stderr, "Unable to fdopen() temp file: %s\n", strerror(errno));
		err = -1;
		goto palette_gen_fdopen;
	}

	fprintf(fp, "P3\n");
	fprintf(fp, "%d 1\n", color_palette->len);
	fprintf(fp, "255\n");
	for (size_t i = 0; i < color_palette->len; i += 1) {
		palette *elt = &g_array_index(color_palette, palette, i);
		fprintf(fp, "%hhu %hhu %hhu\n", elt->color.R, elt->color.G, elt->color.B);
	}

	fclose(fp);

palette_gen_fdopen:
	return err;
}



/*
 * Pixel-related functions
 */

bool pixel_compare(pixel A, pixel B)
{
	if ( (A.color.R == B.color.R) &&
	     (A.color.G == B.color.G) &&
	     (A.color.B == B.color.B) ) {
		return true;
	} else {
		return false;
	}
}

bool pixel_isblack(pixel A)
{
	if (A.color.R == 0 && A.color.G == 0 && A.color.B == 0)
		return true;
	else
		return false;
}

bool pixel_onedge(const picture *pic, point position)
{
	point direction = {1, 0};
	point pt_neighbour;
	bool onedge = false;

	for (int i = 0; i < 4; i += 1) {
		pixel px;
		pt_neighbour = point_next(position, direction);
		px = picture_get_pixel(pic, pt_neighbour);
		if (pixel_isblack(px)) {
			onedge = true;
			break;
		}
		direction = point_rotate_left(direction);
	}

	return onedge;
}

void pixel_set_checked(picture *pic, point pt, bool flag)
{
	pic->pixels[pt.x + pt.y*pic->width].checked = flag;
}


/*
 * Point-related functions
 */

point point_rotate_left(point pt)
{
	return (point){.x = -pt.y, .y = pt.x};
}

point point_rotate_right(point pt)
{
	return (point){.x = pt.y, .y = -pt.x};
}

point point_next(point pt, point direction)
{
	return (point){pt.x + direction.x, pt.y + direction.y};
}

bool point_compare(point A, point B)
{
	if ( (A.x != B.x) || (A.y != B.y) )
		return false;
	else
		return true;
}


/*
 * Picture-related functions
 */

pixel picture_get_pixel(const picture *pic, point pt)
{
	pixel black = { .color.R = 0, .color.G = 0, .color.B = 0, .checked = true };

	if ( (pt.x < 0) || (pt.x >= (ssize_t)pic->width) ||
	     (pt.y < 0) || (pt.y >= (ssize_t)pic->height) ) {
		return black;
	} else {
		return pic->pixels[pt.x+pt.y*pic->width];
	}
}

picture *picture_allocate(size_t width, size_t height)
{
	picture *pic;

	pic = malloc(sizeof(picture));
	if (pic == NULL) {
		fprintf(stderr, "Unable to allocate memory for picture: %s\n", strerror(errno));
		goto picture_allocate_pic;
	}

	pic->pixels = malloc(sizeof(pixel) * width * height);
	if (pic->pixels == NULL) {
		fprintf(stderr, "Unable to allocate memory for picture pixels:"
			" %s\n", strerror(errno));
		goto picture_allocate_pixels;
	}
	memset(pic->pixels, 0, sizeof(pixel) * width * height);

	pic->width = width;
	pic->height = height;

	return pic;

picture_allocate_pixels:
	free(pic);
picture_allocate_pic:
	return NULL;
}

void picture_free(picture *pic)
{
	free(pic->pixels);
	free(pic);
}

picture *picture_read(const char *name)
{
	picture *pic = NULL;
	uint32_t width, height;
	uint32_t *buffer;
	int err;

	TIFF *tiffp;
	tiffp = TIFFOpen(name, "r");
	if (tiffp == NULL) {
		fprintf(stderr, "Unable to open %s\n", name);
		goto picture_read_tiff;
	}

	if (!TIFFGetField(tiffp, TIFFTAG_IMAGEWIDTH, &width)) {
		fprintf(stderr, "Unable to retrieve picture width\n");
		goto picture_read_getfield;
	}
	if (!TIFFGetField(tiffp, TIFFTAG_IMAGELENGTH, &height)) {
		fprintf(stderr, "Unable to retrieve picture height\n");
		goto picture_read_getfield;
	}

	if ( (buffer = malloc(width * height * sizeof(uint32_t))) == NULL ) {
		fprintf(stderr, "Unable to allocate temporary memory for "
			"picture buffer: %s\n", strerror(errno));
		goto picture_read_malloc;
	}

	err = TIFFReadRGBAImageOriented(tiffp, width, height, buffer, ORIENTATION_TOPLEFT, 0);
	if (err == 0) {
		fprintf(stderr, "Unable to read picture %s\n", name);
		goto picture_read_read;
	}

	TIFFClose(tiffp);
	tiffp = NULL;

	pic = picture_allocate(width, height);
	if (pic == NULL)
		goto picture_read_allocate;

	pic->width = width;
	pic->height = height;

	for (size_t y = 0; y < height; y += 1) {
		for (size_t x = 0; x < width; x += 1) {
			size_t idx = x + y*width;
			pic->pixels[idx].color.R = TIFFGetR(buffer[idx]);
			pic->pixels[idx].color.G = TIFFGetG(buffer[idx]);
			pic->pixels[idx].color.B = TIFFGetB(buffer[idx]);
		}
	}

picture_read_allocate:
picture_read_read:
	free(buffer);
picture_read_malloc:
picture_read_getfield:
	if (tiffp)
		TIFFClose(tiffp);
picture_read_tiff:
	return pic;

}

/*
 * Picture analysis functions
 */

GArray *picture_analysis(picture *pic)
{
	GArray *pattern_list;

	pattern_list = g_array_new(FALSE, FALSE, sizeof(pattern));
	if (pattern_list == NULL) {
		fprintf(stderr, "Unable to allocate memory for pattern_list\n");
		return NULL;
	}

	// gmic output: colored zoned oulined with black lines. The width of the
	// colored zones is at least 2 pixel large. The outline of each zone is
	// detected and recorded in "pattern_list".
	// pattern_list is an array of patterns: list of zone edges and color of
	// the zone
	//
	// Picture coordinates: origin == south west, horizintal axis == x
	// (row-major order)
	for (size_t y = 0; y < pic->height; y += 1) {
		for (size_t x = 0; x < pic->width; x += 1) {
			// Initial direction: x-axis (1, 0), position (x, y)
			point direction = {1, 0};
			point position = {x, y};

			// If the pixel is black, we are not in a colored zone:
			// next
			if (pixel_isblack(picture_get_pixel(pic, position)))
				continue;

			// Next, go to the origin of the zone
			while ( (! pixel_isblack(picture_get_pixel(pic, (point){position.x-1, position.y}))) ||
				(! pixel_isblack(picture_get_pixel(pic, (point){position.x, position.y - 1}))) ) {
				if (! pixel_isblack(picture_get_pixel(pic, (point){position.x-1, position.y})) )
					position.x -= 1;
				if (! pixel_isblack(picture_get_pixel(pic, (point){position.x, position.y-1})) )
					position.y -= 1;
			}

			// If the pixel at the zone origin is already marked as
			// checked, next one
			if (picture_get_pixel(pic, position).checked)
				continue;

			// We store the list of points (edges) in
			// pattern_current
			pattern pattern_current;
			pattern_current.points = g_array_new(FALSE, FALSE, sizeof(point));


			// Store the position of the first edge: S/W of the
			// origin
			point edge_dir;
			point edge_pos;
			// Direction == (-1, -1), or ( [1, -1], [-1, 1] ) . (1,
			// 0)
			edge_dir.x = -direction.x + direction.y;
			edge_dir.y = -direction.x - direction.y;
			edge_pos = point_next(position, edge_dir);
			g_array_append_val(pattern_current.points, edge_pos);
			// Store the color of the current zone
			pattern_current.color = picture_get_pixel(pic, position).color;

			// The vertices are followed along "direction", the
			// black line remaining on the righ (relative to pixel,
			// direction).

			for(;;) {
				pixel pixel_current = picture_get_pixel(pic, position);
				if (pixel_current.checked)
					break;

				point pt_neighbour;
				point dir_neighbour;
				pixel px_neighbour;

				// If the pixel on the right is not black → turn
				// right
				dir_neighbour = point_rotate_right(direction);
				pt_neighbour = point_next(position, dir_neighbour);
				px_neighbour = picture_get_pixel(pic, pt_neighbour);
				if (!pixel_isblack(px_neighbour)) {
					// Next vertex is located at an angle of
					// -3π/4 related to the current
					// direction. Using here the rotation
					// matrix ( [-1, -1], [1, -1] ) to
					// obtain its coordinates
					point edge_dir;
					point edge_pos;
					edge_dir.x = -direction.x + direction.y;
					edge_dir.y = -direction.x - direction.y;
					edge_pos = point_next(position, edge_dir);
					g_array_append_val(pattern_current.points, edge_pos);

					direction = point_rotate_right(direction);
				}
				// If next (forward) pixel is black, turnleft
				// (the black line is on the right)
				// Vertex is located at an angle of -π/4,
				// related to the current direction
				dir_neighbour = direction;
				pt_neighbour = point_next(position, dir_neighbour);
				px_neighbour = picture_get_pixel(pic, pt_neighbour);
				if (pixel_isblack(px_neighbour)) {
					point edge_dir;
					point edge_pos;
					edge_dir.x = direction.x + direction.y;
					edge_dir.y = -direction.x + direction.y;
					edge_pos = point_next(position, edge_dir);
					g_array_append_val(pattern_current.points, edge_pos);
					direction = point_rotate_left(direction);
				}

				dir_neighbour = point_rotate_right(direction);

				pixel_set_checked(pic, position, true);
				position = point_next(position, direction);
			}

			g_array_append_val(pattern_list, pattern_current);
		}
	}

	return pattern_list;
}

/*
 * SVG Generation functions
 */

int pattern_to_svg(const char *output, const GArray *pattern_list, const GArray *color_palette, bool soluce, double svg_text_size)
{
	FILE *fp;

	if (output) {
		fp = fopen(output, "w");
		if (fp == NULL) {
			fprintf(stderr, "Unabel to write to file %s: %s\n", output, strerror(errno));
			return -1;
		}
	} else if (!soluce) {
		fp = stdout;
	} else {
		return 0;
	}

	// SVG !

	fprintf(fp, "<svg version=\"1.1\"\nbaseProfile=\"full\"\nwidth=\"300\" height=\"200\"\nxmlns=\"http://www.w3.org/2000/svg\">\n");

	for (size_t idx = 0; idx < pattern_list->len; idx += 1) {
		pattern *pattern_current;
		pattern_current = &g_array_index(pattern_list, pattern, idx);
		fprintf(fp, "\t<polygon style=\"fill: %s; stroke: lightgrey; stroke-width: .5px\" points=\"", soluce ? color2str(pattern_current->color) : "white");
		for (size_t edge = 0; edge < pattern_current->points->len; edge += 1) {
			point *pt = &g_array_index(pattern_current->points, point, edge);
			fprintf(fp, "%d %d, ", pt->x, pt->y);
		}
		point *pt = &g_array_index(pattern_current->points, point, 0);
		fprintf(fp, "%d %d", pt->x, pt->y);
		fprintf(fp, "\"/>\n");
		fprintf(fp, "\t<text x=\"%g\" y=\"%g\" font-size=\"%g\" font-family=\"Nimbus Sans L\" style=\"fill: dimgray\">", (double)pt->x+0.75, (double)(pt->y+7), svg_text_size);
		fprintf(fp, "%s", palette_color2tag(color_palette, pattern_current->color));

		fprintf(fp, "</text>\n");
	}

	fprintf(fp, "</svg>\n");

	// output file close

	if (fp != stdout)
		fclose(fp);

	return 0;
}


/*
 * Other functions
 */

const char *color2str(color col)
{
	static char str[16];
	snprintf(str, sizeof(str), "#%02hhx%02hhx%02hhx",
		 col.R, col.G, col.B);
	return str;
}

void strchomp(char *str)
{
	while (str) {
		if (*str == '\n') {
			*str = '\0';
			break;
		}
		str++;
	}
}

int gmic_launch(char *input, char *output, char *palette, int svg_size)
{
#define PIECE_SZ	"8"
#define COMPLEXITY	"3"
#define RELIEF_AMP	"0"
#define RELIEF_SZ	"0"
#define OUTLINE		"1"
	char *str = NULL;
	char *svg_size_str1= NULL;
	char *svg_size_str2= NULL;
	int err;

	err = asprintf(&str, "tiff:%s,uchar", output);
	if (err == -1) {
		fprintf(stderr, "Unable to allocate memory for string: %s\n", strerror(errno));
		goto gmic_launch_err1;
	}

	err = asprintf(&svg_size_str1, "{min(%d,w)}", svg_size);
	if (err == -1) {
		fprintf(stderr, "Unable to allocate memory for string: %s\n", strerror(errno));
		goto gmic_launch_err2;
	}

	err = asprintf(&svg_size_str2, "{min(%d,h)}", svg_size);
	if (err == -1) {
		fprintf(stderr, "Unable to allocate memory for string: %s\n", strerror(errno));
		goto gmic_launch_err3;
	}

	char *cmdline[] = {
		"gmic",
		"-input", input,
		"-if", "{w>h}", "-r2dx", svg_size_str1 , "-else", "-r2dy", svg_size_str2, "-endif",
		"-split_opacity", "-l[0]",
		"-input", palette, "-index[-2]", "[-1],1",
		"[0],[0],1,1", "-rand[2]", "0,1", "-dilate[2]", COMPLEXITY, "-+[0,2]",
		"-r[0]", PIECE_SZ "\"\"00%," PIECE_SZ "\"\"00%",
		"--g[0]", "xy,1", "-!=[-2,-1]", "0", "--f[0]", "'i(x+1,y+1)-i(x,y)'", "-!=[-3--1]", "0", "-|[-3--1]",
		"-z[0,-1]", "0,0,{w-2},{h-2}",
		"-if", OUTLINE, "[-1]", "-endif",
		"--shift[-1]", "1,1", "-*[-2]", "-1", "-+[-2,-1]", "-b[-1]", "{"RELIEF_SZ  "*" PIECE_SZ "/5}", "-n[-1]", "-" RELIEF_AMP "," RELIEF_AMP,
		"-map[0]", "[1]", "-rm[1]", "-+[0,-1]",
		"-if", OUTLINE, "-==[1]", "0", "-*", "-endif",
		"-endl", "-r[-1]", "[0],[0],1,100%", "-a", "c",
		"-c", "0,255",
		"-output", str,
		NULL,
	};

	execvp(cmdline[0], cmdline);

	fprintf(stderr, "Unable to exec() gmic: %s\n", strerror(errno));

	free(svg_size_str2);
gmic_launch_err3:
	free(svg_size_str1);
gmic_launch_err2:
	free(str);
gmic_launch_err1:
	return -1;
}

void regex_err_print(int err, regex_t *regex)
{
	int len;
	char *str;

	len = regerror(err, regex, NULL, 0);
	str = malloc(len);

	if (str == NULL) {
		fprintf(stderr, "Unable to allocate memory for regex error message: %s\n", strerror(errno));
		return;
	}

	regerror(err, regex, str, len);
	fprintf(stderr, "Regex compile error: %s\n", str);
	free(str);

	return;
}

/*
 * Main
 */

int main(int argc, char *argv[])
{
	GArray *color_palette;
	GArray *pattern_list;
	char palette_file[] = TMPNAME "XXXXXX";
	char gmic_output[] = TMPNAME "XXXXXX";
	int fd;

	/*
	 * Opts
	 */

	char *opt_input = NULL;
	char *opt_output = NULL;
	char *opt_soluce = NULL;
	char *opt_palette = NULL;
	int svg_size = 88;
	double svg_text_size = 5.5;
	bool opt_force = false;
	int opt;

	while ( (opt = getopt(argc, argv, "i:o:s:fp:w:ht:")) != -1 ) {
		char *ptr;
		long int val;
		double dval;

		switch (opt) {
		case 'h':
			help_die(argv[0]);
			break;
		case 'i':
			opt_input = optarg;
			break;
		case 'o':
			opt_output = optarg;
			break;
		case 's':
			opt_soluce = optarg;
			break;
		case 'f':
			opt_force = true;
			break;
		case 'p':
			opt_palette = optarg;
			break;
		case 'w':
			errno = 0;
			val = strtol(optarg, &ptr, 0);
			if (ptr == optarg || errno == ERANGE || val == LONG_MAX || val == LONG_MIN || val < 0)
				fprintf(stderr, "Invalid picture size value: %s\n", optarg);
			else
				svg_size = val;
			break;
		case 't':
			errno = 0;
			dval = strtod(optarg, &ptr);
			if (ptr == optarg || errno == ERANGE || dval <= 0)
				fprintf(stderr, "Invalid text size: %s\n", optarg);
			else
				svg_text_size = dval;
			break;
		default:
			fprintf(stderr, "Unknown option: %c\n", opt);
		}
	}

	// mandatory options
	if (opt_input == NULL)
		help_die(argv[0]);
	if (opt_palette == NULL)
		help_die(argv[0]);
	// Testing the existence of the file with access() is not reliable as it
	// does not guarantee that the file is writeable
	if ( (opt_output != NULL) && (access(opt_output, F_OK) == 0) && !opt_force) {
		fprintf(stderr, "%s already exists, use option -f to overwrite\n", opt_output);
		exit(EXIT_FAILURE);
	}
	if ( (opt_soluce != NULL) && (access(opt_soluce, F_OK) == 0) && !opt_force) {
		fprintf(stderr, "%s already exists, use option -f to overwrite\n", opt_soluce);
		exit(EXIT_FAILURE);
	}

	// Read palette
	color_palette = palette_read(opt_palette);
	if (color_palette == NULL)
		exit(EXIT_FAILURE);

	// Generate temporary files
	fd = mkstemp(gmic_output);
	if (fd == 0) {
		fprintf(stderr, "Unable to create temp file: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	close(fd);

	fd = mkstemp(palette_file);
	if (fd == 0) {
		fprintf(stderr, "Unable to create temp file: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	};

	// Generate temporary palette file (ASCII pbm)
	if (palette_gen(color_palette, fd) == -1)
		exit(EXIT_FAILURE);

	close(fd);

	// Launch gmic (fork(), exec(), etc.)
	pid_t pid;
	if ( (pid = fork()) == -1 ) {
		fprintf(stderr, "Unable to fork(): %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (pid == 0) {
		// child
		gmic_launch(opt_input, gmic_output, palette_file, svg_size);
		// The previous function returns only on malloc() or exec() error
		exit(EXIT_FAILURE);
	}

	int status;
	waitpid(pid, &status, 0);
	if (WEXITSTATUS(status) != 0) {
		fprintf(stderr, "gmic return error %ld\n", (long int)WEXITSTATUS(status));
		exit(EXIT_FAILURE);
	}

	// Palette no longer required
	unlink(palette_file);


	/*
	 * TIFF/Open/Read/etc.
	 */

	picture *pic;

	pic = picture_read(gmic_output);
	if (pic == NULL)
		exit(EXIT_FAILURE);

	// picture temp file no longer required
	unlink(gmic_output);


	/*
	 * Image parsing
	 */

	pattern_list = picture_analysis(pic);
	if (pattern_list == NULL)
		exit(EXIT_FAILURE);

	picture_free(pic);

	/*
	 * SVG output
	 */

	if (pattern_to_svg(opt_output, pattern_list, color_palette, false, svg_text_size))
		exit(EXIT_FAILURE);
	if (opt_soluce)
		if (pattern_to_svg(opt_soluce, pattern_list, color_palette, true, svg_text_size))
			exit(EXIT_FAILURE);

	// Output files


	// free() !

	for (size_t i = 0; i < pattern_list->len; i += 1) {
		pattern *pat = &g_array_index(pattern_list, pattern, i);
		g_array_free(pat->points, TRUE);
	}
	g_array_free(pattern_list, TRUE);
	palette_array_free(color_palette);

	return EXIT_SUCCESS;
}
