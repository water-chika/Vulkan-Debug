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
    struct buffer_name {
        uint32_t value;
        buffer_name get_next_name() {
            return buffer_name{ ++value };
        }
        constexpr bool operator<(const buffer_name& rhs) const {
            return value < rhs.value;
        }
    };
	class buffer {

	};
    class buffer_manager {
    public:
        void generate_buffers(int32_t n, buffer_name* names) {
            for (int i = 0; i < n; i++) {
                while (buffer_name_space.find(next_name) != buffer_name_space.end()) {
                    next_name = next_name.get_next_name();
                }
                names[i] = next_name;
                buffer_name_space.emplace(names[i], nullptr);
                next_name = next_name.get_next_name();
            }
        }
    private:
        buffer_name next_name;
        name_space<buffer_name, std::shared_ptr<buffer>> buffer_name_space;
    };
    class context {
    public:
        auto& get_buffer_manager() { return buffer_manager; }
    private:
        buffer_manager buffer_manager;
    };
    context g_context;

    void generate_buffers(int32_t n, uint32_t* buffers) {
        g_context.get_buffer_manager().generate_buffers(n, reinterpret_cast<buffer_name*>(buffers));
    }
}

void glGenBuffers(GLsizei n, GLuint* buffers) {
    return opengl::generate_buffers(n, buffers);
}