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
#include <QVector>

#include <cstdint>
#include <cstdlib>

#include <CImg.h>

#include "any2col.hpp"

using namespace std;
using namespace cimg_library;

int main(int argc, char *argv[])
{
	QVector<struct color> palette;
	CImg<float> cimg_palette;

	if (argc != 2) {
		cerr << "Usage:" << endl;
		cerr << "\t" << argv[0] << " palette.csv" << endl;
		exit(EXIT_FAILURE);
	}

	read_palette(argv[1], palette);
	palette2CImg(palette, cimg_palette);

	cimg_palette.display();


	return EXIT_SUCCESS;
}


