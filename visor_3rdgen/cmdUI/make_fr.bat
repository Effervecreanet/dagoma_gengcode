@echo off
REM This script should be run in x64 Native Tools Command Prompt
REM You have to run "rc resource.rc" to have "resource.res" file
REM rc resource.rc
cl /D LANG_FR Visor3rdGenGenGCODE.cpp Visor3rdGenMain.cpp resource.res /Fe:VisiereAvecCarreaux_dagoma0_g.exe
del *.obj *.res
