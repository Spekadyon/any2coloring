#ifndef _ANY2COL_H_
#define _ANY2COL_H_

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

#include <QString>
#include <QVector>

#include <iostream>
#include <vector>

#include <cstdint>

#include <CImg.h>

struct color {
	struct {
		uint8_t R;
		uint8_t G;
		uint8_t B;
	} rgb;
	QString name;
};

struct col_opt {
	struct {
		double width;
		double height;
	} page;
	struct {
		double top;
		double bottom;
		double right;
		double left;
	} margin;
	double px_size;
	// Lines and text colors, as 0-255 gray level
	uint8_t lineColor;
	uint8_t textColor;
};

struct Coloring {
	cimg_library::CImg<float> picture;
	QVector<struct color> palette;
};


bool read_palette(const char *filename, QVector<struct color> &palette);
void palette2CImg(QVector<struct color> const &palette, cimg_library::CImg<float> &cimg_palette);
void make_coloring(const char *palette_csv_file, const char *original_picture, struct col_opt const &opts, struct Coloring &coloring);
void coloring2svg(const char *filename, struct Coloring const &coloring, struct col_opt const &opts, bool soluce = false);
void coloring2pdf(const char *filename, struct Coloring const &coloring, struct col_opt const &opts, bool soluce = false);

#endif /* _ANY2COL_H_ */
