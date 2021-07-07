#include "pattern_check.h"
#include <ThemidaSDK.h>

void ss::pattern::initialize_skip_table()
{
	//VM_DOLPHIN_RED_START

	for (size_t i = 0; i <= UCHAR_MAX; i++)
		bad_char_skip_[i] = length_;

	const auto last = length_ - 1;

	for (size_t i = 0; i < last; i++)
		if (mask_[i] != '?')
			bad_char_skip_[value_[i]] = last - i;

	//VM_DOLPHIN_RED_END
}

ss::pattern::pattern(const char *name, const char *value, const char *mask, const size_t length) : value_(reinterpret_cast<const unsigned char *>(value)), mask_(reinterpret_cast<const unsigned char *>(mask)), length_(length), name(name)
{
	initialize_skip_table();
}

bool ss::pattern::find_pattern(const unsigned char *buffer, size_t buffer_length)
{
	//VM_DOLPHIN_RED_START

	size_t scan = 0;
	const auto last = length_ - 1;

	while (buffer_length >= length_)
	{
		for (scan = last; mask_[scan] == '?' || buffer[scan] == value_[scan]; scan--)
			if (scan == 0)
				return true;

		const auto skip_amount = bad_char_skip_[buffer[last]];

		buffer_length -= skip_amount;
		buffer += bad_char_skip_[buffer[last]];
	}
	
	//VM_DOLPHIN_RED_END

	return false;
}
