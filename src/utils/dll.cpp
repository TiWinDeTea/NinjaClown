#include "utils/dll.hpp"

#ifdef OS_WINDOWS
#	include "utils/system.hpp"
#endif

namespace utils {

dll::~dll() {
	if (m_handle == nullptr) {
		return;
	}

#if defined OS_WINDOWS
	FreeLibrary(m_handle);
#elif defined OS_LINUX
	dlclose(m_handle);
#endif
}

dll::operator bool() const {
	return m_handle != nullptr;
}

std::string dll::error() const {
#if defined OS_WINDOWS
	return sys_last_error();
#elif defined OS_LINUX
	const char *error = dlerror();
	if (error == nullptr) {
		return "";
	}
	return error;
#endif
}

bool dll::load(const char *dll_path) {
#if defined OS_WINDOWS
	if (m_handle != nullptr) {
		FreeLibrary(m_handle);
	}
	m_handle = LoadLibrary(dll_path);
#elif defined OS_LINUX
	if (m_handle != nullptr) {
		dlclose(m_handle);
	}
	m_handle = dlopen(dll_path, RTLD_NOW);
#endif

	return m_handle != nullptr;
}

bool dll::load(const std::string &dll_path) {
	return load(dll_path.c_str());
}

} // namespace utils
