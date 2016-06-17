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
 * any2colçoring. If not, see <http://www.gnu.org/licenses/>.
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <search.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <tiffio.h>

struct color_s {
	uint8_t R;
	uint8_t G;
	uint8_t B;
	uint8_t A;
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


/*
 * Prototypes
 */

void help_die(const char *str);
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
 * (really) dirty, depends on Faber's palette, to be improved
 */

void hash_add_entry(const char *key, intptr_t number)
{
	ENTRY entry;

	entry.key = strdup(key);
	if (entry.key == NULL) {
		fprintf(stderr, "Unable to duplicate string: %s\n",
			strerror(errno));
		exit(EXIT_FAILURE);
	}
	entry.data = (void *)number;
	if (hsearch(entry, ENTER) == NULL) {
		fprintf(stderr, "Unable to insert %s => %d in the hash map: "
			"%s\n", key, (int)number, strerror(errno));
		exit(EXIT_FAILURE);
	}
}


/*
 * Help
 */

void help_die(const char *str)
{
	fprintf(stderr, "Usage\n");
	fprintf(stderr, "\t%s image.png [output.svg]\n", str);
	exit(EXIT_FAILURE);
}


/*
 * Pixel-related functions
 */

bool pixel_compare(pixel A, pixel B)
{
	if ( (A.color.R == B.color.R) &&
	     (A.color.G == B.color.G) &&
	     (A.color.B == B.color.B) &&
	     (A.color.A == B.color.A) ) {
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
	pixel black = { .color.R = 0, .color.G = 0, .color.B = 0,
		.color.A = 0xFF, .checked = true };

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
	if (argc < 2)
		help_die(argv[0]);

	if (access(argv[1], R_OK) == -1) {
		fprintf(stderr, "Unable to access file %s: %s\n", argv[1],
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	/*
	 * Output file
	 */
	FILE *fp;

	if (argc == 3) {
		if (access(argv[2], F_OK) == 0) {
			fprintf(stderr, "File %s already exists, aborting\n",
				argv[2]);
			exit(EXIT_FAILURE);
		}
		fp = fopen(argv[2], "w");
		if (fp == NULL) {
			fprintf(stderr, "Unable to write to file %s: %s\n",
				argv[2], strerror(errno));
			exit(EXIT_FAILURE);
		}
	} else {
		fp = stdout;
	}

	/*
	 * TIFF/Open/Read/etc.
	 */

	picture *pic;
	uint32_t width, height;
	uint32_t *buffer;
	int err;

	TIFF *tiffp;
	tiffp = TIFFOpen(argv[1], "r");
	if (tiffp == NULL) {
		fprintf(stderr, "Unable to open %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	if (!TIFFGetField(tiffp, TIFFTAG_IMAGEWIDTH, &width)) {
		fprintf(stderr, "Unable to retrieve picture width\n");
		exit(EXIT_FAILURE);
	}
	if (!TIFFGetField(tiffp, TIFFTAG_IMAGELENGTH, &height)) {
		fprintf(stderr, "Unable to retrieve picture height\n");
		exit(EXIT_FAILURE);
	}

	if ( (buffer = malloc(width * height * sizeof(uint32_t))) == NULL ) {
		fprintf(stderr, "Unable to allocate temporary memory for "
			"picture buffer: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	err = TIFFReadRGBAImage(tiffp, width, height, buffer, 0);
	if (err == 0) {
		fprintf(stderr, "Unable to read picture %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	TIFFClose(tiffp);

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

	free(buffer);

	/* Color hash management */
	// 128 >> 36 crayons de couleur
	if (hcreate(128) == 0) {
		fprintf(stderr, "Unable to create hash map: %s\n",
			strerror(errno));
		exit(EXIT_FAILURE);
	}
	hash_add_entry("#ffffff", 101);
	hash_add_entry("#f0f150", 104);
	hash_add_entry("#fcd31d", 107);
	hash_add_entry("#f89c20", 109);
	hash_add_entry("#f36627", 115);
	hash_add_entry("#e8373f", 118);
	hash_add_entry("#ef2e49", 121);
	hash_add_entry("#da1e45", 126);
	hash_add_entry("#8f3b60", 133);
	hash_add_entry("#c4248b", 125);
	hash_add_entry("#da6dbd", 119);
	hash_add_entry("#9f36a0", 134);
	hash_add_entry("#5447a0", 137);
	hash_add_entry("#4546a2", 141);
	hash_add_entry("#b3a6da", 139);
	hash_add_entry("#386cbd", 151);
	hash_add_entry("#2865b9", 143);
	hash_add_entry("#80d6f7", 147);
	hash_add_entry("#519cdb", 152);
	hash_add_entry("#3467ba", 144);
	hash_add_entry("#75d1da", 154);
	hash_add_entry("#4dc085", 162);
	hash_add_entry("#29b46f", 163);
	hash_add_entry("#6ec84e", 166);
	hash_add_entry("#25685f", 158);
	hash_add_entry("#467745", 167);
	hash_add_entry("#6c8b65", 172);
	hash_add_entry("#654f4c", 176);
	hash_add_entry("#8e3742", 192);
	hash_add_entry("#d58c51", 187);
	hash_add_entry("#fab792", 132);
	hash_add_entry("#cbd2d9", 230);
	hash_add_entry("#757779", 273);
	hash_add_entry("#13100f", 199);
	hash_add_entry("#a5b1bc", 251);
	hash_add_entry("#cabf7b", 250);


	/*
	 * Image parsing
	 */

	GArray *pattern_list;
	pattern_list = g_array_new(FALSE, FALSE, sizeof(pattern));

	for (size_t y = 0; y < height; y += 1) {
		for (size_t x = 0; x < width; x += 1) {
			// Direction initiale == horizontale vers la droite
			point direction = {1, 0};
			point position = {x, y};

			// Si le pixel est noir, on passe
			if (pixel_isblack(picture_get_pixel(pic, position)))
				continue;

			// Correction : absolument pas inutile, permet de
			// traiter le cas de structures imbriquées !
			// Recherche le pixel inférieur gauche (SO) de la
			// structure == origine
			while ( (! pixel_isblack(picture_get_pixel(pic, (point){position.x-1, position.y}))) ||
				(! pixel_isblack(picture_get_pixel(pic, (point){position.x, position.y - 1}))) ) {
				if (! pixel_isblack(picture_get_pixel(pic, (point){position.x-1, position.y})) )
					position.x -= 1;
				if (! pixel_isblack(picture_get_pixel(pic, (point){position.x, position.y-1})) )
					position.y -= 1;
			}

			// Si le pixel a déjà été marqué, on passe
			if (picture_get_pixel(pic, position).checked)
				continue;

			pattern pattern_current;
			pattern_current.points = g_array_new(FALSE, FALSE, sizeof(point));


			point edge_dir;
			point edge_pos;
			edge_dir.x = -direction.x + direction.y;
			edge_dir.y = -direction.x - direction.y;
			edge_pos = point_next(position, edge_dir);
			g_array_append_val(pattern_current.points, edge_pos);
			pattern_current.color = picture_get_pixel(pic, position).color;

			for(;;) {
				pixel pixel_current = picture_get_pixel(pic, position);
				if (pixel_current.checked)
					break;

				point pt_neighbour;
				point dir_neighbour;
				pixel px_neighbour;

				// Si voisin à droite == pas noir, on tourne à
				// droite
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

		ENTRY enter_in, *enter_out;
		enter_in.key = color2str(pattern_current->color);
		enter_out = hsearch(enter_in, FIND);
		if (enter_out == NULL) {
			fprintf(stderr, "Unable to find matching color, default to ???\n");
			fprintf(fp, "???");
		} else {
			fprintf(fp, "%ld", (intptr_t)enter_out->data);
		}

		fprintf(fp, "</text>\n");
	}

	fprintf(fp, "</svg>\n");

	// output file close

	if (fp != stdout)
		fclose(fp);

	// free() !

	hdestroy();
	for (size_t i = 0; i < pattern_list->len; i += 1) {
		pattern *pat = &g_array_index(pattern_list, pattern, i);
		g_array_free(pat->points, TRUE);
	}
	g_array_free(pattern_list, TRUE);

	return EXIT_SUCCESS;
}
