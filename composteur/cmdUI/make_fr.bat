@echo off
REM This script should be run in x64 Native Tools Command Prompt
REM You have to run "rc resource.rc" to have "resource.res" file
echo cl /DLANG_FR GarbageGenGCODE.cpp GarbageMain.cpp resource.res /Fe:Composteur_dagoma0_g.exe
cl /DLANG_FR GarbageGenGCODE.cpp GarbageMain.cpp resource.res /Fe:Composteur_dagoma0_g.exe

