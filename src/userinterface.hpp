#pragma once

#include <windows.h>

#include <memory>
#include <map>

#include <boost/dynamic_bitset.hpp>

#include "errorobj.hpp"

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

struct SharedWindowVariables {
	HINSTANCE ghInstance;
	DWORD color;
	HFONT hFont, hFont2, hFontUnderlined;
	HPEN bgPen1, bgPen2, bgPen3, bgPen4, bgPen5, selPen, selPen2, hPen1;
	HBRUSH bgBrush1, bgBrush2, bgBrush3, bgBrush4, bgBrush5, selBrush;
	HBITMAP hListSliceBM, hThumbListBM;
	WNDPROC g_OldEditProc;
	struct wMemEntry **wMemArray;
	uint64_t wMemArrLen, nwMemWnds;

	HCURSOR hDefCrs, hCrsSideWE, hCrsHand;
};

struct WinProcArgs {
	HWND hwnd;
	UINT msg;
	WPARAM wParam;
	LPARAM lParam;
};

//}

class WinCreateArgs;

class PageListWindow;
class StrListWindow;

class WindowClass {
public:
	virtual ~WindowClass(void) = default;
	
	static std::shared_ptr<WindowClass> getWindowPtr(HWND hwnd);
	
	static std::shared_ptr<WindowClass> createWindowMemory(HWND hwnd, WinCreateArgs &winArgs);
	
	static boolean releaseWindowMemory(HWND hwnd);
	
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
	LPVOID lpParam_;
	
	WinCreateArgs(WindowClass *(*constructor_)(void));
	
};

class WinInstancer {
public:
	WinInstancer() = delete;
	WinInstancer(DWORD dwExStyle, LPCWSTR lpWindowName, DWORD dwStyle, int32_t X, int32_t Y, int32_t nWidth, int32_t nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
	
	const DWORD dwExStyle_;
	const LPCWSTR lpWindowName_;
	const DWORD dwStyle_;
	const int32_t x_;
	const int32_t y_;
	const int32_t nWidth_;
	const int32_t nHeight_;
	const HWND hWndParent_;
	const HMENU hMenu_;
	const HINSTANCE hInstance_;
	const LPVOID lpParam_;
	
	HWND create(std::shared_ptr<WinCreateArgs> winArgs_, const std::wstring &winClassName_);
};

class WindowHelper {
public:
	const std::wstring winClassName_;
	
	//void (*const modifyWinStruct)(WNDCLASSW &wc);

	WindowHelper &operator=(WindowHelper &) = delete;
	
	WindowHelper(const std::wstring, void (*)(WNDCLASSW &wc));
	WindowHelper(const std::wstring winClassName);
	WindowHelper() = delete;
	
	virtual bool registerWindowClass();
};

class DeferredRegWindowHelper : public WindowHelper {
public:
	DeferredRegWindowHelper(const std::wstring, void (*)(WNDCLASSW &wc));

	DeferredRegWindowHelper &operator=(DeferredRegWindowHelper &) = delete;
	
	virtual bool registerWindowClass() override;

	bool isRegistered(void) const;

private:
	void (*modifyWinStruct)(WNDCLASSW &wc);
	bool isRegistered_;
};

class MsgHandler : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

	// TODO: set to overrride later
	virtual LRESULT onCreate(WinProcArgs args);
	
	
	
	HWND wHandle_[2];
	HWND wHandle2_[1];
	uint32_t ndirman_ = 2;
	uint32_t nthmbman_ = 1;
	int32_t lastOption_, lastDirNum_;
	
	std::shared_ptr<void> hMapFile_;
	//HANDLE hMapFile;
};

class TabContainerWindow : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

	HWND wHandle_[2];
	HWND wHandle2_[1];
	unsigned int ndirman_ = 2;
	unsigned int nthmbman_ = 1;

	int lastOption_, lastDirNum_;

	HWND dispWindow_;
	
};

class TabWindow : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

	
	HWND wHandle_[2];
	HWND wHandle2_[1];
	unsigned int ndirman_ = 2;
	unsigned int nthmbman_ = 1;
	int lastOption_, lastDirNum_;
	
	
	
	HWND dispWindow_;
};

class ListManWindow : public WindowClass {
public:
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	virtual uint64_t getNumElems() = 0;
	
	HWND parent_;
	
	//int32_t menuCreate(HWND hwnd);
	//int32_t menuUse(HWND hwnd, int32_t menuID);
	
	virtual uint64_t getSingleSelID(void) const;
	
protected:
	std::shared_ptr<PageListWindow> pageListWin_;
	std::shared_ptr<StrListWindow> strListWin_;
	uint8_t winOptions_;
};

class DirManWindow : public ListManWindow {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	virtual uint64_t getNumElems() override;
	
	int32_t menuCreate(HWND hwnd);
	int32_t menuUse(HWND hwnd, int32_t menuID);
	
protected:
	
	uint64_t inum_;
};

class SubDirManWindow : public ListManWindow {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	virtual uint64_t getNumElems() override;
	
