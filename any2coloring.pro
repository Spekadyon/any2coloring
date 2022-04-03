TEMPLATE = app
TARGET = any2coloring
INCLUDEPATH += .
QT += core gui

CONFIG += \
    link_pkgconfig \
    c++17


DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Input
HEADERS += \
    any2col.hpp

SOURCES += \
    any2col.cpp \
    libany2col.cpp

LIBS += -lgmic
PKGCONFIG += x11
