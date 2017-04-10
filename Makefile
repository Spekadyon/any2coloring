CXX?=g++
CXXFLAGS?=-O2 -march=native -g2
LDFLAGS?=-Wl,-O1

LDFLAGS+=-lX11 -pthread -lgmic

SRC=any2col.cpp libany2col.cpp
HEAD=any2col.hpp
OBJ=$(SRC:.cpp=.o)

.PHONY: clean strip all

CXXFLAGS+= -std=c++14 `pkg-config --cflags cairo-svg cairo-pdf`
LDFLAGS+=`pkg-config --libs cairo-svg cairo-pdf`

all: any2col

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -Wall -Wextra $<

any2col.o: any2col.hpp
libany2col.o: any2col.hpp

any2col: $(OBJ)
	$(CXX) -o $@ $(CXXFLAGS) $(OBJ) $(LDFLAGS)

clean:
	rm -f $(OBJ) any2col

strip:
	strip --strip-unneeded --preserve-dates -R .comment any2col

