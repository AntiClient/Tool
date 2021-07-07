// clang-format off



#include <thread>
#include <chrono>
using namespace std::chrono_literals;

#define NOMINMAX
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GL/gl3w.h> 
#include "gui.h"
#include <GLFW/glfw3native.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include <imgui.h>
#include "third_party/imgui/freetype/imgui_freetype.h"
#include "third_party/imgui/variouscontrol/imguivariouscontrols.h"


#include "xray.h"
#include "client_data.h"
#include "coms.h"


#define NOMINMAX
#include "cx_encrypted_string.h"
#include "resource/resource.h"
#include <stb_image.h>
#include <fstream>
#include <streambuf>
#include "phnt_headers.h"


#include "defer.h"


// clang-format on

enum Status { kCheckRunning = 0, kCheckFailed = 1, kCheckSuccess = 2, kCheckWarning = 3 };

static std::string g_check_failed_text;
static nlohmann::json g_client_data;

static std::string GetLauncherProfile() {
  char *buf = nullptr;
  size_t sz = 0;
  _dupenv_s(&buf, &sz, "APPDATA");

  const auto file_path =
      std::string(buf) + (R"(\.minecraft\launcher_profiles.json)");

  std::ifstream t(file_path);

  std::string str((std::istreambuf_iterator<char>(t)),
                  std::istreambuf_iterator<char>());
  return str;
}

static std::string GetEventLog() {

	char *buf = nullptr;
	size_t sz = 0;
	_dupenv_s(&buf, &sz, "APPDATA");

	const auto file_path =
		std::string(buf) + (R"(\anticlient_data.json)");

	std::ifstream t(file_path);

	std::string str((std::istreambuf_iterator<char>(t)),
		std::istreambuf_iterator<char>());
	return str;
}

static void UpdateTextureCallback(ImTextureID &texture_id, int width,
                                  int height, int channels,
                                  const unsigned char *pixels,
                                  bool useMipmapsIfPossible, bool wraps,
                                  bool wrapt);
static void FreeTextureCallback(ImTextureID &texture_id);

Gui::Gui(int width, int height, const char *window_name) : logged_in_(false) {
  Init(width, height, window_name);
}

Gui::~Gui() {}

void Gui::Update() {
  glfwPollEvents();
  ImGui_ImplGlfwGL3_NewFrame();
}

void Gui::SwapBuffers() {
  int width, height;
  glfwGetFramebufferSize(window_, &width_, &height_);
  glViewport(0, 0, width_, height_);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui::Render();
  ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
  glfwSwapBuffers(window_);
}

static __forceinline std::string B2S(const bool b) {
  return b ? "true" : "false";
}

