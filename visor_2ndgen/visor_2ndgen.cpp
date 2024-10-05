#include <algorithm>
#include <ctype.h>
#include <list>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

using namespace std;

#define USER_INPUT_MAXLEN sizeof("000.000") - 1

unsigned short heat_temp;
unsigned short F_WHOLE;
float Z_START = 0.0;

static void genGCODE(void);

static void help(char *progname) {
  printf("usage: %s [Z_START] [HEAT_TEMP] [PRINT_SPEED]\n\n", progname);
  printf(
      "      Where Z_START is a value where the printer head starts\n"
      "      to print, it is the 3D Object first layer or first level.\n"
      "      This value is a floating-point number which can have up to\n"
      "      three digits before and after the decimal dot.\n"
      "      Typical use is to redirect output to dagoma0.g\n\n"
      "      And where HEAT_TEMP is the burning temperature. Whereas\n"
      "      225 deg is strongly encouraged 247 deg may also work. Argument\n"
      "      should be specified as a number between 200 and 250.\n\n"
      "      PRINT_SPEED is the printing speed. It relates to the\n"
      "      whole piece. It should be specified as a number comprised\n"
      "      between 90 and 600. %s advice 150.\n\n",
      progname);
  return;
}

static int parse_user_input1(char *usr_input, float *usrZ_START) {
  char *p_usrinp1;
  bool dotfound = false;
  bool minfound = false;

  if (strlen(usr_input) > USER_INPUT_MAXLEN)
    return -1;

  p_usrinp1 = usr_input;
  while (*p_usrinp1) {
    if (isdigit(*p_usrinp1) == 0) {
      if (!minfound && *p_usrinp1 == '-') {
        minfound = true;
        if (*++p_usrinp1 == '\0')
          return -1;
      } else if (dotfound) {
        return -1;
      } else if (*p_usrinp1 != '.') {
        return -1;
      } else if (p_usrinp1 == usr_input) {
        return -1;
      } else {
        dotfound = true;
        p_usrinp1++;
      }
    } else {
      p_usrinp1++;
    }
  }

  if (*p_usrinp1 != '\0')
    return -1;

  memset(usrZ_START, 0, sizeof(float));
  *usrZ_START = (float)atof(usr_input);

  return 0;
}

static int parse_user_input2(char *usr_input, unsigned short *heat_temp) {
  char *p;

  p = usr_input;
  do {
    if (isdigit(*p) == 0)
      return -1;
  } while (*++p != '\0');

  *heat_temp = (unsigned short)atoi(usr_input);
  if (*heat_temp < 200 || *heat_temp > 560)
    return -1;

  return 0;
}

static int parse_user_input3(char *usr_input) {
  char *p;

  p = usr_input;
  do {
    if (isdigit(*p) == 0)
      return -1;
  } while (*++p != '\0');

  memset(&F_WHOLE, 0, sizeof(unsigned short));

  F_WHOLE = (unsigned short)atoi(usr_input);
  if (F_WHOLE < 90 || F_WHOLE > 600)
    return -1;

  return 0;
}

int main(int argc, char **argv) {

  if (argc != 4) {
    help(argv[0]);
    return 1;
  } else if (parse_user_input1(argv[1], &Z_START) != 0 ||
             parse_user_input2(argv[2], &heat_temp) != 0 ||
             parse_user_input3(argv[3]) != 0) {
    printf("Bad user input\n");
    return 2;
  }

  genGCODE();

  return 0;
}

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

static float E = 0.000, Z = 0.000, Y = 0.000, X = 0.000;

#define EXTRUDE(E)                                                             \
  do {                                                                         \
    fprintf(stdout, "G1 F%hu E%.3f\n", F_WHOLE, E);                            \
    E += STEP_E1;                                                              \
  } while (0)

static void heat0deg(void) {
  /* Down heat to 0 deg */
  fprintf(stdout, "M106 S0\n");
  fprintf(stdout, "M109 S0\n");
  fprintf(stdout, "G92 E0.0\n");

  /* Return to origin position, auto home */
  fprintf(stdout, "G28\n");

  return;
}

