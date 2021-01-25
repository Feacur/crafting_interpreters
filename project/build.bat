@echo off
chcp 65001

set debug=dummy

set project=%1
if [%project%] == [] (
	echo provide project [*.csproj]
	exit /b 0
)
set project=%project:"=%

rem > PREPARE TOOLS

set VSLANG=1033

rem > OPTIONS
set conf=
if defined debug (
	set conf=%conf% -c Debug
) else (
	set conf=%conf% -c Release
)

rem > COMPILE AND LINK
set timeStart=%time%
dotnet build %project% -o "../bin" %conf%
set timeStop=%time%

rem > REPORT
echo start: %timeStart%
echo stop:  %timeStop%
