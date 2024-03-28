#include <Windows.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <iostream>
#include <list>
#include <vector>

#include "VisorDefs.h"

using namespace std;

#define VISOR_TOTAL_GCODE_LINES 4116329

#define STEP_X 0.100
#define STEP_Y 0.040
#define STEP_Z 0.100

#define STEP_E2 0.02
#define STEP_E1 0.01
#define STEP_E3 0.003

#define LAYER_VISOR  242
#define LAYER_BRANCH  70

#define X_START -70.000
#define Y_START  72.000
#define Y_END   -72.000

extern char Z_START[10];
extern char heatTemp[10];
extern char F_WHOLE[10];

static HANDLE hPipe;
static float E, X, Y, Z;

int
WritePipe(HANDLE hPipe, CHAR *buffer)
{
	DWORD read;
	DWORD written;
	CHAR EOT[2];


	WriteFile(hPipe, buffer, strlen(buffer), &written, NULL);
	
	ZeroMemory(EOT, 2);


	while(!ReadFile(hPipe, EOT, 1, &read, NULL) || read != 1)
		Sleep(1);

	if (EOT[0] != '-')
		return -1;

	return 1;
}


void
heat0deg(HANDLE hPipe)
{
	/* Down heat to 0 deg */
	WritePipe(hPipe, "M106 S0\n");
	WritePipe(hPipe, "M109 S0\n");
	WritePipe(hPipe, "G92 E0.0\n");

	/* Return to origin position, auto home */
	WritePipe(hPipe, "G28\n");

	return;
}

#define EXTRUDE1(E) do {                                    \
						ZeroMemory(chBuf, 255);             \
                        sprintf(chBuf, "G1 E%.3f\n", E);    \
						WritePipe(hPipe, chBuf);            \
                        E += STEP_E1;                       \
                    } while(0)


#define EXTRUDE(E) do {                                             \
                        ZeroMemory(chBuf, 255);                     \
                        sprintf(chBuf, "G1 E%.3f\n", E);            \
						WritePipe(hPipe, chBuf);                    \
                        E += STEP_E2;                               \
                    } while(0)

class Branch : public std::list<float> {
private:
	void pOut(float y, bool noextrude) {
		char buffer[255];

		ZeroMemory(buffer, 255);

		if (noextrude) {
			sprintf(buffer, "G1 F%s Y%.3f\n", F_WHOLE, y);
		}
		else {
			E += STEP_E3;
			sprintf(buffer, "G1 F%s E%.3f Y%.3f\n", F_WHOLE, E, y);
		}
		// printf("G1 F%s Y%.3f\n", y);

		WritePipe(hPipe, buffer);

		return;
	}
	void pOut2(float y, bool noextrude) {
		char buffer[255];

		ZeroMemory(buffer, 255);

		if (noextrude == true) {
			sprintf(buffer, "G1 F%s X%.3f\n", F_WHOLE, (y < 0.009 && y > -0.001) ? 0.0 : y);
		}
		else {
			E += STEP_E3;
			sprintf(buffer, "G1 F%s E%.3f X%.3f\n", F_WHOLE, E,
				(y < 0.009 && y > -0.001) ? 0.0 : y);
		}
		// printf("G1 F%s Y%.3f\n", y);

		WritePipe(hPipe, buffer);

		return;
	}

public:
	void New(float ystart, float yend);
	void New2(float ystart, float yend);
	void Go(bool noextrude = false);
	void Return(void);
	void Go2(bool noextrude = false);
	void Return2(void);
};

void Branch::New(float ystart, float yend) {
	float Y;

	for (Y = ystart; Y > yend; Y -= STEP_Y)
		this->push_back(Y);

	return;
}

void Branch::New2(float ystart, float yend) {
	float Y;

	for (Y = ystart; Y < yend; Y += STEP_Y)
		this->push_back(Y);

	return;
}
void Branch::Go(bool noextrude) {
	for (std::list<float>::iterator it = this->begin(); it != this->end(); ++it)
		this->pOut(*it, noextrude);

	return;
}

void Branch::Go2(bool noextrude) {
	for (std::list<float>::iterator it = this->begin(); it != this->end(); ++it)
		this->pOut2(*it, noextrude);

	return;
}

void Branch::Return(void) {
	this->reverse();
	for (std::list<float>::iterator it = this->begin(); it != this->end(); ++it)
		this->pOut(*it, false);
	this->reverse();

	return;
}
void Branch::Return2(void) {
	this->reverse();
	for (std::list<float>::iterator it = this->begin(); it != this->end(); ++it)
		this->pOut2(*it, false);
	this->reverse();

	return;
}

