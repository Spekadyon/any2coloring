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

#include <vector>

#include <cstdlib>

#include "any2col.hpp"

using namespace std;
using namespace cimg_library;


int main(int argc, char *argv[])
{
	if (argc != 3)
		exit(EXIT_FAILURE);

	struct Coloring coloring;
	struct col_opt opts;
	string svg;

	// Once again... cpp crap...
	// Default options (A4 paper, 5 mm margins, 2 mm pixel size)
	opts.page.width    = 210.0;
	opts.page.height   = 297.0;
	opts.margin.top    = 5.0;
	opts.margin.bottom = 5.0;
	opts.margin.right  = 5.0;
	opts.margin.left   = 5.0;
	opts.px_size       = 2.0;

	make_coloring(argv[1], argv[2], opts, coloring);
	coloring2svg(coloring, opts, svg, true);

	cout << svg;

	return EXIT_SUCCESS;
}


