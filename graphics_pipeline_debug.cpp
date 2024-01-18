#include <stdexcept>
#include <iostream>
#include <vector>
#include <cassert>
#include <fstream>
#include <array>
#include <vulkan/vulkan.h>
#include "vulkan_helper.hpp"


template<class PD>
class queue_family : public PD {
public:
    queue_family() : queue_family_index{
        PD::find_queue_family_if(
            [](VkQueueFamilyProperties properties) {
                return VK_QUEUE_GRAPHICS_BIT & properties.queueFlags;
            }
    }
    {}
            uint32_t get_queue_family_index() {
                return queue_family_index;
            }
private:
    uint32_t queue_family_index;
};

using app_parent = 
queue_family<vulkan_helper::first_physical_device>;

class App : public app_parent {
public:
    void run() {

    }
};

int main() {
    try {
        App app;
        app.run();
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
	return 0;
}