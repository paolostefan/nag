#include <memory>
#include <string_view>
#include <vector>

struct Asset
{
  std::string_view name;
  std::vector<uint8_t> data;
  size_t file_size;
};

class Engine
{
public:
  Engine() = default;
  ~Engine() = default;

  bool LoadAsset(const std::string_view &assetName) noexcept;

private:
  std::vector<Asset> assets;
};