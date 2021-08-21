@echo off

DEL *.obj
CL /c /EHsc *.cpp
LINK *.obj user32.lib /OUT:dumb.dummy
DEL *.exe
DEL *.obj
MOVE dumb.dummy wfilesearcher.exe