void Gui::DoRender() {
  static TabApi::application_tabs Tabs;
  auto &io = ImGui::GetIO();

  static Status check_one_status = kCheckRunning;
  static Status check_two_status = kCheckRunning;
  static Status check_three_status = kCheckRunning;

  static std::string check_one_text;
  static std::string check_two_text;
  static std::string check_three_text;

  static auto selected_tab = 0;

  ImGui::SetNextWindowPos(ImVec2(-10, -10), ImGuiCond_Once);
  ImGui::SetNextWindowSize(wallpaper_.size + ImVec2(20, 20));
  ImGui::Begin("##Main", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

  {
    static ImGui::AnimatedImage animated_image{};
    static bool loaded = false;
    if (!loaded) {
      animated_image.SetGenerateOrUpdateTextureCallback(&UpdateTextureCallback);
      animated_image.SetFreeTextureCallback(&FreeTextureCallback);

      const auto loading_gif = LoadBinaryAsset(IDR_LOADING_GIF);

      animated_image.load_from_memory(loading_gif->data, loading_gif->size);
      loaded = true;
    }

    auto cursor_position = ImGui::GetCursorPos();
    cursor_position.y += 10;
    if (logged_in_) {
      if (Tabs.GetSelectedTab() == 1) {
        ImGui::Image(login_wallpaper_.texture_id, login_wallpaper_.size);
      } else {
        ImGui::Image(wallpaper_.texture_id, wallpaper_.size);
      }

    } else if (!logged_in_) {
      ImGui::Image(login_wallpaper_.texture_id, login_wallpaper_.size);
    }

    ImGui::SetCursorPos(cursor_position);

    if (!logged_in_) {
      ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 8.0f);
      ImGui::SetCursorPos(
          ImVec2((width_ / 2) - 350 / 2, (height_ / 2) - 150 / 2));
      ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1.f, 1.f, 1.f, 1.f));
      ImGui::BeginChild("Login Page", ImVec2(350, 150), true);
      ImGui::SetCursorPos(ImVec2((ImGui::GetContentRegionAvailWidth() / 2) - 60,
                                 ImGui::GetCursorPosY()));
      ImGui::PushFont(io.Fonts->Fonts[1]);
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.10f, 0.49f, 0.80f, 1.0f));
      ImGui::Text("Enter PIN");
      ImGui::NewLine();

      static char display_buffer[5] = {0};
      ImGui::SetCursorPos(
          ImVec2(ImGui::GetCursorPosX() + 23.f, ImGui::GetCursorPosY() - 35.f));

      ImGui::Image(key_logo_.texture_id, ImVec2(32, 32));
      ImGui::SameLine();

      static auto failed_login = false;

      const auto frame_bg = failed_login
                                ? ImVec4(0.85f, 0.10f, 0.10f, 0.50f)
                                : ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);

      const auto text_color = failed_login
                                  ? ImVec4(0.85f, 0.10f, 0.10f, 0.80f)
                                  : ImGui::GetStyleColorVec4(ImGuiCol_Text);

      static auto login = [&]() -> void {
        auto pin_number = std::string((display_buffer));

        if (pin_number.length() == 4) {
          client_data_ = GetClientData(display_buffer);

          if (client_data_.has_value()) {
            failed_login = false;
            logged_in_ = true;
            g_client_data = client_data_.value();
            username_ = g_client_data["username"].get<std::string>();
            pin_ = display_buffer;
			sid_ = g_client_data["SID"].get<std::string>();

            std::thread t([&]() {
              char username_buffer[MAX_PATH];
              DWORD buffer_count = MAX_PATH;
              if (GetComputerNameA(username_buffer, &buffer_count)) {
                for (auto &name :
                     client_data_.value()["blacklisted_machines"]) {
                  if (name.is_string()) {
                    if (name.get<std::string>() == username_buffer) {
                      ClientComms::SendMachineName(username_, username_buffer);
                      NtTerminateProcess(NtCurrentProcess(), 0);
					  __fastfail(-1);
                      break;
                    }
                  }
                }
              }

              while (ClientComms::IsSessionValid(sid_)) {
				std::this_thread::sleep_for(5000ms);
              }

              NtTerminateProcess(NtCurrentProcess(), -1);
              __fastfail(-1);
            });
            t.detach();

          } else {
            failed_login = true;
          }

        } else {
          failed_login = true;
        }
      };

      ImGui::PushStyleColor(ImGuiCol_FrameBg, frame_bg);
      ImGui::PushStyleColor(ImGuiCol_Text, text_color);

      if (ImGui::InputText("##Password", display_buffer, sizeof(display_buffer),
                           ImGuiInputTextFlags_Password |
                               ImGuiInputTextFlags_EnterReturnsTrue)) {
        login();
      }

      ImGui::PopStyleColor(3);

      ImGui::PopFont();
      ImGui::SetCursorPos(
          ImVec2(ImGui::GetCursorPosX() + 63.f, ImGui::GetCursorPosY()));
      ImGui::PushFont(io.Fonts->Fonts[2]);

      if (ImGui::Button("Login", ImVec2(227, 36))) {
        login();
      }

      ImGui::PopFont();

      ImGui::EndChild();
      ImGui::PopStyleColor();
      ImGui::PopStyleVar();
      ImGui::End();

      return;
    }
    static bool tabs_created = false;

    static bool should_render_status = false;
    static bool checks_done = false;
    const auto start_position = ImGui::GetCursorPos();

    if (!tabs_created) {
      Tabs.AddTab(" Scanner");
      Tabs.AddTab(" Utilities");
    }

    ImGui::SetCursorPosX((ImGui::GetContentRegionAvailWidth() / 2) - 250 / 2);
    ImGui::BeginChild("Tabs", ImVec2(270, 50), true,
                      ImGuiWindowFlags_NoScrollbar);
    Tabs.RenderTabs(TabApi::application_tabs::kHorizontal, ImVec2(125, 25));
    ImGui::EndChild();
    selected_tab = Tabs.GetSelectedTab();

    switch (Tabs.GetSelectedTab()) {
      case 0: {
        ImGui::SetCursorPos(ImVec2(
            (ImGui::GetContentRegionAvailWidth() / 2) - 270 / 2, height_ - 75));
        ImGui::PushFont(io.Fonts->Fonts[2]);	
        if (ImGui::Button("Start Scan", ImVec2(270, 50))) {
			if (!ClientComms::IsSessionValid(sid_)) {
				NtTerminateProcess(NtCurrentProcess(), -1);
				__fastfail(-1);
			}
          if (!checks_done) {
            should_render_status = true;
            std::thread run_checks([&]() {
              GenericCheck check{};
              check.Reset();

              std::string client_1;
              std::string client_2;

              RunRecordingSoftwareCheck(g_client_data, &check);
              RunWindowChecks(g_client_data, &check);

              if (check.status == kFailed) {
                check_one_text = check.message;
                client_1 = check.client;
                check_one_status = kCheckFailed;
              } else {
                check_one_status = kCheckSuccess;
              }

              check.Reset();

              RunProcessesCheck(g_client_data, &check);

              if (check.status == kFailed) {
                check_two_text = check.message;
                client_2 = check.client;
                check_two_status = kCheckFailed;
              } else {
                check_two_status = kCheckSuccess;
              }

              check.Reset();

			  runPatternScan(&check);

			  if (check.status == kFailed) {
				  check_three_text = check.message;
				  check_three_status = kCheckFailed;
			  }
			  else if (check.status == kWarning) {
				  check_three_text = check.message;
				  check_three_status = kCheckWarning;
              } else {
                check_three_status = kCheckSuccess;
              }

              check.Reset();

              if (!pin_.empty()) {
                static auto launcher_profile = GetLauncherProfile();
                static auto jj = nlohmann::json::parse(launcher_profile);
                static auto accounts = GetPlayerAccounts(jj);

                std::string json =
                    R"({"username": ")" + username_ + R"(", "pin": ")" + pin_ +
                    R"(",)" + R"("checks":{"check1": ")" +
                    ((check_one_status == kCheckSuccess) ? "Passed"
                                                         : client_1) +
                    R"(","check2": ")" +
                    ((check_two_status == kCheckSuccess) ? "Passed"
                                                         : client_2) +
                    R"(","check3": ")" +
                    B2S(check_three_status == kCheckSuccess) + R"("},"alts":[)";

                for (auto i = 0; i < accounts.size(); ++i) {
                  if (i == accounts.size() - 1) {
                    json += R"(")" + accounts[i] + R"("],)";
                  } else {
                    json += R"(")" + accounts[i] + R"(",)";
                  }
                }

                json += R"("recycleBin":[)";

                auto write_dates = GetRecycleBinWriteDates();

                for (auto i = 0; i < write_dates.size(); i++) {
                  auto date = write_dates[i].write_date;
                  if (i == write_dates.size() - 1) {
                    json.append(R"(")" + write_dates[i].write_date + R"("],)");
                  } else {
                    json.append(R"(")" + write_dates[i].write_date + R"(",)");
                  }
                }
                auto start_times = GetProcessStartTimes(client_data_.value());
                json += R"("processStartTime":{)";

                for (auto i = 0; i < start_times.size(); i++) {
                  auto g = start_times[i];
                  if (i == start_times.size() - 1) {
                    json +=
                        R"(")" + g.process + R"(":")" + g.start_time + R"("}})";
                  } else {
                    json +=
                        R"(")" + g.process + R"(":")" + g.start_time + R"(",)";
                  }
                }

                ClientComms::UploadCheckResults(json, sid_);
              }

              checks_done = true;
            });
            run_checks.detach();
          } else {
            should_render_status = false;

            check_one_status = kCheckRunning;
            check_two_status = kCheckRunning;
            check_three_status = kCheckRunning;
            checks_done = false;
          }
        }


        ImGui::PopFont();

        const auto this_position = ImGui::GetCursorPos();

        ImGui::SetCursorPos(start_position);
        if (should_render_status) {
          ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.f);
          if (check_one_status == kCheckSuccess) {
            check_one_text = charenc("Check #1 Passed");
            ImGui::Image(checkmark_.texture_id, ImVec2(25, 25));
          } else if (check_one_status == kCheckFailed) {
            ImGui::Image(error_icon_.texture_id, ImVec2(25, 25));
          } else {
            animated_image.render(ImVec2(32, 32));
            check_one_text = charenc("Running Check #1");
          }

          ImGui::SameLine();
          ImGui::SetCursorPos(
              ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 8.f));
          ImGui::Text(check_one_text.c_str());

          ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.f);

          if (check_one_status != kCheckRunning) {
            if (check_two_status == kCheckFailed) {
              ImGui::Image(error_icon_.texture_id, ImVec2(25, 25));
            } else if (check_two_status == kCheckSuccess) {
              ImGui::Image(checkmark_.texture_id, ImVec2(25, 25));
              check_two_text = charenc("Check #2 Passed");
            } else {
              animated_image.render(ImVec2(32, 32));
              check_two_text = charenc("Running Check #2");
            }

            ImGui::SameLine();
            ImGui::SetCursorPos(
                ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 8.f));
            ImGui::Text(check_two_text.c_str());
          }

          ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.f);

          if (check_two_status != kCheckRunning &&
              check_one_status != kCheckRunning) {
            if (check_three_status == kCheckFailed) {
              ImGui::Image(error_icon_.texture_id, ImVec2(25, 25));
			}
			else if (check_three_status == kCheckSuccess) {
				ImGui::Image(checkmark_.texture_id, ImVec2(25, 25));
				check_three_text = "Check #3 Passed";
			}
			else if (check_three_status == kCheckWarning) {
				ImGui::Image(warning_icon_.texture_id, ImVec2(25, 25));
            } else {
              animated_image.render(ImVec2(32, 32));
              check_three_text = "Running Check #3";
            }
            ImGui::SameLine();
            ImGui::SetCursorPos(
                ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 8.f));
            ImGui::Text(check_three_text.c_str());
          }

          ImGui::SetCursorPos(this_position);
        }
      } break;
      case 1: {
        ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 20.f, 20.f));

        ImGui::SetCursorPos(
            ImVec2((ImGui::GetContentRegionAvailWidth() / 2) - 650 / 2,
                   height_ - 400));

        ImGui::BeginChild("Utilities", ImVec2(700, 300), true);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 1.f, 1.f));
        ImGui::PushFont(io.Fonts->Fonts[3]);
        if (ImGui::CollapsingHeader("Accounts")) {
          static auto launcher_profile = GetLauncherProfile();

          if (!launcher_profile.empty()) {
            static auto json = nlohmann::json::parse(launcher_profile);
            static auto accounts = GetPlayerAccounts(json);

            for (auto &account : accounts) {
              ImGui::Text(account.c_str());
            }
          }
        }

        if (ImGui::CollapsingHeader("Process Start Times")) {
			if (!ClientComms::IsSessionValid(sid_)) {
				NtTerminateProcess(NtCurrentProcess(), -1);
				__fastfail(-1);
			}
          static auto start_times = GetProcessStartTimes(client_data_.value());
          for (auto &t : start_times) {
            ImGui::Text(std::string(t.process + " - " + t.start_time).c_str());
          }
        }

        if (ImGui::CollapsingHeader("Recycle Bin Dates")) {
          static auto write_dates = GetRecycleBinWriteDates();
          for (auto &d : write_dates) {
            ImGui::TextWrapped("Path: %s", d.dir.c_str());
            ImGui::TextWrapped("Last Write Date: %s", d.write_date.c_str());
            ImGui::Separator();
          }
        }

		if (ImGui::CollapsingHeader("XRay Packs")) {
			char *buf = nullptr;
			size_t sz = 0;
			_dupenv_s(&buf, &sz, "APPDATA");

			const auto resource_pack_dir =
				std::string(buf) + charenc(R"(\.minecraft\resourcepacks)");
			
			static auto xRayPacks = RunXrayPackCheck(std::wstring(resource_pack_dir.begin(),
				resource_pack_dir.end())
								 .c_str());
			//static auto write_dates = GetRecycleBinWriteDates();
			for (auto &d : xRayPacks) {
				ImGui::TextWrapped("Path: %s", d.c_str());
				ImGui::Separator();
			}
		}
		/*
		if (ImGui::CollapsingHeader("PC Date/Time change")) {
			static auto event_log = GetEventLog();
			if (!event_log.empty()) {

				static auto json = nlohmann::json::parse(event_log);
				static auto change_dates = GetEventsChangeTime(json);
				for (auto &d : change_dates) {
					ImGui::Text("Change date: %s", d.c_str());
					ImGui::Separator();
				}

			}
			
		}*/

		if (ImGui::CollapsingHeader("Executed Files")) {

			for (auto &f : executed_files()) {
				ImGui::TextWrapped("%s", f.c_str());
				ImGui::Separator();
			}
		}

        ImGui::PopFont();
        ImGui::PopStyleColor();
        ImGui::EndChild();

      } break;
      default:
        break;
    }
  }
  ImGui::End();
}

