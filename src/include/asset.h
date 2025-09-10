#ifndef NAG_ENGINE_ASSET_H
#define NAG_ENGINE_ASSET_H

#include <cstdint>
#include <string_view>
#include <vector>

struct Asset
{
  std::string_view name;
  std::vector<uint8_t> data;
  size_t file_size{};
};

#endif // NAG_ENGINE_ASSET_H