#pragma once

#include "stdio.h"
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <future>
#include <chrono>

// Global variables
#define _InGarage 0x08E3614
#define _HasFocus 0x0882E41
#define _IsPaused 0x00874E58
#define _EngineVolSlider 0x904D80
#define _MasterVolSlider 0x904D98
#define _LoadState 0x08ECDC4
#define _GameState 0x08FC730
uintptr_t gBase;
HANDLE gConsoleHandle;

bool gGlobalEnable = true;
bool f1WasPressed = false;
bool hasPlayed = false, triedToPlay = false, hasInit = false, skipGear = true, gPlaySnd = false;
bool IsPaused = false, IsOnFocus = true, InGarage = true, IsLoading = false;
DWORD TheGameFlowManager = 6;
//3 FrontEnd, 6 gameplay, 4/5 load

float IniMult = 0;
float gTransVolMod = 1.0f;
bool gDetectDiff = true;
uint16_t gShiftValue;
float gPitchModifier = 0.0f, gPitchModTarget = 0.0f;
bool gWhineEnabled = false;
bool gWhineAll = false;
float gSpeed = 0.0f;
float gSpeedLast = 0.0f;
float gPitchMin = 0.3, gPitchMax = 1.0;
float gKMmax = 330.0;
float sndTarget = 1.0f, sndCurrentLvl = 1.0f;
float gEngineVolSlider = 1.0f, gMasterVolSlider = 1.0f;
float gReverseVol = 0.070f;
uint8_t gGear, gGearOld = 1;
char* gCarNameChar;
uintptr_t gTrannyCode = 0x0;
std::string gCarName = "";
std::string gTrannyPart = "";
std::string CarInput;
std::vector<std::string> carList;
uintptr_t gGearInstruction = 0;