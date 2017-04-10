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

#include <fstream>
#include <stdexcept>
#include <vector>

#include <cstdio>
#include <cstring>

#include <CImg.h>
#include <gmic.h>

#include "any2col.hpp"

#include <cairo-pdf.h>
#include <cairo-svg.h>

inline double mm_to_pt(double mm)
{
	return mm / 25.4 * 72.0;
}

using namespace cimg_library;

void read_palette(const char *filename, std::vector<struct color> &palette)
{
	std::ifstream fp;
	std::string str_colrgb;
	std::string str_colname;

	fp.open(filename);

	while (fp >> str_colrgb >> str_colname) {
		color Color;
		uint32_t rawcolor;
		Color.name = str_colname;
		rawcolor = stoi(str_colrgb, NULL, 16);
		Color.rgb.R = rawcolor >> 2*8;
		Color.rgb.G = (uint32_t)(rawcolor >> 8) & (uint32_t)(0xFFU);
		Color.rgb.B = (uint32_t)(rawcolor) & (uint32_t)(0xFFU);
		palette.push_back(Color);
	}

	fp.close();
}

void palette2CImg(std::vector<struct color> const &palette, CImg<float> &cimg_palette)
{
	cimg_palette.assign(palette.size(), 1, 1, 3);
	for (size_t i = 0; i < palette.size(); i += 1) {
		cimg_palette(i, 0, 0, 0) = palette[i].rgb.R;
		cimg_palette(i, 0, 0, 1) = palette[i].rgb.G;
		cimg_palette(i, 0, 0, 2) = palette[i].rgb.B;
	}
}

void make_coloring(const char *palette_csv_file, const char *original_picture, struct col_opt const &opts, struct Coloring &coloring)
{
	CImgList<float> cimgList(2);
	CImgList<char> cimgNames(2);
	std::vector<struct color> palette;
	std::string gmic_cmdline;
	gmic gmic_obj;

	double pic_width = opts.page.width - opts.margin.right - opts.margin.left;
	double pic_height = opts.page.height - opts.margin.top - opts.margin.bottom;

	// Read palette from file
	read_palette(palette_csv_file, palette);
	palette2CImg(palette, cimgList[1]);

	// Read original picture
	cimgList[0].load(original_picture);

	// build cmdline
	gmic_cmdline =
		"-l[0] "
		"-if {w>h} -rotate 90 -endif "
		"-if {h/w>" + std::to_string(pic_height/pic_width) + "} "
		"-r2dy {int(" + std::to_string(pic_height/opts.px_size) + ")} "
		"-else "
		"-r2dx {int(" + std::to_string(pic_width/opts.px_size) + ")} "
		"-endif "
		"-endl "
		"-index.. .,1 --map[0] [1] -rm..";

	// Set names (useless?)
	cimgNames[1] = "palette";
	cimgNames[0] = "picture";


	gmic_obj.run(gmic_cmdline.c_str(), cimgList, cimgNames);

	coloring.picture = cimgList[0];
	coloring.palette = palette;
}

