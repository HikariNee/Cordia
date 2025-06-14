#pragma once

#include <filesystem>

namespace rootFS {
  auto mountRootFS(std::filesystem::path newroot) -> void;
  auto preMountRootFS(std::filesystem::path newroot) -> void;
}
