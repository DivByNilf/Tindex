

#include <string>

std::string ToLowerCase(const std::string &str);

std::string ToUpperCase(const std::string &str);

std::string u16_to_u8(const std::wstring &wstr);

std::string u16_cstr_to_u8(const wchar_t *wbuf);

std::wstring u8_to_u16(const std::string &str);