#include "checks.h"

#include "defer.h"
#include "directory_iterator.h"
#include "process_iterator.h"
#include "string_iterator.h"

#include <string>
#include <unordered_set>
#include <ThemidaSDK.h>

#define NOMINMAX
#include "cx_encrypted_string.h"
#include "phnt_headers.h"

#include "asm_check.h"
#include <TlHelp32.h>
#include <algorithm>
#include <cwctype>
#include <sstream>
#include <regex>

void check_modules(const HANDLE process, GenericCheck *generic_check);

NTSTATUS NiggerOpenProcess(_Out_ PHANDLE ProcessHandle,
                           _In_ ACCESS_MASK DesiredAccess,
                           _In_ HANDLE ProcessId) {
  OBJECT_ATTRIBUTES object_attributes;
  CLIENT_ID id;

  id.UniqueProcess = ProcessId;
  id.UniqueThread = NULL;


  InitializeObjectAttributes(&object_attributes, NULL, 0, NULL, NULL);
  auto status =
      NtOpenProcess(ProcessHandle, DesiredAccess, &object_attributes, &id);

  return status;
}

std::string FormatDateTime(PSYSTEMTIME DateTime) {
	//VM_DOLPHIN_RED_START
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
  //VM_DOLPHIN_RED_END
  return buffer.get();
}

std::vector<Directory> GetRecycleBinWriteDates() {
	//VM_DOLPHIN_RED_START
  std::vector<Directory> write_dates;
  DirectoryIterator iter(LR"(C:\$Recycle.Bin\)");
  iter.ForEach([&](wchar_t *name, void *file_data) -> bool {
    auto *data = (WIN32_FIND_DATAW *)file_data;
    auto path = std::wstring(name);

    if (data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      SYSTEMTIME system_time{};
      FILETIME local_time{};

      FileTimeToLocalFileTime(&data->ftLastWriteTime, &local_time);
      FileTimeToSystemTime(&local_time, &system_time);

      const auto formatted_time = FormatDateTime(&system_time);

      Directory directory;
      directory.write_date = formatted_time;
      directory.dir = std::string(path.begin(), path.end());

      write_dates.push_back(directory);
    }
    return true;
  });
  //VM_DOLPHIN_RED_END
  return write_dates;
}

std::vector<std::string> executedFiles = {};

std::vector<std::string> executed_files() {
	return executedFiles;
}

std::vector<std::string> GetPlayerAccounts(
    const nlohmann::json &launcher_profile) {
	//VM_DOLPHIN_RED_START
  std::vector<std::string> accounts;
  for (auto i = launcher_profile["authenticationDatabase"].begin();
       i != launcher_profile["authenticationDatabase"].end(); ++i) {
    for (auto ii = i.value().begin(); ii != i.value().end(); ++ii) {
      if (ii.key() == "profiles") {
        for (auto iii = ii.value().begin(); iii != ii.value().end(); ++iii) {
          for (auto profile = iii.value().begin(); profile != iii.value().end();
               ++profile) {
            if (profile.key() == "displayName") {
              accounts.push_back(profile.value().get<std::string>());
            }
          }
        }
      }
    }
  }
  //VM_DOLPHIN_RED_END
  return accounts;
}

std::vector<std::string> GetEventsChangeTime(
	const nlohmann::json &launcher_profile) {
	//VM_DOLPHIN_RED_START
	std::vector<std::string> times;
	for (auto i = launcher_profile.begin();
		i != launcher_profile.end(); ++i) {
		for (auto ii = i.value().begin(); ii != i.value().end(); ++ii) {
			if (ii.key() == "TimeCreated") {
				times.push_back(ii.value().get<std::string>());
			}
		}
	}
	//VM_DOLPHIN_RED_END
	return times;
}

constexpr const auto FNV1_32A_INIT = 0x811c9dc5;
constexpr const auto FNV_32_PRIME = 0x01000193;

const uint32_t FNV_32_HASH_START = 2166136261UL;
const uint64_t FNV_64_HASH_START = 14695981039346656037ULL;

inline uint32_t fnv32a(const wchar_t *str) {
	//VM_DOLPHIN_RED_START
  uint32_t hval = FNV1_32A_INIT;

  for (int i = 0; i < wcslen(str) * 2; i++) {
    hval ^= static_cast<uint32_t>(*str++);
    hval *= FNV_32_PRIME;
  }
  //VM_DOLPHIN_RED_END
  return hval;
}

