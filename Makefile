CC?=gcc
CXX?=g++
CFLAGS?=-O3 -march=native -g3 -ggdb -std=c99 -flto
CXXFLAGS?=-Og -march=native -ggdb -std=c++14
LDFLAGS?=-Wl,-O1

LDFLAGS+=-lm

PKG_CONFIG_MODULES=glib-2.0 libtiff-4
CFLAGS+=`pkg-config --cflags $(PKG_CONFIG_MODULES)`
LDFLAGS+=`pkg-config --libs $(PKG_CONFIG_MODULES)`

SRC=any2coloring.c
HEAD=

BINARY=any2coloring
OBJS= $(SRC:.c=.o)

.PHONY: clean strip all

all: $(BINARY) any2cpp

%.o: %.c $(HEAD)
	$(CC) -c $(CFLAGS) -Wall -Wextra $<

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -Wall -Wextra $<

$(BINARY): $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(CFLAGS) $(OBJS)

any2cpp: any2cpp.o
	$(CXX) -o $@ $(CXXFLAGS) $< -lgmic -pthread -lX11

clean:
	rm -f $(OBJS) $(BINARY) any2cpp any2cpp.o

strip:
	strip --strip-unneeded --preserve-dates -R .comment $(BINARY)

