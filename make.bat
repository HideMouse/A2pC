@echo off
set SOURCES=./src/*.cpp
set INCLUDES=-I./src/include

g++ -std=c++20 %INCLUDES% %SOURCES% -o ./build/a2pc.exe