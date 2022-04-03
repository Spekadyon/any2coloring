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

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QTimer>

#include <QDebug>

#include "any2col.hpp"

void printMissingOption(char const *str)
{
    fprintf(stderr, "%s: \"%s\"\n",
            qPrintable(QCoreApplication::translate("main", "Mandatory option missing")),
            str);
}

int main(int argc, char *argv[])
{
    // QApp
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("PixArtGen");
    QCoreApplication::setApplicationVersion("0.0.1");
    QLocale locale;

    // Options
    struct Coloring coloring;
    struct col_opt opts;
    QString paletteFile;
    QString inputFile;
    QString outputFile;
    bool needColour = false;

    // Command line parsing
    bool mandatoryOptionsMissing = false;
    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Pixel art and colouring generator"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOptions({
                          // color palette (mandatory)
                          {{"p", "palette"},
                           QCoreApplication::translate("main", "Set color palette to <palette.csv> (mandatory)"),
                           QCoreApplication::translate("main", "palette.csv")},
                          // Input file (mandatory)
                          {{"i", "input"},
                           QCoreApplication::translate("main", "Input picture (mandatory)"),
                           QCoreApplication::translate("main", "file")},
                          // Output file (mandatory)
                          {{"o", "output"},
                           QCoreApplication::translate("main", "Output file (mandatory)"),
                           QCoreApplication::translate("main", "file")},
                          // Output line color
                          {{"g", "line-color"},
                           QCoreApplication::translate("main", "Line color, 0-255 integer gray level (default: 176)"),
                           QCoreApplication::translate("main", "integer")},
                          // Output text color
                          {{"k", "text-color"},
                           QCoreApplication::translate("main", "Text color, 0-255 integer gray level (default: 80)"),
                           QCoreApplication::translate("main", "integer")},
                          // Pixel size (mm)
                          {{"x", "pixel-size"},
                           QCoreApplication::translate("main", "Pixel size, in millimeters (default: 2mm)"),
                           QCoreApplication::translate("main", "float")},
                          // Page width
                          {{"w", "page-width"},
                           QCoreApplication::translate("main", "Page width in millimeters (defaults to A4 width: 210mm)"),
                           QCoreApplication::translate("main", "float")},
                          // Page height
                          {{"f", "page-height"},
                           QCoreApplication::translate("main", "Page height in millimeters (defaults to A4 height: 297mm)"),
                           QCoreApplication::translate("main", "float")},
                          // Page top margin
                          {{"t", "margin-top"},
                           QCoreApplication::translate("main", "Page top margin  (default: 5mm)"),
                           QCoreApplication::translate("main", "float")},
                          // Page left margin
                          {{"l", "margin-left"},
                           QCoreApplication::translate("main", "Page left margin (default: 5mm)"),
                           QCoreApplication::translate("main", "float")},
                          // Page right margin
                          {{"r", "margin-right"},
                           QCoreApplication::translate("main", "Page right margin (default: 5mm)"),
                           QCoreApplication::translate("main", "float")},
                          // Bottom margin
                          {{"b", "margin-bottom"},
                           QCoreApplication::translate("main", "Page bottom margin (default: 5 mm)"),
                           QCoreApplication::translate("main", "float")},
                          // Coloured output
                          {{"c", "color-output"},
                           QCoreApplication::translate("main", "Coloured output")}
                      });

    // Parse command line
    parser.process(app);

    if (!parser.isSet("palette")) {
        printMissingOption("palette");
        mandatoryOptionsMissing = true;
    }
    if (!parser.isSet("input")) {
        printMissingOption("input");
        mandatoryOptionsMissing = true;
    }
    if (!parser.isSet("output")) {
        printMissingOption("output");
        mandatoryOptionsMissing = true;
    }

    if (mandatoryOptionsMissing) {
        exit(EXIT_FAILURE);
    }

    // Retrieve parameters from command line
    paletteFile = parser.value("palette");
    inputFile = parser.value("input");
    outputFile = parser.value("output");
    if (parser.isSet("line-color")) {
        QString str = parser.value("line-color");
        bool ok;
        int value = locale.toInt(str, &ok);
        if (!ok) {
            fprintf(stderr, "%s: %s\n",
                    qPrintable(QCoreApplication::translate("main", "Invalid line color value, can't be converted to an int")),
                    qPrintable(str));
            exit(EXIT_FAILURE);
        }
        if (value < 0 || value > 255) {
            fprintf(stderr, "%s: %d\n",
                    qPrintable(QCoreApplication::translate("main", "Invalid range for line color (valid range: 0-255)")),
                    value);
            exit(EXIT_FAILURE);
        }
        opts.lineColor = value;
    } else {
        opts.lineColor = 176;
    }
    if (parser.isSet("text-color")) {
        QString str = parser.value("text-color");
        bool ok;
        int value = locale.toInt(str, &ok);
        if (!ok) {
            fprintf(stderr, "%s: %s\n",
                    qPrintable(QCoreApplication::translate("main", "Invalid text color value")),
                    qPrintable(str));
            exit(EXIT_FAILURE);
        }
        if (value < 0 || value > 255) {
            fprintf(stderr, "%s: %d\n",
                    qPrintable(QCoreApplication::translate("main", "Invalid range for text color (expected: 0-255):")),
                    value);
            exit(EXIT_FAILURE);
        }
        opts.textColor = value;
    } else {
        opts.textColor = 80;
    }
    if (parser.isSet("page-width")) {
        QString str = parser.value("page-width");
        bool ok;
        double value = locale.toDouble(str, &ok);
        if (!ok) {
            fprintf(stderr, "%s: %s\n",
                    qPrintable(QCoreApplication::translate("main", "Invalid page width value")),
                    qPrintable(str));
            exit(EXIT_FAILURE);
        }
        if (value <= 0) {
            fprintf(stderr, "%s: %f\n",
                    qPrintable(QCoreApplication::translate("main", "Page width must be positive")),
                    value);
            exit(EXIT_FAILURE);
        }
        opts.page.width = value;
    } else {
        opts.page.width = 210;
    }
    if (parser.isSet("page-height")) {
        QString str = parser.value("page-height");
        bool ok;
        double value = locale.toDouble(str, &ok);
        if (!ok) {
            fprintf(stderr, "%s: %s\n",
                    qPrintable(QCoreApplication::translate("main", "Invalid page height value")),
                    qPrintable(str));
            exit(EXIT_FAILURE);
        }
        if (value <= 0) {
            fprintf(stderr, "%s: %f\n",
                    qPrintable(QCoreApplication::translate("main", "Page height must be positive")),
                    value);
            exit(EXIT_FAILURE);
        }
        opts.page.height = value;
    } else {
        opts.page.height = 297;
    }
    if (parser.isSet("margin-top")) {
        QString str = parser.value("margin-top");
        bool ok;
        double value = locale.toDouble(str, &ok);
        if (!ok) {
            fprintf(stderr, "%s: %s\n",
                    qPrintable(QCoreApplication::translate("main", "Invalid top margin value")),
                    qPrintable(str));
            exit(EXIT_FAILURE);
        }
        if (value <= 0) {
            fprintf(stderr, "%s: %f\n",
                    qPrintable(QCoreApplication::translate("main", "Top margin must be positive")),
                    value);
            exit(EXIT_FAILURE);
        }
        if (value >= opts.page.height) {
            fprintf(stderr, "%s\n",
                    qPrintable(QCoreApplication::translate("main", "Top margin must be smaller than page height")));
            exit(EXIT_FAILURE);
        }
        opts.margin.top = value;
    } else {
        opts.margin.top = 5;
    }
    if (parser.isSet("margin-bottom")) {
        QString str = parser.value("margin-bottom");
        bool ok;
        double value = locale.toDouble(str, &ok);
        if (!ok) {
            fprintf(stderr, "%s: %s\n",
                    qPrintable(QCoreApplication::translate("main", "Invalid bottom margin value")),
                    qPrintable(str));
            exit(EXIT_FAILURE);
        }
        if (value <= 0) {
            fprintf(stderr, "%s\n",
                    qPrintable(QCoreApplication::translate("main", "Bottom margin must be positive")));
            exit(EXIT_FAILURE);
        }
        if (value >= opts.page.height - opts.margin.top) {
            fprintf(stderr, "%s\n",
                    qPrintable(QCoreApplication::translate("main", "Top and bottom margins are bigger than page height")));
            exit(EXIT_FAILURE);
        }
        opts.margin.bottom = value;
    } else {
        opts.margin.bottom = 5;
    }
    if (parser.isSet("margin-left")) {
        QString str = parser.value("margin-left");
        bool ok;
        double value = locale.toDouble(str, &ok);
        if (!ok) {
            fprintf(stderr, "%s: %s\n",
                    qPrintable(QCoreApplication::translate("main", "Invalid left margin value")),
                    qPrintable(str));
            exit(EXIT_FAILURE);
        }
        if (value <= 0) {
            fprintf(stderr, "%s\n",
                    qPrintable(QCoreApplication::translate("main", "Left margin must be positive")));
            exit(EXIT_FAILURE);
        }
        if (value >= opts.page.width) {
            fprintf(stderr, "%s\n",
                    qPrintable(QCoreApplication::translate("main", "Left margin must be smaller than the page width")));
            exit(EXIT_FAILURE);
        }
        opts.margin.left = value;
    } else {
        opts.margin.left = 5;
    }
    if (parser.isSet("margin-right")) {
        QString str = parser.value("margin-right");
        bool ok;
        double value = locale.toDouble(str, &ok);
        if (!ok) {
            fprintf(stderr, "%s: %s\n",
                    qPrintable(QCoreApplication::translate("main", "Invalid left margin value")),
                    qPrintable(str));
            exit(EXIT_FAILURE);
        }
        if (value <= 0) {
            fprintf(stderr, "%s\n",
                    qPrintable(QCoreApplication::translate("main", "Bottom margin must be positive")));
            exit(EXIT_FAILURE);
        }
        if (value >= opts.page.width - opts.margin.left) {
            fprintf(stderr, "%s\n",
                    qPrintable(QCoreApplication::translate("main", "Left and right margins exceed page width")));
            exit(EXIT_FAILURE);
        }
        opts.margin.right = value;
    } else {
        opts.margin.right = 5;
    }
    if (parser.isSet("pixel-size")) {
        QString str = parser.value("pixel-size");
        bool ok;
        double value = locale.toDouble(str, &ok);
        if (!ok) {
            fprintf(stderr, "%s: %s\n",
                    qPrintable(QCoreApplication::translate("main", "Invalid pixel size")),
                    qPrintable(str));
            exit(EXIT_FAILURE);
        }
        if (value <= 0) {
            fprintf(stderr, "%s\n",
                    qPrintable(QCoreApplication::translate("main", "Pixel size must be positive")));
            exit(EXIT_FAILURE);
        }
        if (value >= opts.page.height - opts.margin.top - opts.margin.bottom
                || value >= opts.page.width - opts.margin.left - opts.margin.right) {
            fprintf(stderr, "%s\n",
                    qPrintable(QCoreApplication::translate("main", "Pixel size exceeds available width and/or height")));
            exit(EXIT_FAILURE);
        }
        opts.px_size = value;
    } else {
        opts.px_size = 2;
    }
    if (parser.isSet("color-output")) {
        needColour = true;
    } else {
        needColour = false;
    }

    make_coloring(paletteFile.toLocal8Bit().constData(),
                  inputFile.toLocal8Bit().constData(),
                  opts,
                  coloring);
    coloring2pdf(outputFile.toLocal8Bit().constData(),
                 coloring,
                 opts,
                 needColour);

	QTimer::singleShot(0, QCoreApplication::instance(), SLOT(quit()));

    return app.exec();
}

