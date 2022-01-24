#pragma once

#include <iostream>

#include "stdint.h"

class ErrorObject {
public:
	std::string msg_;
	int32_t errorCode_ = 0;
	
	ErrorObject(int32_t errorCode__ = 0, const std::string &msg = std::string());
	ErrorObject(const std::string &msg);

	inline bool hasError(void) const {
		return errorCode_ > 0;
	}
	
	inline bool hasNote(void) const {
		return errorCode_ < 0;
	}
	
	inline bool hasNothing(void) const {
		return errorCode_ == 0;
	}
	
	explicit operator bool() {
		return !this->hasNothing();
	}
	
	ErrorObject operator=(const ErrorObject &other) {
		*this = ErrorObject(other);
		return *this;
	}
};

template <class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const ErrorObject& x) {
	if (x.msg_ != std::string()) {
		return os << "ErrorObject(" << x.errorCode_ << ")";
	} else {
		return os << "ErrorObject(" << x.errorCode_ << ", \"" << x.msg_ << "\")";
	}
}

bool setErrorPtr(ErrorObject *inPtr, const ErrorObject &inObj);

bool setErrorPtr(ErrorObject *inPtr, const int32_t &inCode);

bool setErrorPtrOrPrint(ErrorObject *inPtr, std::ostream &os, const ErrorObject &inObj);