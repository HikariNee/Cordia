#include <iostream>
#include "utilities.hpp"
#include "container.hpp"

auto Container::setupMaps(const std::string& uid, const std::string& gid) -> void
{
  std::filesystem::path uidmap = "/proc/" + std::to_string(cpid) + "/uid_map";
  std::filesystem::path gidmap = "/proc/" + std::to_string(cpid) + "/gid_map";

  writeTo(uidmap, uid, std::ios::trunc);
  writeTo(gidmap, gid, std::ios::trunc);
}


auto Container::getChildPID() -> int
{
  return cpid;
}


Container::~Container()
{
  cleanup();
}
