#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_image.h>
#include <SDL/sge.h>

#include "table.h"
#include "log.h"
#include "iniparser.h"
#include "options.h"
#include "misc.h"
#include "lemonmenu.h"

#define UPDATESNAP	1

bool LoadMenuList();
bool LoadGameList();
bool BuildMenus();

extern Options     				g_Options;
extern Log 		        		g_Log;
extern SortTable<ListItem>		g_tMenuItems;
extern SortTable<ListItem>*		g_ptMenus;

SDL_Color SDL_RGB(Uint8 r, Uint8 g, Uint8 b)
{
	SDL_Color col;
	col.r = r;
	col.g = g;
	col.b = b;
	return col;
}

Uint32 Uint_RGB(Uint8 r, Uint8 g, Uint8 b) 
{ 
	return ((Uint32)r << 16) | ((Uint32)g << 8) | ((Uint32)b); 
}

LemonMenu::LemonMenu(SDL_Surface* p)
{
	pScreen = p;

	iCurItem = 0;
	iCurMenu = 0;

	const char* pszFontName = g_Options.font;

	pTitleFont = TTF_OpenFont(pszFontName, g_Options.titlesize);
	pListFont  = TTF_OpenFont(pszFontName, g_Options.listsize);

	if (pTitleFont == NULL) g_Log.Log1("LemonMenu: Can't open title font\n");
	if (pListFont  == NULL) g_Log.Log1("LemonMenu: Can't open list font\n");

	screen_x = g_Options.xsize;
	screen_y = g_Options.ysize;

    Uint32 rmask = 0x000000ff;
    Uint32 gmask = 0x0000ff00;
    Uint32 bmask = 0x00ff0000;
    Uint32 amask = 0xff000000;

	pBufferSurface = SDL_CreateRGBSurface(0, screen_x, screen_y, 32, rmask, gmask, bmask, amask);
	if (pBufferSurface == NULL)
	{
		g_Log.Log1("LemonMenu: Can't create double buffer\n");
	}
	else
	{
		g_Log.Log3("LemonMenu: double buffer %dx%d created\n", pBufferSurface->w, pBufferSurface->h);
		SDL_SetAlpha(pBufferSurface, 0, SDL_ALPHA_OPAQUE);
	}

	updateTimer  = NULL;
	pSnapSurface = NULL;
}

LemonMenu::~LemonMenu()
{
	SDL_FreeSurface(pBufferSurface);

	TTF_CloseFont(pTitleFont);
	TTF_CloseFont(pListFont);
}

bool LemonMenu::MenuLoop()
{
	HideCursor();
	RenderAll();
	SelChanged();
	bRun = true;

	while (bRun)
	{
		SDL_Event event;
		SDL_WaitEvent(&event);

		switch (event.type)
		{
			case SDL_QUIT:
			{
				HandleExit();
				break;
			}
			case SDL_KEYDOWN:
			{
				int key = event.key.keysym.sym;

				if (key == g_Options.exit)
					HandleExit();

				if (key == g_Options.p1up || key == g_Options.p2up)
					HandleUp();

				if (key == g_Options.p1down || key == g_Options.p2down)
					HandleDown();

				if (key == g_Options.p1pgdown || key == g_Options.p2pgdown)
					HandlePageDown();

				if (key == g_Options.p1pgup || key == g_Options.p2pgup)
					HandlePageUp();

				if (key == g_Options.p1button1 || key == g_Options.p2button1)
					HandleActivate();

				if (key == g_Options.p1button2 || key == g_Options.p2button2)
					HandleUpMenu();

				if (key == g_Options.snap)
					HandleSnap();

				if (key == g_Options.reload)
					HandleReload();

				break;
			}
			case SDL_USEREVENT:
			{
				if (event.user.code == UPDATESNAP)       UpdateSnap();
				break;
			}
		}
	}
	ShowCursor();
	SelChanged();
}

