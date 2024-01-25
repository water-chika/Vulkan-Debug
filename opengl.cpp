#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include <random>
#include <algorithm>

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
        } gl;
        void storage(ptrdiff_t size, const void* data, uint32_t flags) {

        }
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
            buffer->storage(ptrdiff_t size, const void* data, uint32_t flags);
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
        g_context.get_buffer_manager().is_buffer(buffer::name{ buffer });
    }
    void bind_buffer(int target, uint32_t buffer) {
        g_context.bind_buffer(buffer::target{ target }, buffer::name{ buffer });
    }
    void buffer_storage(int target, ptrdiff_t size, const void* data, uint32_t flags) {
        g_context.buffer_storage(buffer::target{ target }, size, data, flags);
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