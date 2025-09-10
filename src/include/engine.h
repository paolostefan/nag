#ifndef NAG_ENGINE_ENGINE_H
#define NAG_ENGINE_ENGINE_H

#include "asset.h"

class Engine
{
public:
  Engine() = default;
  ~Engine() = default;

  bool LoadAsset(const std::string_view &assetName) noexcept;

private:
  std::vector<Asset> assets;
};

#endif // NAG_ENGINE_ENGINE_H