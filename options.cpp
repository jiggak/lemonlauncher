#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>

#include "iniparser.h"
#include "options.h"
#include "log.h"


extern Log g_Log;

Options::Options()
{
	opts = NULL;
}

Options::~Options()
{
	Unload();
}

bool Options:: Load()
{
	char szOptFile[1024];

#ifdef WIN32
    strcpy(szOptFile, "lemonlauncher-win.ini");
#else
	sprintf(szOptFile, "%s/.lemonlauncher/lemonlauncher.ini", getenv("HOME"));
#endif

	FILE* fp = fopen(szOptFile, "rt");
	if (fp == NULL)
	{
		g_Log.Log1("Load: can not open options file\n");
		return false;
	}

	opts = iniparser_load(szOptFile);

	if (opts == NULL)
	{
		g_Log.Log1("Load: can not load options\n");
		return false;
	}

#ifdef WIN32
    bool win32 = true;
#else
    bool win32 = false;
#endif


	debug      	= iniparser_getboolean(opts, "main:debug", 0);
	loglevel	= iniparser_getint(opts, "main:loglevel", 4);

	xsize       = iniparser_getint(opts, "screen:xsize", 800);
	ysize       = iniparser_getint(opts, "screen:ysize", 600);
	bitdepth    = iniparser_getint(opts, "screen:bitdepth", 32);
	fullscreen  = iniparser_getboolean(opts,"screen:fullscreen", 1);
	font	    = iniparser_getstring(opts, "screen:font", "slicker.ttf");
	titlesize   = iniparser_getint(opts,"screeen:titlesize", 40);
	listsize    = iniparser_getint(opts,"screen:listsize", 30);
	pagesize    = iniparser_getint(opts,"screen:pagesize", 15);

    mamepath    = iniparser_getstring(opts, "mame:path", win32 ? (char*)"mame.exe" : (char*)"xmame.SDL");
    mameparams  = iniparser_getstring(opts, "mame:params", win32 ? (char*)"" : (char*)"-fullscreen");
    mamesnaps   = iniparser_getstring(opts, "mame:snaps", win32 ? (char*)"c:\\mame\\xnap" : (char*)"/usr/local/share/xmame/snap");
    rompath     = iniparser_getstring(opts, "mame:rompath", "");

	exit		= iniparser_getint(opts,"keys:exit", SDLK_ESCAPE);
	snap		= iniparser_getint(opts,"keys:snap", SDLK_5);
	reload		= iniparser_getint(opts,"keys:reload", SDLK_6);
	showhide    = iniparser_getint(opts,"keys:hideshow", SDLK_1);

	p1up        = iniparser_getint(opts,"keys:p1up", SDLK_UP);
	p1down		= iniparser_getint(opts,"keys:p1down", SDLK_DOWN);
	p1pgup 		= iniparser_getint(opts,"keys:p1pgup", SDLK_LEFT);
	p1pgdown	= iniparser_getint(opts,"keys:p1pgdown", SDLK_RIGHT);
	p1button1	= iniparser_getint(opts,"keys:p1button1", SDLK_LCTRL);
	p1button2	= iniparser_getint(opts,"keys:p1button2", SDLK_LSHIFT);

	p2up        = iniparser_getint(opts,"keys:p2up", SDLK_r);
	p2down		= iniparser_getint(opts,"keys:p2down", SDLK_f);
	p2pgup 		= iniparser_getint(opts,"keys:p2pgup", SDLK_d);
	p2pgdown	= iniparser_getint(opts,"keys:p2pgdown", SDLK_g);
	p2button1	= iniparser_getint(opts,"keys:p2button1", SDLK_a);
	p2button2	= iniparser_getint(opts,"keys:p2button2", SDLK_w);

	return true;
}

bool Options::Unload()
{
	if (opts)
		iniparser_freedict(opts);

    return true;
}

