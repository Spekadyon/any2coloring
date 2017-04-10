# any2coloring

Create pixel-art like coloring pages from any pictures.


## Description

This program allows to create coloring pages with a pixel-art look from any
picture supported by [G'MIC](http://gmic.eu/).

The low-resolution 'pixel-art' picture is obtained with G'MIC, using a
[ministeck](https://de.wikipedia.org/wiki/Ministeck)-inspired procedure. The
colors used to map the picture are provided by the user.

### Palette file

The colors used to index the coloring page are extracted from the palette file.
This file must have two columns, separated by a blank character (tab or space).
The first column contains the RGB code of the color as a 6-digit hexadecimal
number (web-like formatted, without the leading #). The second column contains
a color identifier, printed on the pixel-art picture. Works best if the
identifier is made of one or two caracters.

`art_grip_aquarelle_36.csv` is an example for the Art GRIP Aquerelle 36 pencil
box from Faber-Castell (see
[here](http://www.fabercastell.com/service/color-charts)). The RGB values were
extracted from the pdf palette and may not reflect the real color of every
pencil... The last two numbers printed on each pencil are used as the color
code.


## Installation

### Dependencies

* [G'MIC](http://gmic.eu/download.shtml) as a library. The provided Makefile
  expects to find it in standard paths.
* [CImg](http://cimg.eu/) (is a build-time dependency of G'MIC)
* [Cairo](https://cairographics.org/) for SVG and PDF output
* [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/), used to
  automatically set CFLAGS/LDFLAGS for cairo-svg and cairo-pdf (not mandatory,
  but Makefile should be edited accordingly)

### Build

* If required, edit Makefile
* `make`

## Usage

### Typical usage

`any2col -i input_picture.jpg -o output_picture.svg -p palette.csv`

`any2col -i input_picture.jpg -s -o output_picture_soluce.svg -p palette.csv`

### Options

Unless specified, sizes are given in millimeters.

| Flag | Argument | Description |
| :---:| :---: | :--- |
| -h | none | print help and exit |
| -p | palette.csv | file providing the colors used to index the pixel-art picture (mandatory option) |
| -i | input.png | input file, can be any picture supported by CImg/G'MIC. If the file contains multiple pictures, like animated gif, only the first one is processed. (mandatory option) |
| -o | output.svg | output coloring as a SVG file (scalar vector graphics) |
| -a | output.pdf | output coloring as a PDF file |
| -x | pixel size | the size of one pixel, as rendered on the svg or pdf file |
| -w | page width | defaults to A4 width (210 mm) |
| -f | page height | defaults to A4 height (297 mm) |
| -t | top margin | minimal top margin, defaults to 5 mm. May be larger due to input file geometry |
| -b | bottom margin | minimal bottom margin, defaults to 5 mm. May be larger due to input file geometry |
| -l | left margin | minimal left margin, defaults to 5 mm. May be larger due to input file geometry |
| -r | right margin | minimal right margin, defaults to 5 mm. May be larger due to input file geometry |
| -s | none | colored outputs (color labels are replaced by the color they actually represents) |

Some parameters are hard-coded and may only be changed by recompiling the
program. This includes:
* the color of both the text (color IDs) and the grid;
* the thickness of the grid;
* the size of the text;
* the font.


## Example

### Original

<img src="https://raw.githubusercontent.com/Spekadyon/Spekadyon.github.io/master/images/rouge_gorge.jpg" alt="Rouge-gorge, original" width="640">

European robin, picture adapted from
[mediawiki](https://commons.wikimedia.org/wiki/File:European_Robin_%28erithacus_rubecula%29.JPG)
(Author: Charlesjsharp, license: [CC BY-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/legalcode))

### Processed pictures

| Coloring page | Answer |
| :---: | :---: |
| <img src="https://raw.githubusercontent.com/Spekadyon/Spekadyon.github.io/master/images/rouge_gorge.png" alt="Rouge-gorge, coloriage" width="480"> | <img src="https://raw.githubusercontent.com/Spekadyon/Spekadyon.github.io/master/images/rouge_gorge.soluce.png" alt="Rouge-gorge, solution" width="480"> |

(Both picture under license [CC BY-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/legalcode))

### Colored picture

<img src="https://raw.githubusercontent.com/Spekadyon/Spekadyon.github.io/master/images/rouge_gorge_colorie.jpg" alt="Rouge-gorge, colorié" width="640">

(Picture under license [CC BY-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/legalcode))

