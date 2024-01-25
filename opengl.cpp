#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include <random>

#include "KHR/khrplatform.h"
#include "GL/glext.h"
#include "GL/glcorearb.h"

static_assert(sizeof(GLsizei) == sizeof(int32_t));
static_assert(sizeof(GLuint) == sizeof(uint32_t));

namespace opengl {
    struct buffer_name {
        uint32_t value;
    };
	class buffer {

	};
    template<class N, class T>
    using name_space = std::map<uN,T>;
    class context {
        name_space<std::shared_ptr<buffer>> buffer_namespace;
        std::minstd_rand0 random_generator;
    };

    void generate_buffers(int32_t n, uint32_t* buffers) {
        std::uniform_int_distribution dist{};
    }
}

void glGenBuffers(GLsizei n, GLuint* buffers) {
    return opengl::generate_buffers(n, buffers);
}