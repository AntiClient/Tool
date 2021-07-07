// clang-format off
#include "phnt_headers.h"
#include "defer.h"
#include "string_util.h"
#include "string_iterator.h"
#include "obfy.h"


#include <unordered_set>


#include <boost/crc.hpp>    
#include <boost/cstdint.hpp>
#include <vector>
#include <regex>
#include <sstream>
#include <fstream>

// clang-format on

#define PTR_ADD_OFFSET(ptr, offset) \
  ((PVOID)((ULONG_PTR)(ptr) + (ULONG_PTR)(offset)))

#define PAGE_SIZE 0x1000
#define BUFFER_SIZE (0x1000 * 64)  // 16 MB
#define DISPLAY_BUFFER_COUNT (0x1000 * 2 - 1)

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

bool fexists(const char *filename)
{
	std::ifstream ifile(filename);
	return ifile.good();
}


BOOLEAN CharIsPrintable[256] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    1,
    0,
    0,
    1,
    0,
    0,
    /* 0 - 15 */  // TAB, LF and CR are printable
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, /* 16 - 31 */
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1, /* ' ' - '/' */
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1, /* '0' - '9' */
    1,
    1,
    1,
    1,
    1,
    1,
    1, /* ':' - '@' */
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1, /* 'A' - 'Z' */
    1,
    1,
    1,
    1,
    1,
    1, /* '[' - '`' */
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1, /* 'a' - 'z' */
    1,
    1,
    1,
    1,
    0,
    /* '{' - 127 */  // DEL is not printable
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, /* 128 - 143 */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, /* 144 - 159 */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, /* 160 - 175 */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, /* 176 - 191 */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, /* 192 - 207 */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, /* 208 - 223 */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, /* 224 - 239 */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0 /* 240 - 255 */
};

__forceinline uint32_t fnv32a(const wchar_t* str) {
  auto hval = 0x811c9dc5;
  while (*str) {
    hval ^= static_cast<uint32_t>(*str++);
    hval *= 0x01000193;
  }
  return hval;
}

void IterateStrings(void* process_handle, int32_t min_string_length,
                    bool unicode_support, bool* found, std::string* client,
                    std::vector<std::pair<uint32_t, std::string>> targets,
					std::vector<std::string>* executables

) {
  if (!process_handle) return;

  const auto minimum_length = min_string_length;
  const auto detect_unicode = unicode_support;

  void* base_address = nullptr;

  size_t buffer_size = PAGE_SIZE * 64;
  size_t display_buffer_count = DISPLAY_BUFFER_COUNT;

  auto* buffer = reinterpret_cast<uint8_t*>(malloc(buffer_size));

  if (!buffer) return;

  std::vector<std::string> get_executables{};

  auto display_buffer = std::make_unique<wchar_t[]>(display_buffer_count + 1);

  MEMORY_BASIC_INFORMATION basic_info = {0};
  while (NT_SUCCESS(NtQueryVirtualMemory(
      process_handle, base_address, MemoryBasicInformation, &basic_info,
      sizeof(MEMORY_BASIC_INFORMATION), NULL))) {
    ULONG_PTR offset;
	size_t read_size{};

    ULONG type_mask = 0;

    type_mask |= MEM_PRIVATE;
    type_mask |= MEM_MAPPED;
    type_mask |= MEM_IMAGE;

    if (basic_info.State != MEM_COMMIT) goto ContinueLoop;

    if ((basic_info.Type & type_mask) == 0) goto ContinueLoop;

    if (basic_info.Protect == PAGE_NOACCESS) goto ContinueLoop;
    if (basic_info.Protect & PAGE_GUARD) goto ContinueLoop;

    read_size = basic_info.RegionSize;

    if (basic_info.RegionSize > buffer_size) {
      if (basic_info.RegionSize <= 16 * 1024 * 1024)  // 16 MB
      {
        free(buffer);
        buffer_size = basic_info.RegionSize;
        buffer = reinterpret_cast<uint8_t*>(malloc(buffer_size));

        if (!buffer) break;
      } else {
        read_size = buffer_size;
      }
    }

    for (offset = 0; offset < basic_info.RegionSize; offset += read_size) {
      ULONG_PTR i;
      UCHAR byte;   // current byte
      UCHAR byte1;  // previous byte
      UCHAR byte2;  // byte before previous byte
      BOOLEAN printable;
      BOOLEAN printable1;
      BOOLEAN printable2;
      ULONG length;

      if (!NT_SUCCESS(NtReadVirtualMemory(process_handle,
                                          PTR_ADD_OFFSET(base_address, offset),
                                          buffer, read_size, NULL)))
        continue;

      byte1 = 0;
      byte2 = 0;
      printable1 = FALSE;
      printable2 = FALSE;
      length = 0;

      for (i = 0; i < read_size; i++) {
        byte = buffer[i];

        if (detect_unicode && !iswascii(byte))
          printable = !!iswprint(byte);
        else
          printable = CharIsPrintable[byte];

        if (printable2 && printable1 && printable) {
          if (length < display_buffer_count) display_buffer[length] = byte;

          length++;
        } else if (printable2 && printable1 && !printable) {
          if (length >= minimum_length) {
            goto CreateResult;
          } else if (byte == 0) {
            length = 1;
            display_buffer[0] = byte1;
          } else {
            length = 0;
          }
        } else if (printable2 && !printable1 && printable) {
          if (byte1 == 0) {
            if (length < display_buffer_count) display_buffer[length] = byte;

            length++;
          }
        } else if (printable2 && !printable1 && !printable) {
          if (length >= minimum_length) {
            goto CreateResult;
          } else {
            length = 0;
          }
        } else if (!printable2 && printable1 && printable) {
          if (length >=
              minimum_length +
                  1)  // length - 1 >= minimumLength but avoiding underflow
          {
            length--;  // exclude byte1
            goto CreateResult;
          } else {
            length = 2;
            display_buffer[0] = byte1;
            display_buffer[1] = byte;
          }
        } else if (!printable2 && printable1 && !printable) {
          // Nothing
        } else if (!printable2 && !printable1 && printable) {
          if (length < display_buffer_count) display_buffer[length] = byte;

          length++;
        } else if (!printable2 && !printable1 && !printable) {
          // Nothing
        }

        goto AfterCreateResult;

      CreateResult : {
        const auto display_length =
            min(length, display_buffer_count) * sizeof(wchar_t);

        auto result_buffer = std::make_unique<wchar_t[]>(display_length + 1);
        memcpy(result_buffer.get(), display_buffer.get(), display_length);

		const auto string = result_buffer.get();
		std::wstring ws(string);
		std::string str(ws.begin(), ws.end());
		std::regex e("^file:\/\/\/.*.exe");

		if (std::regex_match(ws.begin(), ws.end(), e)) {
			str = str.substr(8, str.length());
			replaceAll(str, "%20", " ");
			if (!fexists(str.c_str())) str = str + " (Deleted)";
			get_executables.insert(get_executables.begin(), str);
		}
        const auto hash = fnv32a(result_buffer.get());

        for (auto& target : targets) {
          if (hash == target.first) {
            *found = true;
			*client = target.second;
            goto Exit;
          }
        }

        length = 0;
      }
      AfterCreateResult:

        byte2 = byte1;
        byte1 = byte;
        printable2 = printable1;
        printable1 = printable;
      }
    }

  ContinueLoop:
    base_address = PTR_ADD_OFFSET(base_address, basic_info.RegionSize);
  }
  *executables = get_executables;
Exit:
  if (buffer) free(buffer);
}
