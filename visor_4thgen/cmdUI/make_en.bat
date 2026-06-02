@echo off
REM This script should be run in x64 Native Tools Command Prompt
REM You have to run "rc resource.rc" to have "resource.res" file
rc resource.rc
cl Visor4thGenGenGCODE.cpp Visor4thGenMain.cpp resource.res /Fe:VisorWithGlass_dagoma0_g.exe
del *.obj *.res