std::vector<StartTime> GetProcessStartTimes(nlohmann::json &data) {
	//VM_DOLPHIN_RED_START
  std::vector<StartTime> start_times{};
  IterateProcesses([&](const std::string &process_name, uint32_t process_id,
                       uint32_t parent_id,
                       const std::string &start_date) -> void {
    for (auto &process : data["start_time_check"]) {
      if (process == process_name) {
        start_times.push_back({process_name, start_date});
      }
    }
  });
  //VM_DOLPHIN_RED_END
  return start_times;
}

PVOID GetPEBPtr(const HANDLE process) {
	//VM_DOLPHIN_RED_START
  PROCESS_BASIC_INFORMATION basic_info = {0};
  if (NT_SUCCESS(NtQueryInformationProcess(process, ProcessBasicInformation,
                                           &basic_info, sizeof(basic_info),
                                           nullptr))) {
	  //VM_DOLPHIN_RED_END
    return basic_info.PebBaseAddress;
  }
  
  return nullptr;
}

typedef enum _PEB_OFFSET { kCurrentDirectory, kCommandLine } PEB_OFFSET;

bool GetProcessPebString(const HANDLE process, const PEB_OFFSET peb_offset,
                         std::wstring *out) {
  ULONG offset;

#define PEB_OFFSET_CASE(e, f)                              \
  case e:                                                  \
    offset = FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, f); \
    break

  switch (peb_offset) {
    PEB_OFFSET_CASE(kCurrentDirectory, CurrentDirectory);
    PEB_OFFSET_CASE(kCommandLine, CommandLine);
    default:
      return false;
  }
#undef PEB_OFFSET_CASE

  auto peb = GetPEBPtr(process);
  if (peb == nullptr) {
    return false;
  }

  PVOID proc_params;

#define PTR_ADD_OFFSET(Pointer, Offset) \
  ((PVOID)((ULONG_PTR)(Pointer) + (ULONG_PTR)(Offset)))

  if (!ReadProcessMemory(
          process, PTR_ADD_OFFSET(peb, FIELD_OFFSET(PEB, ProcessParameters)),
          &proc_params, sizeof(PVOID), NULL)) {
    return false;
  }

  auto unicode_string = UNICODE_STRING{};
  if (!ReadProcessMemory(process, PTR_ADD_OFFSET(proc_params, offset),
                         &unicode_string, sizeof(UNICODE_STRING), NULL)) {
    return false;
  }
#undef PTR_ADD_OFFSET

  auto buffer = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                unicode_string.MaximumLength);

  if (!buffer) {
    return false;
  }

  if (!ReadProcessMemory(process, unicode_string.Buffer, buffer,
                         unicode_string.MaximumLength, NULL)) {
    RtlFreeHeap(GetProcessHeap(), 0, buffer);
    return false;
  }
  *out = std::wstring((wchar_t *)buffer,
                      (unicode_string.Length / sizeof(wchar_t)));
  RtlFreeHeap(GetProcessHeap(), 0, buffer);
  return true;
}

void RunRecordingSoftwareCheck(nlohmann::json &data,
                               GenericCheck *generic_check) {
  if (!data[charenc("username")].is_string()) __fastfail(-1);

  IterateProcesses([&](const std::string &process_name, uint32_t process_id,
                       uint32_t parent_id,
                       const std::string &start_date) -> void {
    for (auto ii = data["recording_software"].begin();
         ii != data["recording_software"].end(); ++ii) {
      if (ii.key() == "executables") {
        for (auto target = ii.value().begin(); target != ii.value().end();
             ++target) {
          auto target_name = target.value().get<std::string>();
          if (!target_name.empty()) {
            if (process_name == target_name) {
              generic_check->message = "Recording Software Found:\n" +
                                       target.value().get<std::string>();
              generic_check->client = target.value().get<std::string>();
              generic_check->status = CheckStatus::kFailed;
              return;
            }
          }
        }
      }
    }
  });
}

static std::vector<uint32_t> hash_cache;

