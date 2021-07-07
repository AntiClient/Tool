#pragma once
#include <climits>

namespace ss {

	class pattern {

		const unsigned char *value_;
		const unsigned char *mask_;
		const size_t length_;

		size_t bad_char_skip_[UCHAR_MAX + 1]{};

		void initialize_skip_table();

	public:

		const char *name;

		pattern(const char *name, const char *value, const char *mask, size_t length);

		bool find_pattern(const unsigned char *buffer, size_t buffer_length);

	};

}