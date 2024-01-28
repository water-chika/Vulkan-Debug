#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include <random>
#include <algorithm>
#include <span>
#include <array>

#include "GL/glcorearb.h"
#include "color_2d_buffer.hpp"

static_assert(sizeof(GLsizei) == sizeof(int32_t));
static_assert(sizeof(GLuint) == sizeof(uint32_t));

namespace opengl {
    template<class N, class T>
    using name_space = std::map<N, T>;
    template<class N, class T>
    class name_manager {
    public:
        name_manager() : m_next_name{ 1 }, m_name_space{ std::pair<N,T>{0,T{}} } {}
        N alloc_a_name() {
            while (m_name_space.find(m_next_name) != m_name_space.end()) {
                ++m_next_name;
            }
            return m_next_name++;
        }
        void dealloc_name(N name) {
            m_name_space.erase(name);
        }
        T get(N name) {
            return m_name_space[name];
        }
        void set(N name, T t) {
            m_name_space.emplace(name, t);
        }
        bool is_allocated(N name) {
            return m_name_space.find(m_next_name) != m_name_space.end();
        }
    private:
        N m_next_name;
        name_space<N, T> m_name_space;
    };
    template<class T, class N>
    using target_buffer_map = std::map<T, N>;
	class buffer {
    public:
        struct name {
            uint32_t value;
            name operator++() {
                return name{ ++value };
            }
            name operator++(int) {
                return name{ value++ };
            }
            constexpr bool operator<(const name& rhs) const {
                return value < rhs.value;
            }
        };
        struct target {
            int value;
            constexpr bool operator<(const target& rhs) const {
                return value < rhs.value;
            }
        };
        struct{
            GLint64 size;
            GLenum usage;
            GLenum access;
            GLint access_flags;
            GLboolean immutable_storage;
            GLboolean mapped;
            GLvoid* map_pointer;
            GLint64 map_offset;
            GLint64 map_length;
            GLint storage_flags;
        } m_gl;
        void storage(size_t size, const void* data, uint32_t flags) {
            m_gl.size = size;
            m_gl.usage = GL_DYNAMIC_DRAW;
            m_gl.access = GL_READ_WRITE;
            m_gl.access_flags = 0;
            m_gl.immutable_storage = true;
            m_gl.mapped = GL_FALSE;
            m_gl.map_pointer = nullptr;
            m_gl.map_offset = 0;
            m_gl.map_length = 0;
            m_gl.storage_flags = flags;

            data_store.resize(size);
            std::ranges::copy(std::span{reinterpret_cast<const char*>(data), size}, data_store.begin());
        }
        void data(size_t size, const void* data, int usage) {
            m_gl.size = size;
            m_gl.usage = usage;
            m_gl.access = GL_READ_WRITE;
            m_gl.access_flags = 0;
            m_gl.immutable_storage = GL_FALSE;
            m_gl.mapped = GL_FALSE;
            m_gl.map_pointer = nullptr;
            m_gl.map_offset = 0;
            m_gl.map_length = 0;
            m_gl.storage_flags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT;

            data_store.resize(size);
            std::ranges::copy(std::span{ reinterpret_cast<const char*>(data), size }, &data_store[0]);
        }
        void sub_data(intptr_t offset, size_t size, const void* data) {
            std::ranges::copy(std::span{ reinterpret_cast<const char*>(data), size }, &data_store[offset]);
        }
        void clear_buffer_sub_data(int internalformat, intptr_t offset, size_t size, int format, int type, const void* data) {

        }
        void* map_range(intptr_t offset, size_t length, int access) {
            return &data_store[offset];
        }
        void* map(int access) {
            return &data_store[0];
        }
        bool unmap() {
            return true;
        }
    private:
        std::vector<char> data_store;
	};
    class buffer_manager {
    public:
        buffer_manager() = default;
        void generate_buffers(int32_t n, buffer::name* names) {
            for (int i = 0; i < n; i++) {
                names[i] = m_name_manager.alloc_a_name();
                m_name_manager.set(names[i], nullptr);
            }
        }
        void delete_buffers(int32_t n, buffer::name* names) {
            for (int i = 0; i < n; i++) {
                m_name_manager.dealloc_name(names[i]);
            }
        }
        bool is_buffer(buffer::name name) {
            return m_name_manager.is_allocated(name);
        }
        std::shared_ptr<buffer> create_buffer(buffer::name name) {
            m_name_manager.set(name, std::make_shared<buffer>());
            return m_name_manager.get(name);
        }
        auto get_or_create_buffer(buffer::name name) {
            if (m_name_manager.get(name) == nullptr) {
                create_buffer(name);
            }
            return m_name_manager.get(name);
        }
    private:
        name_manager<buffer::name, std::shared_ptr<buffer>> m_name_manager;
    };