void RunProcessesCheck(nlohmann::json &data, GenericCheck *generic_check) {
  int32_t minimum_string_length = 0;
  bool unicode_support = false;

  if (!data[charenc("username")].is_string()) __fastfail(-1);

  for (auto ii = data["process_strings"]["options"].begin();
       ii != data["process_strings"]["options"].end(); ii++) {
    if (ii.key() == "minimum_string_length") {
      minimum_string_length = ii.value().get<int32_t>();
    }
  }

  for (auto ii = data["process_strings"]["options"].begin();
       ii != data["process_strings"]["options"].end(); ii++) {
    if (ii.key() == "unicode_support") {
      unicode_support = ii.value().get<bool>();
    }
  }

  IterateProcesses([&](const std::string &process_name, uint32_t process_id,
                       uint32_t parent_id,
                       const std::string &start_date) -> void {
    if (!process_name.empty() && process_id != 0) {
      for (auto it = data["process_strings"]["data"].begin();
           it != data["process_strings"]["data"].end(); it++) {
        if (process_name == it.key()) {

          HANDLE process_handle;

          auto result = NiggerOpenProcess(
              &process_handle,
              PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION,
              (HANDLE)process_id);

          if (result != STATUS_ACCESS_DENIED) {
            std::wstring cmd_line;
            std::vector<std::pair<uint32_t, std::string>> hashes;
            std::string client_name;
            bool found = false;
			std::vector<std::string> executables;

            GetProcessPebString(process_handle, kCommandLine, &cmd_line);

            if (process_name == ("svchost.exe")) {
              if (!(cmd_line.find(L"Dnscache") != std::wstring::npos)) {
                goto ContinueLoop;
              }
			}

            for (auto target = it.value().begin(); target != it.value().end();
                 target++) {
              for (auto v = target.value().begin(); v != target.value().end();
                   v++) {
                if (v.value().is_number_integer()) {
                  auto client_name = v.key().c_str();
                  hashes.push_back(make_pair(v.value().get<uint32_t>(),
                                             std::string(client_name)));
                }
              }
			}
			

            IterateStrings(process_handle, minimum_string_length,
                           unicode_support, &found, &client_name, hashes, &executables);

            if (found) {
              generic_check->message = charenc("Client Found: ") + client_name;
              generic_check->client = client_name;
              generic_check->status = CheckStatus::kFailed;
            }
			if (process_name == ("explorer.exe")) {
				executedFiles = executables;
			}

          ContinueLoop:
            NtClose(process_handle);
		  }
		}
      }
	}
  });
}

struct CallbackData {
  nlohmann::json data;
  GenericCheck *generic_check;
};

BOOL EnumWindowsProc(const HWND window, const LPARAM lparam) {
  auto data = reinterpret_cast<CallbackData *>(lparam);

  char window_text[MAX_PATH];
  char window_class[MAX_PATH];

  GetWindowTextA(window, window_text, sizeof(window_text));
  GetClassNameA(window, window_class, sizeof(window_class));

  auto class_name = std::string(window_class);
  auto window_name = std::string(window_text);

  auto contains = false;
  auto case_sensitive = false;

  for (auto it = data->data["recording_software"].begin();
       it != data->data["recording_software"].end(); ++it) {
    if (it.key() == "options") {
      for (auto ii = it.value().begin(); ii != it.value().end(); ++ii) {
        if (ii.key() == "contains") {
          contains = ii.value().get<bool>();
        }
        if (ii.key() == "case-sensitive") {
          case_sensitive = ii.value().get<bool>();
        }
      }
    }

    if (it.key() == "window_titles") {
      for (auto ii = it.value().begin(); ii != it.value().end(); ++ii) {
        auto target = ii.value().get<std::string>();

        if (!case_sensitive) {
          std::transform(window_name.begin(), window_name.end(),
                         window_name.begin(), ::tolower);
          std::transform(target.begin(), target.end(), target.begin(),
                         ::tolower);
        }
        if (!window_name.empty() && !target.empty()) {
          if (contains) {
            if (window_name.find(target) != std::string::npos) {
              data->generic_check->message =
                  charenc("Recording Software Found:\n") + target;
              data->generic_check->client = target;
              data->generic_check->status = CheckStatus::kFailed;
              return FALSE;
            }
          } else if (target == window_name) {
            data->generic_check->message =
                charenc("Recording Software Found:\n") + target;
            data->generic_check->client = target;
            data->generic_check->status = CheckStatus::kFailed;
            return FALSE;
          }
        }
      }
    }

    if (it.key() == "window_classes") {
      for (auto ii = it.value().begin(); ii != it.value().end(); ++ii) {
        auto target = ii.value().get<std::string>();

        if (!case_sensitive) {
          std::transform(class_name.begin(), class_name.end(),
                         class_name.begin(), ::tolower);
          std::transform(target.begin(), target.end(), target.begin(),
                         ::tolower);
        }

        if (!class_name.empty() && !target.empty()) {
          if (contains) {
            if (class_name.find(target) != std::string::npos) {
              data->generic_check->message =
                  charenc("Recording Software Found:\n") + target;
              data->generic_check->client = target;
              data->generic_check->status = CheckStatus::kFailed;
              return FALSE;
            }
          } else if (target == class_name) {
            data->generic_check->message =
                charenc("Recording Software Found:\n") + target;
            data->generic_check->client = target;
            data->generic_check->status = CheckStatus::kFailed;
            return FALSE;
          }
        }
      }
    }
  }
  return TRUE;
}

