#include <node/node.h>
#include <node/uv.h>
#include <string>
#include <cstring>
#include <thread>
#include "MinifilterInstaller.h"
#include "Communication.h"

MinifilterInstaller installer;
std::wstring driverName = L"FileAware";
Communication monitor;

static void OnExit(void* arg) {
	if (installer.UnloadDriver(driverName)) {
		std::wcout << L"unload success" << std::endl;
	}
	if (installer.RemoveDriver(driverName)) {
		std::wcout << L"remove driver success" << std::endl;
	}

	std::wcout << L"\nMonitoring stopped by process exit" << std::endl;
}

static void DriverInitialize(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();

	// 检查参数是否有效
	if (args.Length() < 1 || !args[0]->IsFunction()) {
		isolate->ThrowException(v8::Exception::TypeError(
			v8::String::NewFromUtf8(isolate, "必须传入一个回调函数").ToLocalChecked()));
		return;
	}

	node::Environment* env = node::GetCurrentEnvironment(isolate->GetCurrentContext());
	if (env)
	{
		node::AtExit(env, OnExit, nullptr);
	}
	else {
		std::wcerr << L"env is null" << std::endl;
	}

	// 获取回调函数
	v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(args[0]);

	std::wstring infPath = L".\\FileAware.inf";

	if (installer.InstallDriverEx(infPath, driverName)) {
		std::wcout << L"driver install success" << std::endl;
	}
	else {
		std::wcerr << L"driver install failed" << std::endl;
	}

	if (installer.LoadDriver(driverName)) {
		std::wcout << L"driver load success" << std::endl;
	}

	if (installer.IsDriverLoaded(driverName)) {
		std::wcout << L"driver is running" << std::endl;
		monitor.SetFileInfoCallback(callback);
		monitor.SetIsolate(isolate);
		if (monitor.Connect()) {
			monitor.Monitor();
		}
	}
	else {
		std::wcerr << L"driver is not running" << std::endl;
	}
}

static void HandleExist(uv_signal_s* handle, int signal) {
	if (installer.UnloadDriver(driverName)) {
		std::wcout << L"unload success" << std::endl;
	}
	if (installer.RemoveDriver(driverName)) {
		std::wcout << L"remove driver success" << std::endl;
	}
	std::wcout << L"\nMonitoring stopped by user" << std::endl;
}

void Initialize(v8::Local<v8::Object> exports) {
	NODE_SET_METHOD(exports, "DriverInitialize", DriverInitialize);

	uv_signal_t* signalHandler = new uv_signal_t;
	uv_signal_init(uv_default_loop(), signalHandler);
	uv_signal_start(signalHandler, HandleExist, SIGTERM);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)