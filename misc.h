#ifndef misc_h
#define misc_h

#define TYPE_NONE		0
#define TYPE_MENU		1
#define TYPE_MAMEGAME	2

class ListItem
{
public:
	ListItem();
	virtual ~ListItem();

	virtual int operator==(const ListItem& b) const = 0;
	virtual int operator<(const ListItem& b) const  = 0;

	virtual const char* GetTitle() const	{ return NULL;		}
	virtual int GetType() 					{ return TYPE_NONE; }

public:
	int iParent;

	SDL_Surface* pNameSurface;
};

class Menu : public ListItem
{
public:
	Menu();
	~Menu();

	int GetType()			        { return TYPE_MENU; }
	const char* GetTitle() const	{ return pszName; 	}

	int operator==(const ListItem& b) const { return !strcmp(GetTitle(), b.GetTitle());     }
	int operator<(const ListItem& b) const  { return  strcmp(GetTitle(), b.GetTitle()) < 0; }

public:
	int 	iMenuId;
	char* 	pszName;
};

class MameGame : public ListItem
{
public:
    MameGame();
    ~MameGame();

	int GetType() 			        { return TYPE_MAMEGAME; 				}
	const char* GetTitle() const	{ return pszTitle ? pszTitle : pszName; }

	int operator==(const ListItem& b) const { return !strcmp(GetTitle(), b.GetTitle());     }
	int operator<(const ListItem& b) const  { return  strcmp(GetTitle(), b.GetTitle()) < 0; }

public:
    char* pszName;
    char* pszTitle;
    char* pszParams;
};

Menu* GetMenu(int id);

#endif
