#include "Utils.h"
#include <iostream>

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
 
BOOL GetModuleDirectory(std::wstring& path) {
	HMODULE hm = NULL;
	BOOL res = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCWSTR)&GetModuleDirectory, &hm);
	if (res)
	{
		WCHAR buffer[MAX_PATH];
		DWORD length = GetModuleFileName(hm, buffer, sizeof(buffer));
		if (length > 0)
		{
			path = std::wstring(buffer, length);
			
			const std::wstring prefix = L"\\\\?\\";
			if (path.compare(0, prefix.length(), prefix) == 0) {
				path = path.substr(prefix.length());
				std::wcout << L"short path: " << path << std::endl;
			}

			size_t pos = path.find_last_of(L"\\");
			if (pos != std::wstring::npos)
			{
				path = path.substr(0, pos);
			}
			return TRUE;
		}
		else
		{
            return FALSE;
		}
	} 
	else
	{
		return FALSE;
	}
}