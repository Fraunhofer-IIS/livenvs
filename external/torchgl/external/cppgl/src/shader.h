#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;
#include <map>
#include <memory>
#include <vector>
#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm.hpp>
#include "named_handle.h"
#include "texture.h"

CPPGL_NAMESPACE_BEGIN

// ------------------------------------------
// Shader

class ShaderImpl {
public:
    ShaderImpl(const std::string& name);
    ShaderImpl(const std::string& name, const fs::path& compute_source);
    ShaderImpl(const std::string& name, const fs::path& vertex_source, const fs::path& fragment_source);
    ShaderImpl(const std::string& name, const fs::path& vertex_source, const fs::path& geometry_source, const fs::path& fragment_source);
    virtual ~ShaderImpl();

    // prevent copies and moves, since GL buffers aren't reference counted
    ShaderImpl(const ShaderImpl&) = delete;
    ShaderImpl(const ShaderImpl&&) = delete; // TODO allow moves?
    ShaderImpl& operator=(const ShaderImpl&) = delete;
    ShaderImpl& operator=(const ShaderImpl&&) = delete; // TODO allow moves?

    explicit inline operator bool() const  { return glIsProgram(id); }
    inline operator GLuint() const { return id; }

    // bind/unbind to/from OpenGL
    void bind() const;
    void unbind() const;

    // set the path to the source file for the shader type
    void set_source(GLenum type, const fs::path& path);
    void set_vertex_source(const fs::path& path);
    void set_tesselation_control_source(const fs::path& path);
    void set_tesselation_evaluation_source(const fs::path& path);
    void set_geometry_source(const fs::path& path);
    void set_fragment_source(const fs::path& path);
    void set_compute_source(const fs::path& path);

    // compile and link shader from previously given source files
    void compile();

    // compute shader dispatch (call with actual amount of threads, will internally divide by workgroup size), memory_barrier_bits is option for automatic glMemoryBarrier(memory_barrier_bits)
    void dispatch_compute(uint32_t w, uint32_t h = 1, uint32_t d = 1, GLbitfield memory_barrier_bits = GL_ALL_BARRIER_BITS) const;

    // uniform upload handling
    void uniform(const std::string& name, int val) const;
    void uniform(const std::string& name, uint32_t val) const;
    void uniform(const std::string& name, int* val, uint32_t count) const;
    void uniform(const std::string& name, uint32_t* val, uint32_t count) const;
    void uniform(const std::string& name, float val) const;
    void uniform(const std::string& name, float* val, uint32_t count) const;
    void uniform(const std::string& name, const glm::vec2& val) const;
    void uniform(const std::string& name, const glm::vec3& val) const;
    void uniform(const std::string& name, const glm::vec4& val) const;
    void uniform(const std::string& name, const glm::ivec2& val) const;
    void uniform(const std::string& name, const glm::ivec3& val) const;
    void uniform(const std::string& name, const glm::ivec4& val) const;
    void uniform(const std::string& name, const glm::uvec2& val) const;
    void uniform(const std::string& name, const glm::uvec3& val) const;
    void uniform(const std::string& name, const glm::uvec4& val) const;
    void uniform(const std::string& name, const glm::mat3& val) const;
    void uniform(const std::string& name, const glm::mat4& val) const;
    void uniform(const std::string& name, const Texture2D& tex, uint32_t unit) const;
    void uniform(const std::string& name, const Texture3D& tex, uint32_t unit) const;
    void uniform(const std::string& name, const Texture2DArray& tex, uint32_t unit) const;

    // clear shader
    void clear();
    // check and reload if modified (return true if reloaded)
    bool reload_if_modified();
    
    // set default paths to search for shader source files
    static void add_shader_search_path(fs::path path);

    // data
    const std::string name;
    GLuint id;
    std::map<GLenum, fs::path> source_files;
    std::map<GLenum, fs::file_time_type> timestamps;
    std::map<fs::path, fs::file_time_type> include_timestamps;
    
    static std::vector<fs::path> shader_search_paths;
};

using Shader = NamedHandle<ShaderImpl>;
template class _API NamedHandle<ShaderImpl>; //needed for Windows DLL export

bool reload_modified_shaders();

CPPGL_NAMESPACE_END
