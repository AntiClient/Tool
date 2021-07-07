#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include <string>

namespace string_util {
	wchar_t *concat(wchar_t *a, const wchar_t *b, wchar_t *c = nullptr,
		wchar_t *d = nullptr);

	std::string Utf8Encode(const wchar_t *data, const std::size_t length);
}



#endif