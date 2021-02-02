@echo off
chcp 65001

set debug=dummy
rem set unity_build=dummy

rem https://docs.microsoft.com/cpp/build/reference/compiler-options
rem https://docs.microsoft.com/cpp/build/reference/linker-options

rem > PREPARE TOOLS
rem set "PATH=%PATH%;C:/Program Files/LLVM/bin"
rem possible `clang-cl` instead `cl -std:c11`
rem possible `lld-link` instead `link`

set VSLANG=1033
pushd "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Auxiliary/Build"
call "vcvarsall.bat" x64
popd

rem > OPTIONS
set includes=-I".."
set defines=-D_CRT_SECURE_NO_WARNINGS
set libs=
set warnings=-WX -W4
set compiler=-nologo -diagnostics:caret -EHa- -GR- %includes% %defines% -Fo"./temp/"
set linker=-nologo -WX -subsystem:console %libs%

if defined debug (
	set compiler=%compiler% -Od -Zi
	set linker=%linker% -debug:full
) else (
	set compiler=%compiler% -O2
	set linker=%linker% -debug:no
)

if defined unity_build (
	set linker=-link %linker%
)

rem > COMPILE AND LINK
set timeStart=%time%
cd ..
if not exist bin mkdir bin
cd bin

if not exist temp mkdir temp

if defined unity_build (
	cl -std:c11 "../project/unity_build.c" -Fe"interpreter.exe" %compiler% %warnings% %linker%
) else ( rem alternatively, compile a set of translation units
	if exist "./temp/unity_build*" del ".\temp\unity_build*"
	cl -std:c11 -c "../code/*.c" %compiler% %warnings%
	link "./temp/*.obj" -out:"interpreter.exe" %linker%
)

cd ../project
set timeStop=%time%

rem > REPORT
echo start: %timeStart%
echo stop:  %timeStop%
