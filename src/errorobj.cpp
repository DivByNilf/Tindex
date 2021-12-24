#include "errorobj.hpp"

bool setErrorPtr(ErrorObject *inPtr, const ErrorObject &inObj) {
	if (inPtr) {
		*inPtr = inObj;
		return true;
	} else {
		return false;
	}
}

bool setErrorPtr(ErrorObject *inPtr, const int32_t &inCode) {
	if (inPtr) {
		*inPtr = inCode;
		return true;
	} else {
		return false;
	}
}

bool setErrorPtrOrPrint(ErrorObject *inPtr, std::ostream &os, const ErrorObject &inObj) {
	if (inPtr) {
		*inPtr = inObj;
		return true;
	} else {
		os << "setErrorPtrOrPrint: " << inObj << std::flush;
		return false;
	}
}