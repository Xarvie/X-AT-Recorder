#pragma once
#ifndef SOURCE_H
#define SOURCE_H
#include <Windows.h>
#include <string.h>
class Window;
extern "C"
{
__declspec(dllexport)Window* loadPlugin(HWND win, int x, int y, int w, int d);
__declspec(dllexport)const char* getPluginName();
}
#endif