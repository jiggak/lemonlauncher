#ifndef lemonmenu_h
#define lemonmenu_h

class LemonMenu
{
public:
	LemonMenu(SDL_Surface* pScreen);
	~LemonMenu();

	bool MenuLoop();

protected:
	void HandleExit();
	void HandleUp();
	void HandleDown();
	void HandlePageUp();
	void HandlePageDown();
	void HandleRun();
	void HandleSnap();
	void HandleUpMenu();
	void HandleDownMenu();
	void HandleActivate();
	void HandleReload();
	void HandleShowHide();

	bool RenderAll();

	void HideCursor();
	void ShowCursor();

	void SelChanged();

	void UpdateSnap();
	void UpdateMameGameSnap();
	void UpdateMenuSnap();

	SDL_Surface* GetNameSurface(int idx);

	static Uint32 UpdateCallback(Uint32 inteval, void* param);

	SDL_Surface* pScreen;
	bool bRun;

	int iCurItem;
	int iCurMenu;

	int screen_x;
	int screen_y;

	TTF_Font* pTitleFont;
	TTF_Font* pListFont;

	SDL_Surface*	pBufferSurface;
	SDL_Surface*	pSnapSurface;

	SDL_TimerID		updateTimer;

	bool bShowHidden;
};

#endif
