DESTDIR         = $(OPIEDIR)/bin
TEMPLATE        = app
CONFIG          = warn_on debug
#CONFIG         = warn_on release
HEADERS         = source/config.hh source/daemon.hh
SOURCES         = source/daemon.cc
INCLUDEPATH     += ../../libwellenreiter/source
DEPENDPATH      += $(OPIEDIR)/include
LIBS            += -lpcap -L$(QTDIR)/lib -lwellenreiter
INTERFACES      = 
TARGET          = wellenreiterd
