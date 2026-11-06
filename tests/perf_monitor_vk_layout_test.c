// The panel asks the Vulkan loader for the card's name and its device-local
// memory without linking Vulkan (perf_monitor.c's vulkan_device), so the few
// structures it hands the driver are declared there by hand. This checks those
// declarations against the real header when the build has it: a field that moved
// would otherwise show as a garbled name or a wrong VRAM total, not as a build
// error. It also holds on a build without the header (the sizes should not change).
#include "../runtime/host/src/perf_monitor.c"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#if defined(__has_include)
#if __has_include(<vulkan/vulkan.h>)
#define BW_HAVE_VULKAN_HEADERS 1
#include <vulkan/vulkan.h>
#endif
#endif

#if defined(BW_HAVE_VULKAN_HEADERS)
int main(void) {
    // What the driver writes into, and what is read back out of it.
    assert(sizeof(BwVkApplicationInfo) == sizeof(VkApplicationInfo));
    assert(sizeof(BwVkInstanceCreateInfo) == sizeof(VkInstanceCreateInfo));
    assert(offsetof(BwVkInstanceCreateInfo, enabledExtensionCount) ==
           offsetof(VkInstanceCreateInfo, enabledExtensionCount));
    assert(sizeof(BwVkMemoryType) == sizeof(VkMemoryType));
    assert(sizeof(BwVkMemoryHeap) == sizeof(VkMemoryHeap));
    assert(sizeof(BwVkMemoryProperties) == sizeof(VkPhysicalDeviceMemoryProperties));
    assert(offsetof(BwVkMemoryProperties, memoryHeaps) == offsetof(VkPhysicalDeviceMemoryProperties, memoryHeaps));
    assert(offsetof(BwVkDeviceHead, deviceName) == offsetof(VkPhysicalDeviceProperties, deviceName));
    assert(sizeof(((BwVkDeviceHead*)0)->deviceName) == sizeof(((VkPhysicalDeviceProperties*)0)->deviceName));
    // The structure types and the heap flag are the only other numbers passed in.
    assert(BW_VK_APPLICATION_INFO == (int)VK_STRUCTURE_TYPE_APPLICATION_INFO);
    assert(BW_VK_INSTANCE_CREATE_INFO == (int)VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
    assert(BW_VK_DEVICE_LOCAL_HEAP == (unsigned)VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
    assert(BW_VK_API_1_0 == VK_API_VERSION_1_0);
    printf("perf_monitor_vk_layout_test: the hand-written Vulkan structures match the header\n");
    return 0;
}
#else
int main(void) {
    // Without <vulkan/vulkan.h> there is nothing to compare against; the structures
    // are still the ones the loader writes, and the panel says "-" rather than a
    // wrong number when they are not.
    printf("perf_monitor_vk_layout_test: no Vulkan headers in the build, skipped\n");
    return 0;
}
#endif
