//
// Copyright (C) C0000374
//

#include <CHIP-8.h>

GdiplusStartupInput	C8_GDIPlusInput = {
	1,
	NULL,
	0,
	0
};
ULONG_PTR			C8_GDIPlusToken;
BOOLEAN				C8_GDIPlusInitialized = FALSE;
WNDCLASSEXW			C8_MainWindowClass = {
	sizeof(WNDCLASSEXW),
	CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW,
	&C8_MainWindowProcedure,
	0,
	0,
	NULL, //hInstance
	NULL, //hIcon
	NULL, //hCursor
	NULL, //hbrBackground
	NULL, //lpszMenuName
	L"C8_MainWindowClass",
	NULL // hIconSm
};
HWND				C8_hMainWindow;
HANDLE				C8_hHeap;
GpBrush*			C8_WhiteBrush;
GpBrush*			C8_BlackBrush;

void stdcall C8_Main(void) {

	PPWSTR	argv;
	DWORD	argc;
	MSG		Message;

	C8_hHeap = GetProcessHeap();

	argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argc == 1) {

		MessageBoxW(
			NULL,
			L"Использование:\r\nCHIP-8.exe <имя программы>",
			L"CHIP-8 emulator",
			MB_OK
		);
		ExitProcess(1);
	}

	C8_InitializeEmulator(argv[1]);

	if (GdiplusStartup(&C8_GDIPlusToken, &C8_GDIPlusInput, NULL) != Ok) FATAL(L"Не удалось инициализировать GDI+");
	C8_GDIPlusInitialized = TRUE;

	C8_MainWindowClass.hInstance = GetModuleHandleW(NULL);
	C8_MainWindowClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
	C8_MainWindowClass.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
	if (RegisterClassExW(&C8_MainWindowClass) == 0) FATAL(L"Не удалось зарегистрировать класс окна");

	C8_hMainWindow = CreateWindowExW(
		0,
		C8_MainWindowClass.lpszClassName,
		L"CHIP-8 emulator",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		700,
		400,
		NULL,
		NULL,
		C8_MainWindowClass.hInstance,
		NULL
	);
	if (C8_hMainWindow == NULL) FATAL(L"Не удалось создать окно");

	ShowWindow(C8_hMainWindow, SW_SHOWNORMAL);
	UpdateWindow(C8_hMainWindow);

	while (GetMessageW(&Message, NULL, 0, 0) != FALSE) {

		TranslateMessage(&Message);
		DispatchMessageW(&Message);
	}

	GdiplusShutdown(C8_GDIPlusToken);
	ExitProcess(0);
}

LRESULT stdcall C8_MainWindowProcedure(
	HWND	hWindow,
	UINT	MessageID,
	WPARAM	WP,
	LPARAM	LP
) {
	HDC			DC;
	PAINTSTRUCT	Paint;
	GpGraphics*	Graphics;
	DWORD		i;
	DWORD		j;
	GpBrush*	Brush;

	switch (MessageID) {

	case WM_CREATE:
		SetTimer(
			hWindow,
			1,
			1000 / 60, // 60 Гц
			NULL
		);
		GdipCreateSolidFill(0xFF000000, &C8_BlackBrush);
		GdipCreateSolidFill(0xFFFFFFFF, &C8_WhiteBrush);
		break;

	case WM_CLOSE:
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_TIMER:
		C8_UpdateEmulator();
		break;

	case WM_PAINT:
		DC = BeginPaint(hWindow, &Paint);
		GdipCreateFromHDC(DC, &Graphics);

		for (i = 0; i < 64; i++) for (j = 0; j < 32; j++) {

			if (C8_VideoMemory[j * 64 + i] == TRUE) Brush = C8_WhiteBrush;
			else Brush = C8_BlackBrush;

			GdipFillRectangleI(
				Graphics,
				Brush,
				i * 10,
				j * 10,
				10,
				10
			);
		}

		GdipDeleteGraphics(Graphics);
		EndPaint(hWindow, &Paint);
		break;

	case WM_KEYDOWN:
		if (WP >= '0' && WP <= '9') C8_KeyboardBuffer[WP - '0'] = TRUE;
		if (WP >= 'A' && WP <= 'F') C8_KeyboardBuffer[WP - 'A' + 10] = TRUE;
		break;

	case WM_KEYUP:
		if (WP >= '0' && WP <= '9') C8_KeyboardBuffer[WP - '0'] = FALSE;
		if (WP >= 'A' && WP <= 'F') C8_KeyboardBuffer[WP - 'A' + 10] = FALSE;
		break;

	default:
		return DefWindowProcW(hWindow, MessageID, WP, LP);
	}

	return 0;
}