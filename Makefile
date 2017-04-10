CXX?=g++
CXXFLAGS?=-O2 -march=native -g2
LDFLAGS?=-Wl,-O1

LDFLAGS+=-lX11 -pthread -lgmic
CXXFLAGS+=-std=c++14

SRC=any2col.cpp libany2col.cpp
HEAD=any2col.hpp
OBJ=$(SRC:.cpp=.o)

.PHONY: clean strip all utils

CXXFLAGS_ALL= $(CXXFLAGS) `pkg-config --cflags cairo-svg cairo-pdf`
LDFLAGS+_ALL=$(CXXFLAGS) `pkg-config --libs cairo-svg cairo-pdf`

all: any2col

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -Wall -Wextra $<

any2col.o: any2col.hpp
libany2col.o: any2col.hpp

any2col: $(OBJ)
	$(CXX) -o $@ $(CXXFLAGS) $(OBJ) $(LDFLAGS)

clean:
	rm -f $(OBJ) any2col display_palette display_palette.o

strip:
	strip --strip-unneeded --preserve-dates -R .comment any2col

utils: display_palette

display_palette: display_palette.o
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

