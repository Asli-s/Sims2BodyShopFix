@echo off
rem Builds BodyShopFix.exe with Visual Studio 2022 (Desktop C++ workload).
rem The patcher asks for administrator rights because Body Shop lives in Program Files.
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" >nul || exit /b 1
if not exist build mkdir build
cl /nologo /std:c++17 /O2 /W4 /EHsc /MT /DUNICODE /D_UNICODE src\bodyshop_fix.cpp ^
   /Fo:build\ /Fe:build\BodyShopFix.exe ^
   /link bcrypt.lib advapi32.lib user32.lib comdlg32.lib /SUBSYSTEM:WINDOWS /MANIFEST:EMBED /MANIFESTINPUT:src\app.manifest /MANIFESTUAC:"level='requireAdministrator'"
