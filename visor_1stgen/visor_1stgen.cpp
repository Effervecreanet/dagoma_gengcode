#include <algorithm>
#include <ctype.h>
#include <list>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

static float E, X, Y, Z;

#define EXTRUDE1(E) do {                                    \
                        fprintf(stdout, "G1 E%.3f\n", E);   \
                        E += STEP_E1;                       \
                    } while(0)


#define EXTRUDE(E) do {                                             \
                        fprintf(stdout, "G1 E%.3f\n", E);           \
                        E += STEP_E2;                               \
                    } while(0)

class Branch : public std::list<float> {
private:
	void pOut(float y, bool noextrude) {
		if (noextrude) {
			fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, y);
		}
		else {
			E += STEP_E3;
			fprintf(stdout, "G1 F%hu E%.3f Y%.3f\n", F_WHOLE, E, y);
		}

		return;
	}
	void pOut2(float y, bool noextrude) {
		if (noextrude == true) {
			fprintf(stdout, "G1 F%hu X%.3f\n", F_WHOLE, (y < 0.009 && y > -0.001) ? 0.0 : y);
		}
		else {
			E += STEP_E3;
			fprintf(stdout, "G1 F%hu E%.3f X%.3f\n", F_WHOLE, E,
				(y < 0.009 && y > -0.001) ? 0.0 : y);
		}

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

static void heat0deg(void) {
  /* Down heat to 0 deg */
  fprintf(stdout, "M106 S0\n");
  fprintf(stdout, "M109 S0\n");
  fprintf(stdout, "G92 E0.0\n");

  /* Return to origin position, auto home */
  fprintf(stdout, "G28\n");

  return;
}

void genGCODE(void) {
	unsigned short F;
	int layer = 0;
	Branch BaseLeft2Right;
        struct tm *tmv;
        time_t timet;
        char buff[512];

        printf("; Generated at ");
        time(&timet);
        tmv = localtime(&timet);
        strftime(buff, 512,"%a %b %e %H:%M:%S %Y", tmv);
        printf("%s\n", buff);
        printf("; Author: Franck Lesage (effervecreanet@orange.fr) http://www.effervecrea.net\n");

	X = X_START;
	Y = Y_START;
	Z = Z_START;

	E = 0.0;
	fprintf(stdout, "G90\n");

	fprintf(stdout, "G1 F%hu X%.3f Y%.3f Z%.3f\n", F_WHOLE, X_START, Y_START, Z);
	fprintf(stdout, "M106 S%hu\n", heat_temp);
	fprintf(stdout, "M109 S%hu\n", heat_temp);

	BaseLeft2Right.New2(X_START - 1, (float)abs(X_START - 1));
	BaseLeft2Right.Go2();

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	BaseLeft2Right.Return2();

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	BaseLeft2Right.Go2();

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	BaseLeft2Right.Return2();

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	fprintf(stdout, "G1 Y%.3f\n", Y_START - 0.80 * 1);
	

	BaseLeft2Right.Go2();

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	BaseLeft2Right.Return2();

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	BaseLeft2Right.Go2();

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	BaseLeft2Right.Return2();

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	fprintf(stdout, "G1 Y%.3f\n", Y_START - 0.80 * 2);
	
	// ##

	BaseLeft2Right.Go2();

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	BaseLeft2Right.Return2();

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	BaseLeft2Right.Go2();

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	BaseLeft2Right.Return2();

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	fprintf(stdout, "G1 Y%.3f\n", Y_START - 0.80 * 3);
	

	BaseLeft2Right.Go2();

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	BaseLeft2Right.Return2();

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	BaseLeft2Right.Go2();

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	BaseLeft2Right.Return2();

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z -= STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	fprintf(stdout, "G1 Y%.3f\n", Y_START - 0.80 * 4);
	

	BaseLeft2Right.Go2();

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	BaseLeft2Right.Return2();

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	BaseLeft2Right.Go2();

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	Z += STEP_Z;
	fprintf(stdout, "G1 Z%.3f\n", Z);
	

	BaseLeft2Right.Return2();

	fprintf(stdout, "G1 F%hu X%.3f Y%.3f Z%.3f\n", F_WHOLE, X_START, Y_START, Z);

	/* Trip until LAYER_VISOR reached */

	while (layer < LAYER_VISOR) {
		/* Go */

		/* Branches are less high than visor */

		if (layer < LAYER_BRANCH) {
			/* Left branch go */

			while(Y > Y_END) {
				EXTRUDE1(E);
				
				fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);
				
				Y -= STEP_Y;
			}
		}

		/* Triangle left side */

		do {
			EXTRUDE(E);

            
			fprintf(stdout, "G1 F%hu X%.3f\n", F_WHOLE, X);
			
			X += STEP_X;
			
			fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);
			
			Y -= STEP_Y;

		} while (X < 0.000);

		/* Triangle right side */

		do {
			EXTRUDE(E);

            
			fprintf(stdout, "G1 F%hu X%.3f\n", F_WHOLE, X);
			
			X += STEP_X;
			
			fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);
			
			Y += STEP_Y;

		} while (X < 70.000);

		/* level up after bed layer */

		if (layer > 0) {
			Z += STEP_Z;
			
			fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);
			

			Z += STEP_Z;
			
			fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);
			

			Z += STEP_Z;
			
			fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);
			

			Z += STEP_Z / 2;
			
			fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);
			
		}


		/* Branches are less high than visor */

		if (layer < LAYER_BRANCH) {
			/* Right branch go */

			while(Y < Y_START) {
				EXTRUDE1(E);
				
				fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);
				
				Y += STEP_Y;
			}

			/* Right branch return */

			while(Y > Y_END) {
				EXTRUDE1(E);
				
				fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);
				
				Y -= STEP_Y;
			}
		}

		/* Return */

		/* Triangle right side */

		do {
			EXTRUDE(E);
            
			fprintf(stdout, "G1 F%hu X%.3f\n", F_WHOLE, X);
			
			X -= STEP_X;
			
			fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);
			
			Y -= STEP_Y;

		} while (X > 0.000);

		/* Triangle left side */
		
		do {
			EXTRUDE(E);
            
			fprintf(stdout, "G1 F%hu X%.3f\n", F_WHOLE, X);
			
			X -= STEP_X;
			
			fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);
			
			Y += STEP_Y;

		} while (X > -70.000);	

		layer++;

		if (layer < LAYER_BRANCH) {
			/* Left branch return */

			while(Y < Y_START) {
				EXTRUDE1(E);
				
				fprintf(stdout, "G1 F%hu Y%.3f\n", F_WHOLE, Y);
				
				Y += STEP_Y;
			}
		}
			
		/* level up */

		Z += STEP_Z;
		
		fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);
		

		Z += STEP_Z;
		
		fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);
		

		Z += STEP_Z;
		
		fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);
				

		Z += STEP_Z / 2;
		
		fprintf(stdout, "G1 F%hu Z%.3f\n", F_WHOLE, Z);
				
	}

	heat0deg();

	return;
}
