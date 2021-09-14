#pragma once

#include <windows.h>

#include <memory>
#include <map>

//{ Structs

struct MainInitStruct {
	HANDLE hMutex;
	
	bool operator==(const MainInitStruct&) const = default;
};

struct ListPage {
	char type;
	char *str;
	int32_t left;
	int32_t right;
	
	~ListPage(void) {
		if (str != nullptr) {
			free(str);
		}
	}
};

typedef struct StrListVariables {
	char ***strs;
//	oneslnk **strchn;
	char **header;
	int64_t nRows;
	uint32_t lastsel;
	int32_t xspos, yspos;
	uint8_t *StrListSel, option;
	int16_t *bpos, LastKey, drgpos;
	char hvrd, drag, nChns, timed;
	POINT point;
} STRLISTV;

typedef struct ThumbListVariables {
	ImgF **thumb;
	oneslnk **strchn;
	uint8_t *ThumbSel;
	int32_t nThumbs;
	uint32_t width, height;
	int32_t yspos;
	unsigned lastsel;
	int16_t LastKey;
	char hvrd, timed, nChns;
	POINT point;
} THMBLISTV;

typedef struct PageListVariables {
	uint64_t curPage, lastPage;
	struct ListPage *PageList;
	int32_t hovered;
} PAGELISTV;

typedef struct ImageViewVariables {
	ImgF *FullImage;
	ImgF *DispImage;
	char *imgpath;
	uint64_t fnum;
	int32_t zoomp, xpos, ypos, xdrgpos, ydrgpos, dispframe;
	int64_t xdisp, ydisp, xdrgstart, ydrgstart;
	long double midX, midY;
	uint8_t fit, option, drag, timed;	// fit: 1 image is being shrunk to window, 2 means the display image is the original image (whole image fits), 4 means zoomed image fits horizontally and 8 vertically, 16: image is stretched and shrunk to window
} IMGVIEWV;

typedef struct ThumbManVariables {
	PAGELISTV plv;
	THMBLISTV tlv;
	IMGVIEWV ivv;
	char *dname, *tfstr;
	uint64_t dnum;
	uint8_t option;
	uint64_t nitems;
	HWND parent;
} THMBMANV;

typedef struct FileTagEditVariables {
	uint64_t dnum;
	oneslnk *fnumchn;
	oneslnk *tagnumchn, *aliaschn;	// original tags and their aliases
	oneslnk *rmnaliaschn, *regaliaschn, *remtagnumchn, *addtagnumchn;
	uint8_t clean_flag;
} FTEV;

typedef struct MainIndexManArgs {
	uint8_t option;
	HWND parent;
} MIMANARGS;

typedef struct DirManArgs {
	uint8_t option;
	HWND parent;
	uint64_t inum;
} DIRMANARGS;

typedef struct SubDirManArgs {
	uint8_t option;
	HWND parent;
	uint64_t inum;
} SDIRMANARGS;

typedef struct ThumbManArgs {
	uint8_t option;
	HWND parent;
	uint64_t dnum;
} THMBMANARGS;
//}

class WinCreateArgs;

class PageListClass;
class StrListClass;

class WindowClass {
public:
	virtual ~WindowClass(void) = default;
	
	static std::shared_ptr<WindowClass> getWindowPtr(HWND hwnd);
	
	static std::shared_ptr<WindowClass> createWindowMemory(HWND hwnd, WinCreateArgs &winArgs);
	
	static boolean WindowClass::releaseWindowMemory(HWND hwnd);
	
