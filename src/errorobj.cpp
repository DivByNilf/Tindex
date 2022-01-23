#include "errorobj.hpp"

ErrorObject::ErrorObject(int32_t errorCode, const std::string &msg) :
	errorCode_{errorCode},
	msg_{msg}
{}

ErrorObject::ErrorObject(const std::string &msg) : errorCode_{1}, msg_{msg} {}

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