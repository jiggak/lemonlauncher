#ifndef log_h
#define log_h

class Log
{
public:
	Log();
	~Log();

    void Init();

	void SetLevel(int level);

	int Log1(const char* format, ...);
	int Log2(const char* format, ...);
	int Log3(const char* format, ...);
	int Log4(const char* fornat, ...);

protected:
	int level;
	FILE* fp;
};

#endif
