#ifndef options_h
#define options_h

class Options
{
public:
	Options();
	~Options();

	bool Load();
	bool Unload();

public:
	int debug;
	int loglevel;

	int xsize;
	int ysize;
	int bitdepth;
	int fullscreen;
	char* font;
	int titlesize;
	int listsize;
	int pagesize;

	char* mamepath;
	char* mameparams;
	char* mamesnaps;
    char* rompath;

	int exit;
	int snap;
	int reload;
	int showhide;

	int p1up;
	int p1down;
	int p1pgup;
	int p1pgdown;
	int p1button1;
	int p1button2;

	int p2up;
	int p2down;
	int p2pgup;
	int p2pgdown;
	int p2button1;
	int p2button2;

protected:
	dictionary* opts;
};

#endif
