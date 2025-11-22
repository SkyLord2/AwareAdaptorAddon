#include "Communication.h"
#include "Utils.hpp"

#include <fltUser.h>
#include <iostream>
#include <iomanip>
#include <sstream>


Communication::~Communication() {
	Disconnect();
}

bool Communication::Connect() {
	HRESULT hr = FilterConnectCommunicationPort(
		FILEAWARE_PORT_NAME,
		0,
		nullptr,
		0,
		nullptr,
		&hPort
	);

	if (FAILED(hr)) {
		std::wcout << L"Failed to connect to filter port: 0x" << std::hex << hr << std::endl;
		return false;
	}

	hCompletion = CreateIoCompletionPort(hPort, nullptr, 0, 0);
	if (hCompletion == nullptr) {
		std::wcout << L"Failed to create completion port: " << GetLastError() << std::endl;
		CloseHandle(hPort);
		hPort = INVALID_HANDLE_VALUE;
		return false;
	}

	std::wcout << L"Connected to file monitor driver successfully" << std::endl;
	return true;
}

void Communication::Disconnect() {
	if (hCompletion != INVALID_HANDLE_VALUE) {
		CloseHandle(hCompletion);
		hCompletion = INVALID_HANDLE_VALUE;
	}

	if (hPort != INVALID_HANDLE_VALUE) {
		CloseHandle(hPort);
		hPort = INVALID_HANDLE_VALUE;
	}
}

void Communication::SetFileInfoCallback(v8::Local<v8::Function> callback) {
	m_fileInfoCallback = callback;
}

void Communication::SetIsolate(v8::Isolate* isolate) {
	m_Isolate = isolate;
}

std::wstring Communication::FormatTime(const LARGE_INTEGER& timeStamp) {
	FILETIME ft;
	SYSTEMTIME st;
	std::wstringstream ss;

	ft.dwLowDateTime = timeStamp.LowPart;
	ft.dwHighDateTime = timeStamp.HighPart;

	FileTimeToSystemTime(&ft, &st);

	ss << std::setw(2) << std::setfill(L'0') << st.wHour << L":"
		<< std::setw(2) << std::setfill(L'0') << st.wMinute << L":"
		<< std::setw(2) << std::setfill(L'0') << st.wSecond << L"."
		<< std::setw(3) << std::setfill(L'0') << st.wMilliseconds;

	return ss.str();
}

void Communication::Monitor() {
	if (hPort == INVALID_HANDLE_VALUE) {
		std::wcout << L"Not connected to driver" << std::endl;
		return;
	}

	std::wcout << L"Starting file monitor..." << std::endl;

	while (true) {
		FILEAWARE_MESSAGE message = { 0 };
		FILEAWARE_REPLY reply = { 0 };
		DWORD bytesReturned = 0;
		OVERLAPPED overlapped = { 0 };
		ULONG_PTR key = 0;
		LPOVERLAPPED lpOverlapped = nullptr;

		HRESULT hr = FilterGetMessage(hPort,
			reinterpret_cast<PFILTER_MESSAGE_HEADER>(&message),
			sizeof(message),
			&overlapped);

		if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
			std::wcout << L"IO pending..." << std::endl;
			DWORD waitResult = WaitForSingleObject(hCompletion, INFINITE);
			if (waitResult == WAIT_OBJECT_0) {
				std::wcout << L"Wait result: WAIT_OBJECT_0" << std::endl;
				BOOL bResult = GetQueuedCompletionStatus(hCompletion,
					&bytesReturned,
					&key,
					&lpOverlapped,
					INFINITE);
				if (bResult) {
					if (bytesReturned >= sizeof(FILEAWARE_MESSAGE)) {
						std::wcout << L"get async message success" << std::endl;
						//DisplayFileEvent(&message);
						FireFileEvent(&message);
					} 
					else
					{
						std::wcout << L"message content is not enough..." << std::endl;
					}
				}
				else
				{
					DWORD error = GetLastError();
					std::wcout << L"get message failed: " << error << std::endl;
					if (error == ERROR_INSUFFICIENT_BUFFER) {
						std::wcout << L"GetQueuedCompletionStatus failed, error code: " << error
							<< L", bytesReturned: " << bytesReturned
							<< L", expected: " << sizeof(FILEAWARE_MESSAGE) << std::endl;
					}
				}
			}
		}
		else if (SUCCEEDED(hr)) {
			std::wcout << L"get message success 1" << std::endl;
			//DisplayFileEvent(&message);
			FireFileEvent(&message);
		}
		else {
			std::wcout << L"FilterGetMessage failed: 0x" << std::hex << hr << std::endl;
			break;
		}

		// 发送回复
		reply.Status = 0x12345678;
		FilterReplyMessage(hPort,
			reinterpret_cast<PFILTER_REPLY_HEADER>(&reply),
			sizeof(reply));
	}
}

void Communication::DisplayFileEvent(const PFILEAWARE_MESSAGE message) {
	/*std::wcout << L"Type: " << message->FileType
		<< L" | File: " << message->FileName
		<< L" | Path: " << message->FilePath
		<< L" | Volume: " << message->Volume
		<< std::endl;*/
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	std::wstringstream path;
	path << message->Volume << L":" << message->FilePath;
	std::wstringstream ss;
	ss << L"Type: " << message->FileType
		<< L" | File: " << message->FileName
		<< L" | Path: " << path.str()
		<< L" | Volume: " << message->Volume
		<< std::endl;

	std::wstring output = ss.str();
	WriteConsoleW(hConsole, output.c_str(), output.length(), NULL, NULL);
}

void Communication::FireFileEvent(const PFILEAWARE_MESSAGE message) {
	if (m_fileInfoCallback.IsEmpty() || m_fileInfoCallback->IsNull() || m_Isolate == NULL) {
		return;
	}

	// 将 WCHAR 转换为 UTF-8
	std::string fileNameUtf8 = WcharToUtf8(message->FileName);
	std::string filePathUtf8 = WcharToUtf8(message->FilePath);
	std::string fileTypeUtf8 = WcharToUtf8(message->FileType);
	std::string volumeUtf8 = WcharToUtf8(message->Volume);

	// 创建4个字符串参数
	v8::Local<v8::Value> argv[4] = {
		v8::String::NewFromUtf8(m_Isolate, fileNameUtf8.c_str()).ToLocalChecked(),
		v8::String::NewFromUtf8(m_Isolate, filePathUtf8.c_str()).ToLocalChecked(),
		v8::String::NewFromUtf8(m_Isolate, fileTypeUtf8.c_str()).ToLocalChecked(),
		v8::String::NewFromUtf8(m_Isolate, volumeUtf8.c_str()).ToLocalChecked()
	};

	// 调用回调函数
	m_fileInfoCallback->Call(m_Isolate->GetCurrentContext(),
		Null(m_Isolate),
		4, argv).ToLocalChecked();
}
