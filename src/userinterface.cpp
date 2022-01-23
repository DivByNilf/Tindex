#include "userinterface.hpp"
#include "uiutils.hpp"
#include "prgdir.hpp"

#include "errorf.hpp"
#define errorf(str) g_errorfStdStr(str)

#include <filesystem>
namespace std {
	namespace fs = std::filesystem;
}


extern "C" {
	#include "stringchains.h"
	#include "dupstr.h"
	#include "arrayarithmetic.h"
	#include "bytearithmetic.h"
	#include "tfiles.h"
}

//  TODO: see if this should instead be a method for a window class

void DelRer(HWND hwnd, uint8_t option, STRLISTV const *slv) {	// changing would require replacing option and slv with nRows, StrListSel, strchn[0] and strchn[1]
	wchar_t *wbuf;
	oneslnk *link, *flink, *inlink;
	int i, j, pos;
	char *buf, confirmed = 0;
	RECT rect;

	//char (*rerf)(uint64_t dnum, char *dpath) = dReroute;
	//char (*rmvf)(uint64_t dnum) = dRemove;
	//int (*crmvf)(oneslnk *dnumchn) = chainDRemove;
	char (*rerf)(uint64_t dnum, char *dpath) = NULL;
	char (*rmvf)(uint64_t dnum) = NULL;
	int (*crmvf)(oneslnk *dnumchn) = NULL;

	link = flink = (oneslnk *) malloc(sizeof(oneslnk));
	for (pos = 0, i = 0; pos < slv->nRows; pos++) {
		if (slv->StrListSel[(pos)/8] & (1 << pos%8)) {
			link = link->next = (oneslnk *) malloc(sizeof(oneslnk));
			link->ull = ctou(slv->strs[0][pos]);
			i++;
		}
	}
	link->next = 0;
	if (pos != slv->nRows) {
		errorf("didn't go through all rows");
		killoneschn(link, 0);
	}
	link = flink, flink = flink->next;
	free(link);
	if (i == 0)
		return;

	switch(option) {

		case 1:

			buf = (char *) malloc(46+20+MAX_PATH*4);
			wbuf = (wchar_t *) malloc(2*(46+20+MAX_PATH));
			if (i == 1) {
				char *temp;
				char *SelDir = utoc(flink->ull);
				for (pos = 0; pos < slv->nRows; pos++) {
					if (slv->StrListSel[pos/8] & (1 << pos%8)) {
						break;
					}
				}
				if (pos < slv->nRows && slv->strs[1][pos]) {
					sprintf(buf, "Are you sure you want to remove Directory %s: \"%s\"?", SelDir, slv->strs[1][pos]);
				} else {
					sprintf(buf, "Are you sure you want to remove Directory %s?", SelDir);
				}
				if ((MultiByteToWideChar(65001, 0, buf, -1, wbuf, 45+20+MAX_PATH*4)) == 0) {
					errorf("MultiByteToWideChar Failed");
				}
				if (MessageBoxW(hwnd, wbuf, L"Confirm", MB_YESNO) == IDYES) {
					rmvf(flink->ull);
					confirmed = 1;
				}
				free(SelDir);
			} else {
				sprintf(buf, "Are you sure you want to remove %d directories?", i);
				if ((MultiByteToWideChar(65001, 0, buf, -1, wbuf, 46+7)) == 0) {
					errorf("MultiByteToWideChar Failed");
				}
				if (MessageBoxW(hwnd, wbuf, L"Confirm", MB_YESNO) == IDYES) {
					crmvf(flink);
					confirmed = 1;
				}
			}
			free(buf), free(wbuf);

			if (confirmed) {
				for (pos = 0; pos < slv->nRows; pos++) {
					if (slv->StrListSel[(pos)/8] & (1 << pos%8)) {
						j = pos;
					}
				}
				for (pos = slv->nRows/8+!!(slv->nRows%8)-1; pos >= 0; pos--) {
					if (pos < 0)
						break;
					slv->StrListSel[pos] = 0;
				}
				if (slv->nRows-i != 0) {
					if (j > slv->nRows-i-1) {
						j = slv->nRows-i-1;
					}
					slv->StrListSel[(j)/8] |= (1 << j%8);
				}
			}

			break;

		case 2:
		case 3:

			if (GetClientRect(hwnd, &rect) == 0) {
				errorf("GetClientRect failed");
			}
			buf = (char *) malloc(MAX_PATH*4);
			pos = i;

			for (link = flink; link != 0; link = link->next) {	//! change binfo.lpszTitle to include dnum and dpath of dir being rerouted
				//! TODO: add error return argument and print in window
				std::fs::path retPath = SeekDir(hwnd);
				if (!retPath.empty()) {
					bool abort = false;

					if (option == 3) {

						//! TODO: add error return argument and print in window
						retPath = makePathRelativeToProgDir(retPath);

						if (!retPath.empty()) {
							if (!std::fs::is_directory(g_fsPrgDir / retPath)) {
								errorf("retPath was not directory");
								g_errorfStream << "retPath was: " << retPath << std::flush;

								retPath.clear();
							}
						}
					}

					if (retPath.empty()) {
						abort = true;
					}

					if (!abort) {
						if (pos == 1) {
							rerf(link->ull, buf);
						} else {
							rerf(link->ull, buf);		//! add to queue here
						}
					}
				} else if (link->next != 0) {
					if (MessageBoxW(hwnd, L"Stop rerouting process?", L"Confirm", MB_YESNO) == IDYES) {
						break;
					}
				}
			}
			if (pos != 1 && MessageBoxW(hwnd, L"Apply changes?", L"Confirm", MB_YESNO) == IDYES) {
				//! pop queue here
			} else {

			}
			free(buf);

			break;
	}
	killoneschn(flink, 1);
}