#ifndef DIRECTORY_ITERATOR_H
#define DIRECTORY_ITERATOR_H
// clang-format off
#include <string_view>
#include <functional>
// clang-format on

class DirectoryIterator {
public:
  explicit DirectoryIterator(const wchar_t *base_directory);
  ~DirectoryIterator();

  bool ForEach(const std::function<bool(wchar_t *full_name,
                                        void* file_data)> &cb);

private:
  wchar_t *base_dir_;
};

#endif