void LemonMenu::UpdateSnap()
{
	SortTable<ListItem>& tMenu = g_ptMenus[iCurMenu];

	pSnapSurface = NULL;
	if (tMenu[iCurItem]->GetType() == TYPE_MAMEGAME) UpdateMameGameSnap();
	if (tMenu[iCurItem]->GetType() == TYPE_MENU)	 UpdateMenuSnap();
}

void LemonMenu::UpdateMameGameSnap()
{
	SortTable<ListItem>& tMenu = g_ptMenus[iCurMenu];
	MameGame* pGame = (MameGame*)tMenu[iCurItem];

    // draw the games screen shot
	char szSnap[1024];
	sprintf(szSnap, "%s/%s.png", g_Options.mamesnaps, pGame->pszName);
	g_Log.Log3("UpdateMameGameSnap: loading snap %s\n", szSnap);

	pSnapSurface = IMG_Load(szSnap);

	if (pSnapSurface)
		RenderAll();
}

void LemonMenu::UpdateMenuSnap()
{
	SortTable<ListItem>& tCurMenu = g_ptMenus[iCurMenu];
	Menu* pMenu = (Menu*)tCurMenu[iCurItem];

	int surfaceCnt = 0;
	SDL_Surface* pGameSurfaces[4] = { NULL, NULL, NULL, NULL };

	SortTable<ListItem>& tMenu = g_ptMenus[pMenu->iMenuId];

	// get four snaps
	for (int i = 0; i < tMenu.GetCount(); i++)
	{
		if (tMenu[i]->GetType() == TYPE_MAMEGAME)
		{
			MameGame* pGame = (MameGame*)tMenu[i];

    		// draw the games screen shot
			char szSnap[1024];
			sprintf(szSnap, "%s/%s.png", g_Options.mamesnaps, pGame->pszName);
			g_Log.Log3("UpdateMenuSnap: loading snap %s\n", szSnap);

			pGameSurfaces[surfaceCnt] = IMG_Load(szSnap);

			if (pGameSurfaces[surfaceCnt])
				surfaceCnt++;
		}
		if (surfaceCnt == 4) break;
	}
	g_Log.Log3("UpdateMenuSnap: game snaps found %d\n", surfaceCnt);

	if (surfaceCnt < 4)
	{
		for (int i = 0; i < surfaceCnt; i++)
			SDL_FreeSurface(pGameSurfaces[i]);
		return;
	}

	int maxx = 0;
	int maxy = 0;

	for (int i = 0; i < 4; i++)
	{
		if (pGameSurfaces[i]->w > maxx) maxx = pGameSurfaces[i]->w;
		if (pGameSurfaces[i]->h > maxy) maxy = pGameSurfaces[i]->h;
	}

    Uint32 rmask = 0x000000ff;
    Uint32 gmask = 0x0000ff00;
    Uint32 bmask = 0x00ff0000;
    Uint32 amask = 0xff000000;

	pSnapSurface = SDL_CreateRGBSurface(0, maxx * 2 , maxy * 2, 32, rmask, gmask, bmask, amask);
	if (pSnapSurface == NULL)
	{
		g_Log.Log1("UpdateMenuSnap: Can't menu snap surface\n");
		return;
	}
	else
	{
		g_Log.Log3("UpdateMenuSnap: Created menu snap %dx%d\n", pSnapSurface->w, pSnapSurface->h);
	}

	SDL_SetAlpha(pSnapSurface, 0, SDL_ALPHA_OPAQUE);

	int x;
	int y;

	SDL_Rect rc;

	// draw snap 1
	x = 0;
	y = 0;

	rc.x = ((maxx / 2) - (pGameSurfaces[0]->w / 2)) + x;
	rc.y = ((maxy / 2) - (pGameSurfaces[0]->h / 2)) + y;

	SDL_BlitSurface(pGameSurfaces[0], NULL, pSnapSurface, &rc);

	// draw snap 2
	x = maxx;
	y = 0;

	rc.x = ((maxx / 2) - (pGameSurfaces[1]->w / 2)) + x;
	rc.y = ((maxy / 2) - (pGameSurfaces[1]->h / 2)) + y;

	SDL_BlitSurface(pGameSurfaces[1], NULL, pSnapSurface, &rc);

	// draw snap 3
	x = 0;
	y = maxy;

	rc.x = ((maxx / 2) - (pGameSurfaces[2]->w / 2)) + x;
	rc.y = ((maxy / 2) - (pGameSurfaces[2]->h / 2)) + y;

	SDL_BlitSurface(pGameSurfaces[2], NULL, pSnapSurface, &rc);

	// draw snap 4
	x = maxx;
	y = maxy;

	rc.x = ((maxx / 2) - (pGameSurfaces[3]->w / 2)) + x;
	rc.y = ((maxy / 2) - (pGameSurfaces[3]->h / 2)) + y;

	SDL_BlitSurface(pGameSurfaces[3], NULL, pSnapSurface, &rc);

	for (int i = 0; i < 4; i++)
		SDL_FreeSurface(pGameSurfaces[i]);

	if (pSnapSurface)
		RenderAll();
}

