#include "utils/system.hpp"

#ifdef OS_WINDOWS
#	include <Windows.h>
#else
#	include <unistd.h>
#	include <string.h>
#	include <array>
#endif

std::filesystem::path utils::binary_directory() {
	std::size_t array_size = 128;

	for (;;) {
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

		if (result < array_size) {
			return std::filesystem::path{ptr.get(), ptr.get() + result}.parent_path();
		}
	}
}

std::filesystem::path utils::resources_directory() {
	return binary_directory() / "resources";
}

std::filesystem::path utils::config_directory() {
	return resources_directory();
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

	std::array<char, 1024> buffer{};
	std::string error = strerror_r(errno, buffer.data(), buffer.size());
	return error;
#endif
}