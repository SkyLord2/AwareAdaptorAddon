// MinifilterInstaller.h
#pragma once
#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <iostream>
#include <string>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")



class MinifilterInstaller {
public:
	MinifilterInstaller();
	~MinifilterInstaller();

	// 安装Minifilter驱动
	BOOL InstallDriver(const std::wstring& infPath,
		const std::wstring& driverName);
	BOOL InstallDriverEx(const std::wstring& infPath,
		const std::wstring& driverName);
	BOOL InstallDriverByShell(const std::wstring& infPath,
		const std::wstring& driverName);

	BOOL ManualCopyDriverFile(const std::wstring& driverName, const std::wstring& targetPath);

	// 加载Minifilter驱动
	BOOL LoadDriver(const std::wstring& driverName);

	// 卸载Minifilter驱动
	BOOL UnloadDriver(const std::wstring& driverName);

	// 检查驱动是否已加载
	BOOL IsDriverLoaded(const std::wstring& driverName);

	// 完全移除驱动（包括注册表等）
	BOOL RemoveDriver(const std::wstring& driverName);

private:
	// 获取驱动程序路径
	std::wstring GetDriverPath(const std::wstring& infPath);

	// 获取驱动程序Altitude
	std::wstring GetDriverAltitude(const std::wstring& infPath);

	// 创建服务
	BOOL CreateDriverService(const std::wstring& driverName,
		const std::wstring& driverPath);

	// 删除服务
	BOOL DeleteDriverService(const std::wstring& driverName);

	// 执行FltMgr命令
	BOOL ExecuteFltMgrCommand(const std::wstring& command,
		const std::wstring& driverName);
};