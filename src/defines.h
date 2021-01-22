#pragma once
#include <windows.h>

#define HOTKEY_START    0x1001
#define MAX_HOTKEY_SIZE 5

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

struct KeyAuto
{
    int index = 0;
    int span = 0;
    bool isRightStop = false;
};

struct KeyExcept
{
    int index = 0;
    int time = 0;
};

struct Config
{
    int index = 0;

    KeyAuto keyAuto[5];
    KeyExcept keyExcept[5];
};
