#include "ServiceManage.h"
#include <iostream>
#include <windows.h>

bool loadService(const std::wstring& driverPath, const std::wstring& serviceName) {
	SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!scm) {
		std::wcerr << L"打开服务控制管理器失败: " << GetLastError() << std::endl;
		return false;
	}

	// 创建服务
	SC_HANDLE service = CreateService(
		scm,
		serviceName.c_str(),
		serviceName.c_str(),
		SERVICE_ALL_ACCESS,
		SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		driverPath.c_str(),
		NULL, NULL, NULL, NULL, NULL
	);

	if (!service) {
		DWORD error = GetLastError();
		if (error == ERROR_SERVICE_EXISTS) {
			// 服务已存在，打开服务
			service = OpenService(scm, serviceName.c_str(), SERVICE_ALL_ACCESS);
		}
		else {
			std::wcerr << L"创建服务失败: " << error << std::endl;
			CloseServiceHandle(scm);
			return false;
		}
	}

	// 启动服务
	if (!StartService(service, 0, NULL)) {
		DWORD error = GetLastError();
		if (error != ERROR_SERVICE_ALREADY_RUNNING) {
			std::wcerr << L"启动服务失败: " << error << std::endl;
			CloseServiceHandle(service);
			CloseServiceHandle(scm);
			return false;
		}
	}

	std::wcout << L"驱动加载成功" << std::endl;
	CloseServiceHandle(service);
	CloseServiceHandle(scm);
	return true;
}

bool unloadService(const std::wstring& serviceName) {
	SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!scm) {
		std::wcerr << L"打开服务控制管理器失败: " << GetLastError() << std::endl;
		return false;
	}

	SC_HANDLE service = OpenService(scm, serviceName.c_str(), SERVICE_ALL_ACCESS);
	if (!service) {
		std::wcerr << L"打开服务失败: " << GetLastError() << std::endl;
		CloseServiceHandle(scm);
		return false;
	}

	// 停止服务
	SERVICE_STATUS status;
	if (!ControlService(service, SERVICE_CONTROL_STOP, &status)) {
		DWORD error = GetLastError();
		if (error != ERROR_SERVICE_NOT_ACTIVE) {
			std::wcerr << L"停止服务失败: " << error << std::endl;
		}
	}

	// 删除服务
	if (!DeleteService(service)) {
		DWORD error = GetLastError();
		if (error != ERROR_SERVICE_MARKED_FOR_DELETE) {
			std::wcerr << L"删除服务失败: " << error << std::endl;
			CloseServiceHandle(service);
			CloseServiceHandle(scm);
			return false;
		}
	}

	std::wcout << L"驱动卸载成功" << std::endl;
	CloseServiceHandle(service);
	CloseServiceHandle(scm);
	return true;
}