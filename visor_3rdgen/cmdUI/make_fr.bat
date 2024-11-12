rc resource.rc
cl Visor3rdGenGenGCODE.cpp Visor3rdGenMain.cpp /D LANG_FR /link resource.res /Fe:Visor3rdGen_GenGCODE.exe
del *.obj *.res