bool Gui::ShouldRender() { return !glfwWindowShouldClose(window_); }

std::unique_ptr<BinaryAsset> Gui::LoadBinaryAsset(int asset_id) {
  auto resource =
      FindResourceA(nullptr, MAKEINTRESOURCEA(asset_id), charenc("BINARY"));

  if (!resource) return nullptr;

  BinaryAsset asset{};
  asset.data = (uint8_t *)LockResource(LoadResource(nullptr, resource));
  asset.size = (size_t)SizeofResource(nullptr, resource);

  return std::make_unique<BinaryAsset>(asset);
}

void Gui::BindFont() {
  ImFontConfig regular_config;
  regular_config.FontDataOwnedByAtlas = false;

  ImFontConfig material_config;
  material_config.FontDataOwnedByAtlas = false;
  material_config.MergeMode = true;

  auto regular_font = LoadBinaryAsset(IDR_FONT);

  if (!regular_font) return;

  auto bold_font = LoadBinaryAsset(IDR_FONT_BOLD);

  if (!bold_font) return;

  auto &io = ImGui::GetIO();

  io.Fonts->AddFontFromMemoryTTF(regular_font->data, regular_font->size, 15.f,
                                 &regular_config);

  io.Fonts->AddFontFromMemoryTTF(regular_font->data, regular_font->size, 30.f,
                                 &regular_config);

  io.Fonts->AddFontFromMemoryTTF(bold_font->data, bold_font->size, 20.f,
                                 &regular_config);

  io.Fonts->AddFontFromMemoryTTF(bold_font->data, bold_font->size, 15.f,
                                 &regular_config);

  ImGuiFreeType::BuildFontAtlas(io.Fonts, ImGuiFreeType::NoHinting);
}

