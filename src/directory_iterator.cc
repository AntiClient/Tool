// clang-format off
#include "phnt_headers.h"
#include "defer.h"
#include "string_util.h"
#include "directory_iterator.h"
// clang-format on

DirectoryIterator::DirectoryIterator(const wchar_t *base_directory) {
  base_dir_ = static_cast<wchar_t *>(malloc(wcslen(base_directory) * 2));
  lstrcpyW(base_dir_, base_directory);
}

DirectoryIterator::~DirectoryIterator() {}

bool DirectoryIterator::ForEach(
    const std::function<bool(wchar_t *full_name, void *file_data)> &cb) {
  auto wildcard_name = string_util::concat(base_dir_, L"\\*");
  defer { free(wildcard_name); };

  WIN32_FIND_DATAW find_data{};
  const auto handle = FindFirstFileW(wildcard_name, &find_data);
  if (handle == INVALID_HANDLE_VALUE) return false;

  while (true) {
    auto full_name = string_util::concat(base_dir_, L"\\", find_data.cFileName);
    defer { free(full_name); };

    if (!cb(full_name, &find_data)) break;

    const auto success = FindNextFileW(handle, &find_data);
    if (!success) break;
  }
  FindClose(handle);
  return true;
}
