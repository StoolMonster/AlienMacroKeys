#pragma once

#include "resource.h"
#include "hid.h"
#include <shellapi.h>
#include <iostream>

#define AW_KB_VID       "0x0D62"
#define AW_KB_PID       "0xCCBC"
#define AW_USAGEPAGE    0x0c
#define AW_USAGE        0x01

#define MACROA          0x51
#define MACROB          0x52
#define MACROC          0x53
#define MACROD          0x54
#define MACROE          0x55

#define READ_THREAD_TIMEOUT     1000

DWORD StartMonitor(WORD targetVID, WORD targetPID);
void HandleMacroKey(USAGE macroKey);