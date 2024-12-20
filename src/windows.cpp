#include <Geode/Geode.hpp>
#include <Geode/modify/CCApplication.hpp>
#include <Geode/modify/CCEGLView.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/CCDirector.hpp>

#include <Windows.h>
#include <thread>
#include <atomic>
#include <array>

#include "input.hpp"

// time code by mat. thank you mat
std::int64_t query_performance_counter() {
	LARGE_INTEGER result;
	QueryPerformanceCounter(&result);
	return result.QuadPart;
}

std::int64_t query_performance_frequency() {
	static auto freq = [] {
		LARGE_INTEGER result;
		QueryPerformanceFrequency(&result);
		return result.QuadPart;
	}();
	return freq;
}

std::uint64_t platform_get_time() {
	return query_performance_counter() * 1000 / query_performance_frequency();
}

LRESULT CALLBACK inputWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {
	case WM_INPUT: {
		UINT dwSize = sizeof(RAWINPUT);
		static std::array<BYTE, sizeof(RAWINPUT)> lpb;


		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb.data(), &dwSize, sizeof(RAWINPUTHEADER));

		RAWINPUT* raw = (RAWINPUT*)lpb.data();
		switch (raw->header.dwType) {
		case RIM_TYPEMOUSE: {
			// mouse input recieved
			USHORT mouseFlags = raw->data.mouse.usButtonFlags;
			if ((mouseFlags & RI_MOUSE_LEFT_BUTTON_DOWN) || (mouseFlags & RI_MOUSE_LEFT_BUTTON_UP)) {
				auto timestamp = platform_get_time();

				ExtendedCCTouchDispatcher::setTimestamp(timestamp);
			}
			break;
		}
		case RIM_TYPEKEYBOARD: {
			// keyboard input recieved
			auto timestamp = platform_get_time();

			//std::string dbgMsg = "keyboard input recieved, timestamp " + timestamp;
			//geode::log::debug(dbgMsg);

			ExtendedCCKeyboardDispatcher::setTimestamp(timestamp);

			break;
		}
		default:
			return 0;
		}

		break;
	}
	default:
		return DefWindowProcA(hwnd, uMsg, wParam, lParam);
	}

	return 0;
}

// pretty much all of this code is just taken from syzi's CBF
// i take credit for nothing!!!
void inputWindowThread() {
	WNDCLASS wc = {};
	wc.lpfnWndProc = inputWindowProc;
	wc.hInstance = GetModuleHandleA(NULL);
	wc.lpszClassName = "COS";

	RegisterClass(&wc);
	HWND hwnd = CreateWindow("COS", "Raw Input Window", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, wc.hInstance, 0);
	if (!hwnd) {
		const DWORD err = GetLastError();
		geode::log::error("Unable to create COS input window");
		return;
	}

	std::array<RAWINPUTDEVICE,2> dev;
	dev[0].usUsagePage = 0x01;        // generic desktop controls
	dev[0].usUsage = 0x02;            // mouse
	dev[0].dwFlags = RIDEV_INPUTSINK; // allow inputs without being in the foreground
	dev[0].hwndTarget = hwnd;         // raw input window

	dev[1].usUsagePage = 0x01;
	dev[1].usUsage = 0x06;            // keyboard
	dev[1].dwFlags = RIDEV_INPUTSINK;
	dev[1].hwndTarget = hwnd;

	if (!RegisterRawInputDevices(dev.data(), dev.size(), sizeof(dev[0]))) {
		geode::log::error("Unable to register Raw Input Devices for COS Window!");
		return;
	}

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	MSG msg;
	while (GetMessage(&msg, hwnd, 0, 0)) {
		DispatchMessage(&msg);
	}
}

$execute{
	std::thread(inputWindowThread).detach();
}