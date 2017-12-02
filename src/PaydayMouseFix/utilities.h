#pragma once

#include <windows.h>

static void PatchMemory(ULONG_PTR Address, PBYTE Data, SIZE_T Size)
{
	DWORD d = 0;
	VirtualProtect((LPVOID)Address, Size, PAGE_EXECUTE_READWRITE, &d);

	for (SIZE_T i = 0; i < Size; i++)
		*(volatile BYTE *)(Address + i) = *Data++;

	VirtualProtect((LPVOID)Address, Size, d, &d);

	FlushInstructionCache(GetCurrentProcess(), (LPVOID)Address, Size);
}

static uint8_t *DetourVTable(uint8_t *Target, uint8_t *Detour, uint32_t TableIndex)
{
	// Each function is stored in an array
	uint8_t *virtualPointer = (Target + (TableIndex * sizeof(uintptr_t)));

	DWORD dwOld = 0;
	if (!VirtualProtect(virtualPointer, sizeof(uintptr_t), PAGE_EXECUTE_READWRITE, &dwOld))
		return nullptr;

	uint8_t *original = (uint8_t *)InterlockedExchange((volatile LONG*)virtualPointer, (LONG)Detour);

	VirtualProtect(virtualPointer, sizeof(uintptr_t), dwOld, &dwOld);
	return original;
}

static LPVOID *FindIAT(HMODULE hModule, LPSTR lpFunctionName)
{
	uintptr_t hm = (uintptr_t)hModule;

	for (PIMAGE_IMPORT_DESCRIPTOR iid = (PIMAGE_IMPORT_DESCRIPTOR)(hm + ((PIMAGE_NT_HEADERS)(hm + ((PIMAGE_DOS_HEADER)hm)->e_lfanew))->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress); iid->Name; iid++)
	{
		LPVOID *p;
		for (SIZE_T i = 0; *(p = i + (LPVOID *)(hm + iid->FirstThunk)); i++)
		{
			LPSTR fn = (LPSTR)(hm + *(i + (SIZE_T *)(hm + iid->OriginalFirstThunk)) + 2);
			if (!((uintptr_t)fn & IMAGE_ORDINAL_FLAG) && !_stricmp(lpFunctionName, fn))
			{
				return p;
			}
		}
	}
	return NULL;
}

static void DetourIAT(HMODULE hModule, LPSTR lpFuncName, LPVOID *lpOldAddress, LPVOID lpNewAddress)
{
	LPVOID *lpAddress = FindIAT(hModule, lpFuncName);
	if (!lpAddress || *lpAddress == lpNewAddress)
	{
#if _DEBUG
		printf("Failed to locate address\n");
#endif
		return;
	}

	DWORD flOldProtect;
	DWORD flNewProtect = PAGE_READWRITE;
	VirtualProtect(lpAddress, sizeof(LPVOID), flNewProtect, &flOldProtect);
	if (lpOldAddress)
	{
		*lpOldAddress = *lpAddress;
	}
#if _DEBUG
	printf("Modified %s import address: 0x%X => 0x%X\n", lpFuncName, *lpAddress, lpNewAddress);
#endif
	*lpAddress = lpNewAddress;
	VirtualProtect(lpAddress, sizeof(LPVOID), flOldProtect, &flNewProtect);
}