#ifndef STRING_ITERATOR_H
#define STRING_ITERATOR_H
// clang-format off
#include <functional>
#include <string>
#include <vector>
#include <unordered_set>
// clang-format on

using StringCallback = std::function<bool(uint32_t hash)>;

void IterateStrings(void* process_handle, int32_t min_string_length,
                    bool unicode_support, bool* found, std::string* client,
                    std::vector<std::pair<uint32_t, std::string>> targets,
					std::vector<std::string>* executables);

#endif
