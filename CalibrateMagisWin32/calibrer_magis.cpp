#include <windows.h>
#include <stdio.h>


#pragma comment (lib, "user32.lib")


#define APP_CONSOLE_TITLE           "Effervecreanet | Magis"

// #define LANG_EN

#ifdef LANG_FR

#define APP_MSG_TITLE               "Effervecreanet - Calibrer Magis Dagoma"
#define APP_MSG_CALIBRATE_POINT     "Point de calibrage:        "
#define APP_MSG_ARROW_UP            "Flèche haut:   monter"
#define APP_MSG_ARROW_DOWN          "Flèche bas:    descendre"
#define APP_MSG_BED	            "_    (plateau lit ou support d'impression magis)"
#define APP_MSG_ERR_1               "Erreur:    port comm serial introuvable."

#else

#define APP_MSG_TITLE               "Effervecreanet - Calibrate Magis Dagoma"
#define APP_MSG_CALIBRATE_POINT     "Calibrating point:        "
#define APP_MSG_ARROW_UP            "Up arrow:      up"
#define APP_MSG_ARROW_DOWN          "Down arrow:    down"
#define APP_MSG_BED	            "_    (magis bed)"
#define APP_MSG_ERR_1               "Error: unable to find serial comm"

#endif

#define PATH_COMM     "\\\\.\\COM"
#define MAX_COMM_PORT 21

#define STEP_Z  0.4