	int32_t menuCreate(HWND hwnd);
	int32_t menuUse(HWND hwnd, int32_t menuID);
	
protected:
	
	
	uint64_t inum_;
};

class MainIndexManWindow : public ListManWindow {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	virtual uint64_t getNumElems() override;
	
	int32_t menuCreate(HWND hwnd);
	int32_t menuUse(HWND hwnd, int32_t menuID);
	
	
};

class ThumbManWindow : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	//virtual uint64_t getNumElems() override;
	
	
	
	PAGELISTV pageListWin_;
	THMBLISTV tlv_;
	IMGVIEWV ivv_;
	int8_t *dname_, *tfstr_;
	uint64_t dnum_;
	uint8_t winOptions_;
	uint64_t nitems_;
	HWND parent_;
};

class PageListWindow : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	bool isPainting();
	
	void startPaint();
	
	void pausePaint();
	
	uint64_t curPage_, lastPage_;
	int32_t hovered_;
	
protected:
	std::shared_ptr<std::vector<struct ListPage>> pageListPtr_;
	bool doPaint_ = false;
	
	void getPages(int32_t edge);
};

class BoolSelectionList {
public:
	std::vector<uint8_t> selVec_;
	uint64_t len_ = 0;

};

class StrListWindow : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	static const int32_t kMinColWidth = 2;
	// arbitrary pixel cap
	static const int32_t kMaxColWidth = 20000;
	
	uint32_t lastSel_;
	int32_t xspos_, yspos_;
	uint8_t winOptions_;
	int16_t lastKey_, drgpos_;
	char hvrd_, isDragged_, isTimed_;
	POINT point_;
	
	inline int64_t getSingleSelPos(void) const;
	
	std::shared_ptr<std::vector<std::string>> getSingleSelRowCopy(void) const;
	
	std::shared_ptr<std::forward_list<int64_t>> getSelPositions(void) const;
	
	std::string getSingleSelRowCol(int32_t argCol, ErrorObject *retError = nullptr) const;
	
	bool setNColumns(int32_t);
	
	bool setHeaders(std::shared_ptr<std::vector<std::string>> argHeaderPtr);
	
	bool setColumnWidth(int32_t colArg, int32_t width);
	
	int32_t getNColumns(void) const;
	
	inline int32_t getTotalColumnsWidth(void) const;
	
	bool clearRows(void);
	
	bool assignRows(std::shared_ptr<std::vector<std::vector<std::string>>> inputVectorPtr);
	
	int64_t getNRows(void) const;
	
	bool clearSelections(void);
	
protected:
	int32_t nColumns_;
	int64_t nRows_;

	BoolSelectionList boolSelList_;
	
	std::shared_ptr<std::vector<std::string>> sectionHeadersPtr_;
	std::shared_ptr<std::vector<int16_t>> columnWidthsPtr_;
	std::shared_ptr<std::vector<std::vector<std::string>>> rowsPtr_;
	std::shared_ptr<boost::dynamic_bitset<>> rowSelsPtr_;
	
};

class ThumbListWindow : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	
	
	ImgF **thumbs_;
	oneslnk **strchn_;
	uint8_t *thumbSel_;

	int32_t nThumbs_, nColumns_;
	uint32_t width_, height_;
	int32_t yspos_;
	unsigned lastSel_;
	int16_t lastKey_;
	char hvrd_, isTimed_;
	POINT point_;
};

class ViewImageWindow : public WindowClass {
public:
	ViewImageWindow(void);
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	HBITMAP hThumbListBitmap;
	
	ImgF *fullImage_;
	ImgF *dispImage_;
	char *imagePath_;
	uint64_t firstNum_;
	int32_t zoomPercent_, xPos_, yPos_, xDragPos_, yDragPos_, displayFramePos_;
	int64_t displayXLength, displayYLength_, dragStartXPos_, dragStartYPos;
	long double midX_, midY_;
	uint8_t fitOption_, winOptions_, isDragged_, isTimed_;	// fit: 1 image is being shrunk to window, 2 means the display image is the original image (whole image fits), 4 means zoomed image fits horizontally and 8 vertically, 16: image is stretched and shrunk to window
};

class FileTagEditWindow : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	
	
	uint64_t dnum_;
	oneslnk *fnumchn_;
	oneslnk *tagnumchn_, *aliaschn_;	// original tags and their aliases
	oneslnk *rmnaliaschn_, *regaliaschn_, *remtagnumchn_, *addtagnumchn_;
	uint8_t clean_flag_;
};

class CreateAliasWindow : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	std::shared_ptr<FileTagEditWindow> parent_;
	
};

class EditWindowSuperClass : public WindowClass {
public:
//! TODO: make the helper a reference in each windowclass
	static DeferredRegWindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
};

class SearchBarWindow : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
};

class TextEditDialogWindow : public WindowClass {
public:
	static const WindowHelper helper;
	static HWND createWindowInstance(WinInstancer);
	
	virtual LRESULT CALLBACK winProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
	
	
	uint8_t clean_flag_;
};

