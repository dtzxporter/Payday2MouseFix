#ifdef PAYDAYMOUSEFIX_EXPORTS
#define PAYDAYMOUSEFIX_API __declspec(dllexport)
#else
#define PAYDAYMOUSEFIX_API __declspec(dllimport)
#endif

extern "C" PAYDAYMOUSEFIX_API DWORD WINAPI XInputSetState
(
_In_ DWORD             dwUserIndex,  // Index of the gamer associated with the device
_In_ void* pVibration    // The vibration information to send to the controller
);

extern "C" PAYDAYMOUSEFIX_API DWORD WINAPI XInputGetState
(
_In_  DWORD         dwUserIndex,  // Index of the gamer associated with the device
_Out_ void* pState        // Receives the current state
);

// Our custom windows processing pump
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

namespace PaydayMouseFix
{
	void Initialize();
	void Shutdown();
}