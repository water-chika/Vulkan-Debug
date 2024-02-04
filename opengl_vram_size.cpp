#include <glad/wgl.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32 1
#include <GLFW/glfw3native.h>

#include <iostream>
#include <cassert>
#include <vector>

int main() {
	assert(glfwInit() == GLFW_TRUE);
	GLFWwindow* window = glfwCreateWindow(128, 128, "OpenGL VRAM size", NULL, NULL);
	glfwMakeContextCurrent(window);
	int version = gladLoadGL(glfwGetProcAddress);
	assert(version != 0);
	HWND hwnd = glfwGetWin32Window(window);
	gladLoadWGL(GetDC(hwnd), glfwGetProcAddress);
	{
		std::vector<UINT> ids(32);
		auto count = wglGetGPUIDsAMD(ids.size(), ids.data());
		ids.resize(count);
		for (auto id : ids) {
			GLuint vram_size;
			wglGetGPUInfoAMD(id, WGL_GPU_RAM_AMD, GL_UNSIGNED_INT, sizeof(vram_size), &vram_size);
			std::vector<char> renderer(128);
			wglGetGPUInfoAMD(id, WGL_GPU_RENDERER_STRING_AMD, GL_UNSIGNED_BYTE, renderer.size(), renderer.data());
			std::cout << renderer.data() << ": " << vram_size << std::endl;
		}
	}
	glfwTerminate();
	return 0;
}