    class texture {
    public:
    private:
        color_2d_buffer<format::rgba8> m_buffer;
    };
    class renderbuffer {
    public:
    private:
        color_2d_buffer<format::rgba8> m_buffer;
    };
    struct texture_target {
        int value;
    };
    struct texture_name {
        uint32_t value;
        constexpr texture_name operator++() {
            return texture_name{ ++value };
        }
        constexpr texture_name operator++(int) {
            return texture_name{ value++ };
        }
        friend constexpr bool operator<(texture_name lhs, texture_name rhs);
    };
    constexpr bool operator<(texture_name lhs, texture_name rhs) {
        return lhs.value < rhs.value;
    }
    class texture_manager {
    public:
        void gen_textures(int32_t n, texture_name* names) {
            for (int i = 0; i < n; i++) {
                names[i] = m_name_manager.alloc_a_name();
                m_name_manager.set(names[i], nullptr);
            }
        }
    private:
        name_manager<texture_name, std::shared_ptr<texture>> m_name_manager;
    };
    struct texture_unit_selector {
        int value;
    };

    class framebuffer {

    };
    class context {
    public:
        auto& get_buffer_manager() { return m_buffer_manager; }
        void bind_buffer(buffer::target target, buffer::name name) {
            auto buffer = m_buffer_manager.get_or_create_buffer(name);
            m_target_buffer_map.emplace(target, buffer);
        }
        void buffer_storage(buffer::target target, size_t size, const void* data, uint32_t flags) {
            auto buffer = m_target_buffer_map[target];
            buffer->storage(size, data, flags);
        }
        void buffer_data(buffer::target target, size_t size, const void* data, int usage) {
            auto buffer = m_target_buffer_map[target];
            buffer->data(size, data, usage);
        }
        void buffer_sub_data(buffer::target target, intptr_t offset, size_t size, const void* data) {
            auto buffer = m_target_buffer_map[target];
            buffer->sub_data(offset, size, data);
        }
        void clear_buffer_sub_data(buffer::target target, int internalformat, intptr_t offset, size_t size, int format, int type, const void* data) {
            auto buffer = m_target_buffer_map[target];
            buffer->clear_buffer_sub_data(internalformat, offset, size, format, type, data);
        }
        void* map_buffer_range(buffer::target target, intptr_t offset, size_t length, int access) {
            auto buffer = m_target_buffer_map[target];
            return buffer->map_range(offset, length, access);
        }
        void* map_buffer(buffer::target target, int access) {
            auto buffer = m_target_buffer_map[target];
            return buffer->map(access);
        }
        bool unmap_buffer(buffer::target target) {
            auto buffer = m_target_buffer_map[target];
            return buffer->unmap();
        }
        void active_texture(texture_unit_selector selector) {
            m_active_texture_unit_selector = selector;
        }
        void gen_textures(int32_t n, texture_name* names) {
            m_texture_manager.gen_textures(n, names);
        }
    private:
        buffer_manager m_buffer_manager;
        target_buffer_map<buffer::target, std::shared_ptr<buffer>> m_target_buffer_map;

