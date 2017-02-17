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
#include <vector>

#include <cstdio>
#include <cstring>

#include <CImg.h>
#include <gmic.h>

#include "any2col.hpp"

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

void coloring2svg(struct Coloring const &coloring, struct col_opt const &opts, std::string &svg, bool soluce)
{
	// clear svg
	svg.clear();
	// SVG header
	svg += "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
	svg += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"" + std::to_string(opts.page.width) + "mm\" height=\""+ std::to_string(opts.page.height) + "mm\">\n";

	// Image offset // SVG crap, don't work with units...
	//svg += "\t<g transform=\"translate(" + std::to_string(opts.margin.left) + "mm, " + std::to_string(opts.margin.top) + "mm)\">\n";
	double offset_x, offset_y;
	offset_x = (opts.page.width + opts.margin.left - opts.margin.right)/2.0 - coloring.picture.width()*opts.px_size / 2.0;
	offset_y = (opts.page.height + opts.margin.top - opts.margin.bottom)/2.0 - coloring.picture.height()*opts.px_size / 2.0;
	// -> draw the page layout (margins + pixelized picture...) best
	// compromise is to align the center of the expected picture and the
	// pixelized one

	// SVG body
	for (int y = 0; y < coloring.picture.height(); y += 1) {
		for (int x = 0; x < coloring.picture.width(); x += 1) {
			char cpp_crap[7]; // Or maybe I should add libQtCore as a dep just to have QString::asprintf...
			svg += "\t\t<rect x=\"" + std::to_string(offset_x + x*opts.px_size) + "mm\" y=\"" + std::to_string(offset_y + y*opts.px_size) + "mm\" width=\"" + std::to_string(opts.px_size) + "mm\" height=\"" + std::to_string(opts.px_size) + "mm\" ";
			if (soluce) {
				snprintf(cpp_crap, 7, "%02hhX%02hhX%02hhX",
					 coloring.palette.at(coloring.picture(x, y, 0)).rgb.R,
					 coloring.palette.at(coloring.picture(x, y, 0)).rgb.G,
					 coloring.palette.at(coloring.picture(x, y, 0)).rgb.B);
			} else {
				std::memcpy(cpp_crap, "FFFFFF", sizeof(cpp_crap));
			}
			svg += "fill=\"#";
			svg += cpp_crap;
			svg += "\" stroke-width=\"" + std::to_string(opts.px_size/12.0) + "mm\" stroke=\"grey\" />\n";

			svg += "\t\t<text x=\"" + std::to_string(offset_x + (x+0.5)*opts.px_size) + "mm\" y=\"" + std::to_string(offset_y + (y+1.0-0.2)*opts.px_size) + "mm\" font-family=\"DejaVu Sans\" font-size=\"" + std::to_string(opts.px_size*0.6) + "mm\" fill=\"grey\" style=\"text-anchor:middle;\">";
			svg += coloring.palette.at(coloring.picture(x, y, 0)).name.c_str();
			svg += "</text>\n";
		}
	}
	// SVG end
	//svg += "\t</g>\n";
	svg += "</svg>\n";
}
