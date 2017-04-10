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

	if (argc != 2) {
		cerr << "Usage:" << endl;
		cerr << "\t" << argv[0] << " palette.csv" << endl;
		exit(EXIT_FAILURE);
	}

	read_palette(argv[1], palette);
	cimg_palette.assign(palette.size(), 1, 1, 3);
	for (size_t i = 0; i < palette.size(); i += 1) {
		cimg_palette(i, 0, 0, 0) = palette[i].rgb.R;
		cimg_palette(i, 0, 0, 1) = palette[i].rgb.G;
		cimg_palette(i, 0, 0, 2) = palette[i].rgb.B;
	}

	cimg_palette.display();


	return EXIT_SUCCESS;
}


