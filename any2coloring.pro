TEMPLATE = app
TARGET = any2coloring
INCLUDEPATH += .
QT += gui svg

# Input
HEADERS += any2col.hpp
SOURCES += any2col.cpp libany2col.cpp
LIBS += -lgmic -lX11
