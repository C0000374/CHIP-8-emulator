//
// Copyright (C) C0000374
//

#include <CHIP-8.h>

PBYTE	C8_RAM;
BYTE	C8_V[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
WORD	C8_I = 0;
BYTE	C8_DT = 0;
BYTE	C8_ST = 0;
WORD	C8_PC = 0x200;
BYTE	C8_SP = 0;
WORD	C8_Stack[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
BYTE	C8_KeyboardBuffer[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
BOOLEAN	C8_IsWaitingForKeyboard = FALSE;
PBYTE	C8_VideoMemory;
BYTE	C8_Symbols[80] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0,
	0x20, 0x60, 0x20, 0x20, 0x70,
	0xF0, 0x10, 0xF0, 0x80, 0xF0,
	0xF0, 0x10, 0xF0, 0x10, 0xF0,
	0x90, 0x90, 0xF0, 0x10, 0x10,
	0xF0, 0x80, 0xF0, 0x10, 0xF0,
	0xF0, 0x80, 0xF0, 0x90, 0xF0,
	0xF0, 0x10, 0x20, 0x40, 0x40,
	0xF0, 0x90, 0xF0, 0x90, 0xF0,
	0xF0, 0x90, 0xF0, 0x10, 0xF0,
	0xF0, 0x90, 0xF0, 0x90, 0x90,
	0xE0, 0x90, 0xE0, 0x90, 0xE0,
	0xF0, 0x80, 0x80, 0x80, 0xF0,
	0xE0, 0x90, 0x90, 0x90, 0xE0,
	0xF0, 0x80, 0xF0, 0x80, 0xF0,
	0xF0, 0x80, 0xF0, 0x80, 0x80
};
BYTE	C8_DestReg;
BOOLEAN	C8_IsHalted = FALSE;

void stdcall C8_InitializeEmulator(PCWSTR ProgramName) {

	HANDLE	hProgramFile;
	DWORD	NumberOfBytesRead;

	C8_RAM = C8_AllocateMemory(0, 4096);

	hProgramFile = CreateFileW(
		ProgramName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);
	if (hProgramFile == INVALID_HANDLE_VALUE) FATAL(L"Не удалось открыть программу %s", ProgramName);

	ReadFile(
		hProgramFile,
		&(C8_RAM[0x200]),
		3584,
		&NumberOfBytesRead,
		NULL
	);

	CloseHandle(hProgramFile);

	asm {
		      PUSH  EDI
		      PUSH  ESI
		      MOV   EDI, C8_RAM
		      MOV   ESI, OFFSET C8_Symbols
		      MOV   ECX, 20
		      CLD
		REP   MOVSD
		      POP   ESI
		      POP   EDI
	}

	C8_VideoMemory = C8_AllocateMemory(HEAP_ZERO_MEMORY, 64 * 32);
}

void stdcall C8_UpdateEmulator(void) {

	WORD	CurrentInstruction;
	WORD	Result;
	BYTE	Vx;
	BYTE	Vy;
	DWORD	i;

	if (C8_IsHalted == TRUE) return;

	if (C8_DT != 0) C8_DT--;
	if (C8_ST != 0) {
		C8_ST--;
		Beep(1000, 10);
	}

	if (C8_IsWaitingForKeyboard == TRUE) {

		for (i = 0; i < 16; i++) if (C8_KeyboardBuffer[i] == 1) {
			C8_V[C8_DestReg] = (BYTE)i;
			C8_IsWaitingForKeyboard = FALSE;
			break;
		}
		return;
	}

	CurrentInstruction = (C8_RAM[C8_PC] << 8) + C8_RAM[C8_PC + 1];
	Vx = (CurrentInstruction >> 8) & 0x0F;
	Vy = (CurrentInstruction >> 4) & 0x0F;
	C8_PC += 2;
	C8_PC &= 0x0FFF;

	switch (CurrentInstruction >> 12) {

	case 0:
		switch (CurrentInstruction) {

		case 0x00E0: // CLS
			asm{
					  PUSH  EDI
					  MOV   EDI, C8_VideoMemory
					  MOV   ECX, 512
					  XOR   EAX, EAX
					  CLD
				REP   STOSD
					  POP   EDI
			}
			break;

		case 0x00EE: // RET
			C8_PC = C8_Stack[C8_SP];
			C8_SP--;
			C8_SP &= 0x0F;
			break;

		default:
			C8_IsHalted = TRUE;
			FATAL(L"Неизвестная инструкция 0x%4X", CurrentInstruction);
		}
		break;

	case 1: // JP addr
		C8_PC = CurrentInstruction & 0x0FFF;
		break;

	case 2: // CALL addr
		C8_SP++;
		C8_SP &= 0x0F;
		C8_Stack[C8_SP] = C8_PC;
		C8_PC = CurrentInstruction & 0x0FFF;
		break;

	case 3: // SE Vx, byte
		if (C8_V[Vx] == (CurrentInstruction & 0x00FF)) {
			C8_PC += 2;
			C8_PC &= 0x0FFF;
		}
		break;

	case 4: // SNE Vx, byte
		if (C8_V[Vx] != (CurrentInstruction & 0x00FF)) {
			C8_PC += 2;
			C8_PC &= 0x0FFF;
		}
		break;

	case 5: // SE Vx, Vy
		if ((CurrentInstruction & 0x000F) != 0) {
			C8_IsHalted = TRUE;
			FATAL(L"Неизвестная инструкция 0x%4X", CurrentInstruction);
		}

		if (C8_V[Vx] == C8_V[Vy]) {
			C8_PC += 2;
			C8_PC &= 0x0FFF;
		}
		break;

	case 6: // LD Vx, byte
		C8_V[Vx] = CurrentInstruction & 0x00FF;
		break;

	case 7: // ADD Vx, byte
		C8_V[Vx] += CurrentInstruction & 0x00FF;
		break;

	case 8:
		switch (CurrentInstruction & 0x000F) {

		case 0: // LD Vx, Vy
			C8_V[Vx] = C8_V[Vy];
			break;

		case 1: // OR Vx, Vy
			C8_V[Vx] |= C8_V[Vy];
			break;

		case 2: // AND Vx, Vy
			C8_V[Vx] &= C8_V[Vy];
			break;

		case 3: // XOR Vx, Vy
			C8_V[Vx] ^= C8_V[Vy];
			break;

		case 4: // ADD Vx, Vy
			Result = C8_V[Vx] + C8_V[Vy];
			C8_V[0xF] = (Result >> 8) & 1;
			C8_V[Vx] = (BYTE)Result;
			break;

		case 5: // SUB Vx, Vy
			if (C8_V[Vx] > C8_V[Vy]) C8_V[0xF] = 1;
			C8_V[Vx] -= C8_V[Vy];
			break;

		case 6: // SHR Vx, Vy
			C8_V[0xF] = C8_V[Vx] & 1;
			C8_V[Vx] >>= 1;
			break;

		case 7: // SUBN Vx, Vy
			if (C8_V[Vx] < C8_V[Vy]) C8_V[0xF] = 1;
			C8_V[Vx] = C8_V[Vy] - C8_V[Vx];
			break;

		case 0xE: // SHL Vx, Vy
			C8_V[0xF] = C8_V[Vx] >> 7;
			C8_V[Vx] <<= 1;
			break;

		default:
			C8_IsHalted = TRUE;
			FATAL(L"Неизвестная инструкция 0x%4X", CurrentInstruction);
		}
		break;

	case 9: // SNE Vx, Vy
		if ((CurrentInstruction & 0x000F) != 0) {
			C8_IsHalted = TRUE;
			FATAL(L"Неизвестная инструкция 0x%4X", CurrentInstruction);
		}

		if (C8_V[Vx] != C8_V[Vy]) {
			C8_PC += 2;
			C8_PC &= 0x0FFF;
		}
		break;

	case 0xA: // LD I, addr
		C8_I = CurrentInstruction & 0x0FFF;
		break;

	case 0xB: // JP V0, addr
		C8_PC = CurrentInstruction & 0x0FFF;
		C8_PC += C8_V[0];
		C8_PC &= 0x0FFF;
		break;

	case 0xC: // RND Vx, byte
		C8_V[Vx] = Random() & (CurrentInstruction & 0xFF);
		break;

	case 0xD: // DRW Vx, Vy, nibble
		C8_DrawSprite(&(C8_RAM[C8_I]), CurrentInstruction & 0x0F, C8_V[Vx], C8_V[Vy]);
		InvalidateRect(C8_hMainWindow, NULL, TRUE);
		break;

	case 0xE:
		switch (CurrentInstruction & 0xFF) {

		case 0x9E: // SKP Vx
			if (C8_KeyboardBuffer[C8_V[Vx] & 0xF] == TRUE) {
				C8_PC += 2;
				C8_PC &= 0x0FFF;
			}
			break;

		case 0xA1: // SKNP Vx
			if (C8_KeyboardBuffer[C8_V[Vx] & 0xF] != TRUE) {
				C8_PC += 2;
				C8_PC &= 0x0FFF;
			}
			break;

		default:
			C8_IsHalted = TRUE;
			FATAL(L"Неизвестная инструкция 0x%4X", CurrentInstruction);
		}

	case 0xF:
		switch (CurrentInstruction & 0xFF) {

		case 7: // LD Vx, DT
			C8_V[Vx] = C8_DT;
			break;

		case 0xA: // LD Vx, K
			C8_IsWaitingForKeyboard = TRUE;
			C8_DestReg = Vx;
			asm {
				      PUSH  EDI
				      MOV   EDI, OFFSET C8_KeyboardBuffer
				      MOV   ECX, 4
				      XOR   EAX, EAX
				      CLD
				REP   STOSD
				      POP   EDI
			}
			break;

		case 0x15: // LD DT, Vx
			C8_DT = C8_V[Vx];
			break;

		case 0x18: // LD ST, Vx
			C8_ST = C8_V[Vx];
			break;

		case 0x1E: // ADD I, Vx
			C8_I += C8_V[Vx];
			C8_I &= 0xFFF;
			break;

		case 0x29: // LD F, Vx
			C8_I = (C8_V[Vx] & 0xF) * 5;
			break;

		case 0x33: // LD B, Vx
			C8_RAM[C8_I] = C8_V[Vx] / 100;
			C8_RAM[(C8_I + 1) & 0xFFF] = (C8_V[Vx] % 100) / 10;
			C8_RAM[(C8_I + 2) & 0xFFF] = C8_V[Vx] % 10;
			break;

		case 0x55: // LD [I], Vx
			for (i = 0; i < Vx; i++) C8_RAM[(C8_I + i) & 0xFFF] = C8_V[i];
			break;

		case 0x65: // LD [I], Vx
			for (i = 0; i < Vx; i++) C8_RAM[(C8_I + i) & 0xFFF] = C8_V[i];
			break;

		default:
			C8_IsHalted = TRUE;
			FATAL(L"Неизвестная инструкция 0x%4X", CurrentInstruction);
		}
	}
}

void stdcall C8_DrawSprite(
	PBYTE	Sprite,
	BYTE	SpriteHeight,
	BYTE	X,
	BYTE	Y
) {
	DWORD	i;
	DWORD	dwX;
	DWORD	dwY;

	if (X > 63 || Y > 31) return;

	dwX = X;
	dwY = Y;

	for (i = dwY; i < dwY + SpriteHeight; i++) C8_DrawSpriteLine(Sprite[i - dwY], (BYTE)dwX, i & 31);
}

void stdcall C8_DrawSpriteLine(
	BYTE	Line,
	BYTE	X,
	BYTE	Y
) {
	BYTE	LineArray[8];
	DWORD	i;
	BYTE	OldPixel;
	DWORD	j;
	DWORD	dwX;
	DWORD	dwY;

	dwX = X;
	dwY = Y;

	for (i = 0; i < 8; i++) LineArray[i] = (Line >> (7 - i)) & 1;

	for (i = dwX; i < dwX + 8; i++) {
		j = i & 63;
		OldPixel = C8_VideoMemory[64 * dwY + j];
		C8_VideoMemory[64 * dwY + j] ^= LineArray[i - dwX];
		if (OldPixel == 1 && C8_VideoMemory[64 * dwY + j] == 0) C8_V[0xF] = 1;
	}
}