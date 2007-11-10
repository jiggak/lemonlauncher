MAINTAINERCLEANFILES = Makefile.in

INCLUDES = -I$(top_srcdir) -I$(top_builddir)

noinst_PROGRAMS = bin2c
bin2c_SOURCES = bin2c.c

BUILT_SOURCES = default_font.h
CLEANFILES = default_font.h

default_font.h: bin2c
	$(srcdir)/bin2c default_font <$(DEFAULT_FONT) >default_font.h

bin_PROGRAMS = lemonlauncher
lemonlauncher_SOURCES = lemonlauncher.cpp lemonmenu.cpp layout.cpp \
menu.cpp game.cpp options.cpp log.cpp

noinst_HEADERS = lemonmenu.h options.h log.h error.h layout.h \
item.h menu.h game.h
