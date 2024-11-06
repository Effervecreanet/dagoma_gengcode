#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define USER_INPUT_MAXLEN sizeof("000.000") - 1

unsigned short heat_temp;
unsigned short F_WHOLE;
float Z_START = 0.0;


static void genGCODE(void);
static void stamp(void);

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

#define Y_START -64.000

#define Z_STEP 0.100
#define Y_STEP 0.100
#define X_STEP 0.100

#define STEP_E1 0.020

#define X_EDGE 87

#define BED_LAYER_MAX 2
#define BASE_LAYER_MAX 2
#define HIGH_LAYER_MAX 280

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

static void stamp(void)
{
        struct tm *tmv;
        time_t timet;
        char buff[128];

        fprintf(stdout, "; Generated at ");
        time(&timet);
        tmv = localtime(&timet);
        ZeroMemory(buff, 128);
        strftime(buff, 128,"%a %b %e %H:%M:%S %Y", tmv);
        fprintf(stdout, "%s\n", buff);
        fprintf(stdout, "; Author: Franck Lesage (effervecreanet@orange.fr) http://www.effervecrea.net\n");


        return;
}

static void init_marlin(void) {

  fprintf(stdout, "G90\n");
  fprintf(stdout, "G1 F%hu X70.000 Y-64.000 Z%.3f\n", F_WHOLE, Z_START);
  fprintf(stdout, "M106 S%hu\n", heat_temp);
  fprintf(stdout, "M109 S%hu\n", heat_temp);

  return;
}

static void genGCODE(void) {
  double E, X, XDEST, Y, Z;
  double X_END_RIGHT;
  double X_END_LEFT;
  int flag;
  int layer;
  double X_END_STEP_1 = 0.400;

  stamp();
  init_marlin();

  X_END_RIGHT = 70.000;
  X_END_LEFT = -70.000;

  Y = Y_START;
  Z = Z_START;
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

      EXTRUDE(E);

      fprintf(stdout, "G1 F%hu X%.3f\n", F_WHOLE, X);

      EXTRUDE(E);
      Y += Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      EXTRUDE(E);
      Y += Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      EXTRUDE(E);
      Y += Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);
    }

    for (XDEST = -X; X > XDEST; X -= X_STEP) {

      fprintf(stdout, "G1 F%hu X%.3f\n", F_WHOLE, X);

      EXTRUDE(E);
    }

    for (flag = 0; Y > -64.000;) {
      if (X < -X_EDGE)
        flag = 1;

      if (flag == 1)
        X += X_STEP;
      else
        X -= X_STEP;

      EXTRUDE(E);

      fprintf(stdout, "G1 F%hu X%.3f\n", F_WHOLE, X);

      EXTRUDE(E);
      Y -= Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      EXTRUDE(E);
      Y -= Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      EXTRUDE(E);
      Y -= Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);
    }

    for (XDEST = fabs(X); X < XDEST; X += X_STEP) {

      fprintf(stdout, "G1 F%hu X%.3f\n", F_WHOLE, X);

      EXTRUDE(E);
    }

    if (layer == 0)
      break;

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);
  }

  layer = BASE_LAYER_MAX;

  while (layer-- > 0) {
    flag = 0;
    for (; Y < 73.000;) {
      for (; X > X_END_LEFT; X -= X_STEP) {

        fprintf(stdout, "G1 F%hu X%.3f\n", F_WHOLE, X);

        EXTRUDE(E);
      }

      Y += Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      Y += Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      Y += Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      Y += Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      Y += Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      Y += Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      for (; X < X_END_RIGHT; X += X_STEP) {

        fprintf(stdout, "G1 F%hu X%.3f\n", F_WHOLE, X);

        EXTRUDE(E);
      }

      Y += Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      Y += Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      Y += Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      Y += Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      Y += Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      Y += Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      if (X > X_EDGE)
        flag = 1;

      if (flag == 1) {
        X_END_RIGHT -= X_END_STEP_1;
        X_END_LEFT += X_END_STEP_1;
      } else {
        X_END_RIGHT += X_END_STEP_1;
        X_END_LEFT -= X_END_STEP_1;
      }
    }

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    flag = 0;

    for (; Y >= Y_START;) {
      for (; X > X_END_LEFT; X -= X_STEP) {

        fprintf(stdout, "G1 F%hu X%.3f\n", F_WHOLE, X);

        EXTRUDE(E);
      }

      Y -= Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      Y -= Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      Y -= Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      Y -= Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      Y -= Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      Y -= Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      for (; X < X_END_RIGHT; X += X_STEP) {

        fprintf(stdout, "G1 F%hu X%.3f\n", F_WHOLE, X);

        EXTRUDE(E);
      }

      Y -= Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      Y -= Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      Y -= Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      Y -= Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      Y -= Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      Y -= Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      if (X > X_EDGE)
        flag = 1;

      if (flag == 1) {
        X_END_RIGHT -= X_END_STEP_1;
        X_END_LEFT += X_END_STEP_1;
      } else {
        X_END_RIGHT += X_END_STEP_1;
        X_END_LEFT -= X_END_STEP_1;
      }
    }

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);
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

      EXTRUDE(E);

      fprintf(stdout, "G1 F%hu X%.3f\n", F_WHOLE, X);

      EXTRUDE(E);
      Y += Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      EXTRUDE(E);
      Y += Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      EXTRUDE(E);
      Y += Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);
    }

    for (XDEST = -X; X > XDEST; X -= X_STEP) {

      fprintf(stdout, "G1 F%hu X%.3f\n", F_WHOLE, X);

      EXTRUDE(E);
    }

    for (flag = 0; Y > -64.000;) {
      if (X < -X_EDGE)
        flag = 1;

      if (flag == 1)
        X += X_STEP;
      else
        X -= X_STEP;

      EXTRUDE(E);

      fprintf(stdout, "G1 F%hu X%.3f\n", F_WHOLE, X);

      EXTRUDE(E);
      Y -= Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      EXTRUDE(E);
      Y -= Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);

      EXTRUDE(E);
      Y -= Y_STEP;

      fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);
    }

    for (XDEST = fabs(X); X < XDEST; X += X_STEP) {

      fprintf(stdout, "G1 F%hu X%.3f\n", F_WHOLE, X);

      EXTRUDE(E);
    }

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);

    Z += Z_STEP;

    fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);
  }

  heat0deg();

  return;
}
