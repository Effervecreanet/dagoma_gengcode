#include <Windows.h>
#include <math.h>
#include <stdio.h>

#include "GarbageDefs.h"

#pragma comment(lib, "user32.lib")

DWORD64 total_gcode_lines = GARBAGE_TOTAL_GCODE_LINES;
extern int* funcThreadGenGCODE(LPVOID lpParameter);

enum dataType { ZS, HT, FW };

CHAR Z_START[10];
CHAR heatTemp[10];
CHAR F_WHOLE[10];

struct pBar {
	DWORD xStart;
	DWORD xCurrent;
	DWORD xEnd;
	DWORD y;
	DOUBLE percentStep;
};

void pbar_start(struct pBar* pbar) {
	ZeroMemory(pbar, sizeof(struct pBar));
	pbar->xStart = sizeof(USER_INFO_STRING5) + 2;
	pbar->xEnd = pbar->xStart + 100;
	pbar->xCurrent = pbar->xStart + 1;
	pbar->percentStep = 0;

	return;
}

static void consDrawRect(HANDLE hConScreenBuf, COORD cursPosStart) {
	COORD cursPosEnd;
	DWORD written;

	cursPosStart.X = 1;

	SetConsoleCursorPosition(hConScreenBuf, cursPosStart);
	SetConsoleTextAttribute(
		hConScreenBuf, COMMON_LVB_GRID_LVERTICAL | COMMON_LVB_GRID_HORIZONTAL |
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	WriteConsoleA(hConScreenBuf, " ", 1, &written, NULL);

	cursPosStart.X++;

	SetConsoleTextAttribute(hConScreenBuf, COMMON_LVB_GRID_HORIZONTAL |
		FOREGROUND_RED | FOREGROUND_GREEN |
		FOREGROUND_BLUE);

	for (cursPosEnd.X = cursPosStart.X + 115, cursPosEnd.Y = cursPosStart.Y;
		cursPosStart.X <= cursPosEnd.X; cursPosStart.X++) {
		SetConsoleCursorPosition(hConScreenBuf, cursPosStart);
		WriteConsoleA(hConScreenBuf, " ", 1, &written, NULL);
	}

	SetConsoleTextAttribute(
		hConScreenBuf, COMMON_LVB_GRID_HORIZONTAL | COMMON_LVB_GRID_RVERTICAL |
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	WriteConsoleA(hConScreenBuf, " ", 1, &written, NULL);

	SetConsoleTextAttribute(hConScreenBuf, COMMON_LVB_GRID_RVERTICAL |
		FOREGROUND_RED | FOREGROUND_GREEN |
		FOREGROUND_BLUE);

	for (cursPosEnd.Y = cursPosStart.Y + 12, cursPosStart.Y++;
		cursPosStart.Y < cursPosEnd.Y; cursPosStart.Y++) {
		SetConsoleCursorPosition(hConScreenBuf, cursPosStart);
		WriteConsoleA(hConScreenBuf, " ", 1, &written, NULL);
	}

	SetConsoleCursorPosition(hConScreenBuf, cursPosStart);
	SetConsoleTextAttribute(
		hConScreenBuf, COMMON_LVB_GRID_HORIZONTAL | COMMON_LVB_GRID_RVERTICAL |
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	WriteConsoleA(hConScreenBuf, " ", 1, &written, NULL);

	SetConsoleTextAttribute(hConScreenBuf, COMMON_LVB_GRID_HORIZONTAL |
		FOREGROUND_RED | FOREGROUND_GREEN |
		FOREGROUND_BLUE);

	for (cursPosEnd.X = cursPosStart.X - 117, cursPosEnd.Y = cursPosStart.Y;
		cursPosStart.X >= cursPosEnd.X; cursPosStart.X--) {
		SetConsoleCursorPosition(hConScreenBuf, cursPosStart);
		WriteConsoleA(hConScreenBuf, " ", 1, &written, NULL);
	}

	cursPosStart.X++;
	cursPosStart.Y--;
	SetConsoleCursorPosition(hConScreenBuf, cursPosStart);
	SetConsoleTextAttribute(
		hConScreenBuf, COMMON_LVB_GRID_HORIZONTAL | COMMON_LVB_GRID_LVERTICAL |
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	WriteConsoleA(hConScreenBuf, " ", 1, &written, NULL);

	SetConsoleTextAttribute(hConScreenBuf, COMMON_LVB_GRID_LVERTICAL |
		FOREGROUND_RED | FOREGROUND_GREEN |
		FOREGROUND_BLUE);

	for (; cursPosStart.Y > cursPosEnd.Y - 12; cursPosStart.Y--) {
		SetConsoleCursorPosition(hConScreenBuf, cursPosStart);
		WriteConsoleA(hConScreenBuf, " ", 1, &written, NULL);
	}

	return;
}

static int get_user_input(HANDLE consOut, HANDLE consInp, COORD cursPos,
	CHAR dataInput[10], enum dataType dt) {
	INPUT_RECORD inputRecord[7];
	DWORD i, read, written;
	PCHAR p1;
	USHORT user_info_strlen = 0;

	switch (dt) {
	case ZS:
		user_info_strlen = sizeof(USER_INFO_STRING2);
		break;
	case HT:
		user_info_strlen = sizeof(USER_INFO_STRING10);
		break;
	case FW:
		user_info_strlen = sizeof(USER_INFO_STRING11);
		break;
	default:
		break;
	}

	ZeroMemory(dataInput, 10);
	for (i = 0, cursPos.X = 4 + user_info_strlen;;) {
		ZeroMemory(inputRecord, sizeof(INPUT_RECORD) * 7);
		if (GetNumberOfConsoleInputEvents(consInp, &read) && read > 0 &&
			ReadConsoleInput(consInp, inputRecord, 7, &read)) {
			if (inputRecord[0].EventType == KEY_EVENT &&
				inputRecord[0].Event.KeyEvent.wVirtualKeyCode == VK_RETURN) {
				if ((dataInput[0] == '-' && strchr(&dataInput[1], '-') != NULL) ||
					((p1 = strchr(&dataInput[0], '.')) != NULL &&
						strchr(++p1, '.') != NULL) ||
					((dt == ZS && (atof(dataInput) < -40 || atof(dataInput) > 200))) ||
					((dt == HT && (atof(dataInput) < 170 || atof(dataInput) > 270))) ||
					((dt == FW && (atof(dataInput) < 80 || atof(dataInput) > 600)))) {
					cursPos.X = 4;
					cursPos.Y = 14;
					SetConsoleCursorPosition(consOut, cursPos);
					WriteConsole(consOut, USER_INFO_STRING8,
						sizeof(USER_INFO_STRING8) - 1, &written, NULL);
					Sleep(4000);
					SetConsoleCursorPosition(consOut, cursPos);
					WriteConsole(consOut, "                              ", 30, &written,
						NULL);
					return -1;
				}
				else {
					ReadConsoleInput(consInp, inputRecord, 7, &read);
					break;
				}
			}
			else if (inputRecord[0].EventType == KEY_EVENT &&
				inputRecord[0].Event.KeyEvent.bKeyDown) {
				switch (inputRecord[0].Event.KeyEvent.wVirtualKeyCode) {
				case 0x30:
				case VK_NUMPAD0:
					dataInput[i] = '0';
					break;
				case 0x31:
				case VK_NUMPAD1:
					dataInput[i] = '1';
					break;
				case 0x32:
				case VK_NUMPAD2:
					dataInput[i] = '2';
					break;
				case 0x33:
				case VK_NUMPAD3:
					dataInput[i] = '3';
					break;
				case 0x34:
				case VK_NUMPAD4:
					dataInput[i] = '4';
					break;
				case 0x35:
				case VK_NUMPAD5:
					dataInput[i] = '5';
					break;
				case 0x36:
				case VK_NUMPAD6:
					dataInput[i] = '6';
					break;
				case 0x37:
				case VK_NUMPAD7:
					dataInput[i] = '7';
					break;
				case 0x38:
				case VK_NUMPAD8:
					dataInput[i] = '8';
					break;
				case 0x39:
				case VK_NUMPAD9:
					dataInput[i] = '9';
					break;
				case VK_SUBTRACT:
				case VK_OEM_MINUS:
					if (dt == HT || dt == FW)
						return -1;
					dataInput[i] = '-';
					break;
				case VK_DECIMAL:
				case VK_OEM_COMMA:
				case VK_OEM_PERIOD:
					if (dt == HT || dt == FW)
						return -1;
					dataInput[i] = '.';
					break;
				case VK_DELETE:
				case VK_OEM_RESET:
				case VK_BACK:
				case VK_CANCEL:
				case VK_CLEAR:
				case VK_OEM_CLEAR:
					return -1;
					break;
				default:
					break;
				}

				if (dataInput[i] > 0) {
					cursPos.X++;
					// cursPos.Y = 16;
					SetConsoleCursorPosition(consOut, cursPos);
					WriteConsole(consOut, &dataInput[i++], 1, &written, NULL);
				}
			}
		}
		if (i > 9) {
			cursPos.X = 4 + user_info_strlen;
			// cursPos.Y = 14;
			SetConsoleCursorPosition(consOut, cursPos);
			WriteConsole(consOut, "          ", 10, &written, NULL);
			i = 0;
		}
	}

	return 1;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
	HANDLE hNamedPipe;
	HANDLE hThrdObj3d;
	HANDLE hFile;
	DWORD dwThrdId;
	CHAR chBuf[48];
	DWORD read;
	DWORD written;
	DWORD64 current_gcode_lines;
	HANDLE hConsStdOut;
	COORD cursPos, coordPBar;
	DWORD percentStep = 1;
	struct pBar pbar;
	CONSOLE_CURSOR_INFO consCursorInfo;
	HANDLE hConsoleInput;
	HWND consoleWindow;

	AllocConsole();
	SetConsoleTitleA("Effervecreanet | Sigma");
	SetConsoleCP(28591);
	SetConsoleOutputCP(28591);

	consoleWindow = GetConsoleWindow();
	SetWindowLong(consoleWindow, GWL_STYLE,
		GetWindowLong(consoleWindow, GWL_STYLE) & ~WS_MAXIMIZEBOX &
		~WS_SIZEBOX);

clear:
	system("cls");

	hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);

	hConsStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hConsStdOut == INVALID_HANDLE_VALUE) {
		printf("GetStdHandle error\n");
		return -1;
	}

	consCursorInfo.bVisible = FALSE;
	consCursorInfo.dwSize = 1;

	SetConsoleCursorInfo(hConsStdOut, &consCursorInfo);

	cursPos.X = 35;
	cursPos.Y = 4;
	SetConsoleTextAttribute(hConsStdOut, FOREGROUND_RED | FOREGROUND_GREEN);
	SetConsoleCursorPosition(hConsStdOut, cursPos);
	WriteConsole(hConsStdOut, USER_INFO_STRING1, sizeof(USER_INFO_STRING1) - 1,
		&written, NULL);

	cursPos.X = 2;
	cursPos.Y = 9;
	consDrawRect(hConsStdOut, cursPos);
	cursPos.X = 4;
	cursPos.Y = 11;
	SetConsoleTextAttribute(hConsStdOut,
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	SetConsoleCursorPosition(hConsStdOut, cursPos);
	WriteConsole(hConsStdOut, USER_INFO_STRING3, sizeof(USER_INFO_STRING3) - 1,
		&written, NULL);

	cursPos.X = 4;
	cursPos.Y = 12;
	SetConsoleCursorPosition(hConsStdOut, cursPos);
	WriteConsole(hConsStdOut, USER_INFO_STRING9, sizeof(USER_INFO_STRING9) - 1,
		&written, NULL);

	cursPos.X = 4;
	cursPos.Y = 13;
	SetConsoleCursorPosition(hConsStdOut, cursPos);
	WriteConsole(hConsStdOut, USER_INFO_STRING4, sizeof(USER_INFO_STRING4) - 1,
		&written, NULL);

	cursPos.X = 4;
	cursPos.Y = 14;
	SetConsoleCursorPosition(hConsStdOut, cursPos);
	WriteConsole(hConsStdOut, USER_INFO_STRING5, sizeof(USER_INFO_STRING5) - 1,
		&written, NULL);

	cursPos.X = 4;
	cursPos.Y = 16;
	SetConsoleCursorPosition(hConsStdOut, cursPos);
	WriteConsole(hConsStdOut, USER_INFO_STRING2, sizeof(USER_INFO_STRING2) - 1,
		&written, NULL);

	FlushConsoleInputBuffer(hConsoleInput);
	if (get_user_input(hConsStdOut, hConsoleInput, cursPos, Z_START, ZS) < 0)
		goto clear;

	cursPos.X = 4;
	cursPos.Y = 17;
	SetConsoleCursorPosition(hConsStdOut, cursPos);
	WriteConsole(hConsStdOut, USER_INFO_STRING10, sizeof(USER_INFO_STRING10) - 1,
		&written, NULL);

	FlushConsoleInputBuffer(hConsoleInput);
	if (get_user_input(hConsStdOut, hConsoleInput, cursPos, heatTemp, HT) < 0)
		goto clear;

	cursPos.X = 4;
	cursPos.Y = 18;
	SetConsoleCursorPosition(hConsStdOut, cursPos);
	WriteConsole(hConsStdOut, USER_INFO_STRING11, sizeof(USER_INFO_STRING11) - 1,
		&written, NULL);

	FlushConsoleInputBuffer(hConsoleInput);
	if (get_user_input(hConsStdOut, hConsoleInput, cursPos, F_WHOLE, FW) < 0)
		goto clear;

	hFile = CreateFile("dagoma0.g", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);

	hNamedPipe = CreateNamedPipe(STR_PIPE_NAME, PIPE_ACCESS_DUPLEX, PIPE_WAIT, 2,
		255, 255, 0, NULL);

	if (hNamedPipe == INVALID_HANDLE_VALUE) {
		printf("Cannot create named pipe.\n");
		ExitProcess(0);
	}

	hThrdObj3d = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)funcThreadGenGCODE,
		NULL, 0, &dwThrdId);

	if (hThrdObj3d == INVALID_HANDLE_VALUE) {
		printf("Cannot create thread.\n");
		ExitProcess(0);
	}

	ConnectNamedPipe(hNamedPipe, NULL);

	pbar_start(&pbar);

	FlushConsoleInputBuffer(hConsoleInput);

	ZeroMemory(chBuf, 48);
	for (current_gcode_lines = 0, cursPos.Y = 13,
		cursPos.X = 4 + sizeof(USER_INFO_STRING4) - 1;
		;) {
		memset(chBuf, ' ', 47);
		while (!ReadFile(hNamedPipe, chBuf, 47, &read, NULL))
			;

		if (*chBuf == '-' && strncmp(chBuf, "-EOT-", 5) == 0) {
			coordPBar.X = pbar.xCurrent++;
			coordPBar.Y = 14;

			SetConsoleCursorPosition(hConsStdOut, coordPBar);
			WriteConsole(hConsStdOut, "|", 1, &written, NULL);

			WriteFile(hNamedPipe, "-", 1, &written, NULL);

			break;
		}

		if (++current_gcode_lines >
			((total_gcode_lines / 100) * pbar.percentStep)) {
			CHAR strPercent[7];

			coordPBar.X = pbar.xCurrent;
			coordPBar.Y = 14;

			SetConsoleCursorPosition(hConsStdOut, coordPBar);
			WriteConsole(hConsStdOut, "=", 1, &written, NULL);
			pbar.xCurrent++;
			pbar.percentStep += 1.26;

			ZeroMemory(strPercent, 7);
			sprintf(strPercent, "  %d %%", (int)(pbar.percentStep));
			coordPBar.X = 106;
			coordPBar.Y = 14;

			SetConsoleCursorPosition(hConsStdOut, coordPBar);
			WriteConsole(hConsStdOut, strPercent, strlen(strPercent), &written, NULL);
		}

		WriteFile(hFile, chBuf, read, &written, NULL);
		WriteFile(hNamedPipe, "-", 1, &written, NULL);
		SetConsoleCursorPosition(hConsStdOut, cursPos);
		*(chBuf + --read) = ' ';
		WriteConsole(hConsStdOut, chBuf, 47, &written, NULL);
	}

	cursPos.X = 4;
	cursPos.Y = 15;
	SetConsoleCursorPosition(hConsStdOut, cursPos);
	WriteConsole(hConsStdOut, USER_INFO_STRING7, sizeof(USER_INFO_STRING7) - 1,
		&written, NULL);

	DisconnectNamedPipe(hNamedPipe);

	CloseHandle(hNamedPipe);
	CloseHandle(hFile);

	system("pause > NUL");

	ExitProcess(0);
}
