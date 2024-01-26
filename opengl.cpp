#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include <random>
#include <algorithm>
#include <span>

#include "GL/glcorearb.h"

static_assert(sizeof(GLsizei) == sizeof(int32_t));
static_assert(sizeof(GLuint) == sizeof(uint32_t));

namespace opengl {
    template<class N, class T>
    using name_space = std::map<N, T>;
    template<class T, class N>
    using target_buffer_map = std::map<T, N>;
	class buffer {
    public:
        struct name {
            uint32_t value;
            name get_next_name() {
                return name{ ++value };
            }
            constexpr bool operator<(const name& rhs) const {
                return value < rhs.value;
            }
        };
        struct target {
            int value;
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
        void storage(ptrdiff_t size, const void* data, uint32_t flags) {
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
        void data(ptrdiff_t size, const void* data, int usage) {
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
        void sub_data(intptr_t offset, ptrdiff_t size, const void* data) {
            std::ranges::copy(std::span{ reinterpret_cast<const char*>(data), size }, &data_store[offset]);
        }
        void clear_buffer_sub_data(int internalformat, intptr_t offset, ptrdiff_t size, int format, int type, const void* data) {

        }
        void* map_range(intptr_t offset, ptrdiff_t length, int access) {
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
        buffer_manager() : next_name{ 1 }, buffer_name_space{ std::pair{buffer::name{0}, std::shared_ptr<buffer>{} } } {}
        void generate_buffers(int32_t n, buffer::name* names) {
            for (int i = 0; i < n; i++) {
                while (buffer_name_space.find(next_name) != buffer_name_space.end()) {
                    next_name = next_name.get_next_name();
                }
                names[i] = next_name;
                buffer_name_space.emplace(names[i], nullptr);
                next_name = next_name.get_next_name();
            }
        }
        void delete_buffers(int32_t n, buffer::name* names) {
            for (int i = 0; i < n; i++) {
                buffer_name_space.erase(names[i]);
            }
        }
        bool is_buffer(buffer::name name) {
            return buffer_name_space.find(name) != buffer_name_space.end();
        }
        std::shared_ptr<buffer> create_buffer(buffer::name name) {
            buffer_name_space[name] = std::make_shared<buffer>();
            return buffer_name_space[name];
        }
        auto get_or_create_buffer(buffer::name name) {
            if (buffer_name_space[name] == nullptr) {
                create_buffer(name);
            }
            return buffer_name_space[name];
        }
    private:
        buffer::name next_name;
        name_space<buffer::name, std::shared_ptr<buffer>> buffer_name_space;

    };
    class context {
    public:
        auto& get_buffer_manager() { return buffer_manager; }
        void bind_buffer(buffer::target target, buffer::name name) {
            auto buffer = buffer_manager.get_or_create_buffer(name);
            target_buffer_map.emplace(target, buffer);
        }
        void buffer_storage(buffer::target target, ptrdiff_t size, const void* data, uint32_t flags) {
            auto buffer = target_buffer_map[target];
            buffer->storage(size, data, flags);
        }
        void buffer_data(buffer::target target, ptrdiff_t size, const void* data, int usage) {
            auto buffer = target_buffer_map[target];
            buffer->data(size, data, usage);
        }
        void buffer_sub_data(buffer::target target, intptr_t offset, ptrdiff_t size, const void* data) {
            auto buffer = target_buffer_map[target];
            buffer->sub_data(offset, size, data);
        }
        void clear_buffer_sub_data(buffer::target target, int internalformat, intptr_t offset, ptrdiff_t size, int format, int type, const void* data) {
            auto buffer = target_buffer_map[target];
            buffer->clear_buffer_sub_data(internalformat, offset, size, format, type, data);
        }
        void* map_buffer_range(buffer::target target, intptr_t offset, ptrdiff_t length, int access) {
            auto buffer = target_buffer_map[target];
            return buffer->map_range(offset, length, access);
        }
        void* map_buffer(buffer::target target, int access) {
            auto buffer = target_buffer_map[target];
            return buffer->map(access);
        }
        bool unmap_buffer(buffer::target target) {
            auto buffer = target_buffer_map[target];
            return buffer->unmap();
        }
    private:
        buffer_manager buffer_manager;
        target_buffer_map<buffer::target, std::shared_ptr<buffer>> target_buffer_map;
    };


    context g_context;

    void generate_buffers(int32_t n, uint32_t* buffers) {
        g_context.get_buffer_manager().generate_buffers(n, reinterpret_cast<buffer::name*>(buffers));
    }
    void delete_buffers(int32_t n, uint32_t* buffers) {
        g_context.get_buffer_manager().delete_buffers(n, reinterpret_cast<buffer::name*>(buffers));
    }
    GLboolean is_buffer(uint32_t buffer) {
        return g_context.get_buffer_manager().is_buffer(buffer::name{ buffer });
    }
    void bind_buffer(int target, uint32_t buffer) {
        g_context.bind_buffer(buffer::target{ target }, buffer::name{ buffer });
    }
    void buffer_storage(int target, ptrdiff_t size, const void* data, uint32_t flags) {
        g_context.buffer_storage(buffer::target{ target }, size, data, flags);
    }
    void buffer_data(int target, ptrdiff_t size, const void* data, int usage) {
        g_context.buffer_data(buffer::target{ target }, size, data, usage);
    }
    void buffer_sub_data(int target, intptr_t offset, ptrdiff_t size, const void* data) {
        g_context.buffer_sub_data(buffer::target{ target }, offset, size, data);
    }
    void clear_buffer_sub_data(int target, int internalformat, intptr_t offset, ptrdiff_t size, int format, int type, const void* data) {
        g_context.clear_buffer_sub_data(buffer::target{ target }, internalformat, offset, size, format, type, data);
    }
    void* map_buffer_range(int target, intptr_t offset, ptrdiff_t length, int access) {
        return g_context.map_buffer_range(buffer::target{ target }, offset, length, access);
    }
    void* map_buffer(int target, int access) {
        return g_context.map_buffer(buffer::target{ target }, access);
    }
    bool unmap_buffer(int target) {
        return g_context.unmap_buffer(buffer::target{ target });
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