static void init_marlin(void) {

  fprintf(stdout, "G90\n");
  fprintf(stdout, "G1 F%hu X%.3f Y%.3f Z%.3f\n", F_WHOLE, X_START, Y_START, Z_START);
  fprintf(stdout, "M106 S%hu\n", heat_temp);
  fprintf(stdout, "M109 S%hu\n", heat_temp);

  return;
}

class Branch : public std::list<float> {
private:
  void pOut(float y, bool noextrude) {
    char buffer[255];

    if (noextrude) {
      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, y);
    } else {
      E += STEP_E1;
      fprintf(stdout, "G1 F%hu E%.3f Y%.3f\n", F_WHOLE, E, y);
    }
    // printf("G1 F%hu Y%.3f\n", y);

    return;
  }
  void pOut2(float y, bool noextrude) {
    char buffer[255];

    if (noextrude == true) {
      fprintf(stdout, "G1 F%hu X%.3f\n", F_WHOLE,
              (y < 0.009 && y > -0.001) ? 0.0 : y);
    } else {
      E += STEP_E1;
      fprintf(stdout, "G1 F%hu E%.3f X%.3f\n", F_WHOLE, E,
              (y < 0.009 && y > -0.001) ? 0.0 : y);
    }
    // printf("G1 F%hu Y%.3f\n", y);

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

    E += STEP_E2;
    fprintf(stdout, "G1 F%hu E%.3f X%.3f Y%.3f\n", F_WHOLE, E, x, y);
    // printf("G1 F%hu X%.3f Y%.3f\n", x, y);

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

static void marlin_end(void) {
  fprintf(stdout, "G92 E0.0\n");
  fprintf(stdout, "G28\n");
  fprintf(stdout, "M106 S0\n");
  fprintf(stdout, "M109 S0\n");
  return;
}

static void genGCODE(void) {
  double X, XDEST, Y;
  double X_END_RIGHT;
  double X_END_LEFT;
  int flag;
  double X_END_STEP_1 = 0.400;
  int layer;
  float padY;
  Branch branch, subBranch, remainBranch, left2right, BaseLeft2Right,
      right2zero, back2front, headEdge;
  headSide sideRight, sideLeft, subSideLeft0, subSideLeft1, subSideLeft2,
      subSideLeft3, clipsSideLeft, clipsSideRight;
  headSide edgeSideLeft0, edgeSideLeft1, edgeSideRight0, edgeSideRight1;

  BaseLeft2Right.New2(X_START - 1, (float)abs(X_START - 1));
  left2right.New2(X_START, (float)abs(X_START));
  right2zero.New((float)abs(X_START), 0.000);
  back2front.New(Y_START, Y_EDGE);

  branch.New(Y_START, Y_END);
  subBranch.New(Y_START, Y_END + 4.000);
  remainBranch.New(Y_END + 4.000, Y_END);

  clipsSideLeft.New(
      std::make_pair((float)-23.0, (float)(Y_START / 2)),
      std::make_pair((float)((float)abs(X_START) - 28.0), (float)Y_START / 2));
  clipsSideRight.New(
      std::make_pair((float)-23.0, (float)(Y_START / 4)),
      std::make_pair((float)((float)abs(X_START) - 28.0), (float)Y_START / 4));

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

  subSideLeft2.New2(std::make_pair((float)abs(X_START), (float)(Y_END + 4.000)),
                    std::make_pair((float)((float)abs(X_START) - 16.0),
                                   (float)(Y_EDGE - 11.000)));

  subSideLeft3.New2(std::make_pair((float)abs(X_START), (float)(Y_END)),
                    std::make_pair((float)((float)abs(X_START) - 16.0),
                                   (float)(Y_EDGE - 11.000)));

  edgeSideLeft0.New2(std::make_pair((float)0.0, (float)(Y_EDGE)),
                     std::make_pair((float)(-10.0), (float)(Y_EDGE - 11.000)));

  edgeSideLeft1.New2(std::make_pair((float)0.0, (float)(Y_EDGE + 4.000)),
                     std::make_pair((float)(-10.0), (float)(Y_EDGE - 11.000)));

  edgeSideRight0.New(std::make_pair((float)(0.0), (float)(Y_EDGE + 4.000)),
                     std::make_pair((float)(10.0), (float)(Y_EDGE - 11.000)));

  edgeSideRight1.New(std::make_pair((float)(0.0), (float)(Y_EDGE)),
                     std::make_pair((float)(10.0), (float)(Y_EDGE - 11.000)));

  headEdge.New2(Y_EDGE, Y_EDGE + 4.000);

  Z = Z_START;

  /* Init marlin abs_pos */
  init_marlin();

  for (layer = 0, padY = 0.76; layer < 2; ++layer) {
    clipsSideRight.Go();
    clipsSideRight.ShiftY(padY);
    clipsSideRight.Return();
    clipsSideRight.ShiftY(padY);
    clipsSideRight.Go();
    clipsSideRight.ShiftY(padY);
    clipsSideRight.Return();
    clipsSideRight.ShiftY(padY);
    clipsSideRight.Go();
    clipsSideRight.ShiftY(padY);
    clipsSideRight.Return();
    clipsSideRight.ShiftY(padY);
    if (padY <= 0.9 && padY >= 0.7)
      padY = -0.76;
    else
      padY = 0.76;
    clipsSideRight.ShiftY(padY);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP / 2;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);
  }

  padY = 0.76;
  clipsSideRight.ShiftY(padY);

  for (layer = 0, padY = 0.76; layer < 4; ++layer) {

    clipsSideRight.Go();
    clipsSideRight.ShiftY(padY);
    clipsSideRight.Return();
    clipsSideRight.ShiftY(padY);
    clipsSideRight.Go();
    clipsSideRight.ShiftY(padY);
    clipsSideRight.Return();
    clipsSideRight.ShiftY(padY);
    if (padY <= 0.9 && padY >= 0.7)
      padY = -0.76;
    else
      padY = 0.76;

    clipsSideRight.ShiftY(padY);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP / 2;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);
  }

  E -= 0.450;
  fprintf(stdout, "G1 E%.3f\n", E);

  Z = Z_START;

  fprintf(stdout, "G1 F%hu X%.3f Y%.3f Z%.3f\n", F_WHOLE, X_START, Y_START, Z);

  E += 0.450;

  for (layer = 0, padY = 0.76; layer < 2; ++layer) {
    clipsSideLeft.Go();
    clipsSideLeft.ShiftY(padY);
    clipsSideLeft.Return();
    clipsSideLeft.ShiftY(padY);
    clipsSideLeft.Go();
    clipsSideLeft.ShiftY(padY);
    clipsSideLeft.Return();
    clipsSideLeft.ShiftY(padY);
    clipsSideLeft.Go();
    clipsSideLeft.ShiftY(padY);
    clipsSideLeft.Return();
    clipsSideLeft.ShiftY(padY);
    if (padY <= 0.9 && padY >= 0.7)
      padY = -0.76;
    else
      padY = 0.76;
    clipsSideLeft.ShiftY(padY);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP / 2;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);
  }

  padY = 0.76;
  clipsSideLeft.ShiftY(padY);

  for (layer = 0, padY = 0.76; layer < 4; ++layer) {

    clipsSideLeft.Go();
    clipsSideLeft.ShiftY(padY);
    clipsSideLeft.Return();
    clipsSideLeft.ShiftY(padY);
    clipsSideLeft.Go();
    clipsSideLeft.ShiftY(padY);
    clipsSideLeft.Return();
    clipsSideLeft.ShiftY(padY);
    if (padY <= 0.9 && padY >= 0.7)
      padY = -0.76;
    else
      padY = 0.76;

    clipsSideLeft.ShiftY(padY);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP / 2;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);
  }

  E -= 0.450;
  fprintf(stdout, "G1 E%.3f\n", E);

  Z = Z_START;

  fprintf(stdout, "G1 F%hu X%.3f Y%.3f Z%.3f\n", F_WHOLE, X_START, Y_START, Z);

  E += 0.450;

  BaseLeft2Right.Go2();

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  BaseLeft2Right.Return2();

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  BaseLeft2Right.Go2();

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  BaseLeft2Right.Return2();

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  fprintf(stdout, "G1 Y%.3f\n", Y_START - 0.80 * 1);

  BaseLeft2Right.Go2();

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  BaseLeft2Right.Return2();

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  BaseLeft2Right.Go2();

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  BaseLeft2Right.Return2();

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  fprintf(stdout, "G1 Y%.3f\n", Y_START - 0.80 * 2);

  // ##

  BaseLeft2Right.Go2();

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  BaseLeft2Right.Return2();

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  BaseLeft2Right.Go2();

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  BaseLeft2Right.Return2();

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  fprintf(stdout, "G1 Y%.3f\n", Y_START - 0.80 * 3);

  BaseLeft2Right.Go2();

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  BaseLeft2Right.Return2();

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  BaseLeft2Right.Go2();

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  BaseLeft2Right.Return2();

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z -= Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  fprintf(stdout, "G1 Y%.3f\n", Y_START - 0.80 * 4);

  BaseLeft2Right.Go2();

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  BaseLeft2Right.Return2();

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  BaseLeft2Right.Go2();

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  BaseLeft2Right.Return2();

  Z = Z_START;

  fprintf(stdout, "G1 F%hu X%.3f Y%.3f Z%.3f\n", F_WHOLE, X_START, Y_START, Z);

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
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP / 2;
    fprintf(stdout, "G1 Z%.3f\n", Z);

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
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP / 2;
    fprintf(stdout, "G1 Z%.3f\n", Z);
  }

  for (layer = LAYER_BRANCH - (LAYER_BRANCH / 10); --layer;) {
    branch.Go();
    subSideLeft1.Go();
    subSideLeft1.Return();
    remainBranch.Return();
    subSideLeft0.Go();
    subSideLeft0.Return();
    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    subBranch.Return();

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);
  }

  left2right.Go2(true);

  for (layer = LAYER_BRANCH - (LAYER_BRANCH / 10); --layer;) {
    Z -= Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z -= Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z -= Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z -= Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z -= Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z -= Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);
  }

  E -= 0.450;
  fprintf(stdout, "G1 E%.3f\n", E);

  E += 0.450;

  fprintf(stdout, "G4 S7\n");

  E += STEP_E1;
  E += STEP_E1;
  E += STEP_E1;
  E += STEP_E1;

  for (layer = LAYER_BRANCH - (LAYER_BRANCH / 10); --layer;) {
    branch.Go();
    subSideLeft3.Go();
    subSideLeft3.Return();
    remainBranch.Return();
    subSideLeft2.Go();
    subSideLeft2.Return();

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    subBranch.Return();

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);
  }

  E -= 0.450;
  fprintf(stdout, "G1 E%.3f\n", E);

  right2zero.Go2(true);
  back2front.Go(true);

  for (layer = LAYER_BRANCH - (LAYER_BRANCH / 9); --layer;) {
    Z -= Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z -= Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z -= Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z -= Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z -= Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z -= Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);
  }

  E += 0.450;

  fprintf(stdout, "G4 S7\n");

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
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

  } while (--layer);

  headEdge.Go();
  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  Z += Z_STEP;
  fprintf(stdout, "G1 Z%.3f\n", Z);

  headEdge.Return();

  E -= 0.450;
  fprintf(stdout, "G1 E%.3f\n", E);

  fprintf(stdout, "G1 F%hu X%.3f Y%.3f Z%.3f\n", F_WHOLE, X_START, Y_START, Z);

  E += 0.450;

  E += STEP_E1;
  fprintf(stdout, "G4 S120\n");

  E += STEP_E1;
  E += STEP_E1;

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
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP / 2;
    fprintf(stdout, "G1 Z%.3f\n", Z);

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
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP / 2;
    fprintf(stdout, "G1 Z%.3f\n", Z);
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
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP / 2;
    fprintf(stdout, "G1 Z%.3f\n", Z);

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
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP;
    fprintf(stdout, "G1 Z%.3f\n", Z);

    Z += Z_STEP / 2;
    fprintf(stdout, "G1 Z%.3f\n", Z);
  }

  marlin_end();

  branch.clear();
  subBranch.clear();
  remainBranch.clear();
  left2right.clear();
  right2zero.clear();
  back2front.clear();
  headEdge.clear();
  sideRight.clear();
  sideLeft.clear();
  subSideLeft0.clear();
  subSideLeft1.clear();
  subSideLeft2.clear();
  subSideLeft3.clear();
  clipsSideLeft.clear();
  clipsSideRight.clear();
  edgeSideLeft0.clear();
  edgeSideLeft1.clear();
  edgeSideRight0.clear();
  edgeSideRight1.clear();

  return;
}
