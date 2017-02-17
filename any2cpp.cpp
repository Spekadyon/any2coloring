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
#include <vector>

#include <cstdint>
#include <cstdlib>

#include <CImg.h>
#include <gmic.h>

using namespace std;
using namespace cimg_library;

struct color {
	struct {
		uint8_t R;
		uint8_t G;
		uint8_t B;
	} rgb;
	string name;
};

void read_palette(const char *filename, vector<struct color> &palette)
{
	ifstream fp;
	string str_colrgb;
	string str_colname;

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


int main(int argc, char *argv[])
{
	vector<struct color> palette;
	CImg<float> cimg_palette;
	CImg<float> cimg_original;
	CImgList<float> cimg_list;
	gmic gmic_obj;
	CImgList<char> cimg_names;
	string gmic_cmdline;
	double px_size;

	if (argc != 4) {
		cerr << "Usage:" << endl;
		cerr << "\t" << argv[0] << " palette.csv picture.jpg px_size" << endl;
		exit(EXIT_FAILURE);
	}

	read_palette(argv[1], palette);
	cimg_palette.assign(palette.size(), 1, 1, 3);
	for (size_t i = 0; i < palette.size(); i += 1) {
		cimg_palette(i, 0, 0, 0) = palette[i].rgb.R;
		cimg_palette(i, 0, 0, 1) = palette[i].rgb.G;
		cimg_palette(i, 0, 0, 2) = palette[i].rgb.B;
	}

	cimg_original.load(argv[2]);

	px_size = stof(argv[3]);

	cimg_list.push_back(cimg_original);
	cimg_list.push_back(cimg_palette);

	gmic_cmdline =
		"-l[0] "
		"-if {w>h} "
		"-rotate 90 "
		"-endif "
		"-if {h/w>297.0/210.0} "
		"-r2dy {int(297.0/" + to_string(px_size) +")} "
		"-else "
		"-r2dx {int(210.0/" + to_string(px_size) + ")} "
		"-endif "
		"-endl "
		"-index.. .,1 --map[0] [1] -rm..";

	gmic_obj.run(gmic_cmdline.c_str(), cimg_list, cimg_names);

	cout << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
		"<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"210mm\" height=\"297mm\">\n";

	double offset_x, offset_y;
	offset_x = (210.0 - cimg_list[0].width() * px_size) / 2.0;
	offset_y = (297.0 - cimg_list[0].height() * px_size) / 2.0;

	for (int y = 0; y < cimg_list[0].height(); y += 1) {
		for (int x = 0; x < cimg_list[0].width(); x += 1) {
			fprintf(stdout, "\t<rect x=\"%gmm\" y=\"%gmm\" width=\"%gmm\" height=\"%gmm\" fill=\"#%02hhX%02hhX%02hhX\" stroke-width=\"%gmm\" stroke=\"grey\" />\n",
				offset_x + x*px_size, offset_y + y*px_size, px_size, px_size, palette.at(cimg_list[0](x, y, 0)).rgb.R,
				palette.at(cimg_list[0](x, y, 0)).rgb.G, palette.at(cimg_list[0](x, y, 0)).rgb.B,
				px_size/10.0);
			fprintf(stdout, "\t<text x=\"%gmm\" y=\"%gmm\" font-family=\"DejaVu Sans\" font-size=\"%gmm\" fill=\"grey\" style=\"text-anchor:middle\">",
				offset_x + (x+0.5)*px_size, offset_y + (y+1)*px_size - px_size*(0.1+0.1), px_size*0.6);
			fprintf(stdout, "%s", palette.at(cimg_list[0](x, y, 0)).name.c_str());
			fprintf(stdout, "</text>\n");
		}
	}
	cout << "</svg>\n";

	cimg_original.display();

	return EXIT_SUCCESS;
}