bool LemonMenu::RenderAll()
{
	SortTable<ListItem>& tMenu = g_ptMenus[iCurMenu];

	// erase the screen
    sge_FilledRect(pBufferSurface, 0, 0, screen_x, screen_y, Uint_RGB(0, 0, 0));

    // draw the games screen shot
	if (pSnapSurface)
	{
		float xscale = (float)(screen_x - 80) / pSnapSurface->w;
		float yscale = (float)(screen_y - 80) / pSnapSurface->h;

		int xpos;
		int ypos;

		if (xscale > yscale)
		{
			xscale = yscale;
			
			xpos = ((screen_x - 80) / 2) - (int)((float)pSnapSurface->w * xscale / 2.0) + 40;
			ypos = 60;
		}
		else if (yscale > xscale)
		{
			yscale = xscale;

			xpos = 40;
			ypos = ((screen_y - 80) / 2) - (int)((float)pSnapSurface->h * yscale / 2.0) + 60;
		}
		else
		{
			xpos = 40;
			ypos = 60;
		}
		
		sge_transform(pSnapSurface, pBufferSurface, 0.0, xscale, yscale, 0, 0, xpos, ypos, SGE_TSAFE);
		sge_FilledRectAlpha(pBufferSurface, 0, 0, screen_x, screen_y, Uint_RGB(0, 0, 0), 200);
	}

	// find the title
	const char* pszMenuTitle = NULL;
	for (int i = 0; i < g_tMenuItems.GetCount(); i++)
	{
		if (g_tMenuItems[i]->GetType() == TYPE_MENU)
		{
			Menu* pMenu = (Menu*)g_tMenuItems[i];
			if (pMenu->iMenuId == iCurMenu)
			{
				pszMenuTitle = pMenu->GetTitle();
				break;
			}
		}
	}

	// draw the title
	int listTop = 0;
	if (pszMenuTitle)
	{
		SDL_Surface* pTitleSurface;
		pTitleSurface = TTF_RenderText_Blended(pTitleFont, pszMenuTitle, SDL_RGB(0xEF, 0xEF, 0xEF));

		SDL_Rect rcTitle;
		rcTitle.x = pScreen->w / 2 - pTitleSurface->w / 2;
		rcTitle.y = 8;

		SDL_BlitSurface(pTitleSurface, NULL, pBufferSurface, &rcTitle);

		// draw the line under the title
		sge_FilledRect(pBufferSurface, rcTitle.x - 10,                    pTitleSurface->h + 10,
			                           rcTitle.x + pTitleSurface->w + 10, pTitleSurface->h + 13,
									   Uint_RGB(0xEF, 0xEF, 0xEF));

		listTop = pTitleSurface->h + 16;

		SDL_FreeSurface(pTitleSurface);
	}
		
	// draw the selected game
	SDL_Color colSel = tMenu[iCurItem]->GetType() == TYPE_MENU ? SDL_RGB(0x32, 0xE4, 0xFF) : SDL_RGB(0xFF, 0xE4, 0x32);
	SDL_Surface* pSelectedGame = TTF_RenderText_Blended(pListFont, tMenu[iCurItem]->GetTitle(), colSel);

	SDL_Rect rcSelected;
	rcSelected.x = pScreen->w / 2 - pSelectedGame->w / 2;
	rcSelected.y = pScreen->h / 2 - pSelectedGame->h / 2;

	SDL_BlitSurface(pSelectedGame, NULL, pBufferSurface, &rcSelected);

	g_Log.Log4("Drawing selected item %s [%dx%d]\n", tMenu[iCurItem]->GetTitle(), pSelectedGame->w, pSelectedGame->h);

	// draw the games above the selection
    int ypos = rcSelected.y - pSelectedGame->h - 5;
	for (int i = iCurItem - 1; i >= 0; i--)
	{
		if (ypos < listTop) break;

		SDL_Surface* pNameSurface = GetNameSurface(i);

		SDL_Rect rcText;
		rcText.y = ypos;
		rcText.x = pScreen->w / 2 - pNameSurface->w / 2;
		
		SDL_BlitSurface(pNameSurface, NULL, pBufferSurface, &rcText);

		ypos -= pSelectedGame->h + 5;
	}

	// draw the games below the selection
	ypos = rcSelected.y + pSelectedGame->h + 5;	
	for (int i = iCurItem + 1; i < tMenu.GetCount(); i++)
	{
		if (ypos > screen_y) break;

		SDL_Surface* pNameSurface = GetNameSurface(i);

		SDL_Rect rcText;
		rcText.y = ypos;
		rcText.x = pScreen->w / 2 - pNameSurface->w / 2;

		SDL_BlitSurface(pNameSurface, NULL, pBufferSurface, &rcText);

	    ypos += pSelectedGame->h + 5;	
	}

	// clean up memory
	SDL_FreeSurface(pSelectedGame);

	// update the screen
	if (SDL_BlitSurface(pBufferSurface, NULL, pScreen, NULL) != 0)
		g_Log.Log1("LemonMenu: double buffer blit failed\n");

	SDL_UpdateRect(pScreen, 0, 0, 0, 0);
}

