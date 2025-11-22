#pragma once
#include <Windows.h>
#include <string>
#include <node/node.h>
#include "ConnData.h"

#pragma comment(lib, "fltLib.lib")

class Communication {
private:
	HANDLE hPort;
	HANDLE hCompletion;
	v8::Local<v8::Function> m_fileInfoCallback;
	v8::Isolate* m_Isolate;
public:
	Communication() : hPort(INVALID_HANDLE_VALUE), hCompletion(INVALID_HANDLE_VALUE), m_Isolate(NULL) {};
    ~Communication();
    bool Connect();
	void Disconnect();
	void SetFileInfoCallback(v8::Local<v8::Function> callback);
	void SetIsolate(v8::Isolate* isolate);
    void Monitor();
	std::wstring FormatTime(const LARGE_INTEGER& timeStamp);
private:
	void DisplayFileEvent(const PFILEAWARE_MESSAGE message);
	void FireFileEvent(const PFILEAWARE_MESSAGE message);
	
};