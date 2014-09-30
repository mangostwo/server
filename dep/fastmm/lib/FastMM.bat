
lib.exe /def:FastMM.def
lib.exe /def:FastMMx64.def /machine:X64

del *.exp
move FastMM.lib .\..\
move FastMMx64.lib .\..\
