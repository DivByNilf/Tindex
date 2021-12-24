#pragma once

#include <iostream>

#include "stdint.h"

class ErrorObject {
public:
	std::string msg;
	int32_t error_code = 0;
	
	ErrorObject(int32_t error_code_ = 0, const std::string &msg_ = std::string()) : error_code{error_code_}, msg{msg_} {}
	ErrorObject(const std::string &msg_) : error_code{1}, msg{msg_} {}

	inline bool hasError(void) const {
		error_code > 0;
	}
	
	inline bool hasNote(void) const {
		error_code < 0;
	}
	
	inline bool hasNothing(void) const {
		error_code == 0;
	}
	
	explicit operator bool() {
		return !this->hasNothing();
	}
	
	operator =(const int32_t &other) {
		*this = ErrorObject(other);
	}
};

template <class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const ErrorObject& x) {
	if (x.msg != std::string()) {
		return os << "ErrorObject(" << x.error_code << ")";
	} else {
		return os << "ErrorObject(" << x.error_code << ", \"" << x.msg << "\")";
	}
}

bool setErrorPtr(ErrorObject *inPtr, const ErrorObject &inObj);

bool setErrorPtr(ErrorObject *inPtr, const int32_t &inCode);

bool setErrorPtrOrPrint(ErrorObject *inPtr, std::ostream &os, const ErrorObject &inObj);