#ifndef NAG_ENGINE_SHADER_MANAGER_H
#define NAG_ENGINE_SHADER_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>

#include "engine/engine.h"

class ShaderProgram
{

public:
  ShaderProgram() = default;
  ~ShaderProgram();

  bool load_from_files(const std::string &vert_path, const std::string &frag_path);
  bool load_from_source(const std::string &vert_src, const std::string &frag_src);

  void use() const;
  void unuse() const;

  inline GLuint get_program() const { return program; }
  inline bool is_valid() const { return program != 0; }

  // Uniform setters
  void set_uniform(const std::string &name, float value);
  void set_uniform(const std::string &name, int value);
  void set_uniform(const std::string &name, float x, float y);
  void set_uniform(const std::string &name, float x, float y, float z);
  void set_uniform(const std::string &name, float x, float y, float z, float w);

private:
  GLuint program{0};
  GLuint vertex_shader{0};
  GLuint fragment_shader{0};

  std::unordered_map<std::string, GLint> uniform_cache;

  GLuint compile_shader(GLenum type, const std::string &source);
  GLint get_uniform_location(const std::string &name);
  bool link_program();
  void cleanup();
};

class ShaderManager
{
public:
  static ShaderManager &instance();

  std::shared_ptr<ShaderProgram> load(const std::string &name,
                                      const std::string &vert_path,
                                      const std::string &frag_path);

  std::shared_ptr<ShaderProgram> get(const std::string &name);

  void clear();

private:
  ShaderManager() = default;
  
  std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> shaders;
};

#endif // NAG_ENGINE_SHADER_MANAGER_H