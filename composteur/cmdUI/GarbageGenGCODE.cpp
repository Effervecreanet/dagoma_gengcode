#include <Windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "GarbageDefs.h"

#define Y_START -64.000

#define Z_STEP 0.100
#define Y_STEP 0.100
#define X_STEP 0.100

#define STEP_E1 0.020

#define X_EDGE 87

#define BED_LAYER_MAX 2
#define BASE_LAYER_MAX 2
#define HIGH_LAYER_MAX 280

extern char Z_START[10];
extern char heatTemp[10];
extern char F_WHOLE[10];

#define EXTRUDE(P, B, E)                                                       \
  do {                                                                         \
    ZeroMemory(B, 255);                                                        \
    sprintf(B, "G1 F%s E%.3f\n", F_WHOLE, E);                                  \
    WritePipe(P, B);                                                           \
    E += STEP_E1;                                                              \
  } while (0)

int WritePipe(HANDLE hPipe, CHAR* buffer) {
	DWORD read;
	DWORD written;
	CHAR EOT[2];

	WriteFile(hPipe, buffer, strlen(buffer), &written, NULL);

	ZeroMemory(EOT, 2);

	while (!ReadFile(hPipe, EOT, 1, &read, NULL) || read != 1)
		Sleep(1);

	if (EOT[0] != '-')
		return -1;

	return 1;
}

void heat0deg(HANDLE hPipe) {
	/* Down heat to 0 deg */
	WritePipe(hPipe, "M106 S0\n");
	WritePipe(hPipe, "M109 S0\n");

	WritePipe(hPipe, "G92 E0.0\n");

	/* Return to origin position, auto home */
	WritePipe(hPipe, "G28\n");

	return;
}

void init_marlin(HANDLE hPipe) {
	char buf[255];

	WritePipe(hPipe, "G90\n");

	ZeroMemory(buf, 255);
	sprintf(buf, "G1 F%s X70.000 Y-64.000 Z%s\n", F_WHOLE, Z_START);
	WritePipe(hPipe, buf);

	ZeroMemory(buf, 255);
	sprintf(buf, "M106 S%s\n", heatTemp);
	WritePipe(hPipe, buf);

	ZeroMemory(buf, 255);
	sprintf(buf, "M109 S%s\n", heatTemp);
	WritePipe(hPipe, buf);

	return;
}

