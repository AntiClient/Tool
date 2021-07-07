#ifndef GUI_H
#define GUI_H

// clang-format off
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "third_party/imgui/implement/imgui_impl_glfw_gl3.h"
#include <memory>
#include <optional>
#include "checks.h"
// clang-format on

struct BinaryAsset {
  uint8_t *data;
  size_t size;
};

class Gui {
public:
  explicit Gui(int width, int height, const char *window_name);
  ~Gui();

  void Update();
  void SwapBuffers();
  void DoRender();
  bool ShouldRender();

  void Shutdown();



private:
  GLFWwindow *window_;
  int width_;
  int height_;

  ImageTexture wallpaper_;
  ImageTexture logo_;
  ImageTexture checkmark_;
  ImageTexture login_wallpaper_;
  ImageTexture key_logo_;
  ImageTexture error_icon_;
  ImageTexture warning_icon_;

  std::string username_;
  bool logged_in_;
	std::string pin_;
	std::string sid_;

  std::optional<nlohmann::json> client_data_;

private:
  GenericCheck check_;
  std::unique_ptr<BinaryAsset> LoadBinaryAsset(int asset_id);

  void BindFont();

  void BindWallpaper();
  void BindLoginWallpaper();
  void BindLogo();
  void BindCheckmark();
  void BindKeyLogo();
  void SetupWindowIcon();
  void BindErrorIcon();
  void BindWarningIcon();
  void SetupStyle();
  void PollSessionStatus();

  void Init(int width, int height, const char *window_name);
};

namespace TabApi {
struct tab_data {
  inline bool operator==(tab_data &TabData) { return TabData.Name == Name; }

  std::string Name;
  bool Selected;
  int UniqueId;
};

class application_tabs {
public:
  enum RenderPosition { kVertical = 0, kHorizontal = 1 };
  application_tabs() = default;
  ~application_tabs() = default;

  void RenderTabs(RenderPosition RenderPos, ImVec2 TabSize = ImVec2(35, 23));

  int AddTab(const std::string &TabName);
  void RemoveTab(int tab_id);

  int TabCount() { return tab_ids_.size(); }

  int GetSelectedTab() { return current_tab_.UniqueId; }

private:
  tab_data current_tab_;
  std::vector<tab_data> tab_ids_;
};
} // namespace TabApi

#endif