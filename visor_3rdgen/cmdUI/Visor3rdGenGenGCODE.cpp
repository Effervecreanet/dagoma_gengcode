#include <Windows.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <iostream>
#include <list>
#include <vector>

#include "Visor3rdGenDefs.h"

using namespace std;

#define Z_STEP 0.10
#define Y_STEP 0.10
#define X_STEP 0.10

#define STEP_E1 0.012
#define STEP_E2 0.012

#define X_START -70.00
#define Y_START 74.00
#define Y_END -74.00
#define Y_EDGE -96.398


#define LAYER_BRANCH 80

float Z;
float E = 0.0;

extern char Z_START[10];
extern char heatTemp[10];
extern char F_WHOLE[10];

HANDLE hPipe;

int WritePipe(HANDLE hPipe, CHAR* buffer);

static void stamp(void)
{
  struct tm *tmv;
  time_t timet;
  char buff[128];

  time(&timet);
  tmv = localtime(&timet);
  memset(buff, 0, 128);
  strftime(buff, 128,"; Generated at %a %b %e %H:%M:%S %Y\n", tmv);
  WritePipe(hPipe, buff);
  memset(buff, 0, 128);
//  sprintf(buff, "; With z_start: %s heat temp: %s speed: %s\n", Z_START, heatTemp, F_WHOLE);
  sprintf(buff, "; With z_start: %s heat temp: %s speed: %s\n", Z_START, heatTemp, F_WHOLE);
  WritePipe(hPipe, buff);
  WritePipe(hPipe, "; Author: Franck Lesage\n");
  WritePipe(hPipe, "; Website: http://www.effervecrea.net\n");
  WritePipe(hPipe, "; E-mail: effervecreanet@orange.fr\n");

  return;
}