int* funcThreadGenGCODE(LPVOID lpParameter) {
	HANDLE hPipe;
	CHAR pipeBuffer[255];
	double E, X, XDEST, Y, Z;
	double X_END_RIGHT;
	double X_END_LEFT;
	int flag;
	int layer;
	double X_END_STEP_1 = 0.400;

	hPipe = CreateFile(STR_PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, 0, NULL);

	if (hPipe == INVALID_HANDLE_VALUE) {
		printf("Cannot open pipe.\n");
		return (int*)-1;
	}

	init_marlin(hPipe);

	X_END_RIGHT = 70.000;
	X_END_LEFT = -70.000;

	Y = Y_START;
	Z = Z_START[0] == '-' ? -(atof(&Z_START[1])) : atof(Z_START);
	E = 0.0;

	X = X_END_RIGHT;

	layer = BED_LAYER_MAX;
	while (layer-- > 0) {
		for (flag = 0; Y < 73.000;) {
			if (X > X_EDGE)
				flag = 1;

			if (flag == 1)
				X -= X_STEP;
			else
				X += X_STEP;

			EXTRUDE(hPipe, pipeBuffer, E);
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s X%.3f\n", F_WHOLE, X);
			WritePipe(hPipe, pipeBuffer);

			EXTRUDE(hPipe, pipeBuffer, E);
			Y += Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);

			EXTRUDE(hPipe, pipeBuffer, E);
			Y += Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);

			EXTRUDE(hPipe, pipeBuffer, E);
			Y += Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
		}

		for (XDEST = -X; X > XDEST; X -= X_STEP) {
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s X%.3f\n", F_WHOLE, X);
			WritePipe(hPipe, pipeBuffer);
			EXTRUDE(hPipe, pipeBuffer, E);
		}

		for (flag = 0; Y > -64.000;) {
			if (X < -X_EDGE)
				flag = 1;

			if (flag == 1)
				X += X_STEP;
			else
				X -= X_STEP;

			EXTRUDE(hPipe, pipeBuffer, E);
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s X%.3f\n", F_WHOLE, X);
			WritePipe(hPipe, pipeBuffer);

			EXTRUDE(hPipe, pipeBuffer, E);
			Y -= Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);

			EXTRUDE(hPipe, pipeBuffer, E);
			Y -= Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);

			EXTRUDE(hPipe, pipeBuffer, E);
			Y -= Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
		}

		for (XDEST = fabs(X); X < XDEST; X += X_STEP) {
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s X%.3f\n", F_WHOLE, X);
			WritePipe(hPipe, pipeBuffer);
			EXTRUDE(hPipe, pipeBuffer, E);
		}

		if (layer == 0)
			break;

		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);

		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);
	}

	layer = BASE_LAYER_MAX;

	while (layer-- > 0) {
		flag = 0;
		for (; Y < 73.000;) {
			for (; X > X_END_LEFT; X -= X_STEP) {
				ZeroMemory(pipeBuffer, 255);
				sprintf(pipeBuffer, "G1 F%s X%.3f\n", F_WHOLE, X);
				WritePipe(hPipe, pipeBuffer);
				EXTRUDE(hPipe, pipeBuffer, E);
			}

			Y += Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
			Y += Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
			Y += Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
			Y += Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
			Y += Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
			Y += Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);

			for (; X < X_END_RIGHT; X += X_STEP) {
				ZeroMemory(pipeBuffer, 255);
				sprintf(pipeBuffer, "G1 F%s X%.3f\n", F_WHOLE, X);
				WritePipe(hPipe, pipeBuffer);
				EXTRUDE(hPipe, pipeBuffer, E);
			}

			Y += Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
			Y += Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
			Y += Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
			Y += Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
			Y += Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
			Y += Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);

			if (X > X_EDGE)
				flag = 1;

			if (flag == 1) {
				X_END_RIGHT -= X_END_STEP_1;
				X_END_LEFT += X_END_STEP_1;
			}
			else {
				X_END_RIGHT += X_END_STEP_1;
				X_END_LEFT -= X_END_STEP_1;
			}
		}

		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);
		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);
		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);
		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);
		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);
		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);
		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);
		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);
		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);

		flag = 0;

		for (; Y >= Y_START;) {
			for (; X > X_END_LEFT; X -= X_STEP) {
				ZeroMemory(pipeBuffer, 255);
				sprintf(pipeBuffer, "G1 F%s X%.3f\n", F_WHOLE, X);
				WritePipe(hPipe, pipeBuffer);
				EXTRUDE(hPipe, pipeBuffer, E);
			}

			Y -= Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
			Y -= Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
			Y -= Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
			Y -= Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
			Y -= Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
			Y -= Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);

			for (; X < X_END_RIGHT; X += X_STEP) {
				ZeroMemory(pipeBuffer, 255);
				sprintf(pipeBuffer, "G1 F%s X%.3f\n", F_WHOLE, X);
				WritePipe(hPipe, pipeBuffer);
				EXTRUDE(hPipe, pipeBuffer, E);
			}

			Y -= Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
			Y -= Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
			Y -= Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
			Y -= Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
			Y -= Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
			Y -= Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);

			if (X > X_EDGE)
				flag = 1;

			if (flag == 1) {
				X_END_RIGHT -= X_END_STEP_1;
				X_END_LEFT += X_END_STEP_1;
			}
			else {
				X_END_RIGHT += X_END_STEP_1;
				X_END_LEFT -= X_END_STEP_1;
			}
		}

		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);
		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);
		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);
		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);
		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);
		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);
		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);
		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);
		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);
	}

	layer = HIGH_LAYER_MAX;
	while (layer-- > 0) {
		for (flag = 0; Y < 73.000;) {
			if (X > X_EDGE)
				flag = 1;

			if (flag == 1)
				X -= X_STEP;
			else
				X += X_STEP;

			EXTRUDE(hPipe, pipeBuffer, E);
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s X%.3f\n", F_WHOLE, X);
			WritePipe(hPipe, pipeBuffer);

			EXTRUDE(hPipe, pipeBuffer, E);
			Y += Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);

			EXTRUDE(hPipe, pipeBuffer, E);
			Y += Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);

			EXTRUDE(hPipe, pipeBuffer, E);
			Y += Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
		}

		for (XDEST = -X; X > XDEST; X -= X_STEP) {
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s X%.3f\n", F_WHOLE, X);
			WritePipe(hPipe, pipeBuffer);
			EXTRUDE(hPipe, pipeBuffer, E);
		}

		for (flag = 0; Y > -64.000;) {
			if (X < -X_EDGE)
				flag = 1;

			if (flag == 1)
				X += X_STEP;
			else
				X -= X_STEP;

			EXTRUDE(hPipe, pipeBuffer, E);
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s X%.3f\n", F_WHOLE, X);
			WritePipe(hPipe, pipeBuffer);

			EXTRUDE(hPipe, pipeBuffer, E);
			Y -= Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);

			EXTRUDE(hPipe, pipeBuffer, E);
			Y -= Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);

			EXTRUDE(hPipe, pipeBuffer, E);
			Y -= Y_STEP;
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s Y%.3f\n", F_WHOLE, Y);
			WritePipe(hPipe, pipeBuffer);
		}

		for (XDEST = fabs(X); X < XDEST; X += X_STEP) {
			ZeroMemory(pipeBuffer, 255);
			sprintf(pipeBuffer, "G1 F%s X%.3f\n", F_WHOLE, X);
			WritePipe(hPipe, pipeBuffer);
			EXTRUDE(hPipe, pipeBuffer, E);
		}

		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);

		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);

		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);

		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);

		Z += Z_STEP;
		ZeroMemory(pipeBuffer, 255);
		sprintf(pipeBuffer, "G1 F%s Z%.3f\n", F_WHOLE, Z);
		WritePipe(hPipe, pipeBuffer);
	}

	heat0deg(hPipe);

	WritePipe(hPipe, "-EOT-");

	CloseHandle(hPipe);

	return (int*)0;
}
