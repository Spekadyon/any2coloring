# any2coloring

Create pixel-art like coloring pages from any pictures.

## Description

This program allows to create coloring pages with a pixel-art look from any
picture supported by [G'MIC](http://gmic.eu/).

The low-resolution 'pixel-art' picture is obtained with G'MIC
[ministeck](https://de.wikipedia.org/wiki/Ministeck)
procedure, using a provided palette.

### Palette file

The colors used to index the coloring page are extracted from the palette file.
This file must have two columns, separated by a blank character (tab or space).
The first column contains the RGB code of the color, the second one a tag
printed on the picture and represents the "code" of the color.

`art_grip_aquarelle_36.csv` is an example for the Art GRIP Aquerelle 36 pencil
box from Faber-Castell (see
[here](http://www.fabercastell.com/service/color-charts)).
The number printed on each pencil is used as the color code.

## Installation

### Dependencies

* [G'MIC](http://gmic.eu/download.shtml), command line client, must be
  accessible from PATH
* [libtiff](http://www.libtiff.org/)
* [GLib](http://www.gtk.org/)
* [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) (not
  mandatory, but Makefile should be edited accordingly)

### Build

* If required, edit Makefile
* `make`

## Usage

`any2coloring -i input_picture.jpg -o output_picture.svg -s output_picture_solution.svg -p palette.csv`

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

