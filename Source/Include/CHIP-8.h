//
// Copyright (C) C0000374
//

#ifndef CHIP_8_H
#define CHIP_8_H
#include <Windows.h>
#include <CGDIP.h>

typedef PWSTR* PPWSTR;

#define stdcall		__stdcall
#undef  cdecl
#define cdecl		__cdecl
#define noreturn_t	__declspec(noreturn) void
#define asm			__asm

//
// Main.c
//

extern GdiplusStartupInput	C8_GDIPlusInput;
extern ULONG_PTR			C8_GDIPlusToken;
extern BOOLEAN				C8_GDIPlusInitialized;
extern WNDCLASSEXW			C8_MainWindowClass;
extern HWND					C8_hMainWindow;
extern HANDLE				C8_hHeap;
extern GpBrush*				C8_WhiteBrush;
extern GpBrush*				C8_BlackBrush;

void		stdcall C8_Main(void);
LRESULT		stdcall C8_MainWindowProcedure(HWND hWindow, UINT MessageID, WPARAM WP, LPARAM LP);

//
// Utils.c
//

noreturn_t	cdecl   C8_DisplayAndLogFatalError(PCSTR File, PCSTR Function, DWORD Line, PCWSTR Format, ...);
void		stdcall C8_Assert(PCSTR File, PCSTR Function, DWORD Line, BOOL ExpressionResult, PCWSTR ExpressionString);
PVOID		stdcall C8_AllocateMemory(DWORD Flags, SIZE_T Size);

#define             FATAL(e, ...) C8_DisplayAndLogFatalError(__FILE__, __func__, __LINE__, e, __VA_ARGS__)
#define             ASSERT(e) C8_Assert(__FILE__, __func__, __LINE__, (BOOL)(e), L## #e)

//
// Emulator.c
//

extern PBYTE				C8_RAM;
extern BYTE					C8_V[16];
extern WORD					C8_I;
extern BYTE					C8_DT;
extern BYTE					C8_ST;
extern WORD					C8_PC;
extern BYTE					C8_SP;
extern WORD					C8_Stack[16];
extern BYTE					C8_KeyboardBuffer[16];
extern BOOLEAN				C8_IsWaitingForKeyboard;
extern PBYTE				C8_VideoMemory;
extern BYTE					C8_DestReg;

void		stdcall C8_InitializeEmulator(PCWSTR ProgramName);
void		stdcall C8_UpdateEmulator(void);
void		stdcall C8_DrawSprite(PBYTE Sprite, BYTE SpriteHeight, BYTE X, BYTE Y);
void		stdcall C8_DrawSpriteLine(BYTE Line, BYTE X, BYTE Y);

//
// RANDOM.ASM
//

BYTE		stdcall Random(void);
void		stdcall Reset(void);

#endif