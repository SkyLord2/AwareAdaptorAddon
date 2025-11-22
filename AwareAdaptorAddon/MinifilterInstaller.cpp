// MinifilterInstaller.cpp
#include "MinifilterInstaller.h"
#include <devguid.h>
#include <strsafe.h>
#include <codecvt>

MinifilterInstaller::MinifilterInstaller() {
}

MinifilterInstaller::~MinifilterInstaller() {
}

BOOL MinifilterInstaller::InstallDriver(const std::wstring& infPath, const std::wstring& driverName) {
	// 检查INF文件是否存在
	if (GetFileAttributes(infPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
		std::wcerr << L"INF file does not exist: " << infPath << std::endl;
		return FALSE;
	}
	std::wcout << L"INF file exists: " << infPath << std::endl;
	// 获取驱动程序完整路径
	std::wstring driverPath = GetDriverPath(infPath);
	if (driverPath.empty()) {
		std::wcerr << L"can not get driver path from inf file" << std::endl;
		return FALSE;
	}
	std::wcout << L"full driver path is: " << driverPath << std::endl;
	// 使用SetupAPI安装驱动
	HINF hInf = SetupOpenInfFile(infPath.c_str(), NULL, INF_STYLE_WIN4, NULL);
	if (hInf == INVALID_HANDLE_VALUE) {
		std::wcerr << L"open inf file failed: " << GetLastError() << std::endl;
		return FALSE;
	}

	BOOL success = FALSE;
	HSPFILEQ fileQueue = SetupOpenFileQueue();

	if (fileQueue) {
		// 复制驱动文件
		if (SetupInstallFilesFromInfSection(hInf, NULL, fileQueue,
			L"DefaultInstall",
			NULL, SP_COPY_NEWER)) {

			if (SetupCommitFileQueue(NULL, fileQueue, NULL, NULL)) {
				// 创建驱动服务
				if (CreateDriverService(driverName, driverPath)) {
					std::wcout << L"driver create success: " << driverName << std::endl;
					success = TRUE;
				}
				else {
                    std::wcerr << L"driver create failed: " << GetLastError() << std::endl;
				}
			}
			else {
				std::wcerr << L"commit file queue failed: " << GetLastError() << std::endl;
			}
		}
		else {
			std::wcerr << L"install failed from inf section: " << GetLastError() << std::endl;
		}

		SetupCloseFileQueue(fileQueue);
	}

	SetupCloseInfFile(hInf);
	return success;
}

BOOL MinifilterInstaller::InstallDriverEx(const std::wstring& infPath, const std::wstring& driverName) {
	// 检查INF文件是否存在
	if (GetFileAttributes(infPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
		std::wcerr << L"INF file does not exist: " << infPath << std::endl;
		return FALSE;
	}
	std::wcout << L"INF file exists: " << infPath << std::endl;
	// 获取驱动程序完整路径
	std::wstring driverPath = GetDriverPath(infPath);
	if (driverPath.empty()) {
		std::wcerr << L"can not get driver path from inf file" << std::endl;
		return FALSE;
	}
	std::wcout << L"full driver path is: " << driverPath << std::endl;

	// 使用SetupAPI安装驱动
	HINF hInf = SetupOpenInfFile(infPath.c_str(), NULL, INF_STYLE_WIN4, NULL);
	if (hInf == INVALID_HANDLE_VALUE) {
		std::wcerr << L"open inf file failed: " << GetLastError() << std::endl;
		return FALSE;
	}

	HSPFILEQ fileQueue = SetupOpenFileQueue();

	if (fileQueue) {
		// 复制驱动文件
		if (SetupInstallFilesFromInfSection(hInf, NULL, fileQueue,
			L"MiniFilter.DriverFiles",
			NULL, SP_COPY_NEWER)) {
			if (SetupCommitFileQueue(NULL, fileQueue, NULL, NULL)) {
				if (GetFileAttributes(driverPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
					std::wcerr << L"INF file does not exist: " << driverPath << L", copy it manually" << std::endl;
					BOOL Copied = ManualCopyDriverFile(driverName, driverPath);
					if (!Copied)
					{
						return FALSE;
					}
				}
				// 创建驱动服务
				if (CreateDriverService(driverName, driverPath)) {
					std::wcout << L"driver create success: " << driverName << std::endl;
				}
				else {
					std::wcerr << L"driver create failed: " << GetLastError() << std::endl;
					return FALSE;
				}
				HKEY hKey;
				DWORD dwData;
				std::wstring regPath = L"SYSTEM\\ControlSet001\\Services\\" + driverName + L"\\Instances";
				if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, NULL, TRUE, KEY_ALL_ACCESS, NULL, &hKey, (LPDWORD)&dwData) != ERROR_SUCCESS)
				{
					std::wcerr << L"create register key failed: " << regPath << std::endl;
					return FALSE;
				}

				std::wstring instanceName = driverName + L" Instance";
				DWORD dataSize = (DWORD)((instanceName.length() + 1) * sizeof(wchar_t));
				std::wcout << L"set register DefaultInstance value: " << instanceName << std::endl;
				if (RegSetValueEx(hKey, L"DefaultInstance", 0, REG_SZ, (CONST BYTE*)instanceName.c_str(), dataSize) != ERROR_SUCCESS)
				{
					std::wcerr << L"set register DefaultInstance value failed: " << instanceName << std::endl;
					return FALSE;
				}
				RegFlushKey(hKey);
				RegCloseKey(hKey);

				std::wstring fullPath = regPath + L"\\" + instanceName;
				if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, fullPath.c_str(), 0, NULL, TRUE, KEY_ALL_ACCESS, NULL, &hKey, (LPDWORD)&dwData) != ERROR_SUCCESS)
				{
					std::wcerr << L"create register key failed: " << fullPath << std::endl;
					return FALSE;
				}
				std::wstring altitude = GetDriverAltitude(infPath);
				dataSize = (DWORD)((altitude.length() + 1) * sizeof(wchar_t));
				std::wcout << "set register Altitude value: " << altitude << std::endl;
				if (RegSetValueEx(hKey, L"Altitude", 0, REG_SZ, (CONST BYTE*)altitude.c_str(), dataSize) != ERROR_SUCCESS)
				{
					std::wcerr << L"set register altitude value failed: " << altitude << std::endl;
					return FALSE;
				}
				// 注册表驱动程序的Flags 值
				dwData = 0x0;
				if (RegSetValueEx(hKey, L"Flags", 0, REG_DWORD, (CONST BYTE*) & dwData, sizeof(DWORD)) != ERROR_SUCCESS)
				{
					std::wcerr << L"set register Flags value failed: " << dwData << std::endl;
					return FALSE;
				}
				RegFlushKey(hKey);
				RegCloseKey(hKey);
			}
			else {
				std::wcerr << L"commit file queue failed: " << GetLastError() << std::endl;
			}
		}
		else {
			std::wcerr << L"install failed from inf section: " << GetLastError() << std::endl;
		}

		SetupCloseFileQueue(fileQueue);
	}

	SetupCloseInfFile(hInf);

	return TRUE;
}

