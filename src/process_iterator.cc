// clang-format off
#include "process_iterator.h"
#include "defer.h"
#include "phnt_headers.h"
#include "string_util.h"
// clang-format on

static std::string FormatDateTime(PSYSTEMTIME DateTime) {
  const auto time_buffer_size =
      GetTimeFormat(LOCALE_USER_DEFAULT, 0, DateTime, NULL, NULL, 0);
  const auto date_buffer_size =
      GetDateFormatW(LOCALE_USER_DEFAULT, 0, DateTime, NULL, NULL, 0);

  const auto size_required = time_buffer_size + date_buffer_size + 1;

  auto buffer = std::make_unique<char[]>(size_required + 1);

  if (!GetTimeFormatA(LOCALE_USER_DEFAULT, 0, DateTime, NULL, buffer.get(),
                      time_buffer_size)) {
    return nullptr;
  }

  auto count = strlen(buffer.get());

  buffer[count] = ' ';

  if (!GetDateFormatA(LOCALE_USER_DEFAULT, 0, DateTime, NULL,
                      &buffer[count + 1], date_buffer_size)) {
    return nullptr;
  }

  return buffer.get();
}

void IterateProcesses(const ProcessCallback &cb) {
  ULONG buffer_length = 0;

  auto status = NtQuerySystemInformation(SystemProcessInformation, nullptr, 0,
                                         &buffer_length);

  auto buffer = malloc(buffer_length);
  if (!buffer) return;

  defer { free(buffer); };

  status = NtQuerySystemInformation(SystemProcessInformation, buffer,
                                    buffer_length, &buffer_length);
  if (!NT_SUCCESS(status)) return;

  auto process_info = static_cast<SYSTEM_PROCESS_INFORMATION *>(buffer);

  while (process_info->NextEntryOffset) {
    FILETIME creation_time;
    creation_time.dwLowDateTime = process_info->CreateTime.LowPart;
    creation_time.dwHighDateTime = process_info->CreateTime.HighPart;

    SYSTEMTIME converted_ct;
    FILETIME local_file_time;
    FileTimeToLocalFileTime(&creation_time, &local_file_time);
    FileTimeToSystemTime(&local_file_time, &converted_ct);

    const auto time_stamp = FormatDateTime(&converted_ct);

    const auto process_name = string_util::Utf8Encode(
        process_info->ImageName.Buffer,
        process_info->ImageName.Length / sizeof(wchar_t));

    cb(process_name, reinterpret_cast<uint32_t>(process_info->UniqueProcessId),
       reinterpret_cast<uint32_t>(process_info->InheritedFromUniqueProcessId),
       time_stamp);

    process_info = reinterpret_cast<SYSTEM_PROCESS_INFORMATION *>(
        reinterpret_cast<uint8_t *>(process_info) +
        process_info->NextEntryOffset);
  }
}
