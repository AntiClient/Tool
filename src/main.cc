// clang-format off

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "libeay32.lib")
#pragma comment(lib, "ssleay32.lib")
#pragma comment(lib, "cryptlib.lib")
#ifdef _DEBUG
#pragma comment(lib, "glfw3_d.lib")
#else
#pragma comment(lib, "glfw3.lib")
#endif

#ifdef _DEBUG
#pragma comment(lib, "freetype263MTd.lib")
#else
#pragma comment(lib, "freetype263MT.lib")
#endif


#pragma comment(lib, "SecureEngineSDK64.lib")

#define NOMINMAX
#include "cx_encrypted_string.h"
#include "zip_file.h"
#include "coms.h"
#include "phnt_headers.h"
#include "checks.h"
#include "gui.h"
#include <ThemidaSDK.h>

// clang-format on

void EnablePrivileges() {
  HANDLE token_handle{};

  if (NT_SUCCESS(NtOpenProcessToken(NtCurrentProcess(), TOKEN_ADJUST_PRIVILEGES,
                                    &token_handle))) {
    auto privileges_buffer = std::make_unique<uint8_t[]>(
        (FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges) +
         sizeof(LUID_AND_ATTRIBUTES) * 5));

    auto *privileges =
        reinterpret_cast<TOKEN_PRIVILEGES *>(privileges_buffer.get());
    privileges->PrivilegeCount = 5;

    for (auto i = 0; i < privileges->PrivilegeCount; i++) {
      privileges->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
      privileges->Privileges[i].Luid.HighPart = 0;
    }

    privileges->Privileges[0].Luid.LowPart = SE_DEBUG_PRIVILEGE;
    privileges->Privileges[1].Luid.LowPart = SE_INC_BASE_PRIORITY_PRIVILEGE;
    privileges->Privileges[2].Luid.LowPart = SE_INC_WORKING_SET_PRIVILEGE;
    privileges->Privileges[3].Luid.LowPart = SE_PROF_SINGLE_PROCESS_PRIVILEGE;
    privileges->Privileges[4].Luid.LowPart = SE_TAKE_OWNERSHIP_PRIVILEGE;

    NtAdjustPrivilegesToken(token_handle, FALSE, privileges, 0, NULL, NULL);

    NtClose(token_handle);
  }
}

constexpr const auto kMutexName = cx_make_encrypted_string(L"ANTICLIENT");

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance,
                   _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	//VM_DOLPHIN_RED_START
  EnablePrivileges();

  /*auto mutex = CreateMutexW(nullptr, FALSE, kMutexName.c_str());
  if (GetLastError() == ERROR_ALREADY_EXISTS)
    return 0;*/

  auto gui = Gui(960, 530, charenc("AntiClient"));

  while (gui.ShouldRender()) {
    gui.Update();
    gui.DoRender();
    gui.SwapBuffers();
  }

  gui.Shutdown();
  //VM_DOLPHIN_RED_END
  return 0;
}
