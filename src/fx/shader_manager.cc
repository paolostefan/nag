#include "engine/shader_manager.h"

#include <unistd.h>

#include <fstream>
#include <sstream>

#include "spdlog/spdlog.h"

ShaderProgram::~ShaderProgram() {
  cleanup();
}

void ShaderProgram::cleanup() {
  if (vertex_shader) {
    glDeleteShader(vertex_shader);
    vertex_shader = 0;
  }
  if (fragment_shader) {
    glDeleteShader(fragment_shader);
    fragment_shader = 0;
  }
  if (program) {
    glDeleteProgram(program);
    program = 0;
  }
}

bool ShaderProgram::load_from_files(const std::string &vert_path, const std::string &frag_path) {
  char cwd[1024];

  if (getcwd(cwd, sizeof(cwd)) == nullptr) {
    spdlog::error("Failed to get current working directory");
    return false;
  }

  // Read vertex shader
  std::ifstream vert_file(vert_path);
  if (!vert_file.is_open()) {
    spdlog::error("Failed to open vertex shader: {}", vert_path);
    return false;
  }

  std::stringstream vert_stream;
  vert_stream << vert_file.rdbuf();
  std::string vert_src = vert_stream.str();

  // Read fragment shader
  std::ifstream frag_file(frag_path);
  if (!frag_file.is_open()) {
    spdlog::error("Failed to open fragment shader: {}", frag_path);
    return false;
  }
  std::stringstream frag_stream;
  frag_stream << frag_file.rdbuf();
  std::string frag_src = frag_stream.str();

  return load_from_source(vert_src, frag_src);
}

bool ShaderProgram::load_from_source(const std::string &vert_src, const std::string &frag_src) {
  cleanup();

  vertex_shader = compile_shader(GL_VERTEX_SHADER, vert_src);
  if (!vertex_shader)
    return false;

  fragment_shader = compile_shader(GL_FRAGMENT_SHADER, frag_src);
  if (!fragment_shader) {
    cleanup();
    return false;
  }

  program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);

  if (!link_program()) {
    cleanup();
    return false;
  }

  return true;
}

GLuint ShaderProgram::compile_shader(const GLenum type, const std::string &source) {
  const GLuint shader = glCreateShader(type);
  const char *src = source.c_str();

  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    GLchar info_log[512];
    glGetShaderInfoLog(shader, 512, nullptr, info_log);
    spdlog::error("{} shader compilation failed: {}",
                  type == GL_VERTEX_SHADER ? "Vertex" : "Fragment",
                  info_log);
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

bool ShaderProgram::link_program() const {
  glLinkProgram(program);

  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    GLchar info_log[512];
    glGetProgramInfoLog(program, 512, nullptr, info_log);
    spdlog::error("Shader program linking failed: {}", info_log);
    return false;
  }

  return true;
}

void ShaderProgram::use() const {
  if (program)
    glUseProgram(program);
}

GLint ShaderProgram::get_uniform_location(const std::string &name) {
  if (const auto it = uniform_cache.find(name); it != uniform_cache.end())
    return it->second;

  const GLint loc = glGetUniformLocation(program, name.c_str());
  uniform_cache[name] = loc;

  if (loc == -1) {
    spdlog::warn("Uniform '{}' not found in shader {}", name, shader_name);
  }

  return loc;
}

void ShaderProgram::set_uniform(const std::string &name, const float value) {
  if (const GLint loc = get_uniform_location(name); loc != -1)
    glUniform1f(loc, value);
}

void ShaderProgram::set_uniform(const std::string &name, const int value) {
  if (const GLint loc = get_uniform_location(name); loc != -1)
    glUniform1i(loc, value);
}

void ShaderProgram::set_uniform(const std::string &name, const float x, const float y) {
  if (const GLint loc = get_uniform_location(name); loc != -1)
    glUniform2f(loc, x, y);
}

void ShaderProgram::set_uniform(const std::string &name, const float x, const float y, const float z) {
  if (const GLint loc = get_uniform_location(name); loc != -1)
    glUniform3f(loc, x, y, z);
}

void ShaderProgram::set_uniform(const std::string &name, const float x, const float y, const float z, const float w) {
  if (const GLint loc = get_uniform_location(name); loc != -1)
    glUniform4f(loc, x, y, z, w);
}

ShaderManager &ShaderManager::instance() {
  static ShaderManager inst;
  return inst;
}

std::shared_ptr<ShaderProgram> ShaderManager::load(const std::string &name,
                                                   const std::string &vert_src,
                                                   const std::string &frag_src) {
  if (const auto it = shaders.find(name); it != shaders.end()) {
    spdlog::debug("Shader '{}' already loaded, returning cached version", name);
    return it->second;
  }

  auto shader = std::make_shared<ShaderProgram>(name);
  if (!shader->load_from_source(vert_src, frag_src)) {
    spdlog::error("Failed to load shader '{}'", name);
    return nullptr;
  }

  shaders[name] = shader;
  spdlog::info("Shader '{}' loaded successfully", name);
  return shader;
}

std::shared_ptr<ShaderProgram> ShaderManager::get(const std::string &name) {
  if (const auto it = shaders.find(name); it != shaders.end())
    return it->second;

  spdlog::warn("Shader '{}' not found in cache", name);
  return nullptr;
}

void ShaderManager::clear() {
  spdlog::info("Clearing shader cache ({} shaders)", shaders.size());
  shaders.clear();
}

std::shared_ptr<ShaderProgram> ShaderManager::load_from_source(const std::string &name,
                                                               const std::string &vert_src,
                                                               const std::string &frag_src) {
  if (const auto it = shaders.find(name); it != shaders.end()) {
    spdlog::debug("Shader '{}' already loaded, returning cached version", name);
    return it->second;
  }

  auto shader = std::make_shared<ShaderProgram>(name);
  if (!shader->load_from_source(vert_src, frag_src)) {
    spdlog::error("Failed to load shader '{}' from source", name);
    return nullptr;
  }

  shaders[name] = shader;
  spdlog::info("Shader '{}' loaded successfully from source", name);
  return shader;
}
