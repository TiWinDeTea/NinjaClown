#include "utils/system.hpp"

#ifdef OS_WINDOWS
#	include <Windows.h>
#else
#	include <unistd.h>
#	include <string.h>
#endif

std::filesystem::path utils::binary_directory() {
	std::size_t array_size = 128;
	do {
		array_size *= 2;
		auto ptr = std::make_unique<char[]>(array_size);

#ifdef OS_WINDOWS
        DWORD result = GetModuleFileNameA(nullptr, ptr.get(), static_cast<DWORD>(array_size));
#else
		ssize_t result = readlink("/proc/self/exe", ptr.get(), array_size);
#endif
		if (result <= 0) {
			return std::filesystem::current_path();
		}
		else if (result < array_size) {
			return std::filesystem::path{ptr.get(), ptr.get() + result}.parent_path();
		}
	} while (true);
}

std::filesystem::path utils::resources_directory() {
	return binary_directory() / "resources";
}

std::filesystem::path utils::config_directory() {
	return binary_directory() / "resources";
}

std::string utils::sys_last_error() {
#ifdef OS_WINDOWS
	DWORD errorMessageID = GetLastError();
	if (errorMessageID == 0) {
		return {};
	}
	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
	                             errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &messageBuffer, 0, nullptr);
	std::string message(messageBuffer, size);
	LocalFree(messageBuffer);
	return message;
#else
    static_assert(std::is_same_v<std::invoke_result_t<decltype(strerror_r), int, char *, size_t>, char *>,
	              "using XSI-compliant strerror_r instead of GNU-specific");

	constexpr int buffer_sz = 1024;
	auto buffer = std::make_unique<char[]>(buffer_sz);
	return strerror_r(errno, buffer.get(), buffer_sz);
#endif
}