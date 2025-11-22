#pragma once
#include <string>

bool loadService(const std::wstring& driverPath, const std::wstring& serviceName);

bool unloadService(const std::wstring& serviceName);