void coloring2pdf(const char *filename, struct Coloring const &coloring, struct col_opt const &opts, bool soluce)
{
	cairo_surface_t *cairoSurface;
	cairo_t *cairoContext;
	double offset_x, offset_y;

	offset_x = (opts.page.width + opts.margin.left - opts.margin.right)/2.0 - coloring.picture.width()*opts.px_size / 2.0;
	offset_y = (opts.page.height + opts.margin.top - opts.margin.bottom)/2.0 - coloring.picture.height()*opts.px_size / 2.0;

	cairoSurface = cairo_pdf_surface_create(filename, mm_to_pt(opts.page.width), mm_to_pt(opts.page.height));
	cairoContext = cairo_create(cairoSurface);

	if (soluce) {
		for (int y = 0; y < coloring.picture.height(); y += 1) {
			for (int x = 0; x < coloring.picture.width(); x += 1) {
				cairo_set_source_rgb(
						     cairoContext,
						     (double)coloring.palette.at(coloring.picture(x, y, 0)).rgb.R / 255.0,
						     (double)coloring.palette.at(coloring.picture(x, y, 0)).rgb.G / 255.0,
						     (double)coloring.palette.at(coloring.picture(x, y, 0)).rgb.B / 255.0
						    );
				cairo_rectangle(
						cairoContext,
						mm_to_pt(offset_x + x*opts.px_size),
						mm_to_pt(offset_y + y*opts.px_size),
						mm_to_pt(opts.px_size),
						mm_to_pt(opts.px_size)
					       );
				cairo_fill(cairoContext);
			}
		}
	} else {
		// stroke == light grey
		// FIXME: doesn't need to be hardcoded...
		cairo_set_source_rgb(cairoContext, 0xD3/255.0, 0xD3/255.0, 0xD3/255.0);
		for (int y = 0; y < coloring.picture.height(); y += 1) {
			for (int x = 0; x < coloring.picture.width(); x += 1) {
				// print square; stroke width doesn't need to be
				// hardcoded either
				cairo_set_line_width(cairoContext, mm_to_pt(opts.px_size/20.0));
				cairo_rectangle(
						cairoContext,
						mm_to_pt(offset_x + x*opts.px_size),
						mm_to_pt(offset_y + y*opts.px_size),
						mm_to_pt(opts.px_size),
						mm_to_pt(opts.px_size)
					       );
				cairo_stroke(cairoContext);

				cairo_text_extents_t te;
				cairo_select_font_face(cairoContext, "DejaVu Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
				cairo_set_font_size(cairoContext, mm_to_pt(opts.px_size*0.6));
				cairo_text_extents(
						   cairoContext,
						   coloring.palette.at(coloring.picture(x, y, 0)).name.c_str(),
						   &te
						  );
				cairo_move_to(
					      cairoContext,
					      mm_to_pt(offset_x + (x+0.5)*opts.px_size) - 0.5*te.width,
					      mm_to_pt(offset_y + (y+0.5)*opts.px_size) + 0.5*te.height
					     );
				cairo_show_text(cairoContext, coloring.palette.at(coloring.picture(x, y, 0)).name.c_str());

				cairo_fill(cairoContext);
			}
		}
	}

	cairo_surface_finish(cairoSurface);
	cairo_destroy(cairoContext);
	cairo_surface_destroy(cairoSurface);
}

void coloring2svg(const char *filename, struct Coloring const &coloring, struct col_opt const &opts, bool soluce)
{
	cairo_surface_t *cairoSurface;
	cairo_t *cairoContext;
	double offset_x, offset_y;

	offset_x = (opts.page.width + opts.margin.left - opts.margin.right)/2.0 - coloring.picture.width()*opts.px_size / 2.0;
	offset_y = (opts.page.height + opts.margin.top - opts.margin.bottom)/2.0 - coloring.picture.height()*opts.px_size / 2.0;

	cairoSurface = cairo_svg_surface_create(filename, mm_to_pt(opts.page.width), mm_to_pt(opts.page.height));
	cairoContext = cairo_create(cairoSurface);

	if (soluce) {
		for (int y = 0; y < coloring.picture.height(); y += 1) {
			for (int x = 0; x < coloring.picture.width(); x += 1) {
				cairo_set_source_rgb(
						     cairoContext,
						     (double)coloring.palette.at(coloring.picture(x, y, 0)).rgb.R / 255.0,
						     (double)coloring.palette.at(coloring.picture(x, y, 0)).rgb.G / 255.0,
						     (double)coloring.palette.at(coloring.picture(x, y, 0)).rgb.B / 255.0
						    );
				cairo_rectangle(
						cairoContext,
						mm_to_pt(offset_x + x*opts.px_size),
						mm_to_pt(offset_y + y*opts.px_size),
						mm_to_pt(opts.px_size),
						mm_to_pt(opts.px_size)
					       );
				cairo_fill(cairoContext);
			}
		}
	} else {
		// stroke == light grey
		// FIXME: doesn't need to be hardcoded...
		cairo_set_source_rgb(cairoContext, 0xD3/255.0, 0xD3/255.0, 0xD3/255.0);
		for (int y = 0; y < coloring.picture.height(); y += 1) {
			for (int x = 0; x < coloring.picture.width(); x += 1) {
				// print square; stroke width doesn't need to be
				// hardcoded either
				cairo_set_line_width(cairoContext, mm_to_pt(opts.px_size/20.0));
				cairo_rectangle(
						cairoContext,
						mm_to_pt(offset_x + x*opts.px_size),
						mm_to_pt(offset_y + y*opts.px_size),
						mm_to_pt(opts.px_size),
						mm_to_pt(opts.px_size)
					       );
				cairo_stroke(cairoContext);
			}
		}
	}

	cairo_surface_finish(cairoSurface);
	cairo_destroy(cairoContext);
	cairo_surface_destroy(cairoSurface);
}

