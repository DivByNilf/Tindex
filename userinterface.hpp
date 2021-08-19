#pragma once

#include <memory>
#include <map>

#include <windows.h>

//{ Structs

struct MainInitStruct {
	HANDLE hMutex;
};

struct ListPage {
	char type;
	char *str;
	long left;
	long right;
};

typedef struct StrListVariables {
	char ***strs;
//	oneslnk **strchn;
	char **header;
	long long nRows;
	unsigned long lastsel;
	int xspos, yspos;
	unsigned char *StrListSel, option;
	short *bpos, LastKey, drgpos;
	char hvrd, drag, nChns, timed;
	POINT point;
} STRLISTV;

typedef struct ThumbListVariables {
	ImgF **thumb;
	oneslnk **strchn;
	unsigned char *ThumbSel;
	long nThumbs;
	unsigned long width, height;
	int yspos;
	unsigned lastsel;
	short LastKey;
	char hvrd, timed, nChns;
	POINT point;
} THMBLISTV;

typedef struct PageListVariables {
	unsigned long long CurPage, LastPage;
	struct ListPage *PageList;
	int hovered;
} PAGELISTV;

typedef struct ImageViewVariables {
	ImgF *FullImage;
	ImgF *DispImage;
	char *imgpath;
	unsigned long long fnum;
	long zoomp, xpos, ypos, xdrgpos, ydrgpos, dispframe;
	long long xdisp, ydisp, xdrgstart, ydrgstart;
	long double midX, midY;
	unsigned char fit, option, drag, timed;	// fit: 1 image is being shrunk to window, 2 means the display image is the original image (whole image fits), 4 means zoomed image fits horizontally and 8 vertically, 16: image is stretched and shrunk to window
} IMGVIEWV;

typedef struct ThumbManVariables {
	PAGELISTV plv;
	THMBLISTV tlv;
	IMGVIEWV ivv;
	char *dname, *tfstr;
	unsigned long long dnum;
	unsigned char option;
	unsigned long long nitems;
	HWND parent;
} THMBMANV;

typedef struct FileTagEditVariables {
	unsigned long long dnum;
	oneslnk *fnumchn;
	oneslnk *tagnumchn, *aliaschn;	// original tags and their aliases
	oneslnk *rmnaliaschn, *regaliaschn, *remtagnumchn, *addtagnumchn;
	unsigned char clean_flag;
} FTEV;

typedef struct MainIndexManArgs {
	unsigned char option;
	HWND parent;
} MIMANARGS;

typedef struct DirManArgs {
	unsigned char option;
	HWND parent;
	unsigned long long inum;
} DIRMANARGS;

typedef struct SubDirManArgs {
	unsigned char option;
	HWND parent;
	unsigned long long inum;
} SDIRMANARGS;

typedef struct ThumbManArgs {
	unsigned char option;
	HWND parent;
	unsigned long long dnum;
} THMBMANARGS;
//}

class WinCreateArgs;

class PageListClass;
class StrListClass;

class WindowClass {
public:
	static std::shared_ptr<WindowClass> getWindowPtr(HWND hwnd);
	
	static std::shared_ptr<WindowClass> createWindowMemory(HWND hwnd, WinCreateArgs &winArgs);
	
	static boolean WindowClass::releaseWindowMemory(HWND hwnd);
	
	static LRESULT CALLBACK generalWinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
	
protected:
	//HWND defaultWindowInstancer(DWORD dwExStyle, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
	//static HWND defaultWindowInstancer(DWORD dwExStyle, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
private:
	static std::map<HWND, std::shared_ptr<WindowClass>> winMemMap;
};

class WinCreateArgs {
public:
	WindowClass *(&constructor)(void);
	LPVOID lpParam;
	
	WinCreateArgs(WindowClass *(*constructor_)(void));
	
};

class WinInstancer {
public:
	WinInstancer() = delete;
	WinInstancer(DWORD dwExStyle, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
	
	const DWORD dwExStyle;
	const LPCWSTR lpWindowName;
	const DWORD dwStyle;
	const int X;
	const int Y;
	const int nWidth;
	const int nHeight;
	const HWND hWndParent;
	const HMENU hMenu;
	const HINSTANCE hInstance;
	const LPVOID lpParam;
	
	HWND create(std::shared_ptr<WinCreateArgs> winArgs_, const std::wstring &winClassName_);
};

class WindowHelper {
public:
	const std::wstring winClassName;
	
