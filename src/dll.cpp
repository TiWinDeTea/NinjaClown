#include "dll.hpp"

dll::dll(const char *dll_path) noexcept
{
#if defined OS_WINDOWS
	m_handle = LoadLibrary(dll_path);
#elif defined OS_LINUX
	m_handle = dlopen(dll_path, RTLD_NOW);
#endif
}

dll::dll(const std::string &dll_path) noexcept
    : dll(dll_path.c_str())
{
}

dll::~dll()
{
	if (m_handle == nullptr) {
		return;
	}

#if defined OS_WINDOWS
	FreeLibrary(m_handle);
#elif defined OS_LINUX
	dlclose(m_handle);
#endif
}

dll::operator bool() const
{
	return m_handle != nullptr;
}

std::string dll::error() const
{
#if defined OS_WINDOWS
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0) {
		return {};
	}
	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
	                             errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &messageBuffer, 0, nullptr);
	std::string message(messageBuffer, size);
	LocalFree(messageBuffer);
	return message;
#elif defined OS_LINUX
	const char *error = dlerror();
	if (error == nullptr) {
		return "";
	}
	return error;
#endif
}

void dll::reload(const char* dll_path) {
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
}

void dll::reload(const std::string& dll_path){
	reload(dll_path.c_str());
}
