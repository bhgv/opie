CONFIG += qt warn_on

HEADERS         = multiauthconfig.h

SOURCES         = multiauthconfig.cpp main.cpp

INTERFACES	= loginbase.ui syncbase.ui

INCLUDEPATH     += $(OPIEDIR)/include

LIBS            += -lqpe -lopiecore2 -lopieui2 -lopiesecurity2

DESTDIR = $(OPIEDIR)/bin
TARGET = security

include ( $(OPIEDIR)/include.pro )
