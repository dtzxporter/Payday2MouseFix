#include "stdafx.h"
#include "dinput8.h"
#include "patternfind.h"
#include "PaydayMouseFix.h"

uintptr_t OriginalWndProc = NULL;

uintptr_t XInGetState = NULL;
uintptr_t XInSetState = NULL;

typedef DWORD(WINAPI *XInSetStateFn)(DWORD dwUserIndex, void* pVibration);
typedef DWORD(WINAPI *XInGetStateFn)(DWORD dwUserIndex, void* pState);
typedef LRESULT(CALLBACK *WndProcFn)(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

bool IsClosingState = false;
bool WindowHasFocus = false;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//
	// Handle window gain / lose focus to prevent shitty alt-tab
	//
	if (uMsg == WM_ACTIVATEAPP || uMsg == WM_ACTIVATE || uMsg == WM_SETFOCUS)
	{
		if ((uMsg == WM_ACTIVATEAPP && wParam == FALSE) || (uMsg == WM_ACTIVATE && wParam == WA_INACTIVE))
		{
			// Handle when the application loses focus
#if _DEBUG
			printf("Lost focus!\n");
#endif

			if (!IsClosingState)
			{
				ProxyIDirectInputDevice8A::ToggleGlobalInput(false);
				while (ShowCursor(TRUE) < 0);
				WindowHasFocus = false;
			}
		}
		else
		{
			DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	}
	else if (uMsg == WM_LBUTTONUP || uMsg == WM_LBUTTONDBLCLK || uMsg == WM_RBUTTONUP || uMsg == WM_RBUTTONDBLCLK)
	{
		// Attempt to re-focus window when focus was lost
		if (!WindowHasFocus && !IsClosingState)
		{
			ProxyIDirectInputDevice8A::ToggleGlobalInput(true);
			while (ShowCursor(FALSE) >= 0);
			WindowHasFocus = true;
		}
	}
	else if (uMsg == WM_NCLBUTTONDOWN)
	{
		// Check for closing
		if (wParam == HTCLOSE) { IsClosingState = true; }
	}

	// Return original logic for now
	return ((WndProcFn)OriginalWndProc)(hwnd, uMsg, wParam, lParam);
}

DWORD WINAPI XInputSetState(DWORD dwUserIndex, void* pVibration)
{
	return ((XInSetStateFn)XInSetState)(dwUserIndex, pVibration);
}

DWORD WINAPI XInputGetState(DWORD dwUserIndex, void* pState)
{
	return ((XInGetStateFn)XInGetState)(dwUserIndex, pState);
}

void PaydayMouseFix::Initialize()
{
#if _DEBUG
	AllocConsole();
	freopen("CON", "w", stdout);
#endif

	// Load the original dxinput
	auto OriginalInput = LoadLibraryA("xinput1_3o.dll");

	// Load original states
	XInGetState = (uintptr_t)GetProcAddress(OriginalInput, "XInputGetState");
	XInSetState = (uintptr_t)GetProcAddress(OriginalInput, "XInputSetState");

	// Search for the wndproc handle
	std::vector<PatternByte> PatternResult;
	patterntransform("57 89 7D AC C7 45 B0", PatternResult);

	// Grab main module
	auto MainModuleHandle = GetModuleHandle(NULL);

	// Find it
	auto BaseCall = patternfind((const unsigned char*)MainModuleHandle, 0x567000, PatternResult);

	if (BaseCall == -1)
	{
		// Alert user
		MessageBoxA(nullptr, "Failed to find required offeset info!", "PaydayMouseFix", MB_ICONWARNING | MB_OK);
		ExitProcess(0xBADC0DE);
	}
	else
	{
		// Skip to the function assignment (WndProc)
		BaseCall += (uintptr_t)MainModuleHandle + 7;

		// Store original
		OriginalWndProc = *(uint32_t*)BaseCall;

		// We must patch the existing call
		uint32_t WindowProcHook = (uint32_t)&WindowProc;

		// Patch existing memory
		PatchMemory((uintptr_t)(BaseCall), (PBYTE)&WindowProcHook, 4);

		// Patch input device
		ProxyIDirectInputDevice8A::PatchDInput();
	}
}

void PaydayMouseFix::Shutdown()
{
#if _DEBUG
	FreeConsole();
#endif
}