int* funcThreadGenGCODE(LPVOID lpParameter)
{
	CHAR chBuf[255];
	unsigned short F;
	int layer = 0;
	Branch BaseLeft2Right;

	hPipe = CreateFile(STR_PIPE_NAME,
					   GENERIC_READ | GENERIC_WRITE,
					   0,
					   NULL,
					   OPEN_EXISTING,
					   0,
					   NULL);
	
	if (hPipe == INVALID_HANDLE_VALUE) {
		printf("Cannot open pipe.\n");
		return (int*)-1;
	}
	
	char buf[255];

	X = X_START;
	Y = Y_START;
	F = atoi(F_WHOLE);

	Z = Z_START[0] == '-' ? -(atof(&Z_START[1])) : atof(Z_START);

	E = 0.0;

	WritePipe(hPipe, "G90\n");

	ZeroMemory(buf, 255);
	sprintf(buf, "G1 F600 X%.3f Y%.3f Z%.3f\n", X_START, Y_START, Z);
	WritePipe(hPipe, buf);

	ZeroMemory(buf, 255);
	sprintf(buf, "M106 S%s\n", heatTemp);
	WritePipe(hPipe, buf);

	ZeroMemory(buf, 255);
	sprintf(buf, "M109 S%s\n", heatTemp);
	WritePipe(hPipe, buf);

	BaseLeft2Right.New2(X_START - 1, (float)abs(X_START - 1));
	BaseLeft2Right.Go2();

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	BaseLeft2Right.Return2();

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	BaseLeft2Right.Go2();

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	BaseLeft2Right.Return2();

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	sprintf(chBuf, "G1 Y%.3f\n", Y_START - 0.80 * 1);
	WritePipe(hPipe, chBuf);

	BaseLeft2Right.Go2();

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	BaseLeft2Right.Return2();

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	BaseLeft2Right.Go2();

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	BaseLeft2Right.Return2();

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	sprintf(chBuf, "G1 Y%.3f\n", Y_START - 0.80 * 2);
	WritePipe(hPipe, chBuf);
	// ##

	BaseLeft2Right.Go2();

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	BaseLeft2Right.Return2();

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	BaseLeft2Right.Go2();

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	BaseLeft2Right.Return2();

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	sprintf(chBuf, "G1 Y%.3f\n", Y_START - 0.80 * 3);
	WritePipe(hPipe, chBuf);

	BaseLeft2Right.Go2();

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	BaseLeft2Right.Return2();

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	BaseLeft2Right.Go2();

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	BaseLeft2Right.Return2();

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z -= STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	sprintf(chBuf, "G1 Y%.3f\n", Y_START - 0.80 * 4);
	WritePipe(hPipe, chBuf);

	BaseLeft2Right.Go2();

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	BaseLeft2Right.Return2();

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	BaseLeft2Right.Go2();

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	Z += STEP_Z;
	sprintf(chBuf, "G1 Z%.3f\n", Z);
	WritePipe(hPipe, chBuf);

	BaseLeft2Right.Return2();

	ZeroMemory(buf, 255);
	sprintf(buf, "G1 F600 X%.3f Y%.3f Z%.3f\n", X_START, Y_START, Z);
	WritePipe(hPipe, buf);

	/* Trip until LAYER_VISOR reached */

	while (layer < LAYER_VISOR) {
		/* Go */

		/* Branches are less high than visor */

		if (layer < LAYER_BRANCH) {
			/* Left branch go */

			while(Y > Y_END) {
				EXTRUDE1(E);
				ZeroMemory(chBuf, 255);
				sprintf(chBuf, "G1 F%d Y%.3f\n", F, Y);
				WritePipe(hPipe, chBuf);
				Y -= STEP_Y;
			}
		}

		/* Triangle left side */

		do {
			EXTRUDE(E);

            ZeroMemory(chBuf, 255);
			sprintf(chBuf, "G1 F%d X%.3f\n", F, X);
			WritePipe(hPipe, chBuf);
			X += STEP_X;
			ZeroMemory(chBuf, 255);
			sprintf(chBuf, "G1 F%d Y%.3f\n", F, Y);
			WritePipe(hPipe, chBuf);
			Y -= STEP_Y;

		} while (X < 0.000);

		/* Triangle right side */

		do {
			EXTRUDE(E);

            ZeroMemory(chBuf, 255);
			sprintf(chBuf, "G1 F%d X%.3f\n", F, X);
			WritePipe(hPipe, chBuf);
			X += STEP_X;
			ZeroMemory(chBuf, 255);
			sprintf(chBuf, "G1 F%d Y%.3f\n", F, Y);
			WritePipe(hPipe, chBuf);
			Y += STEP_Y;

		} while (X < 70.000);

		/* level up after bed layer */

		if (layer > 0) {
			Z += STEP_Z;
			ZeroMemory(chBuf, 255);
			sprintf(chBuf, "G1 F%d Z%.3f\n", F, Z);
			WritePipe(hPipe, chBuf);

			Z += STEP_Z;
			ZeroMemory(chBuf, 255);
			sprintf(chBuf, "G1 F%d Z%.3f\n", F, Z);
			WritePipe(hPipe, chBuf);

			Z += STEP_Z;
			ZeroMemory(chBuf, 255);
			sprintf(chBuf, "G1 F%d Z%.3f\n", F, Z);
			WritePipe(hPipe, chBuf);

			Z += STEP_Z / 2;
			ZeroMemory(chBuf, 255);
			sprintf(chBuf, "G1 F%d Z%.3f\n", F, Z);
			WritePipe(hPipe, chBuf);
		}


		/* Branches are less high than visor */

		if (layer < LAYER_BRANCH) {
			/* Right branch go */

			while(Y < Y_START) {
				EXTRUDE1(E);
				ZeroMemory(chBuf, 255);
				sprintf(chBuf, "G1 F%d Y%.3f\n", F, Y);
				WritePipe(hPipe, chBuf);
				Y += STEP_Y;
			}

			/* Right branch return */

			while(Y > Y_END) {
				EXTRUDE1(E);
				ZeroMemory(chBuf, 255);
				sprintf(chBuf, "G1 F%d Y%.3f\n", F, Y);
				WritePipe(hPipe, chBuf);
				Y -= STEP_Y;
			}
		}

		/* Return */

		/* Triangle right side */

		do {
			EXTRUDE(E);
            ZeroMemory(chBuf, 255);
			sprintf(chBuf, "G1 F%d X%.3f\n", F, X);
			WritePipe(hPipe, chBuf);
			X -= STEP_X;
			ZeroMemory(chBuf, 255);
			sprintf(chBuf, "G1 F%d Y%.3f\n", F, Y);
			WritePipe(hPipe, chBuf);
			Y -= STEP_Y;

		} while (X > 0.000);

		/* Triangle left side */
		
		do {
			EXTRUDE(E);
            ZeroMemory(chBuf, 255);
			sprintf(chBuf, "G1 F%d X%.3f\n", F, X);
			WritePipe(hPipe, chBuf);
			X -= STEP_X;
			ZeroMemory(chBuf, 255);
			sprintf(chBuf, "G1 F%d Y%.3f\n", F, Y);
			WritePipe(hPipe, chBuf);
			Y += STEP_Y;

		} while (X > -70.000);	

		layer++;

		if (layer < LAYER_BRANCH) {
			/* Left branch return */

			while(Y < Y_START) {
				EXTRUDE1(E);
				ZeroMemory(chBuf, 255);
				sprintf(chBuf, "G1 F%d Y%.3f\n", F, Y);
				WritePipe(hPipe, chBuf);
				Y += STEP_Y;
			}
		}
			
		/* level up */

		Z += STEP_Z;
		ZeroMemory(chBuf, 255);
		sprintf(chBuf, "G1 F%d Z%.3f\n", F, Z);
		WritePipe(hPipe, chBuf);

		Z += STEP_Z;
		ZeroMemory(chBuf, 255);
		sprintf(chBuf, "G1 F%d Z%.3f\n", F, Z);
		WritePipe(hPipe, chBuf);

		Z += STEP_Z;
		ZeroMemory(chBuf, 255);
		sprintf(chBuf, "G1 F%d Z%.3f\n", F, Z);
		WritePipe(hPipe, chBuf);		

		Z += STEP_Z / 2;
		ZeroMemory(chBuf, 255);
		sprintf(chBuf, "G1 F%d Z%.3f\n", F, Z);
		WritePipe(hPipe, chBuf);		
	}

	heat0deg(hPipe);

	WritePipe(hPipe, "-EOT-");
	
	CloseHandle(hPipe);

	return (int*)0;
}
