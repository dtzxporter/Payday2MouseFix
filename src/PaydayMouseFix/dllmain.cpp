#include "stdafx.h"
#include "PaydayMouseFix.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		PaydayMouseFix::Initialize();
		break;
	case DLL_PROCESS_DETACH:
		PaydayMouseFix::Shutdown();
		break;
	}
	return TRUE;
}

