// All code written and coryright by roland rabien and released under GPL unless otherwise noted
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_image.h>

#include "table.h"
#include "log.h"
#include "iniparser.h"
#include "options.h"
#include "misc.h"
#include "lemonmenu.h"

#define VERSION "0.0.2"

Options 				g_Options;
Log						g_Log;
SortTable<ListItem>		g_tMenuItems;
SortTable<ListItem>*	g_ptMenus;

bool Init(SDL_Surface** ppMainScreen)
{
	// initialize the log file
	g_Log.SetLevel(g_Options.loglevel);

	// log version
	g_Log.Log1("Init: Lemon Launcher version: %s\n", VERSION);

	// load the options
	bool optionsLoaded = g_Options.Load();
	if (!optionsLoaded) g_Log.Log2("Init: options not loaded\n");

	// initialize sdl
	SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER);

	// create a screen to draw on
	SDL_Surface* screen;

	int xres = g_Options.xsize;
	int yres = g_Options.ysize;
	int bits = g_Options.bitdepth;
	int full = g_Options.fullscreen;

	g_Log.Log2("Init: using graphics mode: %dx%dx%d\n", xres, yres, bits);

	screen = SDL_SetVideoMode(xres, yres, bits, SDL_SWSURFACE | (full ? SDL_FULLSCREEN : 0));
	*ppMainScreen = screen;
	if (screen == NULL)
	{
		fprintf(stderr, "error: opening screen\n");
		g_Log.Log1("Init: unable to open screen\n");
		return false;
	}

	// init the font engine
	if (TTF_Init() == -1)
		g_Log.Log1("Init: unable to start font engine\n");

	return true;
}

bool UnInit()
{
	// shutdown fonts
	TTF_Quit();

	// shutdown sdl
	SDL_Quit();

	return true;
}

bool LoadMenuList(bool loadHidden)
{
	g_tMenuItems.SetOwnsElements(true);

	char szMenuFile[1024];
	sprintf(szMenuFile, "%s/.lemonlauncher/menulist", getenv("HOME"));
	FILE* fp = fopen(szMenuFile, "rt");
	if (fp == NULL) { g_Log.Log1("LoadMenuList: menulist %s not found\n", szMenuFile); return false; }

	char buf[1024];
	while (fgets(buf, sizeof(buf), fp))
	{
		if (buf[0] == '#') continue;
		char* c = strchr(buf, '\n');
		if (c) *c = '\0';

		if (strlen(buf) == 0) continue;

		Menu* pMenu = new Menu();

		// find the menu id
		c = strchr(buf, ' ');
		if (c) { *c = '\0'; c++; }

		pMenu->iMenuId = atoi(buf);

		// find the parent id
		char* d = c;
		c = strchr(d, ' ');
		if (c) { *c = '\0'; c++; }

		pMenu->iParent = atoi(d);

		// find the name
		pMenu->pszName = strdup(c);

		// add the menu to the list
		if (pMenu->pszName[0] != '.' || loadHidden)
		{
			if (pMenu->pszName[0] == '.') memmove(pMenu->pszName, pMenu->pszName + 1, strlen(pMenu->pszName));

			g_tMenuItems.Add(pMenu);

			g_Log.Log3("LoadMenuList: menu %s [id = %d][parent = %d] loaded\n",
		               pMenu->pszName, pMenu->iMenuId, pMenu->iParent);
		}
		else
		{
			delete pMenu;
		}
	}
	return true;
}

bool LoadGameList(bool loadHidden)
{
	char szGameFile[1024];
	sprintf(szGameFile, "%s/.lemonlauncher/gamelist", getenv("HOME"));
	FILE* fp = fopen(szGameFile, "rt");
	if (fp == NULL) { g_Log.Log1("LoadGameList: gamelist %s not found\n", szGameFile); return false; }

	char buf[1024];
	while (fgets(buf, sizeof(buf), fp))
	{
		if (buf[0] == '#') continue;

		char* c = strchr(buf, '\n');
		if (c) *c = '\0';

		if (strlen(buf) == 0) continue;

		MameGame* pGame = new MameGame();

		// get the parent id
		c = strchr(buf, ' ');
		if (c) { *c = '\0'; c++; }

		pGame->iParent = atoi(buf);

		// get the game of the game
		char* pszName = c;

		c = strchr(c, ' ');
		if (c) { *c = '\0'; c++; }

		pGame->pszName = strdup(pszName);

		// see if the is a title
		if (c)
		{
			char* pszTitle = c + 1;
			c = strchr(pszTitle, '\"');
			if (c) { *c = '\0'; c++; }

			pGame->pszTitle = strdup(pszTitle);
		}

		// see if there are params
		if (c)
		{
			c++;
			pGame->pszParams = strdup(c);
		}

		if (pGame->pszTitle[0] != '.' || loadHidden)
		{
			if (pGame->pszName[0] == '.') memmove(pGame->pszName, pGame->pszName + 1, strlen(pGame->pszName));

			g_tMenuItems.Add(pGame);

			g_Log.Log3("LoadGameList: %s\n", pGame->pszTitle ? pGame->pszTitle : pGame->pszName);
		}
		else
		{
			delete pGame;
		}
	}
	fclose(fp);

	g_tMenuItems.Sort();

	return true;
}

bool BuildMenus()
{
	int menuCnt = 0;
	for (int i = 0; i < g_tMenuItems.GetCount(); i++)
	{
		if (g_tMenuItems[i]->GetType() == TYPE_MENU)
		{
			Menu* pMenu = (Menu*)g_tMenuItems[i];
			if (pMenu->iMenuId > menuCnt - 1)
				menuCnt = pMenu->iMenuId + 1;
		}
	}

	g_Log.Log3("BuildMenus: building %d menus\n", menuCnt);

	g_ptMenus = new SortTable<ListItem>[menuCnt];

	for (int i = 0; i < g_tMenuItems.GetCount(); i++)
	{
		switch (g_tMenuItems[i]->GetType())
		{
			case TYPE_MENU:
			{
				Menu* pMenu = (Menu*)g_tMenuItems[i];
				if (pMenu->iMenuId == 0 && pMenu->iParent == 0)	break;
				if (g_tMenuItems[i]->iParent < menuCnt)
					g_ptMenus[g_tMenuItems[i]->iParent].Add(g_tMenuItems[i]);
				break;
			}
			case TYPE_MAMEGAME:
			{
				if (g_tMenuItems[i]->iParent < menuCnt)
					g_ptMenus[g_tMenuItems[i]->iParent].Add(g_tMenuItems[i]);
				break;
			}
		}
	}

	for (int i = 0; i < menuCnt; i++)
		g_ptMenus[i].Sort();
}

bool Go()
{
	SDL_Surface* screen;

	bool res;

	// init the system
	res = Init(&screen);
	if (!res) return false;

	// load the menu list
	res = LoadMenuList(false);
	if (!res) return false;

	// load the game list
	res = LoadGameList(false);
	if (!res) return false;

	// build the menus
	res = BuildMenus();
	if (!res) return false;

	// start the menu loop
	LemonMenu lm(screen);
	res = lm.MenuLoop();

	return res;
}

int main(void)
{
	Go();

	// shut down
	bool res = UnInit();
}
