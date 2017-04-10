/*
 * Copyright 2017 - Geoffrey Brun <geoffrey@spekadyon.org>
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

#include <iostream>
#include <fstream>

#include <cstdlib>

#include "any2col.hpp"

using namespace std;
using namespace cimg_library;

// getopt()
#include <unistd.h>

void help_exit(char *str)
{
	fprintf(stdout, "Usage:\n");
	fprintf(stdout, "\t%s [options] -p palette.csv -i <input_image> -o <coloring.svg>\n\n", basename(str));
	fprintf(stdout, "Options:\n");
	fprintf(stdout, "\t-h                  print help and exit\n");
	fprintf(stdout, "\t-p <palette.csv>    color palette (mandatory)\n");
	fprintf(stdout, "\t-i <input.png>      input picture (mandatory)\n");
	fprintf(stdout, "\t-o <output.svg>     coloring output, as a svg file\n");
	fprintf(stdout, "\t-a <output.pdf>     coloring output, pdf format\n");
	fprintf(stdout, "\t-x <pixel size>     size of one pixel, in millimeters (default: 2.0)\n");
	fprintf(stdout, "\t-w <page width>     page width in millimeters (default: 210.0)\n");
	fprintf(stdout, "\t-f <page height>    page height in millimeters (default: 297.0)\n");
	fprintf(stdout, "\t-t <top margin>     top page margin in millimeters (all margins default to 5.0)\n");
	fprintf(stdout, "\t-b <bottom margin>  bottom page margin\n");
	fprintf(stdout, "\t-l <left margin>    left page margin\n");
	fprintf(stdout, "\t-r <right margin>   right page margin\n");
	fprintf(stdout, "\t-s                  print colored outputs, instead of numbered ones\n");

	exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
	// Main vars
	struct Coloring coloring;
	struct col_opt opts;
	string svg;

	/*
	 * Default opt values
	 */
	// Once again... cpp crap...
	// Default options (A4 paper, 5 mm margins, 2 mm pixel size)
	opts.page.width    = 210.0;
	opts.page.height   = 297.0;
	opts.margin.top    = 5.0;
	opts.margin.bottom = 5.0;
	opts.margin.right  = 5.0;
	opts.margin.left   = 5.0;
	opts.px_size       = 2.0;

	/*
	 * command line parsing
	 */
	int opt;
	char *palette_file = NULL;
	char *input_file = NULL;;
	char *output_svg = NULL;
	char *output_pdf = NULL;
	bool need_colors = false;

	while ( (opt = getopt(argc, argv, "hp:i:o:a:x:w:f:t:b:l:r:s")) != -1 ) {
		string str;
		double convstr;
		switch (opt) {
		case 'h':
			help_exit(argv[0]);
			break;
		case 'p':
			palette_file = optarg;
			break;
		case 'i':
			input_file = optarg;
			break;
		case 'o':
			output_svg = optarg;
			break;
		case 'a':
			output_pdf = optarg;
			break;
		case 'x':
			str = optarg;
			convstr = stod(str);
			if (convstr <= 0) {
				fprintf(stderr, "Invalid pixel size (must be positive)\n");
				exit(EXIT_FAILURE);
			}
			opts.px_size = convstr;
			break;
		case 'w':
			str = optarg;
			convstr = stod(str);
			if (convstr <= 0) {
				fprintf(stderr, "Page width must be positive\n");
				exit(EXIT_FAILURE);
			}
			opts.page.width = convstr;
			break;
		case 'f':
			str = optarg;
			convstr = stod(str);
			if (convstr <= 0) {
				fprintf(stderr, "Page height must be positive\n");
				exit(EXIT_FAILURE);
			}
			opts.page.height = convstr;
			break;
		case 't':
			str = optarg;
			convstr = stod(str);
			if (convstr < 0) {
				fprintf(stderr, "Top margin must be positive\n");
				exit(EXIT_FAILURE);
			}
			opts.margin.top = convstr;
			break;
		case 'b':
			str = optarg;
			convstr = stod(str);
			if (convstr < 0) {
				fprintf(stderr, "Bottom margin must be positive\n");
				exit(EXIT_FAILURE);
			}
			opts.margin.bottom = convstr;
			break;
		case 'l':
			str = optarg;
			convstr = stod(str);
			if (convstr < 0) {
				fprintf(stderr, "Left margin must be positive\n");
				exit(EXIT_FAILURE);
			}
			opts.margin.left = convstr;
			break;
		case 'r':
			str = optarg;
			convstr = stod(str);
			if (convstr < 0) {
				fprintf(stderr, "Right margin must be positive\n");
				exit(EXIT_FAILURE);
			}
			opts.margin.right = convstr;
			break;
		case 's':
			need_colors = true;
			break;
		default:
			fprintf(stderr, "Unhandled option: %c\n", (char)opt);
		}
	}
	// Check for missing mandatory options
	int missing_opts = 0;
	if (palette_file == NULL) {
		fprintf(stderr, "Palette file is missing\n");
		missing_opts += 1;
	}
	if (input_file == NULL) {
		fprintf(stderr, "Input file is missing\n");
		missing_opts += 1;
	}
	if ( (output_svg == NULL) && (output_pdf == NULL) ) {
		fprintf(stderr, "No output file: nothing to do.\n");
		missing_opts += 1;
	}
	if (missing_opts) {
		fprintf(stderr, "Aborting\n");
		exit(EXIT_FAILURE);
	}

	make_coloring(palette_file, input_file, opts, coloring);
	if (output_pdf)
		coloring2pdf(output_pdf, coloring, opts, need_colors);
	if (output_svg)
		coloring2svg(output_svg, coloring, opts, need_colors);

