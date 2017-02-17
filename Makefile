CXX?=g++
CXXFLAGS?=-Og -march=native -ggdb -std=c++14
LDFLAGS?=-Wl,-O1

LDFLAGS+=-lX11 -pthread -lgmic

SRC=any2col.cpp libany2col.cpp
HEAD=any2col.hpp
OBJ=$(SRC:.cpp=.o)

.PHONY: clean strip all

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

