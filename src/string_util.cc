#include "string_util.h"

#include <Windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

wchar_t * string_util::concat(wchar_t *a, const wchar_t *b, wchar_t *c,
	wchar_t *d) {
	// Concatenate up to 4 wide strings together. Allocated with malloc.
	// If you don't like that, use a programming language that actually
	// helps you with using custom allocators. Or just edit the code.

	const auto len_a = wcslen(a);
	const auto len_b = wcslen(b);

	auto len_c = 0;
	if (c)
		len_c = wcslen(c);

	auto len_d = 0;
	if (d)
		len_d = wcslen(d);

	const auto result = static_cast<wchar_t *>(malloc((len_a + len_b + len_c + len_d + 1) * 2));
	memcpy(result, a, 2 * len_a);
	memcpy(result + len_a, b, len_b * 2);

	if (c)
		memcpy(result + len_a + len_b, c, len_c * 2);
	if (d)
		memcpy(result + len_a + len_b + len_c, d, len_d * 2);

	result[len_a + len_b + len_c + len_d] = 0;

	return result;
}

std::string string_util::Utf8Encode(const wchar_t *data, const std::size_t length) {
	if (length == 0)
		return std::string();

	const auto size_required =
		WideCharToMultiByte(CP_UTF8, 0, data, length, NULL, 0, NULL, NULL);

	std::string encoded(size_required, '\0');
	WideCharToMultiByte(CP_UTF8, 0, data, length, encoded.data(), size_required,
		NULL, NULL);
	return encoded;
}