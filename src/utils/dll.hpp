#ifndef NINJACLOWN_UTILS_DLL_HPP
#define NINJACLOWN_UTILS_DLL_HPP

#include <string>

#if defined OS_LINUX
#	include <dlfcn.h> // dlsym, dlopen, dlclose, dlerror
#elif defined OS_WINDOWS
#	include <Windows.h> // GetProcAddress, LoadLibrary, FreeLibrary, GetLastError, FormatMessageA
#else
#	error "Unsupported system"
#endif

namespace utils {

class dll {
public:
	dll() noexcept = default;
	~dll();

	explicit operator bool() const;

	[[nodiscard]] std::string error() const;
	[[nodiscard]] bool load(const char *dll_path);
	[[nodiscard]] bool load(const std::string &dll_path);

	template <typename FuncPtr>
	[[nodiscard]] FuncPtr get_address(const char *func_name) {
#if defined OS_WINDOWS
		return reinterpret_cast<FuncPtr>(GetProcAddress(m_handle, func_name));
#elif defined OS_LINUX
		return reinterpret_cast<FuncPtr>(dlsym(m_handle, func_name));
#endif
	}

	template <typename FuncPtr>
	[[nodiscard]] FuncPtr get_address(const std::string &func_name) {
		return get_address<FuncPtr>(func_name.c_str());
	}

private:
#if defined OS_WINDOWS
	HMODULE m_handle{};
#elif defined OS_LINUX
	void *m_handle{};
#endif
};

} // namespace utils

#endif // NINJACLOWN_UTILS_DLL_HPP