class Branch : public std::list<float> {
private:
    void pOut(float y, bool noextrude) {
        char buffer[255];

        ZeroMemory(buffer, 255);
        
        if (noextrude) {
            sprintf(buffer, "G1 F%s Y%.3f\n", F_WHOLE, y);
        }
        else {
            E += STEP_E1;
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
            E += STEP_E1;
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

    for (Y = ystart; Y > yend; Y -= Y_STEP)
        this->push_back(Y);

    return;
}

void Branch::New2(float ystart, float yend) {
    float Y;

    for (Y = ystart; Y < yend; Y += Y_STEP)
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

class headSide : public std::list<pair<float, float>> {
private:
    void pOut(float x, float y) {
        char buffer[255];

        ZeroMemory(buffer, 255);

        E += STEP_E2;
        sprintf(buffer, "G1 F%s E%.3f X%.3f Y%.3f\n", F_WHOLE, E, x, y);
        // printf("G1 F%s X%.3f Y%.3f\n", x, y);

        WritePipe(hPipe, buffer);

        return;
    }

public:
    void New(std::pair<float, float> startXY, std::pair<float, float> endXY);
    void New2(std::pair<float, float> startXY, std::pair<float, float> endXY);
    void Go(void);
    void Return(void);
    void ShiftY(float pad);
};

void headSide::New(std::pair<float, float> startXY,
    std::pair<float, float> endXY) {
    float Y, X;

    for (X = startXY.first, Y = startXY.second; X < endXY.first;
        X += X_STEP, Y = Y + (endXY.first > 0 ? 0.032 : -0.032))
        this->push_back(std::make_pair(X, Y));

    return;
}

void headSide::New2(std::pair<float, float> startXY,
    std::pair<float, float> endXY) {
    float Y, X;

    for (X = startXY.first, Y = startXY.second; X > endXY.first;
        X -= X_STEP, Y = Y + (endXY.first < 0 ? 0.032 : -0.032))
        this->push_back(std::make_pair(X, Y));

    return;
}

void headSide::Go(void) {
    for (std::list<pair<float, float>>::iterator it = this->begin();
        it != this->end(); ++it)
        this->pOut(it->first, it->second);

    return;
}

void headSide::Return(void) {
    this->reverse();
    for (std::list<pair<float, float>>::iterator it = this->begin();
        it != this->end(); ++it)
        this->pOut(it->first, it->second);
    this->reverse();

    return;
}

void headSide::ShiftY(float pad) {
    for (std::list<pair<float, float>>::iterator it = this->begin();
        it != this->end(); ++it)
        it->second += pad;

    return;
}

void
init_marlin(HANDLE hPipe)
{
    char buf[255];

    ZeroMemory(buf, 255);
	WritePipe(hPipe, "G90\n");
	
    ZeroMemory(buf, 255);
    sprintf(buf, "G1 F600 Z%.3f\n", Z);
    WritePipe(hPipe, buf);

    ZeroMemory(buf, 255);
    sprintf(buf, "M106 S%s\n", heatTemp);
    WritePipe(hPipe, buf);

    ZeroMemory(buf, 255);
    sprintf(buf, "M109 S%s\n", heatTemp);
    WritePipe(hPipe, buf);

    return;
}

void marlin_end(HANDLE hPipe)
{
    WritePipe(hPipe, "G92 E0.0\n");
	WritePipe(hPipe, "G28\n");
	WritePipe(hPipe, "M106 S0\n");
	WritePipe(hPipe, "M109 S0\n");
	return;
}

int
WritePipe(HANDLE hPipe, CHAR *buffer)
{
	DWORD read;
	DWORD written;
	CHAR EOT[2];


	WriteFile(hPipe, buffer, strlen(buffer), &written, NULL);
	
	ZeroMemory(EOT, 2);


	while(!ReadFile(hPipe, EOT, 1, &read, NULL) || read != 1);

	if (EOT[0] != '-')
		return -1;

	return 1;
}

class Clips : public std::list<pair<float, float>> {
private:
  void pOut(float x, float y) {
    char chBuf[255];

    E += STEP_E2;
    sprintf(chBuf, "G1 F%s E%.3f X%.3f Y%.3f\n", F_WHOLE, E, x, y);
    WritePipe(hPipe, chBuf);
    // printf("G1 F%hu X%.3f Y%.3f\n", F_WHOLE, x, y);

    return;
  }

public:
  void New(std::pair<float, float> startXY, std::pair<float, float> endXY);
  void Go(void);
  void Return(void);
  void ShiftX(float pad);
};

void Clips::New(std::pair<float, float> startXY,
		   std::pair<float, float> endXY) {
    float X, Y;

  for (X = startXY.first, Y = startXY.second; Y < endXY.second;
       Y += Y_STEP)
    this->push_back(std::make_pair(X, Y));

	return;
}

void Clips::Go(void) {
  for (std::list<pair<float, float>>::iterator it = this->begin();
       it != this->end(); ++it)
    this->pOut(it->first, it->second);

  return;
}

void Clips::Return(void) {
  this->reverse();
  for (std::list<pair<float, float>>::iterator it = this->begin();
       it != this->end(); ++it)
    this->pOut(it->first, it->second);
  this->reverse();

  return;
}

void Clips::ShiftX(float pad) {
  for (std::list<pair<float, float>>::iterator it = this->begin();
       it != this->end(); ++it)
    it->first += pad;
}

int* funcThreadGenGCODE(LPVOID lpParameter)
{
	CHAR chBuf[255];
	DWORD read;
	DWORD err;
	CHAR pipeBuffer[255];
    double X, XDEST, Y;
	double X_END_RIGHT;
	double X_END_LEFT;
	int flag;
	double X_END_STEP_1 = 0.400;
	int layer;
	float padY, padX;
	Branch branch, subBranch, remainBranch, left2right, BaseLeft2Right, right2zero, back2front, headEdge;
	headSide sideRight, sideLeft, subSideLeft0, subSideLeft1, subSideLeft2, subSideLeft3;
	headSide edgeSideLeft0, edgeSideLeft1, edgeSideRight0, edgeSideRight1;
      Clips clipsLeft, clipsRight;


    BaseLeft2Right.New2(X_START - 1, (float)abs(X_START - 1));
    left2right.New2(X_START, (float)abs(X_START));
    right2zero.New((float)abs(X_START), 0.000);
    back2front.New(Y_START, Y_EDGE);

    branch.New(Y_START, Y_END);
    subBranch.New(Y_START, Y_END + 4.000);
    remainBranch.New(Y_END + 4.000, Y_END);


    sideLeft.New(std::make_pair((float)X_START, (float)Y_END),
        std::make_pair((float)0.0, (float)Y_EDGE));

    sideRight.New(std::make_pair((float)0.0, (float)(Y_EDGE)),
        std::make_pair((float)abs(X_START), (float)Y_END));

    subSideLeft0.New(
        std::make_pair((float)X_START, (float)(Y_END + 4.000)),
        std::make_pair((float)(X_START + 16.0), (float)(Y_EDGE - 11.000)));

    subSideLeft1.New(
        std::make_pair((float)X_START, (float)(Y_END)),
        std::make_pair((float)(X_START + 16.0), (float)(Y_EDGE - 11.000)));

    subSideLeft2.New2(
        std::make_pair((float)abs(X_START), (float)(Y_END+4.000)),
        std::make_pair((float)((float)abs(X_START) - 16.0), (float)(Y_EDGE - 11.000)));

    subSideLeft3.New2(
        std::make_pair((float)abs(X_START), (float)(Y_END)),
        std::make_pair((float)((float)abs(X_START) - 16.0), (float)(Y_EDGE - 11.000)));

    edgeSideLeft0.New2(std::make_pair((float)0.0, (float)(Y_EDGE)),
        std::make_pair((float)(-10.0), (float)(Y_EDGE - 11.000)));

    edgeSideLeft1.New2(std::make_pair((float)0.0, (float)(Y_EDGE + 4.000)),
        std::make_pair((float)(-10.0), (float)(Y_EDGE - 11.000)));

    edgeSideRight0.New(std::make_pair((float)(0.0), (float)(Y_EDGE + 4.000)),
        std::make_pair((float)(10.0), (float)(Y_EDGE - 11.000)));

    edgeSideRight1.New(std::make_pair((float)(0.0), (float)(Y_EDGE)),
        std::make_pair((float)(10.0), (float)(Y_EDGE - 11.000)));

    headEdge.New2(Y_EDGE, Y_EDGE + 4.000);

      clipsLeft.New(std::make_pair((float) -90.00, (float) -35.00),
	  	std::make_pair((float) -90.00, (float) 32.00));

  clipsRight.New(std::make_pair((float) 90.00, (float) -35.00),
	  	 std::make_pair((float) 90.00, (float) 32.00));

    Z = Z_START[0] == '-' ? -(atof(&Z_START[1])) : atof(Z_START);

	hPipe = CreateFile("\\\\.\\pipe\\pipe_visor",
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

    Z = Z_START[0] == '-' ? -(atof(&Z_START[1])) : atof(Z_START);
    
    stamp();
    
    /* Init marlin abs_pos */
    init_marlin(hPipe);

 for (layer = 0, padX = 0.76; layer < 2; ++layer) {
    clipsLeft.Go();
    clipsLeft.ShiftX(padX);
    clipsLeft.Return();
    clipsLeft.ShiftX(padX);
    clipsLeft.Go();
    clipsLeft.ShiftX(padX);
    clipsLeft.Return();
    clipsLeft.ShiftX(padX);
    clipsLeft.Go();
    clipsLeft.ShiftX(padX);
    clipsLeft.Return();
    clipsLeft.ShiftX(padX);
    if (padX <= 0.9 && padX >= 0.7)
      padX = -0.76;
    else
      padX = 0.76;
    clipsLeft.ShiftX(padX);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

    Z += Z_STEP / 2;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);  }

  padX = 0.76;
  clipsLeft.ShiftX(padX);

  for (layer = 0, padX = 0.76; layer < 4; ++layer) {

    clipsLeft.Go();
    clipsLeft.ShiftX(padX);
    clipsLeft.Return();
    clipsLeft.ShiftX(padX);
    clipsLeft.Go();
    clipsLeft.ShiftX(padX);
    clipsLeft.Return();
    clipsLeft.ShiftX(padX);
    if (padX <= 0.9 && padX >= 0.7)
      padX = -0.76;
    else
      padX = 0.76;

    clipsLeft.ShiftX(padX);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

    Z += Z_STEP / 2;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
}


  E -= 1.000;
  sprintf(chBuf, "G1 E%.3f\n", E);
    WritePipe(hPipe, chBuf);

    Z = Z_START[0] == '-' ? -(atof(&Z_START[1])) : atof(Z_START);

  sprintf(chBuf, "G1 F%s X%.3f Y%.3f Z%.3f\n", F_WHOLE, (float)90.00, (float)-35.00, Z);
    WritePipe(hPipe, chBuf);

  E += 0.900;

  for (layer = 0, padX = 0.76; layer < 2; ++layer) {
    clipsRight.Go();
    clipsRight.ShiftX(padX);
    clipsRight.Return();
    clipsRight.ShiftX(padX);
    clipsRight.Go();
    clipsRight.ShiftX(padX);
    clipsRight.Return();
    clipsRight.ShiftX(padX);
    clipsRight.Go();
    clipsRight.ShiftX(padX);
    clipsRight.Return();
    clipsRight.ShiftX(padX);
    if (padX <= 0.9 && padX >= 0.7)
      padX = -0.76;
    else
      padX = 0.76;
    clipsRight.ShiftX(padX);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

    Z += Z_STEP / 2;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
  }

  padX = 0.76;
  clipsRight.ShiftX(padX);

  for (layer = 0, padX = 0.76; layer < 4; ++layer) {

    clipsRight.Go();
    clipsRight.ShiftX(padX);
    clipsRight.Return();
    clipsRight.ShiftX(padX);
    clipsRight.Go();
    clipsRight.ShiftX(padX);
    clipsRight.Return();
    clipsRight.ShiftX(padX);
    if (padX <= 0.9 && padX >= 0.7)
      padX = -0.76;
    else
      padX = 0.76;

    clipsRight.ShiftX(padX);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

    Z += Z_STEP / 2;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);  }




    E -= 0.450;
    sprintf(chBuf, "G1 E%.3f\n", E);
    WritePipe(hPipe, chBuf);

    Z = Z_START[0] == '-' ? -(atof(&Z_START[1])) : atof(Z_START);
    ZeroMemory(chBuf, 255);
    sprintf(chBuf, "G1 F%s X%.3f Y%.3f Z%.3f\n", F_WHOLE, X_START, Y_START, Z);
    WritePipe(hPipe, chBuf);

    E += 0.450;
    
    BaseLeft2Right.Go2();

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    BaseLeft2Right.Return2();
    
    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    BaseLeft2Right.Go2();
    
    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    BaseLeft2Right.Return2();
    
    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    sprintf(chBuf, "G1 Y%.3f\n", Y_START - 0.80 * 1);
    WritePipe(hPipe, chBuf);

    BaseLeft2Right.Go2();

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    BaseLeft2Right.Return2();

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    BaseLeft2Right.Go2();

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    BaseLeft2Right.Return2();

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    sprintf(chBuf, "G1 Y%.3f\n", Y_START - 0.80 * 2);
    WritePipe(hPipe, chBuf);
    // ##

    BaseLeft2Right.Go2();

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    BaseLeft2Right.Return2();

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    BaseLeft2Right.Go2();

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    BaseLeft2Right.Return2();

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    sprintf(chBuf, "G1 Y%.3f\n", Y_START - 0.80 * 3);
    WritePipe(hPipe, chBuf);

    BaseLeft2Right.Go2();

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    BaseLeft2Right.Return2();

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    BaseLeft2Right.Go2();

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    BaseLeft2Right.Return2();

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z -= Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    sprintf(chBuf, "G1 Y%.3f\n", Y_START - 0.80 * 4);
    WritePipe(hPipe, chBuf);

    BaseLeft2Right.Go2();

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    BaseLeft2Right.Return2();

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    BaseLeft2Right.Go2();

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);

    BaseLeft2Right.Return2();

    Z = Z_START[0] == '-' ? -(atof(&Z_START[1])) : atof(Z_START);
    ZeroMemory(chBuf, 255);
    sprintf(chBuf, "G1 F%s X%.3f Y%.3f Z%.3f\n", F_WHOLE, X_START, Y_START, Z);
    WritePipe(hPipe, chBuf);

    for (layer = 0, padY = 1; layer < LAYER_BRANCH / 10; ++layer) {

        branch.Go();
        sideLeft.Go();
        sideRight.Go();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideRight.Return();
        sideLeft.Return();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideLeft.Go();
        sideRight.Go();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideRight.Return();
        sideLeft.Return();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideLeft.Go();
        sideRight.Go();

        branch.Return();
        branch.Go();

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP / 2;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        padY = -1;

        sideRight.Return();
        sideLeft.Return();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideLeft.Go();
        sideRight.Go();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideRight.Return();
        sideLeft.Return();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideLeft.Go();
        sideRight.Go();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideRight.Return();
        sideLeft.Return();

        branch.Return();

        padY = 1;

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP / 2;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
    }

    for (layer = LAYER_BRANCH - (LAYER_BRANCH / 10); --layer;) {
        branch.Go();
        subSideLeft1.Go();
        subSideLeft1.Return();
        remainBranch.Return();
        subSideLeft0.Go();
        subSideLeft0.Return();
        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        subBranch.Return();
        
        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
    }


    left2right.Go2(true);

    for (layer = LAYER_BRANCH - (LAYER_BRANCH / 10); --layer;) {
        Z -= Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z -= Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z -= Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z -= Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z -= Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z -= Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
    }

  E += 1.400;
  sprintf(chBuf, "G1 E%.3f\n", E);
    WritePipe(hPipe, chBuf);

  WritePipe(hPipe, "G4 S2\n");

  E += 1.400;
    sprintf(chBuf, "G1 E%.3f\n", E);
    WritePipe(hPipe, chBuf);

  E += STEP_E1 * 11;

    for (layer = LAYER_BRANCH - (LAYER_BRANCH / 10); --layer;) {
        branch.Go();
        subSideLeft3.Go();
        subSideLeft3.Return();
        remainBranch.Return();
        subSideLeft2.Go();
        subSideLeft2.Return();

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        subBranch.Return();

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
    }

    E -= 0.450;
    sprintf(chBuf, "G1 E%.3f\n", E);
    WritePipe(hPipe, chBuf);

    right2zero.Go2(true);
    back2front.Go(true);

    for (layer = LAYER_BRANCH - (LAYER_BRANCH / 9); --layer;) {
        Z -= Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z -= Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z -= Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z -= Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z -= Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z -= Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
    }

  E += 1.000;
  sprintf(chBuf, "G1 E%.3f\n", E);
  WritePipe(hPipe, chBuf);

  WritePipe(hPipe, "G4 S2\n");

  E += 1.000;
  sprintf(chBuf, "G1 E%.3f\n", E);
  WritePipe(hPipe, chBuf);

  E += STEP_E1;
  E += STEP_E1;
  E += STEP_E1;
  E += STEP_E1;

    layer = LAYER_BRANCH - (LAYER_BRANCH / 10);
    layer--;

    do {
        edgeSideLeft1.Go();
        edgeSideLeft1.Return();
        headEdge.Return();
        edgeSideLeft0.Go();
        edgeSideLeft0.Return();

        headEdge.Go();

        edgeSideRight0.Go();
        edgeSideRight0.Return();
        headEdge.Return();
        edgeSideRight1.Go();
        edgeSideRight1.Return();

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
    } while (--layer);


    headEdge.Go();
    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);
    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);
    Z += Z_STEP;
    sprintf(chBuf, "G1 Z%.3f\n", Z);
    WritePipe(hPipe, chBuf);
    headEdge.Return();

  E -= 1.000;
  sprintf(chBuf, "G1 E%.3f\n", E);
    WritePipe(hPipe, chBuf);

  sprintf(chBuf, "G1 F%s X%.3f Y%.3f Z%.3f\n", F_WHOLE, X_START, Y_START, Z);
      WritePipe(hPipe, chBuf);

  WritePipe(hPipe, "G4 S60\n");

  E += 0.100;
  sprintf(chBuf, "G1 E%.3f\n", E);
      WritePipe(hPipe, chBuf);

  E += STEP_E1 * 4;

    for (layer = 6, padY = 0.76; --layer;) {

        branch.Go();
        sideLeft.Go();
        sideRight.Go();
        ;
        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideRight.Return();
        sideLeft.Return();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideLeft.Go();
        sideRight.Go();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideRight.Return();
        sideLeft.Return();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideLeft.Go();
        sideRight.Go();

        branch.Return();
        branch.Go();

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
        Z += Z_STEP / 2;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        padY = -0.76;

        sideRight.Return();
        sideLeft.Return();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideLeft.Go();
        sideRight.Go();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideRight.Return();
        sideLeft.Return();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideLeft.Go();
        sideRight.Go();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideRight.Return();
        sideLeft.Return();

        branch.Return();

        padY = 0.76;

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
        Z += Z_STEP / 2;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
    }

    branch.Go();

    for (layer = 204, padY = 0.76; --layer;) {
        sideLeft.Go();
        sideRight.Go();
        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideRight.Return();
        sideLeft.Return();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideLeft.Go();
        sideRight.Go();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideRight.Return();
        sideLeft.Return();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideLeft.Go();
        sideRight.Go();

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
        Z += Z_STEP / 2;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);

        padY = -0.76;

        sideRight.Return();
        sideLeft.Return();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideLeft.Go();
        sideRight.Go();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideRight.Return();
        sideLeft.Return();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideLeft.Go();
        sideRight.Go();

        sideRight.ShiftY(padY);
        sideLeft.ShiftY(padY);

        sideRight.Return();
        sideLeft.Return();

        padY = 0.76;

        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
        Z += Z_STEP;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
        Z += Z_STEP / 2;
        sprintf(chBuf, "G1 Z%.3f\n", Z);
        WritePipe(hPipe, chBuf);
    }



	marlin_end(hPipe);

	WritePipe(hPipe, "-EOT-");
	
	CloseHandle(hPipe);

    branch.clear(); subBranch.clear(); remainBranch.clear(); left2right.clear(); right2zero.clear(); back2front.clear(); headEdge.clear();
    sideRight.clear(); sideLeft.clear(); subSideLeft0.clear(); subSideLeft1.clear(); subSideLeft2.clear(); subSideLeft3.clear();
    clipsLeft.clear(); clipsRight.clear(); edgeSideLeft0.clear(); edgeSideLeft1.clear(); edgeSideRight0.clear(); edgeSideRight1.clear();

    return (int*)0;
}
