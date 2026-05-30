#include "stdafx.h"
#include "stdio.h"
#include "Utils.h"
#include "memscan.h"
#include <string>
#include <algorithm>
#include <future>
#include <chrono>


#define MINIAUDIO_IMPLEMENTATION
#include "..\includes\miniaudio.h"

void InitPatterns()
{
	uintptr_t base = 0;
	size_t size = 0;

	if (!GetModuleInfoData(nullptr, base, size))
		return;

	gGearInstruction = FindPattern(
		base,
		size,
		"8B 4E 38 3B C8 74 ?? 7D 0F 83 4E 2C 01"
	);
}

uint16_t GetShiftValue()
{
	DWORD address = gBase + 0x00977FA4;
	unsigned int offsets[] = { address, 0x0, 0x38, 0x244, 0x4, 0x1AC};
	uint16_t* ptr = (uint16_t*)GetPtr(offsets, 6);
	if (ptr)
	{
		if (*ptr > 4096)
		{
			return 4096;
		}
		if (*ptr < 0)
		{
			return 0;
		}
		return *ptr;
	}

	return 0;
}

float GetSpeed()
{
	DWORD addressSpeed = gBase + 0x09C346C;
	float metersPerSecond = (*(float*)addressSpeed);
	return 3.59999 * metersPerSecond;
}

uint8_t GetGear()
{
	//0 = reverse
	//1 = neutral
	//2 = 1 
	//3 = 2
	//4 = 3
	//5 = 4
	//6 = 5
	//7 = 6
	//8 = 7

	DWORD address = gBase + 0x00977FA4;
	unsigned int offsets[] = { address, 0x0, 0x38, 0x244, 0x1C, 0x24, 0xC0, 0xB8};
	uint8_t* ptr = (uint8_t*)GetPtr(offsets, 8);
	if (ptr)
	{
		if (*ptr > 10)
		{
			return 8;
		}
		if (*ptr < 0)
		{
			return 2;
		}
		return *ptr;
	}

	return 0;
}

std::string GetCarName()
{
	DWORD address = gBase + 0x0089E318;
	unsigned int offsets[] = { address, 0x5C, 0x734, 0x10};
	auto ptr = (char*)GetPtr(offsets, 4);
	if (ptr)
	{
		return ptr;
	}
	
	return std::string();
}

void processGearbox(void)
{
	if (gGear == 0)
	{
		gPlaySnd = false;
	}
	else
	{
		gPlaySnd = true;
	}
}

void processVol()
{
	if (IsLoading)
	{
		sndTarget = 0.0f;
		return;
	}

	sndTarget = gMasterVolSlider * gEngineVolSlider;
	sndCurrentLvl = gMasterVolSlider * gEngineVolSlider;

}

bool Shifting()
{
	if (gShiftValue > 4000)
	{
		return true;
	}
	return false;
}

void modulateSnd(void)
{
	processVol();
	if (gSpeed < 1.00)
	{
		ma_sound_set_pitch(&sound, 0.3f);
		ma_sound_set_pitch(&sound_two, 0.0f);
		ma_sound_set_pitch(&sound_reverse, 0.0f);
	}
	else
	{
		gPitchModTarget = mTz((gGear - 1) * 0.010f);
		if (gearJustChanged())
		{
			fade_volume_to_zero(&sound, &sound_two, 100, ma_sound_get_volume(&sound), ma_sound_get_volume(&sound_two));
			gPitchModifier = gPitchModTarget;
			while (Shifting())
			{
				Sleep(1);
			}
			Sleep(10);
		}
		ma_sound_set_pitch(&sound, mapToRangeExp(GetSpeed(), gPitchMin, gPitchMax, gKMmax));
		ma_sound_set_pitch(&sound_two, 1.00f);
		ma_sound_set_pitch(&sound_reverse, mapToRangeExp(GetSpeed(), gPitchMin, gPitchMax, gKMmax));
	}
	fadeSnd();
	ma_sound_set_volume(&sound, 0.030f * sndCurrentLvl * IniMult * gTransVolMod * gPlaySnd);
	ma_sound_set_volume(&sound_two, mapToRange(GetSpeed(), 0.00f, (0.015f * sndCurrentLvl * IniMult * gTransVolMod * gPlaySnd), 140));
	ma_sound_set_volume(&sound_reverse, gReverseVol * IniMult * gTransVolMod * sndCurrentLvl * !(gPlaySnd));
}

void Update(LPVOID)
{
	//Sleep(15000);
	while (true)
	{
		IsOnFocus = (*(bool*)(gBase + _HasFocus));
		InGarage = (*(bool*)(gBase + _InGarage));
		IsLoading = (*(bool*)(gBase + _LoadState));
		TheGameFlowManager = (*(int*)(gBase + _GameState));
		if (TheGameFlowManager == 6)
		{
			gSpeed = GetSpeed();
			gGear = GetGear();
			gCarName = GetCarName();
			gShiftValue = 1;//GetShiftValue();
			gEngineVolSlider = (*(float*)(gBase + _EngineVolSlider));
			gMasterVolSlider = (*(float*)(gBase + _MasterVolSlider));
		}
		toggleSound();
		Sleep(16.6666);
	}
}

bool ProcessWhine()
{
	if (hasInit == false)
	{
		InitData();
		InitSnd();
		hasInit = true;
	}

	while (true)
	{
		if (!gGlobalEnable || (!IsPaused && (IsOnFocus == false || IsLoading == true || InGarage == true)))
		{
			//std::cout << "Game paused/ unfocused / loading, stopping all sounds." << std::endl;
			ma_sound_stop(&sound);
			ma_sound_stop(&sound_two);
			ma_sound_stop(&sound_reverse);
			IsPaused = true;
			hasPlayed = false;
			triedToPlay = false;
			gWhineEnabled = false;
			Sleep(50);
		}

		if (gGlobalEnable == true && TheGameFlowManager == 6 && IsOnFocus && !IsLoading && !InGarage)
		{
			IsPaused = false;
			if (triedToPlay == false)
			{
				InitPatterns();
				hasPlayed = PlaySnd();
				triedToPlay = true;
			}
			else if (hasPlayed == true)
			{
				processGearbox();
				modulateSnd();
			}
		}
		Sleep(16.6666);
	}
	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		gBase = (uintptr_t)GetModuleHandleA(NULL);
		IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)(gBase);
		IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(gBase + dos->e_lfanew);
		if ((gBase + nt->OptionalHeader.AddressOfEntryPoint + (0x400000 - gBase)) == 0x97CEFC) // Check if .exe file is compatible
		{
			AllocConsole();
			if (freopen("CONOUT$", "w", stdout) == NULL)
			{
				MessageBoxA(NULL, "CONOUT failure", "Could not redirect stdout to the console.", MB_ICONERROR);
			}
			gConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&WriteToConsole, NULL, 0, NULL);
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Update, NULL, 0, NULL);
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&ProcessWhine, NULL, 0, NULL);
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&VolSettings, NULL, 0, NULL);
		}
		else
		{
			MessageBoxA(NULL, "This .exe is not supported.\nPlease use v1.9.3 nfsw.exe (10,9 MB (11.452.160 bytes)).", "WorldWhineGen", MB_ICONERROR);
			return FALSE;
		}
	}
	return TRUE;
}