void Gui::BindWallpaper() {
  auto wallpaper = LoadBinaryAsset(IDR_BACKGROUND);

  if (!wallpaper) return;

  ImGui_ImplGlfwGL3_BindImage(wallpaper->data, wallpaper->size, &wallpaper_);
}

void Gui::BindLoginWallpaper() {
  auto login_wallpaper = LoadBinaryAsset(IDR_LOGIN_WALLPAPER);

  if (!login_wallpaper) return;

  ImGui_ImplGlfwGL3_BindImage(login_wallpaper->data, login_wallpaper->size,
                              &login_wallpaper_);
}

void Gui::BindLogo() {
  auto logo = LoadBinaryAsset(IDR_LOGO);

  if (!logo) return;

  ImGui_ImplGlfwGL3_BindImage(logo->data, logo->size, &logo_);
}

void Gui::BindCheckmark() {
  auto checkmark = LoadBinaryAsset(IDR_CHECKMARK);

  if (!checkmark) return;

  ImGui_ImplGlfwGL3_BindImage(checkmark->data, checkmark->size, &checkmark_);
}

void Gui::BindKeyLogo() {
  auto key_logo = LoadBinaryAsset(IDR_KEY_LOGO);

  if (!key_logo) return;

  ImGui_ImplGlfwGL3_BindImage(key_logo->data, key_logo->size, &key_logo_);
}

