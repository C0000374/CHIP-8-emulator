//
// Copyright (C) C0000374
//

#include <CHIP-8.h>

noreturn_t cdecl C8_DisplayAndLogFatalError(
	PCSTR	File,
	PCSTR	Function,
	DWORD	Line,
	PCWSTR	Format,
	...
) {
	va_list	Arguments;
	WCHAR	Buffer[256];
	WCHAR	Buffer2[256];

	va_start(Arguments, Format);
	wvsprintfW(Buffer, Format, Arguments);

	if (File == NULL) wsprintfW(Buffer2, L"Ошибка: %s\r\n", Buffer);
	else wsprintfW(Buffer2, L"[%S,%S,%u]: Ошибка: %s\r\n", File, Function, Line, Buffer);

	MessageBoxW(
		GetFocus(),
		Buffer2,
		L"CHIP-8 emulator",
		MB_OK | MB_ICONERROR
	);

	if (C8_GDIPlusInitialized == TRUE) GdiplusShutdown(C8_GDIPlusToken);
	ExitProcess(1);
}

void stdcall C8_Assert(
	PCSTR	File,
	PCSTR	Function,
	DWORD	Line,
	BOOL	ExpressionResult,
	PCWSTR	ExpressionString
) {
	if (ExpressionResult) return;

	C8_DisplayAndLogFatalError(
		File,
		Function,
		Line,
		L"ASSERT: %s",
		ExpressionString
	);
}

PVOID stdcall C8_AllocateMemory(
	DWORD	Flags,
	SIZE_T	Size
) {
	PVOID	Memory;

	Memory = HeapAlloc(
		C8_hHeap,
		Flags,
		Size
	);
	if (Memory == NULL) FATAL(L"Не удалось выделить 0x%8X байт памяти", Size);

	return Memory;
}