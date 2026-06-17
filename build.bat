@echo off
cd /d "%~dp0"

set APP_NAME=menuv1

set GCC="C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin\arm-none-eabi-gcc.exe"
set OBJCOPY="C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin\arm-none-eabi-objcopy.exe"
set SIZE="C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin\arm-none-eabi-size.exe"

:: SDK root - two levels up from examples/<app>/


set CPU=-mcpu=cortex-m0 -mthumb
set INC=-I src\include -I src\include
set CFLAGS=%CPU% %INC% -Os -ffunction-sections -fdata-sections -Wall -std=c11

if not exist build mkdir build

echo [1/9] startup.s  (vaporware)
%GCC% %CPU% -x assembler-with-cpp -c src\startup.s -o build\startup.o || goto :error

echo [2/9] system.c   (vaporware)
%GCC% %CFLAGS% -c src\system.c  -o build\system.o  || goto :error

echo [3/9] display.c  (vaporware)
%GCC% %CFLAGS% -c src\display.c -o build\display.o || goto :error

echo [4/9] vape.c     (vaporware)
%GCC% %CFLAGS% -c src\vape.c    -o build\vape.o    || goto :error

echo [5/9] button.c   (vaporware)
%GCC% %CFLAGS% -c src\button.c  -o build\button.o  || goto :error

echo [6/9] battery.c  (vaporware)
%GCC% %CFLAGS% -c src\battery.c -o build\battery.o || goto :error

echo [7/9] nv.c       (vaporware)
%GCC% %CFLAGS% -c src\nv.c      -o build\nv.o      || goto :error

echo [8/9] app.c      (vaporware)
%GCC% %CFLAGS% -c src\app.c     -o build\app.o     || goto :error

echo [14/14] sounddriver.c     (app)
%GCC% %CFLAGS% -c src\sounddriver.c -o build\sounddriver.o || goto :error

echo [9/9] main.c     (app)
%GCC% %CFLAGS% -c src\main.c -o build\main.o || goto :error

echo [10/10] pong.c     (app)
%GCC% %CFLAGS% -c src\pong.c -o build\pong.o || goto :error

echo [11/11] raycaster.c     (app)
%GCC% %CFLAGS% -c src\raycaster.c -o build\raycaster.o || goto :error

echo [12/12] fungraphics.c     (app)
%GCC% %CFLAGS% -c src\fungraphics.c -o build\fungraphics.o || goto :error

echo [13/12] brickbreaker.c     (app)
%GCC% %CFLAGS% -c src\brickbreaker.c -o build\brickbreaker.o || goto :error


echo Linking...
%GCC% %CPU% -T n32g031.ld -Wl,--gc-sections -Wl,-Map=build\%APP_NAME%.map -nostdlib -lnosys ^
  build\startup.o build\system.o build\pong.o build\fungraphics.o build\display.o build\vape.o ^
  build\button.o build\battery.o build\brickbreaker.o build\sounddriver.o build\nv.o build\raycaster.o build\app.o ^
  build\main.o ^
  -o build\%APP_NAME%.elf || goto :error

%OBJCOPY% -O binary build\%APP_NAME%.elf build\%APP_NAME%.bin || goto :error
%OBJCOPY% -O ihex   build\%APP_NAME%.elf build\%APP_NAME%.hex || goto :error
%SIZE% build\%APP_NAME%.elf

echo.
echo Build SUCCESS: build\%APP_NAME%.bin
goto :eof

:error
echo BUILD FAILED

pause