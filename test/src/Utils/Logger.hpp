#pragma once
#include <stdio.h>

#define VA_ARGS(...) , ##__VA_ARGS__
#define WriteLog(x, y, ...) printf(y VA_ARGS(__VA_ARGS__)); printf("\n")
enum LogLevels
{
	LL_None,
	LL_Info,
	LL_Warn,
	LL_Error,
	LL_Debug,
	LL_All
};