	//void (*const modifyWinStruct)(WNDCLASSW &wc);
	
	WindowHelper(const std::wstring, void (*)(WNDCLASSW &wc));
	WindowHelper(const std::wstring winClassName_);
	WindowHelper() = delete;
	
	virtual bool registerWindowClass();
};

class DeferredRegWindowHelper : public WindowHelper {
public:
	DeferredRegWindowHelper(const std::wstring, void (*)(WNDCLASSW &wc));
	
	virtual bool registerWindowClass() override;
private:
	void (*modifyWinStruct)(WNDCLASSW &wc);
	bool isRegistered;
};

class MsgHandler : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	
	
	HWND wHandle[2];
	HWND wHandle2[1];
	unsigned int ndirman = 2;
	unsigned int nthmbman = 1;
	int lastoption, lastdnum;
	
	std::shared_ptr<void> hMapFile;
	//HANDLE hMapFile;
};

class TabWindow : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
};

class TabClass : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	
	
	HWND dispWindow;
};

class DmanClass : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	int menuCreate(HWND hwnd);
	int menuUse(HWND hwnd, long int menu_id);
	
	
	
	std::shared_ptr<PageListClass> plv;
	std::shared_ptr<StrListClass> slv;
	unsigned char option;
	HWND parent;
	unsigned long long inum;
};

class SDmanClass : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	int menuCreate(HWND hwnd);
	int menuUse(HWND hwnd, long int menu_id);
	
	
	
	std::shared_ptr<PageListClass> plv;
	std::shared_ptr<StrListClass> slv;
	
	unsigned char option;
	HWND parent;
	unsigned long long inum;
};

class MainIndexManClass : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	int menuCreate(HWND hwnd);
	int menuUse(HWND hwnd, long int menu_id);
	
	
	
	std::shared_ptr<PageListClass> plv;
	std::shared_ptr<StrListClass> slv;
	
	unsigned char option;
	HWND parent;
};

class ThumbManClass : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	
	
	PAGELISTV plv;
	THMBLISTV tlv;
	IMGVIEWV ivv;
	char *dname, *tfstr;
	unsigned long long dnum;
	unsigned char option;
	unsigned long long nitems;
	HWND parent;
};

class PageListClass : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	
	
	unsigned long long CurPage, LastPage;
	struct ListPage *PageList;
	int hovered;
};

class StrListClass : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	unsigned long long getSingleSelPos(void);
	
	char ***strs;
//	oneslnk **strchn;
	char **header;
	long long nRows;
	unsigned long lastsel;
	int xspos, yspos;
	unsigned char *StrListSel, option;
	short *bpos, LastKey, drgpos;
	char hvrd, drag, nChns, timed;
	POINT point;
	
	
};

class ThumbListClass : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	
	
	ImgF **thumb;
	oneslnk **strchn;
	unsigned char *ThumbSel;
	long nThumbs;
	unsigned long width, height;
	int yspos;
	unsigned lastsel;
	short LastKey;
	char hvrd, timed, nChns;
	POINT point;
};

class ViewImageClass : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	
	
	ImgF *FullImage;
	ImgF *DispImage;
	char *imgpath;
	unsigned long long fnum;
	long zoomp, xpos, ypos, xdrgpos, ydrgpos, dispframe;
	long long xdisp, ydisp, xdrgstart, ydrgstart;
	long double midX, midY;
	unsigned char fit, option, drag, timed;	// fit: 1 image is being shrunk to window, 2 means the display image is the original image (whole image fits), 4 means zoomed image fits horizontally and 8 vertically, 16: image is stretched and shrunk to window
};

class FileTagEditClass : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	
	
	unsigned long long dnum;
	oneslnk *fnumchn;
	oneslnk *tagnumchn, *aliaschn;	// original tags and their aliases
	oneslnk *rmnaliaschn, *regaliaschn, *remtagnumchn, *addtagnumchn;
	unsigned char clean_flag;
};

class CreateAliasClass : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	std::shared_ptr<FileTagEditClass> parent;
	
};

class EditSuperClass : public WindowClass {
public:
//! TODO: make the helper a reference in each windowclass
	static const DeferredRegWindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
};

class SearchBarClass : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
};

class TextEditDialogClass : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	
	
	unsigned char clean_flag;
};

