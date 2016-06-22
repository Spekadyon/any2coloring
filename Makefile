CC?=gcc
CFLAGS?=-O3 -march=native -g3 -ggdb -std=c99 -fwhole-program
LDFLAGS?=-Wl,-O1 -g3 -ggdb

LDFLAGS+=-lm

PKG_CONFIG_MODULES=glib-2.0 libtiff-4
CFLAGS+=`pkg-config --cflags $(PKG_CONFIG_MODULES)`
LDFLAGS+=`pkg-config --libs $(PKG_CONFIG_MODULES)`

SRC=any2coloring.c
HEAD=

BINARY=any2coloring
OBJS= $(SRC:.c=.o)

.PHONY: clean strip all

all: $(BINARY)

%.o: %.c $(HEAD)
	$(CC) -c $(CFLAGS) -Wall -Wextra $<

$(BINARY): $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS)

clean:
	rm -f $(OBJS) $(BINARY)

strip:
	strip --strip-unneeded --preserve-dates -R .comment $(BINARY)

