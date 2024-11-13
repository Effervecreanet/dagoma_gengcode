rc resource.rc
cl Visor3rdGenGenGCODE.cpp Visor3rdGenMain.cpp /link resource.res /Fe:Visor3rdGen_GenGCODE_EN.exe
del *.obj *.res