static void
consDrawRect(HANDLE hConScreenBuf, COORD cursPosStart) {
	COORD cursPosEnd;
	DWORD written;

	cursPosStart.X = 1;

	SetConsoleCursorPosition(hConScreenBuf, cursPosStart);
	SetConsoleTextAttribute(hConScreenBuf, COMMON_LVB_GRID_LVERTICAL | COMMON_LVB_GRID_HORIZONTAL | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	WriteConsoleA(hConScreenBuf, " ", 1, &written, NULL);

	cursPosStart.X++;

	SetConsoleTextAttribute(hConScreenBuf, COMMON_LVB_GRID_HORIZONTAL | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	for (cursPosEnd.X = cursPosStart.X + 115, cursPosEnd.Y = cursPosStart.Y;
		cursPosStart.X <= cursPosEnd.X;
		cursPosStart.X++) {
		SetConsoleCursorPosition(hConScreenBuf, cursPosStart);
		WriteConsoleA(hConScreenBuf, " ", 1, &written, NULL);
	}

	SetConsoleTextAttribute(hConScreenBuf, COMMON_LVB_GRID_HORIZONTAL | COMMON_LVB_GRID_RVERTICAL | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	WriteConsoleA(hConScreenBuf, " ", 1, &written, NULL);

	SetConsoleTextAttribute(hConScreenBuf, COMMON_LVB_GRID_RVERTICAL | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	for (cursPosEnd.Y = cursPosStart.Y + 19, cursPosStart.Y++;
		cursPosStart.Y < cursPosEnd.Y;
		cursPosStart.Y++) {
		SetConsoleCursorPosition(hConScreenBuf, cursPosStart);
		WriteConsoleA(hConScreenBuf, " ", 1, &written, NULL);
	}

	SetConsoleCursorPosition(hConScreenBuf, cursPosStart);
	SetConsoleTextAttribute(hConScreenBuf, COMMON_LVB_GRID_HORIZONTAL | COMMON_LVB_GRID_RVERTICAL | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	WriteConsoleA(hConScreenBuf, " ", 1, &written, NULL);

	SetConsoleTextAttribute(hConScreenBuf, COMMON_LVB_GRID_HORIZONTAL | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	for (cursPosEnd.X = cursPosStart.X - 117, cursPosEnd.Y = cursPosStart.Y;
		cursPosStart.X >= cursPosEnd.X;
		cursPosStart.X--) {
		SetConsoleCursorPosition(hConScreenBuf, cursPosStart);
		WriteConsoleA(hConScreenBuf, " ", 1, &written, NULL);
	}

	cursPosStart.X++;
	cursPosStart.Y--;
	SetConsoleCursorPosition(hConScreenBuf, cursPosStart);
	SetConsoleTextAttribute(hConScreenBuf, COMMON_LVB_GRID_HORIZONTAL | COMMON_LVB_GRID_LVERTICAL | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	WriteConsoleA(hConScreenBuf, " ", 1, &written, NULL);

	SetConsoleTextAttribute(hConScreenBuf, COMMON_LVB_GRID_LVERTICAL | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	for (;
		cursPosStart.Y > cursPosEnd.Y - 19;
		cursPosStart.Y--) {
		SetConsoleCursorPosition(hConScreenBuf, cursPosStart);
		WriteConsoleA(hConScreenBuf, " ", 1, &written, NULL);
	}

	return;
}

int
CalOpenSerial(HANDLE* hSerial, CHAR comm_port[20])
{
	DCB dcb;
	COMMTIMEOUTS commTimeouts;

	*hSerial = CreateFile(comm_port,
		GENERIC_READ | GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		0,
		NULL);

	if (*hSerial == INVALID_HANDLE_VALUE) {
		/* printf("Impossible d'ouvrir %s.\n", comm_port); */
		return -1;
	}

	memset(&dcb, 0, sizeof(dcb));
	dcb.DCBlength = sizeof(dcb);
	if (!BuildCommDCB("250000,n,8,1", &dcb)) {
		/* printf("Can't build DCB serial communication.\n"); */
		return -1;
	}

	SetCommState(*hSerial, &dcb);

	ZeroMemory(&commTimeouts, sizeof(COMMTIMEOUTS));
	commTimeouts.ReadIntervalTimeout = 1;
	commTimeouts.ReadTotalTimeoutConstant = 1;
	commTimeouts.ReadTotalTimeoutMultiplier = 1;
	SetCommTimeouts(*hSerial, &commTimeouts);

	return 1;
}

int
CalOpenSerialLoop(HANDLE* hSerial) {
	unsigned short count_comm_port;
	CHAR comm_port[20];

	for (count_comm_port = 0; count_comm_port < MAX_COMM_PORT; count_comm_port++) {
		memset(comm_port, 0, 20);
		sprintf(comm_port, "%s%hu", PATH_COMM, count_comm_port);
		if (CalOpenSerial(hSerial, comm_port) > 0) {
			/* printf("comm_port (%s) found.\n", comm_port); */
			break;
		}
	}

	return (count_comm_port == MAX_COMM_PORT) ? -1 : 1;
}

void
CalFlushFirmware(HANDLE hSerial)
{
	DWORD read;
	CHAR c;
	DWORD err;

	Sleep(2222);

	do {
		err = ReadFile(hSerial, &c, 1, &read, NULL);
	} while (read == 1 && !err);

	return;
}
int
main(int argc, char** argv)
{
	HANDLE hSerial;
	HANDLE hConsOut;
	HANDLE hConsInp;
	COORD coordCursor, coordNozzle;
	DWORD written, read;
	INPUT_RECORD inputRecord[7];
	float Z = 200.000;
	CHAR marlinBuffer[255], * p1;
	HWND consoleWindow;

	system("chcp 28591");
	system("cls");
	consoleWindow = GetConsoleWindow();
	SetWindowLong(consoleWindow, GWL_STYLE, GetWindowLong(consoleWindow, GWL_STYLE) & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX);
	SetConsoleTitleA(APP_CONSOLE_TITLE);

	hConsOut = GetStdHandle(STD_OUTPUT_HANDLE);
	hConsInp = GetStdHandle(STD_INPUT_HANDLE);

	coordCursor.X = 2;
	coordCursor.Y = 5;
	consDrawRect(hConsOut, coordCursor);
	coordCursor.X = 37;
	coordCursor.Y = 3;

	SetConsoleTextAttribute(hConsOut, FOREGROUND_RED | FOREGROUND_GREEN);
	SetConsoleCursorPosition(hConsOut, coordCursor);
	WriteConsole(hConsOut, APP_MSG_TITLE, sizeof(APP_MSG_TITLE) - 1, &written, NULL);

	SetConsoleTextAttribute(hConsOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	coordCursor.X = 3;
	coordCursor.Y = 6;
	SetConsoleCursorPosition(hConsOut, coordCursor);
	WriteConsole(hConsOut, APP_MSG_CALIBRATE_POINT, sizeof(APP_MSG_CALIBRATE_POINT) - 1, &written, NULL);

	coordCursor.X = 3;
	coordCursor.Y = 8;
	SetConsoleCursorPosition(hConsOut, coordCursor);
	WriteConsole(hConsOut, APP_MSG_ARROW_UP, sizeof(APP_MSG_ARROW_UP) - 1, &written, NULL);

	coordCursor.X = 3;
	coordCursor.Y = 9;
	SetConsoleCursorPosition(hConsOut, coordCursor);
	WriteConsole(hConsOut, APP_MSG_ARROW_DOWN, sizeof(APP_MSG_ARROW_DOWN) - 1, &written, NULL);

	coordCursor.X = 60;
	coordCursor.Y = 7;
	SetConsoleCursorPosition(hConsOut, coordCursor);
	WriteConsole(hConsOut, "_______", 7, &written, NULL);

	if (CalOpenSerialLoop(&hSerial) < 0) {
		coordCursor.X = 3;
		coordCursor.Y = 10;
		SetConsoleCursorPosition(hConsOut, coordCursor);
		WriteConsole(hConsOut, APP_MSG_ERR_1, sizeof(APP_MSG_ERR_1) - 1, &written, NULL);
		system("pause > NUL");
		goto app_end;
	}

	CalFlushFirmware(hSerial);

	coordNozzle.X = 63;
	coordNozzle.Y = 7 + 12 + 1;
	SetConsoleCursorPosition(hConsOut, coordNozzle);
	WriteConsole(hConsOut, APP_MSG_BED, sizeof(APP_MSG_BED) - 1, &written, NULL);

	coordNozzle.X = 63;
	coordNozzle.Y = 7;

	for (;;) {
		ZeroMemory(inputRecord, sizeof(INPUT_RECORD) * 7);
		if (GetNumberOfConsoleInputEvents(hConsInp, &read) &&
			read > 1 &&
			ReadConsoleInput(hConsInp, inputRecord, 1, &read)) {
			if (inputRecord[0].EventType == KEY_EVENT) {
				if (inputRecord[0].Event.KeyEvent.wVirtualKeyCode == VK_SPACE) {
					break;
				}
				else if (inputRecord[0].Event.KeyEvent.wVirtualKeyCode == VK_UP) {
					Z += STEP_Z;
				}
				else if (inputRecord[0].Event.KeyEvent.wVirtualKeyCode == VK_DOWN) {
					Z -= STEP_Z;
				}
				else {
					continue;
				}

				ZeroMemory(marlinBuffer, 255);
				sprintf(marlinBuffer, "G1 F600 Z%.3f\n", Z);
				WriteFile(hSerial, marlinBuffer, strlen(marlinBuffer), &written, NULL);

				ZeroMemory(marlinBuffer, 255);
				p1 = marlinBuffer;
				do {
					ReadFile(hSerial, p1, 1, &read, NULL);
					if (*p1++ == '\r')
						break;
				} while (read == 1 && strlen(marlinBuffer) < 254);


				coordCursor.X = 3 + sizeof(APP_MSG_CALIBRATE_POINT);
				coordCursor.Y = 6;
				SetConsoleCursorPosition(hConsOut, coordCursor);
				WriteConsole(hConsOut, "          ", 10, &written, NULL);

				coordCursor.X = 3 + sizeof(APP_MSG_CALIBRATE_POINT);
				coordCursor.Y = 6;
				SetConsoleCursorPosition(hConsOut, coordCursor);

				ZeroMemory(marlinBuffer, 255);
				sprintf(marlinBuffer, "%.3f", Z);
				SetConsoleTextAttribute(hConsOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
				WriteConsole(hConsOut, marlinBuffer, strlen(marlinBuffer), &written, NULL);
				SetConsoleTextAttribute(hConsOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

				if (coordNozzle.Y >= 7 + 12)
					coordNozzle.Y = 8;
				else
					coordNozzle.Y++;

				SetConsoleCursorPosition(hConsOut, coordNozzle);
				WriteConsole(hConsOut, "|", 1, &written, NULL);
				coordNozzle.Y++;
				SetConsoleCursorPosition(hConsOut, coordNozzle);
				WriteConsole(hConsOut, "V", 1, &written, NULL);

			}

			Sleep(10);
		}

		Sleep(10);
	}


app_end:

	CloseHandle(hConsInp);
	CloseHandle(hConsOut);
	CloseHandle(hSerial);


	return 0;
}
