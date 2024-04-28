# EFFERVECREANET:DAGOMA/MAGIS(NEVA) (https://dagoma.fr) 3D Objects Source

_3DOBJECTS:_
-	garbage collector for a small kitchen (composteur.c)
-	COVID-19: visor first gen (visor_1stgen.cpp)
-	COVID-19: visor second gen (visor_2ndgen.cpp)

Source code for generating several 3D objects to print with a Dagoma Magis and Neva.
You have to input the Z_START because the start on Z axis is not the same on all the 3d printer Magis.
Once you have specified it with temperature and speed too, redirect the output to a file whose filename
is *dagoma0.g* and put it in a SD card you insert in the 3d printer Magis. These 3d objects source code
are available "AS IS" and are the official way of generating the object you can find in my weblog. Once
you compiled *.c or *.cpp files, you generate the 3D Gcode with the following command:

./composteur -4.999 225 125 > dagoma0.g

./visor_2ndgen -4.999 254 107 > dagoma0.g

A win32 user interface is also featured in the directories cmdUI.

More info on https://www.behance.net/francklesage

http://www.effervecrea.net

Franck Lesage
effervecreanet@gmail.com
