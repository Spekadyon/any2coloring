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
	int num;
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


int palette_color2num(const GArray *pal, color clr)
{
	for (size_t i = 0; i < pal->len; i += 1) {
		palette *elt;

		elt = &g_array_index(pal, struct palette_s, i);

		if (color_compare(elt->color, clr)) {
			return elt->num;
		}
	}
	return -1;
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
		fprintf(stderr, "Unable to allocate memory for picture: %s\n",
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	pic->pixels = malloc(sizeof(pixel) * width * height);
	if (pic->pixels == NULL) {
		fprintf(stderr, "Unable to allocate memory for picture pixels:"
			" %s\n", strerror(errno));
		free(pic);
		exit(EXIT_FAILURE);
	}
	memset(pic->pixels, 0, sizeof(pixel) * width * height);

	pic->width = width;
	pic->height = height;

	return pic;
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
 * Other functions
 */

const char *color2str(color col)
{
	static char str[16];
	snprintf(str, sizeof(str), "#%02hhx%02hhx%02hhx",
		 col.R, col.G, col.B);
	return str;
}


/*
 * Main
 */

int main(int argc, char *argv[])
{
	FILE *fp;

	/*
	 * Opts
	 */

	char *opt_input = NULL;
	char *opt_output = NULL;
	char *opt_soluce = NULL;
	char *opt_palette = NULL;
	int svg_size = 88;
	bool opt_force = false;
	int opt;

	while ( (opt = getopt(argc, argv, "i:o:s:fp:w:h")) != -1 ) {
		char *ptr;
		long int val;
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

	/*
	 * Reading palette
	 * Must be a tab separated-value files, with hex color in the first
	 * color and unique numbers in the second one
	 */

	fp = fopen(opt_palette, "r");
	if (fp == NULL) {
		fprintf(stderr, "Unable to open palette file %s: %s\n", opt_palette, strerror(errno));
		exit(EXIT_FAILURE);
	}

	GArray *color_palette;
	color_palette = g_array_new(FALSE, FALSE, sizeof(palette));

	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	while ( (read = getline(&line, &len, fp)) != -1 ) {
		unsigned int color;
		int number;

		if (sscanf(line, "%x\t%d", &color, &number) == 2) {
			struct color_s col;
			col = (struct color_s) {
				.R = (color >> 16) & 0xFF,
				.G = (color >> 8 ) & 0xFF,
				.B = (color)       & 0xFF
			};
			palette tmp = {.color = col, .hex = color, .num = number};
			g_array_append_val(color_palette, tmp);
		}
	}

	fclose(fp);

	/*
	 * Generate palette file
	 */

	char palette_file[] = TMPNAME "XXXXXX";
	char gmic_output[] = TMPNAME "XXXXXX";
	int fd;

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

	fp = fdopen(fd, "w");
	if (fp == NULL) {
		fprintf(stderr, "Unable to fdopen() temp file: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	fprintf(fp, "P3\n");
	fprintf(fp, "%d 1\n", color_palette->len);
	fprintf(fp, "255\n");
	for (size_t i = 0; i < color_palette->len; i += 1) {
		palette *elt = &g_array_index(color_palette, palette, i);
		fprintf(fp, "%hhu %hhu %hhu\n", elt->color.R, elt->color.G, elt->color.B);
	}

	fclose(fp);
	close(fd);

	pid_t pid;
	if ( (pid = fork()) == -1 ) {
		fprintf(stderr, "Unable to fork(): %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (pid == 0) {
#define SIZE		"88"
#define PIECE_SZ	"8"
#define COMPLEXITY	"3"
#define RELIEF_AMP	"0"
#define RELIEF_SZ	"0"
#define OUTLINE		"1"
		char *str = NULL;
		char *svg_size_str1= NULL;
		char *svg_size_str2= NULL;
		int err;

		err = asprintf(&str, "tiff:%s,uchar", gmic_output);
		if (err == -1) {
			fprintf(stderr, "Unable to allocate memory for string: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		err = asprintf(&svg_size_str1, "{min(%d,w)}", svg_size);
		if (err == -1) {
			fprintf(stderr, "Unable to allocate memory for string: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		err = asprintf(&svg_size_str2, "{min(%d,h)}", svg_size);
		if (err == -1) {
			fprintf(stderr, "Unable to allocate memory for string: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		char * cmdline[] = {
			"gmic",
			"-input", opt_input,
			"-if", "{w>h}", "-r2dx", svg_size_str1 , "-else", "-r2dy", svg_size_str2, "-endif",
			"-split_opacity", "-l[0]",
			"-input", palette_file, "-index[-2]", "[-1],1",
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

	GArray *pattern_list;
	pattern_list = g_array_new(FALSE, FALSE, sizeof(pattern));

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

				// Si voisin à droite == pas noir, on tourne à
				// droite
				// If the next (neighbour) pixel on the right is
				// not black, we turn on the right
				dir_neighbour = point_rotate_right(direction);
				pt_neighbour = point_next(position, dir_neighbour);
				px_neighbour = picture_get_pixel(pic, pt_neighbour);
				if (!pixel_isblack(px_neighbour)) {
					// Le coin correspondant est à -3
					// pi/4... cf. dessin, je ne fais ici
					// que mettre sqrt(2) fois la matrice de
					// rotation correspondante
					point edge_dir;
					point edge_pos;
					edge_dir.x = -direction.x + direction.y;
					edge_dir.y = -direction.x - direction.y;
					edge_pos = point_next(position, edge_dir);
					g_array_append_val(pattern_current.points, edge_pos);

					direction = point_rotate_right(direction);
				}
				// SI le pixel en face est noir, on tourne à
				// droite == mur
				// Le coin est situé à -pi/4 de la direction
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

				/*printf("%d\t%d\n", position.x, position.y);*/
				dir_neighbour = point_rotate_right(direction);

				pixel_set_checked(pic, position, true);
				position = point_next(position, direction);
			}

			g_array_append_val(pattern_list, pattern_current);
		}
	}

	picture_free(pic);

	/*
	 * SVG output
	 */

	// Output files


	if (opt_output) {
		fp = fopen(opt_output, "w");
		if (fp == NULL) {
			fprintf(stderr, "Unabel to write to file %s: %s\n", opt_output,
				strerror(errno));
			exit(EXIT_FAILURE);
		}
	} else {
		fp = stdout;
	}

	// SVG !

	fprintf(fp, "<svg version=\"1.1\"\nbaseProfile=\"full\"\nwidth=\"300\" height=\"200\"\nxmlns=\"http://www.w3.org/2000/svg\">\n");

	for (size_t idx = 0; idx < pattern_list->len; idx += 1) {
		pattern *pattern_current;
		pattern_current = &g_array_index(pattern_list, pattern, idx);
		fprintf(fp, "\t<polygon style=\"fill: %s; stroke: lightgrey; stroke-width: .5px\" points=\"", "white");
		for (size_t edge = 0; edge < pattern_current->points->len; edge += 1) {
			point *pt = &g_array_index(pattern_current->points, point, edge);
			fprintf(fp, "%d %d, ", pt->x, pt->y);
		}
		point *pt = &g_array_index(pattern_current->points, point, 0);
		fprintf(fp, "%d %d", pt->x, pt->y);
		fprintf(fp, "\"/>\n");
		fprintf(fp, "\t<text x=\"%g\" y=\"%g\" font-size=\"4.75\" font-family=\"Nimbus Sans L\" style=\"fill: dimgray\">", pt->x-0.1, (double)(pt->y+7));
		fprintf(fp, "%d", palette_color2num(color_palette, pattern_current->color));

		fprintf(fp, "</text>\n");
	}

	fprintf(fp, "</svg>\n");

	// output file close

	if (fp != stdout)
		fclose(fp);

	// Soluce

	if (opt_soluce) {
		fp = fopen(opt_soluce, "w");
		if (fp == NULL) {
			fprintf(stderr, "Unabel to write to file %s: %s\n", opt_soluce, strerror(errno));
			exit(EXIT_FAILURE);
		}

		// SVG !

		fprintf(fp, "<svg version=\"1.1\"\nbaseProfile=\"full\"\nwidth=\"300\" height=\"200\"\nxmlns=\"http://www.w3.org/2000/svg\">\n");

		for (size_t idx = 0; idx < pattern_list->len; idx += 1) {
			pattern *pattern_current;
			pattern_current = &g_array_index(pattern_list, pattern, idx);
			fprintf(fp, "\t<polygon style=\"fill: %s; stroke: lightgrey; stroke-width: .5px\" points=\"", color2str(pattern_current->color));
			for (size_t edge = 0; edge < pattern_current->points->len; edge += 1) {
				point *pt = &g_array_index(pattern_current->points, point, edge);
				fprintf(fp, "%d %d, ", pt->x, pt->y);
			}
			point *pt = &g_array_index(pattern_current->points, point, 0);
			fprintf(fp, "%d %d", pt->x, pt->y);
			fprintf(fp, "\"/>\n");
			fprintf(fp, "\t<text x=\"%g\" y=\"%g\" font-size=\"4.75\" font-family=\"Nimbus Sans L\" style=\"fill: dimgray\">", pt->x-0.1, (double)(pt->y+7));

			fprintf(fp, "%d", palette_color2num(color_palette, pattern_current->color));
			fprintf(fp, "</text>\n");
		}

		fprintf(fp, "</svg>\n");

		// output file close

		fclose(fp);
	}

	// free() !

	for (size_t i = 0; i < pattern_list->len; i += 1) {
		pattern *pat = &g_array_index(pattern_list, pattern, i);
		g_array_free(pat->points, TRUE);
	}
	g_array_free(pattern_list, TRUE);
	g_array_free(color_palette, TRUE);

	return EXIT_SUCCESS;
}
