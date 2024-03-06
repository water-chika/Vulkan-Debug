#include "boost/asio.hpp"
#include "font_loader.hpp"
#include "vulkan_render.hpp"

#include <GLFW/glfw3.h>

#undef min
#include <algorithm>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <strstream>
#include <thread>
#include <utility>

#include "multidimention_array.hpp"
#include "named_pipe.hpp"
#include "run_result.hpp"

struct glfwContext {
  glfwContext() {
    glfwInit();
    glfwSetErrorCallback([](int error, const char *msg) {
      std::cerr << "glfw: "
                << "(" << error << ") " << msg << std::endl;
    });
    assert(GLFW_TRUE == glfwVulkanSupported());
  }
  ~glfwContext() { glfwTerminate(); }
};

class window_manager {
public:
  window_manager() : window{create_window()} {
    window_map.emplace(window, this);
    glfwSetCharCallback(window, character_callback);
  }
  auto get_window() { return window; }
  auto create_surface(vk::Instance instance) {
    VkSurfaceKHR surface{};
    auto res = glfwCreateWindowSurface(instance, window, NULL, &surface);
    return surface;
  }
  void set_process_character_fun(auto &&fun) { process_character_fun = fun; }
  void process_character(uint32_t codepoint) {
    std::cout << codepoint;
    process_character_fun(codepoint);
  }
  static void character_callback(GLFWwindow *window, unsigned int codepoint) {
    auto manager = window_map[window];
    manager->process_character(codepoint);
  }
  run_result run() {
    glfwPollEvents();
    return glfwWindowShouldClose(window) ? run_result::eBreak
                                         : run_result::eContinue;
  }

private:
  static GLFWwindow *create_window() {
    uint32_t width = 1024, height = 1024;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    return glfwCreateWindow(width, height, "Terminal Emulator", nullptr,
                            nullptr);
  }
  glfwContext glfw_context;
  GLFWwindow *window;
  static std::map<GLFWwindow *, window_manager *> window_map;
  std::function<void(uint32_t)> process_character_fun;
};
std::map<GLFWwindow *, window_manager *> window_manager::window_map;

class terminal_buffer_manager {
public:
  auto &get_buffer() { return m_buffer; }
  void append_string(const std::string &str) {
    auto line_begin = str.begin();
    while (true) {
      auto line_end = std::find(line_begin, str.end(), '\n');
      std::string_view line{line_begin, line_end};
      append_str_data(line);
      if (line_end == str.end()) {
        break;
      }
      line_begin = line_end + 1;
      new_line();
    }
  }
  void append_line(const std::string_view str) {
    append_str_data(str);
    new_line();
  }
  void new_line() {
    auto &[x, y] = m_cursor_pos;
    auto leave_size = m_buffer.get_dim0_size() - m_cursor_pos.first;
    auto current_pos = y * 32 + x;
    std::for_each(m_buffer.begin() + current_pos,
                  m_buffer.begin() + current_pos + leave_size,
                  [](auto &c) { c = ' '; });
    m_cursor_pos.second = (m_cursor_pos.second + 1) % m_buffer.get_dim1_size();
    m_cursor_pos.first = 0;
  }
  void append_str_data(const std::string_view str) {
    auto &[x, y] = m_cursor_pos;
    auto current_pos = y * 32 + x;
    assert(m_buffer.size() > current_pos);
    auto leave_size = m_buffer.size() - current_pos;
    auto count = std::min(str.size(), leave_size);
    std::copy(str.begin(), str.begin() + count, m_buffer.begin() + current_pos);
    if (str.size() > leave_size) {
      auto count = str.size() - leave_size;
      std::copy(str.begin() + leave_size, str.end(), m_buffer.begin());
    }
    x += str.size();
    y += x / m_buffer.get_dim0_size();
    x %= m_buffer.get_dim0_size();
    y %= m_buffer.get_dim1_size();
  }

private:
  multidimention_array<char, 32, 16> m_buffer;
  std::pair<int, int> m_cursor_pos;
};

using namespace std::literals;
class terminal_emulator {
public:
  terminal_emulator(boost::asio::io_context& executor) : m_window_manager{}, m_render{}, m_buffer_manager{} {
    m_render.init(
        [this](vk::Instance instance) {
          return m_window_manager.create_surface(instance);
        },
        m_buffer_manager.get_buffer());
    m_render.notify_update();

    //generated by attribute_dependence_parser from terminal_emulator_run.depend
    using namespace windows;
    class pipe_async {
    public:
        pipe_async(terminal_emulator& emulator, boost::asio::io_context& executor, std::unique_ptr<process>&& shell, std::unique_ptr<boost::asio::readable_pipe>&& read_pipe) 
            : emulator{emulator},
            shell{std::move(shell)},
            read_pipe{std::move(read_pipe)},
            buf{std::make_unique<std::array<char, 128>>()}
        {
            async_read();
        }

        void operator()(const boost::system::error_code& err, std::size_t bytes_count) {
            process_text(bytes_count);
            async_read();
        }
        void process_text(std::size_t count) {
            emulator.m_buffer_manager.append_string(
                std::string{ buf->data(), count });
            emulator.m_render.notify_update();
            emulator.m_render.run();
        }
        void async_read() {
            auto mut_buf = boost::asio::mutable_buffer{ buf->data(), buf->size() };
            read_pipe->async_read_some(
                mut_buf,
                std::move(*this));
        }
    private:
        terminal_emulator& emulator;
        std::unique_ptr<process> shell;
        std::unique_ptr<boost::asio::readable_pipe> read_pipe;
        std::unique_ptr<std::array<char, 128>> buf;
    };
    auto [read_pipe_handle, write_pipe_handle] = create_pipe();
    auto shell = std::make_unique<process>("Debug/sh.exe", write_pipe_handle);
    auto read_pipe = std::make_unique<boost::asio::readable_pipe>(executor, read_pipe_handle);
    pipe_async pipe_async_v{ *this, executor, std::move(shell), std::move(read_pipe) };

    class window_run {
    public:
        window_run(terminal_emulator& emulator, boost::asio::io_context& executor) :
            emulator{emulator},
            executor{executor},
            timer{ executor, 1ms }
        {
            async_run();
        }
        void operator()(const boost::system::error_code& err) {
            if (emulator.m_window_manager.run() == run_result::eContinue) {
                async_run();
            }
            else {
                executor.stop();
            }
        }
        void async_run() {
            timer.expires_after(1ms);
            timer.async_wait(std::move(*this));
        }
    private:
        terminal_emulator& emulator;
        boost::asio::io_context& executor;
        boost::asio::steady_timer timer;
    };
    window_run window_run{ *this, executor };
  }

private:
  window_manager m_window_manager;
  renderer_presenter<vertex_renderer> m_render;
  terminal_buffer_manager m_buffer_manager;
};

int main() {
  try {
      boost::asio::io_context io{};
      terminal_emulator emulator{ io };
    io.run();
  } catch (vk::SystemError &err) {
    std::cout << "vk::SystemError: " << err.what() << std::endl;
    return -1;
  } catch (std::exception &err) {
    std::cout << "std::exception: " << err.what() << std::endl;
  } catch (...) {
    std::cout << "unknown error\n";
    return -1;
  }
  return 0;
}
