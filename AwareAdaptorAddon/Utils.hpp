#include <windows.h>
#include <vector>
#include <string>

std::string WcharToUtf8(const wchar_t* wstr) {
	if (wstr == nullptr) return "";
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
	if (size_needed == 0) return "";

	std::string result(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &result[0], size_needed, nullptr, nullptr);
	// ÒÆ³ýÄ©Î²µÄ null ÖÕÖ¹·û
	if (!result.empty() && result[result.size() - 1] == '\0') {
		result.pop_back();
	}
	return result;
}