	static LRESULT CALLBACK generalWinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
	
protected:
	//HWND defaultWindowInstancer(DWORD dwExStyle, LPCWSTR lpWindowName, DWORD dwStyle, int32_t X, int32_t Y, int32_t nWidth, int32_t nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
	//static HWND defaultWindowInstancer(DWORD dwExStyle, LPCWSTR lpWindowName, DWORD dwStyle, int32_t X, int32_t Y, int32_t nWidth, int32_t nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
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
	WinInstancer(DWORD dwExStyle, LPCWSTR lpWindowName, DWORD dwStyle, int32_t X, int32_t Y, int32_t nWidth, int32_t nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
	
	const DWORD dwExStyle;
	const LPCWSTR lpWindowName;
	const DWORD dwStyle;
	const int32_t X;
	const int32_t Y;
	const int32_t nWidth;
	const int32_t nHeight;
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
	uint32_t ndirman = 2;
	uint32_t nthmbman = 1;
	int32_t lastoption, lastdnum;
	
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

class ListManClass : public WindowClass {
public:
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	virtual uint64_t getNumElems() = 0;
	
	//int32_t menuCreate(HWND hwnd);
	//int32_t menuUse(HWND hwnd, int32_t menu_id);
	
	HWND parent;
	
protected:
	std::shared_ptr<PageListClass> plv;
	std::shared_ptr<StrListClass> slv;
	uint8_t option;
};

class DmanClass : public ListManClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	virtual uint64_t getNumElems() override;
	
	int32_t menuCreate(HWND hwnd);
	int32_t menuUse(HWND hwnd, int32_t menu_id);
	
protected:
	
	uint64_t inum;
};

class SDmanClass : public ListManClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	virtual uint64_t getNumElems() override;
	
	int32_t menuCreate(HWND hwnd);
	int32_t menuUse(HWND hwnd, int32_t menu_id);
	
protected:
	
	
	uint64_t inum;
};

class MainIndexManClass : public ListManClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	virtual uint64_t getNumElems() override;
	
	int32_t menuCreate(HWND hwnd);
	int32_t menuUse(HWND hwnd, int32_t menu_id);
	
	
};

class ThumbManClass : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	//virtual uint64_t getNumElems() override;
	
	
	
	PAGELISTV plv;
	THMBLISTV tlv;
	IMGVIEWV ivv;
	int8_t *dname, *tfstr;
	uint64_t dnum;
	uint8_t option;
	uint64_t nitems;
	HWND parent;
};

class PageListClass : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	bool isPainting();
	
	void startPaint();
	
	void pausePaint();
	
	uint64_t curPage, lastPage;
	int32_t hovered;
	
protected:
	std::shared_ptr<std::vector<struct ListPage>> pageListPtr;
	bool doPaint = false;
	
	void getPages(int32_t edge);
};

class StrListClass : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	uint64_t getSingleSelPos(void);
	
	uint32_t lastSel;
	int32_t xspos, yspos;
	uint8_t option;
	int16_t LastKey, drgpos;
	char hvrd, drag, timed;
	POINT point;
	
	bool setHeaders(std::shared_ptr<std::vector<std::string>> argHeaderPtr);
	
	bool setNColumns(int32_t);
	
	int32_t getNColumns(void);
	
	bool setColumnWidth(int32_t columnN, int32_t width);
	
	bool clearRows(void);
	
	bool assignRows(std::shared_ptr<std::vector<std::vector<std::string>>> inputVectorPtr);
	
	std::shared_ptr<std::vector<uint64_t>> getSelectedPositions(void) const;
	
	int64_t getNRows(void) const;
	
protected:
	std::shared_ptr<std::vector<std::string>> sectionHeadersPtr;
	std::shared_ptr<std::vector<std::vector<std::string>>> rowsPtr;
	std::shared_ptr<std::vector<uint8_t>> rowSelsPtr;
	std::shared_ptr<std::vector<int16_t>> columnWidthsPtr;
	
	int32_t nColumns;
	int64_t nRows;
	
};

class ThumbListClass : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	
	
	ImgF **thumb;
	oneslnk **strchn;
	uint8_t *ThumbSel;
	int32_t nThumbs, nColumns;
	uint32_t width, height;
	int32_t yspos;
	unsigned lastSel;
	int16_t LastKey;
	char hvrd, timed;
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
	uint64_t fnum;
	int32_t zoomp, xpos, ypos, xdrgpos, ydrgpos, dispframe;
	int64_t xdisp, ydisp, xdrgstart, ydrgstart;
	long double midX, midY;
	uint8_t fit, option, drag, timed;	// fit: 1 image is being shrunk to window, 2 means the display image is the original image (whole image fits), 4 means zoomed image fits horizontally and 8 vertically, 16: image is stretched and shrunk to window
};

class FileTagEditClass : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	
	
	uint64_t dnum;
	oneslnk *fnumchn;
	oneslnk *tagnumchn, *aliaschn;	// original tags and their aliases
	oneslnk *rmnaliaschn, *regaliaschn, *remtagnumchn, *addtagnumchn;
	uint8_t clean_flag;
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
	
	
	
	uint8_t clean_flag;
};

