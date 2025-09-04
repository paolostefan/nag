#include "engine.h"

#include <iostream>

#include "spdlog/spdlog.h"

bool Engine::LoadAsset(const std::string_view &assetName) noexcept
{
  try
  {
    // Try to load file into memory
    std::ifstream file(std::string(assetName), std::ios::binary);

    if (!file.is_open())
    {
      spdlog::error("Error loading asset {}", assetName);
      return false;
    }

    // Get an array of bytes of the file size
    file.seekg(0, std::ios::end);
    const size_t file_size = file.tellg();

    if(file_size == 0)
    {
      spdlog::error("Error loading asset {}: file is empty", assetName);
      return false;
    }

    file.seekg(0, std::ios::beg);

    Asset asset;
    asset.name = assetName;
    asset.data = std::vector<uint8_t>(file_size);
    asset.file_size = file_size;

    // read the file into the memory chunk
    file.read((char *)asset.data.data(), file_size);

    assets.push_back(asset);

    spdlog::info("Asset #{} loaded: {}, {} bytes", assets.size(), assetName, file_size);

    return true;
  }
  catch (const std::exception &e)
  {
    spdlog::error("Error loading asset {}: {}", assetName, e.what());
    return false;
  }
}