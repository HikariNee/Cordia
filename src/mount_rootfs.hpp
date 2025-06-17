#pragma once

#include <filesystem>

namespace rootFS {
  auto mountRootFS(const std::filesystem::path& newroot) -> void;
  auto preMountRootFS(const std::filesystem::path& newroot) -> void;
}