BOOL MinifilterInstaller::InstallDriverByShell(const std::wstring& infPath, const std::wstring& driverName) {
	SHELLEXECUTEINFO sei = { 0 };
	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.lpVerb = L"install";
	sei.lpFile = infPath.c_str();
	sei.nShow = SW_HIDE;
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;

	if (ShellExecuteEx(&sei)) {
		// 等待安装完成
		if (sei.hProcess) {
			WaitForSingleObject(sei.hProcess, INFINITE);
			CloseHandle(sei.hProcess);
		}

		// 验证安装是否成功
		std::wstring driverPath = GetDriverPath(infPath);
		if (!driverPath.empty() && CreateDriverService(driverName, driverPath)) {
			return TRUE;
		}
	}
	else {
		std::wcerr << L"ShellExecuteEx failed: " << GetLastError() << std::endl;
	}

	return FALSE;
}

BOOL MinifilterInstaller::ManualCopyDriverFile(const std::wstring& driverName, const std::wstring& targetPath) {
	// 默认同一个文件夹
	std::wstring sourcePath = L".\\" + driverName + L".sys";

	// 检查源文件是否存在
	if (GetFileAttributes(sourcePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
		std::wcerr << L"Source driver file not found: " << sourcePath << std::endl;
		return FALSE;
	}

	// 构建目标路径
	//WCHAR sysDir[MAX_PATH];
	//GetSystemDirectory(sysDir, MAX_PATH);
	//std::wstring targetPath = std::wstring(sysDir) + L"\\drivers\\" + driverName + L".sys";

	// 复制文件
	if (CopyFile(sourcePath.c_str(), targetPath.c_str(), FALSE)) {
		std::wcout << L"Manually copied driver file to " << targetPath << L" success" << std::endl;
		return TRUE;
	}
	else {
		DWORD error = GetLastError();
		std::wcerr << L"Manual copy failed: " << error << std::endl;

		// 如果文件已存在且正在使用，先尝试停止服务再复制
		if (error == ERROR_SHARING_VIOLATION || error == ERROR_ACCESS_DENIED) {
			std::wcout << L"File is in use, attempting to unload driver first..." << std::endl;
			UnloadDriver(driverName);
			Sleep(2000); // 等待驱动卸载

			if (CopyFile(sourcePath.c_str(), targetPath.c_str(), FALSE)) {
				std::wcout << L"Copy successful after unloading driver" << std::endl;
				return TRUE;
			}
		}

		return FALSE;
	}
}

BOOL MinifilterInstaller::LoadDriver(const std::wstring& driverName) {
	return ExecuteFltMgrCommand(L"load", driverName);
}

BOOL MinifilterInstaller::UnloadDriver(const std::wstring& driverName) {
	return ExecuteFltMgrCommand(L"unload", driverName);
}

BOOL MinifilterInstaller::IsDriverLoaded(const std::wstring& driverName) {
	SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!scManager) {
		std::wcerr << L"open sc manager failed: " << GetLastError() << std::endl;
		return FALSE;
	}

	SC_HANDLE service = OpenService(scManager, driverName.c_str(), SERVICE_QUERY_STATUS);
	if (!service) {
		CloseServiceHandle(scManager);
		std::wcerr << L"open sc service failed: " << GetLastError() << std::endl;
		return FALSE;
	}

	SERVICE_STATUS status;
	BOOL isRunning = FALSE;

	if (QueryServiceStatus(service, &status)) {
		isRunning = (status.dwCurrentState == SERVICE_RUNNING);
	}

	std::wcout << L"driver status is: " << isRunning << std::endl;

	CloseServiceHandle(service);
	CloseServiceHandle(scManager);

	return isRunning;
}