#if 0
	coloring2svg(coloring, opts, svg, need_colors);

	if (output_svg) {
		ofstream fp;
		fp.open(output_svg, ofstream::out | ofstream::trunc);
		fp << svg;
		fp.close();
	}


	if (output_pdf) {
		// SVG to  PDF conversion! (powa, toussa...)
		RsvgHandle *svgHandle;
		GError *gerror = NULL;
		RsvgDimensionData dimensions;
		cairo_surface_t *cairoSurface;
		cairo_t *cairoContext;
		cairo_status_t cairoStatus;
		bool ret;

		svgHandle = rsvg_handle_new_with_flags(RSVG_HANDLE_FLAG_UNLIMITED);
		ret = rsvg_handle_write(svgHandle,
					(const unsigned char *)svg.c_str(),
					strlen(svg.c_str()), &gerror);
		if (!ret) {
			fprintf(stderr, "RSvg is unable to load svg: %s\n",
				gerror->message);
			exit(EXIT_FAILURE);
		}
		gerror = NULL;
		ret = rsvg_handle_close(svgHandle, &gerror);
		if (!ret) {
			fprintf(stderr, "RSvg is unable to load svg: %s\n",
				gerror->message);
			exit(EXIT_FAILURE);
		}

		rsvg_handle_get_dimensions(svgHandle, &dimensions);
		fprintf(stderr, "SVG dimensions: %d x %d\n", dimensions.width,
			dimensions.height);

		cairoSurface = cairo_pdf_surface_create(output_pdf, 595, 842);
		//cairoSurface = cairo_pdf_surface_create(output_pdf, dimensions.width,
							//dimensions.height);
		cairoContext = cairo_create(cairoSurface);
		rsvg_handle_render_cairo(svgHandle, cairoContext);
		if ( (cairoStatus = cairo_status(cairoContext)) ) {
			fprintf(stderr, "Unable to convert svg to pdf: %s\n",
				cairo_status_to_string(cairoStatus));
			exit(EXIT_FAILURE);
		}

		cairo_destroy(cairoContext);
		cairo_surface_destroy(cairoSurface);
		// Cleanup
		g_object_unref(svgHandle);
	}
#endif

	return EXIT_SUCCESS;
}


