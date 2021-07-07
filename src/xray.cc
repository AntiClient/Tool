// clang-format off




#include "third_party/stb/stb_image.h"

#include "directory_iterator.h"
#include "zip_file.h"
#include "string_util.h"
#define NOMINMAX
#include "cx_encrypted_string.h"
#include "defer.h"
#include "phnt_headers.h"
#include "xray.h"
// clang-format on

std::vector<std::string> RunXrayPackCheck(const wchar_t *resource_pack_directory/*,
                      GenericCheck *generic_check*/) {
  DirectoryIterator iterator(resource_pack_directory);

  auto transparent_count = 0;

  std::vector<std::string> xRayPacks;

  auto result = iterator.ForEach([&](wchar_t *full_name,
                                     void *file_data) -> bool {
    auto *find_data = static_cast<WIN32_FIND_DATAW *>(file_data);

    if (find_data->dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) {
      auto utf8_name = string_util::Utf8Encode(full_name, wcslen(full_name));

      ZipFile zip_file(full_name);
      if (zip_file.Good()) {
        auto found_texture = false;
        auto is_valid_zip =
            zip_file.ForEach([&](const ZipInfo &zip_info) -> bool {
			
			if (zip_info.file_name ==
				R"(assets/minecraft/textures/blocks/stone.png)") {
				found_texture = true;
				/*    auto bytes = zip_file.Read(zip_info);

					int width, height, channels;
					auto *image_data = stbi_load_from_memory(
						bytes.data(), static_cast<int>(bytes.size()), &width,
						&height, &channels, 3);

					defer { stbi_image_free(image_data); };

					for (auto y = 0; y < height; y++) {
					  for (auto x = 0; x < width; x++) {
						auto *pixel =
							image_data + (height + y * width + x) * channels;

						const auto alpha = (channels >= 4) ? pixel[3] : 0xFF;

						if (alpha != 255) {
							if (transparent_count++ > 50) {
								generic_check->message =
									charenc("Xray ResourcePack Found: \n") +
									std::string(utf8_name) + "\n";

								generic_check->status = CheckStatus::kFailed;
								//return false;
						}

						}
					  }
					}
				  }
				*/
			}
              return true;
            });
        if (!is_valid_zip) {
			/*
          MessageBoxA(nullptr, charenc("Invalid Zip File Found."),
                      charenc("Error"), MB_OK);*/
		  xRayPacks.push_back("Invalid Zip File Found.");
          return true;
        } else if (!found_texture) {
			xRayPacks.push_back(std::string(utf8_name));
			/*
          generic_check->message =
              charenc("Xray ResourcePack Found: \n") + std::string(utf8_name);

          generic_check->status = CheckStatus::kFailed;*/
          return true;
        }
      }
	  transparent_count = 0;
    }
    return true;
  });
  return xRayPacks;
}