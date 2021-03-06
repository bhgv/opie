prefix=/opt/QtPalmtop

ifeq ($(OPIEDIR),)
    export OPIEDIR:=$(TOPDIR)
endif
ifeq ($(IPK_DIR),)
    export IPK_DIR:=$(OPIEDIR)
endif

ifneq ($(wildcard $(TOPDIR)/.config),)
    include $(TOPDIR)/.config
endif

ifndef CONFIG_TARGET_OE
    ifndef QTDIR
    $(error QTDIR not set)
    endif
else
    OEDIR:=$(shell echo $(CONFIG_OE_BUILD_DIR) | sed -e's/"//g')
    OEHOSTSYS:=$(shell echo $(CONFIG_OE_HOST_SYS) | sed -e's/"//g')
    OETARGETSYS:=$(shell echo $(CONFIG_OE_TARGET_SYS) | sed -e's/"//g')
    QTDIR:=$(shell echo $(OEDIR) | sed -e's/"//g')/tmp/staging/$(OETARGETSYS)/qt2
    $(shell mkdir -p $(QTDIR)/src/moc)
    $(shell echo -e "all: \n\t" >>$(QTDIR)/src/moc/Makefile)
    PLATFORM=sharp-linux
endif

ifdef CONFIG_TARGET_X86
    PLATFORM=x86-linux
endif
ifdef CONFIG_TARGET_64BIT
    PLATFORM=64bit-linux
endif
ifdef CONFIG_TARGET_SHARP
  PLATFORM=sharp-linux
endif
ifdef CONFIG_TARGET_IPAQ
  PLATFORM=ipaq-linux
endif
ifdef CONFIG_TARGET_RAMSES
  PLATFORM=ramses-linux
endif
ifdef CONFIG_TARGET_SIMPAD
  PLATFORM=simpad-linux
endif

ifdef CONFIG_TARGET_YOPY
  PLATFORM=yopy-linux
endif
ifdef CONFIG_TARGET_MACOSX
  PLATFORM=macx-darwin
endif
ifdef CONFIG_TARGET_HTC
  PLATFORM=htc-linux
endif

export QMAKE:=$(OPIEDIR)/qmake/qmake
export QMAKESPECSDIR=$(OPIEDIR)/mkspecs

ifeq ($(QPE_VERSION),)
    VERSION_MAJ:=$(shell echo $$(sed -n -e 's/.*QPE_VERSION "\([0-9]*\)\..*\..*".*/\1/p' <$(OPIEDIR)/library/version.h))
    VERSION_MIN:=$(shell echo $$(sed -n -e 's/.*QPE_VERSION ".*\.\([0-9]*\)\..*".*/\1/p' <$(OPIEDIR)/library/version.h))
    VERSION_PAT:=$(shell echo $$(sed -n -e 's/.*QPE_VERSION ".*\..*\.\([0-9]*\).*/\1/p' <$(OPIEDIR)/library/version.h))
    QPE_VERSION:=$(VERSION_MAJ).$(VERSION_MIN).$(VERSION_PAT)

    ifeq ($(QPE_VERSION),..)
        QPE_VERSION=1.1.2
    endif
endif
export QPE_VERSION

SUB_VERSION=$(shell echo $$(sed -n -e 's,.*SUB_VERSION \"\(.*\)\".*,\1,p' <$(OPIEDIR)/library/version.h))
ifeq ($(VERSION_CVS),)
    VERSION_CVS:=$(shell date +%s)
endif
ifneq ($(filter %snapshot",$(shell cat $(OPIEDIR)/library/version.h|grep QPE_VERSION)),)
    SUB_VERSION:=$(VERSION_CVS)
endif
export SUB_VERSION

ifneq ($(strip $(SUB_VERSION)),)
EXTRAVERSION=-$(SUB_VERSION)
endif

ifeq ($(QTE_REVISION),)
    QTE_REVISION=6
endif
export QTE_REVISION

export DEB_VERSION=2.0

ifeq ($(QTE_BASEVERSION),)
    ifneq ($(shell ls $(QTDIR)/include/qglobal.h 2>/dev/null),)
        QTE_BASEVERSION:=$(shell cat $(QTDIR)/include/qglobal.h|grep '^\#define QT_VERSION'|grep -v STR|sed -e 's/\#define QT_VERSION\t*//; s/.*\([0-9]\)\([0-9]\)\([0-9]\).*/\1.\2.\3/;')
    else
        QTE_BASEVERSION=2.3.7
    endif
endif
export QTE_BASEVERSION

ifeq ($(QTE_VERSION),)
    ifneq ($(shell ls $(QTDIR)/include/qglobal.h 2>/dev/null),)
        QTE_VERSION:=$(shell cat $(QTDIR)/include/qglobal.h|grep '^\#define QT_VERSION_STR'|sed -e 's/\#define QT_VERSION_STR\t*//;' -e 's/.*"\([^"]*\)".*/\1/;')
    else
        QTE_VERSION=2.3.7
    endif
endif
export QTE_VERSION

export PATH:=$(OPIEDIR)/scripts:$(PATH)
export QMAKESPEC=$(QMAKESPECSDIR)/$(patsubst "%",%,$(CONFIG_SPECFILE))

ifdef CONFIG_OPTIMIZATIONS
export CFLAGS_RELEASE=$(patsubst "%,%,$(CONFIG_OPTIMIZATIONS))
export CFLAGS_RELEASE:=$(patsubst %",%,$(CFLAGS_RELEASE))
endif

ifeq ($(STRIP),)
    ifneq ($(CONFIG_TARGET_X86),)
        STRIP=strip
    endif
    ifneq ($(CONFIG_TARGET_64BIT),)
        STRIP=strip
    endif
    ifneq ($(CONFIG_TARGET_IPAQ),)
        STRIP=arm-linux-strip
    endif
    ifneq ($(CONFIG_TARGET_SHARP),)
        STRIP=arm-linux-strip
    endif
    ifneq ($(CONFIG_TARGET_RAMSES),)
        STRIP=arm-linux-strip
    endif
    ifneq ($(CONFIG_TARGET_SIMPAD),)
        STRIP=arm-linux-strip
    endif
    ifneq ($(CONFIG_TARGET_YOPY),)
        STRIP=arm-linux-strip
    endif
endif

