#include "utils/system.hpp"

#ifdef OS_WINDOWS
#	include <Windows.h>
#else
#	include <unistd.h>
#endif

std::filesystem::path utils::binary_directory() {
    std::size_t array_size = 128;
    do {
		array_size *= 2;
        auto ptr = std::make_unique<char[]>(array_size);

#ifdef OS_WINDOWS
		DWORD result = GetModuleFileNameA(nullptr, ptr.get(), array_size);
#	else
		ssize_t result = readlink("/proc/self/exe", ptr.get(), array_size);
#endif
		if (result <= 0) {
			return std::filesystem::current_path();
		} else if (result < array_size) {
            return {ptr.get(), ptr.get() + result};
		}
	} while (true);
}