BOOL MinifilterInstaller::RemoveDriver(const std::wstring& driverName) {
	// 先尝试卸载驱动
	UnloadDriver(driverName);

	// 删除服务
	if (!DeleteDriverService(driverName)) {
		return FALSE;
	}

	// 清理注册表
	HKEY hKey;
	std::wstring regPath = L"SYSTEM\\ControlSet001\\Services\\" + driverName;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
		RegDeleteKey(hKey, L"");
		RegCloseKey(hKey);
	}

	std::wcout << L"driver remove success: " << driverName << std::endl;
	return TRUE;
}

std::wstring MinifilterInstaller::GetDriverPath(const std::wstring& infPath) {
	HINF hInf = SetupOpenInfFile(infPath.c_str(), NULL, INF_STYLE_WIN4, NULL);
	if (hInf == INVALID_HANDLE_VALUE) {
		std::wcout << L"open inf file failed..." << std::endl;
		return L"";
	}

	std::wstring driverPath;
	INFCONTEXT context;

	if (SetupFindFirstLine(hInf, L"MiniFilter.DriverFiles", NULL, &context)) {
		do {
			WCHAR fileName[MAX_PATH];
			if (SetupGetStringField(&context, 0, fileName, MAX_PATH, NULL)) {
				// 构建完整路径（简化处理，实际应根据INF结构处理）
				WCHAR sysDir[MAX_PATH];
				GetSystemDirectory(sysDir, MAX_PATH);

				driverPath = std::wstring(sysDir) + L"\\drivers\\" + fileName;
				break;
			}
		} while (SetupFindNextLine(&context, &context));
	}

	SetupCloseInfFile(hInf);
	return driverPath;
}

