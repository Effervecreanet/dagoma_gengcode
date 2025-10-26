@echo off
REM This script should be run in x64 Native Tools Command Prompt
REM You have to run "rc resource.rc" to have "resource.res" file
echo cl /DLANG_FR VisorGenGCODE.cpp VisorMain.cpp resource.res /Fe:VisiereSansCarreaux_dagoma0_g.exe
cl /DLANG_FR VisorGenGCODE.cpp VisorMain.cpp resource.res /Fe:VisiereSansCarreaux_dagoma0_g.exe
