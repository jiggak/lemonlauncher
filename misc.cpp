#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL/SDL.h>

#include "table.h"
#include "misc.h"

extern SortTable<ListItem>		g_tMenuItems;

ListItem::ListItem()
{
	iParent 		= 0;
	pNameSurface 	= NULL;
}

ListItem::~ListItem()
{
	if (pNameSurface)
		SDL_FreeSurface(pNameSurface);
}

Menu::Menu()
{
	iMenuId = 0;
	pszName = NULL;
}

Menu::~Menu()
{
	free(pszName);
}

MameGame::MameGame()
{
    pszName   = NULL;
    pszTitle  = NULL;
    pszParams = NULL;
}

MameGame::~MameGame()
{
    free(pszName);
    free(pszTitle);
    free(pszParams);
}

Menu* GetMenu(int id)
{
	for (int i = 0; i < g_tMenuItems.GetCount(); i++)
	{
		if (g_tMenuItems[i]->GetType() == TYPE_MENU)
		{
			Menu* pMenu = (Menu*)g_tMenuItems[i];
			if (pMenu->iMenuId == id) return pMenu;
		}
	}
	return NULL;
}

