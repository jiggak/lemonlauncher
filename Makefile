#
# Makefile for lemonlauncher
#
CC=gcc
CFLAGS=-g -lstdc++
SDL_CFLAGS := $(shell sdl-config --cflags)
SDL_LDFLAGS := $(shell sdl-config --libs) -lSDL_ttf -lSDL_image -lSGE
SRC=lemonlauncher.cpp options.cpp log.cpp lemonmenu.cpp misc.cpp dictionary.c iniparser.c strlib.c
OBJ=lemonlauncher.o options.o log.o lemonmenu.o misc.o dictionary.o iniparser.o strlib.o

all: lemonlauncher

clean: $(OBJ)
	rm $(OBJ) lemonlauncher

lemonlauncher: $(OBJ)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(SDL_LDFLAGS) -o lemonlauncher $(OBJ)

lemonlauncher.o:lemonlauncher.cpp table.h options.h log.h lemonmenu.h misc.h

options.o:options.cpp table.h iniparser.h

log.o:log.cpp log.h

lemonmenu.o:lemonmenu.cpp lemonmenu.h table.h options.h log.h misc.h

misc.o:misc.cpp misc.h

dictionary.o: dictionary.h dictionary.c

iniparser.o: iniparser.c iniparser.h dictionary.h strlib.h

strlib.o: strlib.c strlib.h