SDL_Surface* LemonMenu::GetNameSurface(int idx)
{
	SortTable<ListItem>& tMenu = g_ptMenus[iCurMenu];
	SDL_Surface* pSurface = tMenu[idx]->pNameSurface;
	if (pSurface == NULL)
	{
		SDL_Color colItem = tMenu[idx]->GetType() == TYPE_MENU ? SDL_RGB(0xC2, 0xF4, 0xFF) : SDL_RGB(0xEF, 0xEF, 0xEF);

		pSurface = TTF_RenderText_Blended(pListFont, tMenu[idx]->GetTitle(), colItem);
		tMenu[idx]->pNameSurface = pSurface;
	}
	return pSurface;
}
			  
void LemonMenu::HandleExit()
{
	bRun = false;
}

void LemonMenu::HandleUp()
{
	iCurItem--;
	if (iCurItem < 0) iCurItem = g_ptMenus[iCurMenu].GetCount() - 1;

	SelChanged();
	RenderAll();
}

void LemonMenu::HandleDown()
{
	iCurItem++;
	if (iCurItem > g_ptMenus[iCurMenu].GetCount() - 1) iCurItem = 0;

	SelChanged();
	RenderAll();
}

void LemonMenu::HandlePageUp()
{
	int cnt = g_ptMenus[iCurMenu].GetCount();
	int step = g_Options.pagesize;
	if (step > cnt) return;

	iCurItem -= step;

	if (iCurItem < 0) iCurItem += cnt;

	SelChanged();
	RenderAll();
}

void LemonMenu::HandlePageDown()
{
	int cnt = g_ptMenus[iCurMenu].GetCount();
	int step = g_Options.pagesize;
	if (step > cnt) return;

	iCurItem += step;

	if (iCurItem >= cnt) iCurItem -= cnt;

	SelChanged();
	RenderAll();
}

