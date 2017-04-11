CXX?=g++
CXXFLAGS?=-O2 -march=native -g2
LDFLAGS?=-Wl,-O1

LDFLAGS+=-lX11 -pthread -lgmic
CXXFLAGS+=-std=c++14

.PHONY: clean strip all utils

all: any2col
utils: display_palette

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_ADD) -Wall -Wextra $<

ANY2COL_SRC=any2col.cpp libany2col.cpp
ANY2COL_OBJ=$(ANY2COL_SRC:.cpp=.o)

any2col: CXXFLAGS+=`pkg-config --cflags cairo-svg cairo-pdf`
any2col: LDFLAGS+=`pkg-config --libs cairo-svg cairo-pdf`
any2col: $(ANY2COL_OBJ)
	$(CXX) -o $@ $(CXXFLAGS) $(ANY2COL_OBJ) $(LDFLAGS)

any2col.o: any2col.hpp
any2col.o: CXXFLAGS+=

libany2col.o: any2col.hpp
libany2col.o: CXXFLAGS+=`pkg-config --cflags cairo-svg cairo-pdf`

display_palette: CXXFLAGS+=`pkg-config --cflags cairo-svg cairo-pdf`
display_palette: LDFLAGS+=`pkg-config --libs cairo-svg cairo-pdf`
display_palette: display_palette.o libany2col.o
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^

display_palette.o: any2col.hpp
display_palette.o: CXXFLAGS+=

clean:
	rm -f $(ANY2COL_OBJ) any2col display_palette display_palette.o

strip:
	strip --strip-unneeded --preserve-dates -R .comment any2col

