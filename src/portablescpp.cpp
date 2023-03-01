#include <string>

#include <unicode/unistr.h>

std::string ToLowerCase(const std::string &str) {
	std::string res;
	return icu::UnicodeString::fromUTF8(str).toLower().toUTF8String(res);
}

std::string ToUpperCase(const std::string &str) {
	std::string res;
	return icu::UnicodeString::fromUTF8(str).toUpper().toUTF8String(res);
}

#include <windows.h>

std::string u16_to_u8(const std::wstring &wstr) {
	if( wstr.empty() ) return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo( size_needed, 0 );
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

std::string u16_cstr_to_u8(const wchar_t *wbuf) {
	if (wbuf == nullptr) {
		return std::string();
	} else {
		return u16_to_u8(std::wstring(wbuf));
	}
}

std::wstring u8_to_u16(const std::string &str) {
	if( str.empty() ) return std::wstring();
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo( size_needed, 0 );
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}