void Gui::SetupWindowIcon() {
  auto logo = LoadBinaryAsset(IDR_LOGO);

  if (!logo) return;

  int32_t width = 0;
  int32_t height = 0;
  int32_t comp = 0;

  auto *pixels = stbi_load_from_memory(logo->data, static_cast<int>(logo->size),
                                       &width, &height, &comp, 0);

  GLFWimage icons[1];
  icons[0].height = width;
  icons[0].width = height;
  icons[0].pixels = pixels;

  glfwSetWindowIcon(window_, 1, icons);
}

void Gui::BindErrorIcon() {
  auto error_icon = LoadBinaryAsset(IDR_ERROR_ICON);

  if (!error_icon) return;

  ImGui_ImplGlfwGL3_BindImage(error_icon->data, error_icon->size, &error_icon_);
}

void Gui::BindWarningIcon() {
	auto warningIcon = LoadBinaryAsset(IDR_WARNING_ICON);

	if (!warningIcon) return;

	ImGui_ImplGlfwGL3_BindImage(warningIcon->data, warningIcon->size, &warning_icon_);
}

void Gui::SetupStyle() {
  auto &style = ImGui::GetStyle();

  style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
  style.Colors[ImGuiCol_Button] = ImVec4(0.17f, 0.44f, 0.90f, 1.0f);
  style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.12f, 0.27f, 0.58f, 1.0f);
  style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.09f, 0.20f, 0.44f, 1.0f);
  style.Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.49f, 0.80f, 0.80f);
  style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.55f, 0.90f, 1.0f);
  style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.10f, 0.49f, 0.80f, 0.80f);
}