std::wstring MinifilterInstaller::GetDriverAltitude(const std::wstring& infPath) {
	HINF hInf = SetupOpenInfFile(infPath.c_str(), NULL, INF_STYLE_WIN4, NULL);
	if (hInf == INVALID_HANDLE_VALUE) {
		std::wcout << L"open inf file failed..." << std::endl;
		return L"370030";
	}

	std::wstring altitude;
	INFCONTEXT context;

	if (SetupFindFirstLine(hInf, L"Instance1.Altitude", NULL, &context)) {
		do {
			WCHAR wAltitude[MAX_PATH];
			if (SetupGetStringField(&context, 0, wAltitude, MAX_PATH, NULL)) {
				altitude = wAltitude;
				break;
			}
		} while (SetupFindNextLine(&context, &context));
	}

	SetupCloseInfFile(hInf);
	return altitude.empty() ? L"370030" : altitude;
}

BOOL MinifilterInstaller::CreateDriverService(const std::wstring& driverName,
	const std::wstring& driverPath) {

	SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!scManager) {
		std::wcerr << L"open sc manager failed: " << GetLastError() << std::endl;
		return FALSE;
	}

	SC_HANDLE service = CreateService(
		scManager,
		driverName.c_str(),
		driverName.c_str(),
		SERVICE_ALL_ACCESS,
		SERVICE_FILE_SYSTEM_DRIVER,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		driverPath.c_str(),
		L"FSFilter Activity Monitor",
		NULL,
		L"FltMgr",
		NULL,
		NULL
	);

	if (!service) {
		DWORD error = GetLastError();
		if (error == ERROR_SERVICE_EXISTS) {
			std::wcout << L"driver already exists: " << driverName << std::endl;
			CloseServiceHandle(scManager);
			return TRUE;
		}

		std::wcerr << L"driver create failed: " << error << std::endl;
		CloseServiceHandle(scManager);
		return FALSE;
	}

	CloseServiceHandle(service);
	CloseServiceHandle(scManager);

	std::wcout << L"driver create success: " << driverPath << std::endl;
	return TRUE;
}

BOOL MinifilterInstaller::DeleteDriverService(const std::wstring& driverName) {
	SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!scManager) {
		return FALSE;
	}

	SC_HANDLE service = OpenService(scManager, driverName.c_str(), DELETE);
	if (!service) {
		CloseServiceHandle(scManager);
		return FALSE;
	}

	BOOL success = DeleteService(service);
	if (!success) {
		DWORD error = GetLastError();
		if (error != ERROR_SERVICE_MARKED_FOR_DELETE) {
			std::wcerr << L"delete driver failed: " << error << std::endl;
		}
	}

	CloseServiceHandle(service);
	CloseServiceHandle(scManager);

	return success;
}

BOOL MinifilterInstaller::ExecuteFltMgrCommand(const std::wstring& command,
	const std::wstring& driverName) {

	SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!scManager) {
		std::wcerr << L"open sc manager failed" << std::endl;
		return FALSE;
	}

	SC_HANDLE service = OpenService(scManager, driverName.c_str(),
		SERVICE_START | SERVICE_STOP);
	if (!service) {
		std::wcerr << L"open sc service failed: " << driverName << std::endl;
		CloseServiceHandle(scManager);
		return FALSE;
	}

	BOOL success = FALSE;

	if (command == L"load") {
		if (StartService(service, 0, NULL)) {
			std::wcout << L"driver load success: " << driverName << std::endl;
			success = TRUE;
		}
		else {
			DWORD error = GetLastError();
			if (error == ERROR_SERVICE_ALREADY_RUNNING) {
				std::wcout << L"driver already running: " << driverName << std::endl;
				success = TRUE;
			}
			else {
				std::wcerr << L"start driver failed: " << error << std::endl;
			}
		}
	}
	else if (command == L"unload") {
		SERVICE_STATUS status;
		if (ControlService(service, SERVICE_CONTROL_STOP, &status)) {
			std::wcout << L"unload driver success: " << driverName << std::endl;
			success = TRUE;
		}
		else {
			DWORD error = GetLastError();
			if (error == ERROR_SERVICE_NOT_ACTIVE) {
				std::wcout << L"driver is not running: " << error << std::endl;
				success = TRUE;
			}
			else {
				std::wcerr << L"stop driver failed: " << error << std::endl;
			}
		}
	}

	CloseServiceHandle(service);
	CloseServiceHandle(scManager);

	return success;
}