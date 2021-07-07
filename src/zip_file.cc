#include "zip_file.h"
#include "phnt_headers.h"
#include "third_party\miniz.h"
#include <cstring>
#include <fstream>

ZipFile::ZipFile() : archive_(new mz_zip_archive()) { Reset(); }

ZipFile::ZipFile(const std::wstring &filename) : ZipFile() { Load(filename); }

ZipFile::ZipFile(const std::vector<uint8_t> &bytes) : ZipFile() { Load(bytes); }

ZipFile::~ZipFile() { Reset(); }



void ZipFile::Load(const std::wstring &file) {
  filename_ = file;
  uint8_t *f_data = nullptr;
  auto *f = _wfopen(file.c_str(), L"rb");
  long f_size = -1;
  size_t f_size_read = 0;
  auto size_out = 0;
  if (fseek(f, 0, SEEK_END) || (f_size = ftell(f)) == -1 ||
      fseek(f, 0, SEEK_SET)) {
    fclose(f);
  }
  f_data = reinterpret_cast<uint8_t *>(malloc(f_size));
  f_size_read = f_size > 0 ? fread(f_data, 1, f_size, f) : 0;
  fclose(f);
  if (f_size_read == 0 || f_size_read != static_cast<size_t>(f_size)) {
    free(f_data);
  }
  size_out = static_cast<int>(f_size);
  Reset();
  buffer_ = std::vector<uint8_t>(&f_data[0], &f_data[size_out]);
  StartRead();
}

void ZipFile::Load(const std::vector<uint8_t> &bytes) {
  Reset();
  buffer_.assign(bytes.begin(), bytes.end());
  StartRead();
}

bool ZipFile::Good() const {
  return archive_->m_zip_mode != MZ_ZIP_MODE_INVALID;
}

void ZipFile::StartRead() {
  if (archive_->m_zip_mode == MZ_ZIP_MODE_READING) {
    return;
  }

  if (!mz_zip_reader_init_mem(archive_.get(), buffer_.data(), buffer_.size(),
                              0)) {
    Reset();
  }
}

void ZipFile::Reset() {
  if (archive_->m_zip_mode == MZ_ZIP_MODE_READING) {
    mz_zip_reader_end(archive_.get());
  }

  buffer_.clear();
  buffer_.shrink_to_fit();
  archive_->m_zip_mode = MZ_ZIP_MODE_INVALID;
}

size_t ZipFile::Count() const {
  return mz_zip_reader_get_num_files(archive_.get());
}

mz_zip_archive *ZipFile::GetArchive() { return archive_.get(); }

bool ZipFile::ForEach(const std::function<bool(const ZipInfo &)> &cb) {
  for (size_t iii = 0; iii < Count(); iii++) {
    const auto info = GetInfo(iii);
    if (info.file_index == kInvalidZipInfo) {
      return false;
    }
    if (!cb(info)) {
      break;
    }
  }
  return true;
}

bool ZipFile::Read(const ZipInfo &file, std::vector<uint8_t> *buf) {
  if (buf && !file.is_directory && file.file_index != kInvalidZipInfo) {
    auto size = size_t{0};
    auto data = static_cast<uint8_t *>(mz_zip_reader_extract_to_heap(
        archive_.get(), static_cast<mz_uint>(file.file_index), &size, 0));
    if (data) {
      if (buf->capacity() < size) {
        buf->reserve(size);
      }
      std::copy(data, data + size, std::back_inserter(*buf));
      mz_free(data);
      return true;
    }
  }
  return false;
}

std::vector<uint8_t> ZipFile::Read(const ZipInfo &file) {
  std::vector<uint8_t> buf;
  Read(file, &buf);
  return buf;
}

ZipInfo ZipFile::GetInfo(const uint32_t index) {
  mz_zip_archive_file_stat stat;
  if (!mz_zip_reader_file_stat(archive_.get(), static_cast<mz_uint>(index),
                               &stat)) {
    return {};
  }

  ZipInfo zip_info;
  zip_info.file_index = stat.m_file_index;
  zip_info.file_name = stat.m_filename;
 
  zip_info.archive = archive_.get();
  zip_info.is_directory =
      mz_zip_reader_is_file_a_directory(archive_.get(), stat.m_file_index);
  return zip_info;
}