        texture_manager m_texture_manager;
        texture_unit_selector m_active_texture_unit_selector;

        std::shared_ptr<framebuffer> m_draw_framebuffer;
        std::shared_ptr<framebuffer> m_read_framebuffer;
    };


    context g_context;

    void generate_buffers(uint32_t n, uint32_t* buffers) {
        g_context.get_buffer_manager().generate_buffers(n, reinterpret_cast<buffer::name*>(buffers));
    }
    void delete_buffers(uint32_t n, uint32_t* buffers) {
        g_context.get_buffer_manager().delete_buffers(n, reinterpret_cast<buffer::name*>(buffers));
    }
    GLboolean is_buffer(uint32_t buffer) {
        return g_context.get_buffer_manager().is_buffer(buffer::name{ buffer });
    }
    void bind_buffer(int target, uint32_t buffer) {
        g_context.bind_buffer(buffer::target{ target }, buffer::name{ buffer });
    }
    void buffer_storage(int target, size_t size, const void* data, uint32_t flags) {
        g_context.buffer_storage(buffer::target{ target }, size, data, flags);
    }
    void buffer_data(int target, size_t size, const void* data, int usage) {
        g_context.buffer_data(buffer::target{ target }, size, data, usage);
    }
    void buffer_sub_data(int target, intptr_t offset, size_t size, const void* data) {
        g_context.buffer_sub_data(buffer::target{ target }, offset, size, data);
    }
    void clear_buffer_sub_data(int target, int internalformat, intptr_t offset, size_t size, int format, int type, const void* data) {
        g_context.clear_buffer_sub_data(buffer::target{ target }, internalformat, offset, size, format, type, data);
    }
    void* map_buffer_range(int target, intptr_t offset, size_t length, int access) {
        return g_context.map_buffer_range(buffer::target{ target }, offset, length, access);
    }
    void* map_buffer(int target, int access) {
        return g_context.map_buffer(buffer::target{ target }, access);
    }
    bool unmap_buffer(int target) {
        return g_context.unmap_buffer(buffer::target{ target });
    }
    void active_texture(int texture) {
        return g_context.active_texture(texture_unit_selector{ texture });
    }
    void gen_textures(uint32_t n, uint32_t* textures) {
        return g_context.gen_textures(n, reinterpret_cast<texture_name*>(textures));
    }
}

GLvoid glGenBuffers(GLsizei n, GLuint* buffers) {
    return opengl::generate_buffers(n, buffers);
}

GLvoid glDeletebuffers(GLsizei n, GLuint* buffers) {
    return opengl::delete_buffers(n, buffers);
}

GLboolean glIsBuffer(GLuint buffer) {
    return opengl::is_buffer(buffer);
}

GLvoid glBindBuffer(GLenum target, GLuint buffer) {
    return opengl::bind_buffer(target, buffer);
}


GLvoid glBufferStorage(GLenum target, GLsizeiptr size, const
    GLvoid* data, GLbitfield flags) {
    return opengl::buffer_storage(target, size, data, flags);
}

GLvoid glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) {
    return opengl::buffer_data(target, size, data, usage);
}

GLvoid glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data) {
    return opengl::buffer_sub_data(target, offset, size, data);
}

GLvoid glClearBufferSubData(GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const GLvoid* data) {
    return opengl::clear_buffer_sub_data(target, internalformat, offset, size, format, type, data);
}

GLvoid* glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {
    return opengl::map_buffer_range(target, offset, length, access);
}

GLvoid* glMapBuffer(GLenum target, GLenum access) {
    return opengl::map_buffer(target, access);
}

GLboolean glUnmapBuffer(GLenum target) {
    return opengl::unmap_buffer(target);
}

GLvoid glActiveTexture(GLenum texture) {
    return opengl::active_texture(texture);
}

GLvoid glGenTextures(GLsizei n, GLuint* textures) {
    return opengl::gen_textures(n, textures);
}

