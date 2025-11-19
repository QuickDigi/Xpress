@echo off

:: Create production folder if not exists
if not exist out mkdir out

g++ -std=c++17 routes/main.cpp package/xpresspp/src/app.cpp ^
    -Iinclude -D_WIN32_WINNT=0x0A00 -lws2_32 ^
    -o out/server.exe