static COORD window_size = { 800, 450 };

void Gui::Init(int width, int height, const char *window_name) {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, 0);

  window_ = glfwCreateWindow(width, height, window_name, nullptr, nullptr);
  glfwMakeContextCurrent(window_);
  glfwSwapInterval(1);
  gl3wInit();
  ImGui::CreateContext();
  auto &io = ImGui::GetIO();
  io.IniFilename = nullptr;
  ImGui_ImplGlfwGL3_Init(window_, true);
  BindFont();
  BindWallpaper();
  BindLogo();
  BindCheckmark();
  BindLoginWallpaper();
  BindKeyLogo();
  BindErrorIcon();
  BindWarningIcon();
  SetupStyle();
  SetupWindowIcon();
}

void Gui::Shutdown() {
  ImGui_ImplGlfwGL3_Shutdown();
  ImGui::DestroyContext();

  if (!pin_.empty()) {
    ClientComms::DestroySession(sid_);
  }
}

void TabApi::application_tabs::RenderTabs(RenderPosition RenderPos,
                                          ImVec2 TabSize) {
  auto &io = ImGui::GetIO();
  for (auto it = tab_ids_.begin(); it != tab_ids_.end(); it++) {
    ImGui::PushFont(io.Fonts->Fonts[2]);
    if (ImGui::Selectable(it->Name.c_str(), &it->Selected, 1 << 3, TabSize)) {
      if (!it->Selected) it->Selected = true;
      current_tab_ = *it;
    }
    ImGui::PopFont();

    if (RenderPos == kHorizontal) {
      ImGui::SameLine(0.0f, 4.f);
      ImGui::VerticalSeparator();

      ImGui::SameLine(0.0f, 1.f);
    }

    if (it->Selected && current_tab_.UniqueId != it->UniqueId) {
      it->Selected = false;
    }

    if (it->Selected) current_tab_ = *it;
  }
}

int TabApi::application_tabs::AddTab(const std::string &tab_name) {
  tab_ids_.push_back({tab_name, tab_ids_.empty(), (int)tab_ids_.size()});
  return tab_ids_.size() - 1;
}

void TabApi::application_tabs::RemoveTab(int tab_id) {
  tab_ids_.erase(tab_ids_.begin() + tab_id);
}

static void FreeTextureCallback(ImTextureID &texture_id) {
  texture_id = nullptr;
}

static void UpdateTextureCallback(ImTextureID &texture_id, int width,
                                  int height, int channels,
                                  const unsigned char *pixels,
                                  bool useMipmapsIfPossible, bool wraps,
                                  bool wrapt) {
  texture_id = CreateAndBindTexture(const_cast<uint8_t *>(pixels), width,
						             height, channels);
}
