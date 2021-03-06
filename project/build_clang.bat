@echo off
chcp 65001

rem set debug=dummy
rem set unity_build=dummy

rem https://clang.llvm.org/docs/index.html
rem https://clang.llvm.org/docs/CommandGuide/clang.html
rem https://clang.llvm.org/docs/UsersManual.html
rem https://clang.llvm.org/docs/ClangCommandLineReference.html
rem https://lld.llvm.org/windows_support.html
rem https://docs.microsoft.com/cpp/build/reference/linker-options

rem > PREPARE TOOLS
set "PATH=%PATH%;C:/Program Files/LLVM/bin"

if not defined unity_build (
	set VSLANG=1033
	pushd "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Auxiliary/Build"
	call "vcvarsall.bat" x64
	popd
)

rem > OPTIONS
set includes=-I".."
set defines=-D_CRT_SECURE_NO_WARNINGS
set libs=
set warnings=-Werror -Weverything -Wno-switch-enum -Wno-float-equal
set compiler=-fno-exceptions -fno-rtti
set linker=-nologo -WX -subsystem:console

if defined debug (
	set defines=%defines% -DLOX_TARGET_DEBUG
	set compiler=%compiler% -O0 -g
	set linker=%linker% -debug:full
) else (
	set compiler=%compiler% -O3
	set linker=%linker% -debug:none
)

set compiler=%compiler% %includes% %defines%
set linker=%linker% %libs%

if defined unity_build (
	set linker=-Wl,%linker: =,%
)

rem > COMPILE AND LINK
set timeStart=%time%
cd ..
if not exist bin mkdir bin
cd bin

if not exist temp mkdir temp

if defined unity_build (
	clang -std=c99 "../project/unity_build.c" -o"interpreter.exe" %compiler% %warnings% %linker%
) else ( rem alternatively, compile a set of translation units
	if exist "./temp/unity_build*" del ".\temp\unity_build*"
	clang -std=c99 -c "../code/*.c" %compiler% %warnings%
	move ".\*.o" ".\temp"
	lld-link "./temp/*.o" libcmt.lib -out:"interpreter.exe" %linker%
)

cd ../project
set timeStop=%time%

rem > REPORT
echo start: %timeStart%
echo stop:  %timeStop%