void runPatternScan(GenericCheck *generic_check) {
	//VM_DOLPHIN_RED_START
	auto minecraft_window = FindWindow(L"LWJGL", nullptr);
	if (!minecraft_window)
	{
		minecraft_window = FindWindow(L"GLFW30", nullptr);
		if (minecraft_window) {
			generic_check->status = CheckStatus::kSuccess;
			return;
		}
		return;
	}
	if (minecraft_window)
	{
		DWORD process_id;
		GetWindowThreadProcessId(minecraft_window, &process_id);

		const auto process = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, false, process_id);

		if (!process)
		{
			generic_check->message = charenc("Minecraft not found!");
			generic_check->status = CheckStatus::kWarning;
			return;
		}

		check_modules(process, generic_check);
		if (generic_check->status != CheckStatus::kFailed) {
			ss::run_scan(process, generic_check);
		}
	}

	else
	{
		generic_check->message = charenc("Minecraft not found!");
		generic_check->status = CheckStatus::kWarning;
		return;
	}
	//VM_DOLPHIN_RED_END
}

HWND g_HWND = NULL;

BOOL CALLBACK EnumWindowsProcMy(HWND hwnd, LPARAM lParam)
{
	DWORD lpdwProcessId;
	GetWindowThreadProcessId(hwnd, &lpdwProcessId);
	if (lpdwProcessId == lParam)
	{
		g_HWND = hwnd;
		return FALSE;
	}
	return TRUE;
}

void check_modules(const HANDLE process, GenericCheck *generic_check)
{
	//VM_DOLPHIN_RED_START
	const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetProcessId(process));
	MODULEENTRY32 module_entry = { 0 };

	module_entry.dwSize = sizeof(module_entry);

	if (!Module32First(snapshot, &module_entry))
		return;

	auto cheats_found = false;

		const DWORD pid = GetProcessId(process);

		EnumWindows(EnumWindowsProcMy, pid);
		/*
		char window_text[MAX_PATH];
		std::string window_name(window_text);

		GetWindowTextA(g_HWND, window_text, sizeof(window_text));
		MessageBoxA(NULL, window_name.c_str(), NULL, NULL);

		BOOL blc;

		std::string contains = "Badlion Minecraft Client";
		if (window_name.find(contains) != std::string::npos) {
			blc = true;
		}
		else {
			blc = false;
		}*/

	do
	{

		std::wstringstream cls;

		cls << module_entry.szModule;

		auto name = cls.str();
		transform(name.begin(), name.end(), name.begin(), towlower);
		/*
		BOOL condition;
		//window_text
		
		if (blc) {
			MessageBoxA(NULL, "Using blc", NULL, NULL);
		}
		if (window_name.find(contains) != std::string::npos) {
			MessageBoxA(NULL, "Using blc", NULL, NULL);
			condition = name == L"instrument.dll";
		}
		else {
			condition = name == L"instrument.dll" || name == L"vcruntime140.dll";
		}*/

		if (name == L"instrument.dll" /*|| name == L"vcruntime140.dll"*/)
		{
			cheats_found = true;
			break;
		}

	} while (Module32Next(snapshot, &module_entry));

	CloseHandle(snapshot);

	if (cheats_found)
	{
		generic_check->message = charenc("Injection detected!");
		generic_check->status = CheckStatus::kFailed;
	}
	//VM_DOLPHIN_RED_END
}


void RunWindowChecks(const nlohmann::json &data, GenericCheck *generic_check) {
  CallbackData callback_data;
  callback_data.data = data;
  callback_data.generic_check = generic_check;
  EnumWindows(&EnumWindowsProc, reinterpret_cast<LPARAM>(&callback_data));
}