void LemonMenu::HandleRun()
{
	MameGame* pGame = (MameGame*)(g_ptMenus[iCurMenu].GetItem(iCurItem));

	char cmd[1024];

	const char* pszGameParam = pGame->pszParams;
	const char* pszGlblParam = g_Options.mameparams;
	
	if (pszGameParam == NULL) pszGameParam = NULL;
	if (pszGlblParam == NULL) pszGlblParam = NULL;

	sprintf(cmd, "%s %s %s %s", g_Options.mamepath, pGame->pszName, pszGameParam, pszGlblParam);
	g_Log.Log2("Running cmd [%s]\n", cmd);

	int fullscreen = g_Options.fullscreen;
	
	if (fullscreen) SDL_WM_ToggleFullScreen(pScreen);
	system(cmd);
	if (fullscreen) SDL_WM_ToggleFullScreen(pScreen);

	// dump the event queue
	SDL_Delay(1500);
	SDL_PumpEvents();

	SDL_Event evt;
	while (SDL_PeepEvents(&evt, 1, SDL_GETEVENT, 0xFFFFFFFF) > 0)
		SDL_PumpEvents();
}

void LemonMenu::HandleSnap()
{
	char szSnap[1024];
	int idx = 0;

	do
	{
		sprintf(szSnap, "%s/.lemonlauncher/snap%d.bmp", getenv("HOME"), idx);
	} while (access(szSnap, F_OK) != 0);

	SDL_SaveBMP(pScreen, szSnap);			 
}

void LemonMenu::HandleUpMenu()
{
	// find the parent menu idx
	Menu* pMenu = GetMenu(iCurMenu);
	int iParent = pMenu ? pMenu->iParent : 0;
	iCurMenu = iParent;


	iCurItem = 0;
	for (int i = 0; i < g_ptMenus[iParent].GetCount(); i++)
		if (g_ptMenus[iParent].GetItem(i) == pMenu)
			iCurItem = i;

	g_Log.Log3("UpMenu: going to menu %d\n", iParent);

	SelChanged();
	RenderAll();
}

void LemonMenu::HandleDownMenu()
{
	Menu* pMenu = (Menu*)(g_ptMenus[iCurMenu].GetItem(iCurItem));
	iCurMenu = pMenu->iMenuId;
	iCurItem = 0;

	g_Log.Log3("DownMenu: going to menu %d\n", iCurMenu);

	SelChanged();
	RenderAll();
}

void LemonMenu::HandleActivate()
{
	SortTable<ListItem>& tMenu = g_ptMenus[iCurMenu];
	switch (tMenu[iCurItem]->GetType())
	{
		case TYPE_MENU:		HandleDownMenu();	break;
		case TYPE_MAMEGAME: HandleRun();		break;
	}
}

void LemonMenu::HandleReload()
{
	g_tMenuItems.DeleteAll();
	delete[] g_ptMenus;

	LoadMenuList();
	LoadGameList();
	BuildMenus();

	iCurMenu = 0;
	iCurItem = 0;
}

void LemonMenu::HideCursor()
{
	SDL_ShowCursor(SDL_DISABLE);
}

void LemonMenu::ShowCursor()
{
	SDL_ShowCursor(SDL_ENABLE);
}

void LemonMenu::SelChanged()
{
	if (updateTimer)
		SDL_RemoveTimer(updateTimer);

	if (pSnapSurface)
	{
		SDL_FreeSurface(pSnapSurface);
		pSnapSurface = NULL;
	}

	updateTimer = SDL_AddTimer(500, UpdateCallback, this);
}

Uint32 LemonMenu::UpdateCallback(Uint32 interval, void* param)
{
	SDL_Event evt;

	evt.type      = SDL_USEREVENT;
	evt.user.code = UPDATESNAP;
	
	SDL_PushEvent(&evt);

	return 0;
}


