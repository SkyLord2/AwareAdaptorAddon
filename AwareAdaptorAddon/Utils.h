#pragma once
#include <string>
#include <windows.h>

std::string WcharToUtf8(const wchar_t* wstr);

BOOL GetModuleDirectory(std::wstring& path);
