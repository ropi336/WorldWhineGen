#include <windows.h>
#include <limits.h>
#include <stdlib.h>
#include "stdio.h"
#include <iostream>
#include <direct.h>
#include <filesystem>
#include "..\includes\miniaudio.h"
#include "GlobalVariables.h"
#include "..\includes\injector\injector.hpp"
#include "..\includes\IniReader.h"
#include "lookuptable.h"

ma_result result;
ma_engine engine;
ma_sound sound, sound_two, sound_reverse, sound_off, sound_on;

bool checkFileExists(std::string filename, bool shouldError = false);

unsigned int* GetPtr(unsigned int* offsets, int count)
{
	auto ptr = (unsigned int*)offsets[0];
	if (ptr == NULL)
	{
		return 0;
	}
	for (int i = 1, size = count; i < size; i++)
	{
		if (ptr == NULL || *ptr == 0)
		{
			return 0;
		}

		ptr = (unsigned int*)(*ptr + offsets[i]);
	}

	return ptr;
}

uintptr_t GetTrannyCode()
{
	DWORD addressNRZ = gBase + 0x009A1A98;
	unsigned int offsetsNRZ[] = { addressNRZ, 0x58, 0xB24 };
	int* ptr = (int*)GetPtr(offsetsNRZ, 3);
	if (ptr)
	{
		return *ptr;
	}

	return 0;
}

std::string GetTrannyPart()
{
	DWORD addressNRZ = gBase + 0x008E39A8;
	unsigned int offsets[] = { addressNRZ, 0x18, 0x314, 0x38, 0x0 };
	auto ptr = (char*)GetPtr(offsets, 5);
	if (ptr)
	{
		return ptr;
	}

	return std::string();
}

void SplitString(std::string stringIn)
{
	stringIn.erase(std::remove(stringIn.begin(), stringIn.end(), ' '), stringIn.end());
	std::stringstream ss(stringIn);
	std::string tmp;
	while (std::getline(ss, tmp, ','))
	{
		carList.push_back(tmp);
	}
}


float mTz(float input)
{
	if (input < 0.0)
	{
		return 0.0;
	}
	
	return input;
}

void VolSettings()
{
	while (true)
	{
		CIniReader iniReader("WorldWhineGen.ini");
		IniMult = iniReader.ReadFloat("Settings", "Volume", -1.00);
		gGlobalEnable = iniReader.ReadBoolean("Settings", "EnableWhine", gGlobalEnable);
		Sleep(1000);
	}
}

void InitData()
{
	CIniReader iniReader("WorldWhineGen.ini");
	if (checkFileExists("scripts/WorldWhineGen.ini") == false)
	{
		iniReader.WriteFloat("Settings", "Volume", 1.0);
		iniReader.WriteBoolean("Settings", "EnableWhine", true);
		iniReader.WriteString("Settings", "Usage ", "Press F1 To toggle whine.");
		iniReader.WriteString("Settings", "UsageVol ", "Volume value is a percentage, wherein 1.0 is 100%, 0.5 is 50% and so on. You can set this value above 1.00 if it's still too quiet.");
	}
	else
	{
		CarInput = iniReader.ReadString("Settings", "CarList", "DEFAULTFAIL");
		if (iniReader.ReadFloat("Settings", "Volume", -1.0) == -1.00)
		{
			MessageBoxA(NULL, "Failed to read Volume setting. Ensure .ini file is formatted correctly. Delete the file to regenerate it.", "WorldWhineGen", MB_ICONERROR);
		}
	}
	
	// Read values from .ini
	

	carList.assign({ "BMWM3GTRE46" });
	//CarInput = iniReader.ReadString("Settings", "CarList", CarInput);
	//if (CarInput.empty() == true)
	//{
	//	//Nothing
	//}
	//else
	//{
	//	if (CarInput == std::string("ALL"))
	//	{
	//		gWhineAll = true;
	//		gWhineEnabled = true;
	//	}
	//	else
	//	{
	//		//Load carlist into array vector
	//		SplitString(CarInput);
	//	}
	//}
}

bool ValidateTransmission()
{
	gTrannyCode = GetTrannyCode();
	if (gTrannyRace.find(gTrannyCode) != gTrannyRace.end())
	{
		gTransVolMod = 0.4f;
		return true;
	}
	if (gTrannyPro.find(gTrannyCode) != gTrannyPro.end())
	{
		gTransVolMod = 0.6f;
		return true;
	}
	if (gTrannyUltra.find(gTrannyCode) != gTrannyUltra.end())
	{
		gTransVolMod = 0.8f;
		return true;
	}
	if (gTrannyElite.find(gTrannyCode) != gTrannyElite.end())
	{
		gTransVolMod = 1.0f;
		return true;
	}
	if (gTrannyExtern.find(gTrannyCode) != gTrannyExtern.end())
	{
		gTransVolMod = 1.00f;
		return true;
	}

	return false;
}

bool ValidateCar()
{
	//Sleep(500);
	if (gWhineAll == true) 
	{ 
		gWhineEnabled = true;
		return true;
	}
	gCarNameChar = &gCarName[0];
	for (auto it = carList.begin(); it != carList.end(); ++it)
	{
		if (gCarNameChar == (*it))
		{
			gWhineEnabled = true;
			gTransVolMod = 1.00f;
			return true;
		}
	}
	if (gWhineEnabled == false)
	{
		gWhineEnabled = ValidateTransmission();
	}
	return gWhineEnabled;
}

void InitSnd(void)
{
	checkFileExists("scripts/snd/car_whine.wav", true);
	checkFileExists("scripts/snd/car_whine_2.wav", true);
	checkFileExists("scripts/snd/car_whine_reverse.wav", true);
	checkFileExists("scripts/snd/snd_off.wav", true);
	checkFileExists("scripts/snd/snd_on.wav", true);
	char cwd[_MAX_PATH];
	//std::cout << "Workingdirectory: " << _getcwd(cwd, sizeof(cwd)) << std::endl;
	result = ma_engine_init(NULL, &engine);
	if (result == MA_SUCCESS)
	{
		//std::cout << "sound engine initiliazed: " << result << std::endl;
	}
	else
	{
		//std::cout << "failed to init snd engine: " << result << std::endl;
	}

	result = ma_sound_init_from_file(&engine, "scripts/snd/car_whine.wav", NULL, NULL, NULL, &sound);
	if (result == MA_SUCCESS)
	{
		//std::cout << "Sound one has init with code: " << result << std::endl;
	}
	else
	{
		//std::cout << "Sound one failed to init with code: " << result << std::endl;
	}
	ma_sound_set_looping(&sound, true);
	ma_sound_set_volume(&sound, 0.00f);
	ma_sound_set_pitch(&sound, 0.3f);

	result = ma_sound_init_from_file(&engine, "scripts/snd/car_whine_2.wav", NULL, NULL, NULL, &sound_two);
	if (result == MA_SUCCESS)
	{
		//std::cout << "Sound two has init with code: " << result << std::endl;
	}
	else
	{
		//std::cout << "Sound two failed to init with code: " << result << std::endl;
	}
	ma_sound_set_looping(&sound_two, true);
	ma_sound_set_volume(&sound_two, 0.00f);
	ma_sound_set_pitch(&sound_two, 0.3f);

	result = ma_sound_init_from_file(&engine, "scripts/snd/car_whine_reverse.wav", NULL, NULL, NULL, &sound_reverse);
	if (result == MA_SUCCESS)
	{
		//std::cout << "Sound reverse has init with code: " << result << std::endl;
	}
	else
	{
		//std::cout << "Sound reverse failed to init with code: " << result << std::endl;
	}
	ma_sound_set_looping(&sound_reverse, true);
	ma_sound_set_volume(&sound_reverse, 0.00f);
	ma_sound_set_pitch(&sound_reverse, 1.0f);

	result = ma_sound_init_from_file(&engine, "scripts/snd/snd_off.wav", NULL, NULL, NULL, &sound_off);
	if (result == MA_SUCCESS)
	{
		//std::cout << "Sound offswitch has init with code: " << result << std::endl;
	}
	else
	{
		//std::cout << "Sound offswitch failed to init with code: " << result << std::endl;
	}
	ma_sound_set_looping(&sound_off, false);
	ma_sound_set_volume(&sound_off, 0.05f);

	result = ma_sound_init_from_file(&engine, "scripts/snd/snd_on.wav", NULL, NULL, NULL, &sound_on);
	if (result == MA_SUCCESS)
	{
		//std::cout << "Sound onswitch has init with code: " << result << std::endl;
	}
	else
	{
		//std::cout << "Sound onswitch failed to init with code: " << result << std::endl;
	}
	ma_sound_set_looping(&sound_on, false);
	ma_sound_set_volume(&sound_on, 0.05f);


	result = ma_sound_start(&sound);
	if (result == MA_SUCCESS)
	{
		//std::cout << "Sound one has started with code: " << result << std::endl;
	}
	else
	{
		//std::cout << "Sound one failed to started with code: " << result << std::endl;
	}

	result = ma_sound_start(&sound_two);
	if (result == MA_SUCCESS)
	{
		//std::cout << "Sound two has started with code: " << result << std::endl;
	}
	else
	{
		//std::cout << "Sound two failed to started with code: " << result << std::endl;
	}

	result = ma_sound_start(&sound_reverse);
	if (result == MA_SUCCESS)
	{
		//std::cout << "Sound reverse has started with code: " << result << std::endl;
	}
	else
	{
		//std::cout << "Sound reverse failed to started with code: " << result << std::endl;
	}
}

bool PlaySnd(void)
{
	if (!ValidateCar())
	{
		//std::cout << "Car/gearbox invalid, returning false" << std::endl;
		return false;
	}
	ma_sound_set_volume(&sound, 0.00f);
	ma_sound_set_volume(&sound_two, 0.00f);
	ma_sound_set_volume(&sound_reverse, 0.00f);

	result = ma_sound_start(&sound);
	if (result == MA_SUCCESS)
	{
		//std::cout << "Sound one started with code: " << result << std::endl;
	}
	else
	{
		//std::cout << "Failed to start sound one with code: " << result << std::endl;
	}

	result = ma_sound_start(&sound_two);
	if (result == MA_SUCCESS)
	{
		//std::cout << "Sound two started with code: " << result << std::endl;
	}
	else
	{
		//std::cout << "Failed to start sound two with code: " << result << std::endl;
	}

	result = ma_sound_start(&sound_reverse);
	if (result == MA_SUCCESS)
	{
		//std::cout << "Sound two started with code: " << result << std::endl;
	}
	else
	{
		//std::cout << "Failed to start sound two with code: " << result << std::endl;
	}
	return true;
}

float mapToRangeExp(float x, float rangemin, float rangemax, float speedlimit) {
	static float modifier;

	if (x < 0) {
		return rangemin; // Handling negative inputs if needed
	}

	// Calculate the scaled value
	float y = rangemin + (x / speedlimit) * (rangemax - rangemin);
	double normalizedSpeed = x / 70.0;
	modifier = 0.5 * sqrt(normalizedSpeed);

	if (gSpeedLast > gSpeed && gPitchModifier > gPitchModTarget)
	{
		gPitchModifier -= 0.00100;
	}

	if (gSpeedLast < gSpeed && gPitchModifier < gPitchModTarget)
	{
		gPitchModifier += 0.00100;
	}

	gSpeedLast = gSpeed;

	if (modifier > 0.6)
	{
		modifier = 0.6;
	}

	return y + modifier + gPitchModifier;
}

float mapToRange(float x, float rangemin, float rangemax, float speedlimit, bool isMute = false) {
	static float modifier;

	if (x < 0) {
		return rangemin; // Handling negative inputs if needed
	}

	// Calculate the scaled value
	float y = rangemin + (x / speedlimit) * (rangemax - rangemin);

	return y;
}

float mapToRangeInverse(float x, float rangemin, float rangemax, float speedlimit, bool isMute = false) {
	if (isMute)
	{
		return 0.0f;
	}

	if (x < 0)
	{
		return rangemin; // Handling negative inputs if needed
	}

	// Calculate the scaled value
	float y = rangemax - (x / speedlimit) * (rangemax - rangemin);

	// Ensure it doesn't exceed the upper limit
	if (y > rangemax)
	{
		return rangemax;
	}

	// Ensure it doesn't exceed the lower limit
	if (y < rangemin)
	{
		return rangemin;
	}

	return y;
}

void fade_volume_to_zero(ma_sound* pSound, ma_sound* pSoundTwo, ma_uint64 duration_in_milliseconds, float initVolume, float initVolume2)
{
	ma_uint64 current_time = 0;
	float initial_volume = initVolume;
	float initial_volume2 = initVolume2;
	float target_volume = 0.0f;  // Target volume is 0

	// Time step in milliseconds (can be adjusted for more precision).
	ma_uint64 time_step = 10; // Adjust volume every 10 milliseconds.

	// Loop to gradually decrease the volume over the specified duration.
	while (current_time < duration_in_milliseconds) {
		// Calculate the volume at this time step.
		float current_volume = initial_volume - ((initial_volume - target_volume) * (float)current_time / (float)duration_in_milliseconds);
		float current_volume2 = initial_volume2 - ((initial_volume2 - target_volume) * (float)current_time / (float)duration_in_milliseconds);

		// Set the new volume for the sound.
		ma_sound_set_volume(pSound, current_volume);
		ma_sound_set_volume(pSoundTwo, current_volume2);

		// Wait for the next time step (sleep or delay function).
		Sleep(time_step);  // Sleeps for `time_step` milliseconds.

		// Increment the current time by the time step.
		current_time += time_step;
	}

	// Finally, set the volume to exactly 0 to ensure it fully fades out.
	ma_sound_set_volume(pSound, 0.0f);
	ma_sound_set_volume(pSoundTwo, 0.0f);
}

void fadeSnd()
{
	if (sndTarget == 0.0)
	{
		sndCurrentLvl = 0.0000000;
		return;
	}

	static float difference;
	if (sndCurrentLvl != sndTarget && gDetectDiff)
	{
		if (sndCurrentLvl < sndTarget)
		{
			difference = (sndTarget - sndCurrentLvl) / 12.0f;
		}
		if (sndCurrentLvl > sndTarget)
		{
			difference = (sndCurrentLvl - sndTarget) / 12.0f;
		}
		gDetectDiff = false;
	}
	if (sndCurrentLvl == sndTarget)
	{
		gDetectDiff = true;
	}

	if (difference < 0.001)
	{
		sndCurrentLvl == sndTarget;
	}

	if (sndCurrentLvl != sndTarget)
	{
		if (sndCurrentLvl < sndTarget)
		{
			sndCurrentLvl += difference;
		}
		if (sndCurrentLvl > sndTarget)
		{
			sndCurrentLvl -= difference;
		}
	}

}

bool gearJustChanged()
{
	if (gGearOld == 0)
	{
		gGearOld = gGear;
		return false;
	}

	if (skipGear == true)
	{
		gGearOld = gGear;
		skipGear = false;
		return false;
	}

	if (gGear < gGearOld)
	{
		gGearOld = gGear;
	}

	if (gGear > gGearOld)
	{
		gGearOld = gGear;
		return true;
	}

	return false;
}

void toggleSound()
{
	CIniReader iniReader("WorldWhineGen.ini");
	if (GetAsyncKeyState(VK_F1) & 0x8000) // If high-order bit is set, F1 is down
	{ 
		if (!f1WasPressed) 
		{ // If it wasn't already pressed
			f1WasPressed = true;
			if (gGlobalEnable == true)
			{
				gGlobalEnable = false;
				ma_sound_start(&sound_off);
			}
			else
			{
				gGlobalEnable = true;
				ma_sound_start(&sound_on);
			}
			iniReader.WriteBoolean("Settings", "EnableWhine", gGlobalEnable);
		}
	}
	else 
	{
		f1WasPressed = false; // Reset the flag when F1 is not pressed
	}
}

bool checkFileExists(std::string filename, bool shouldError)
{
	std::ifstream file(filename);
	if (file)
	{
		//std::cout << "Ini File exists.\n";
		return true;
	}
	else 
	{
		if (shouldError)
		{
			std::string message = filename + " is missing. Please ensure you've installed it in the correct location.";
			LPCSTR cstr = message.c_str();
			MessageBoxA(NULL, cstr, "WorldWhineGen", MB_ICONERROR);
		}
		//std::cout << "Ini File does not exist.\n";
	}

	return false;
}

void setCursorPosition(int x, int y) 
{
	COORD position = { (SHORT)x, (SHORT)y };
	SetConsoleCursorPosition(gConsoleHandle, position);
}

void renderToConsole(const std::vector<std::wstring>& buffer)
{
	int y = 0; // Track the current line number

	for (const std::wstring& line : buffer) {
		size_t start = 0;
		size_t pos = line.find(L'?'); // Find the first occurrence of 'X'

		while (pos != std::wstring::npos) {
			// Write the segment up to 'X'
			std::wstring segment = line.substr(start, pos - start);
			DWORD charsWritten;
			setCursorPosition(0, y); // Move to the start of the current line
			WriteConsoleW(gConsoleHandle, segment.c_str(), segment.size(), &charsWritten, nullptr);

			// Move to the next line
			y++;
			start = pos + 1;               // Skip over 'X'
			pos = line.find(L'?', start);  // Find the next 'X' if it exists
		}

		// Write any remaining text after the last 'X'
		std::wstring remaining = line.substr(start);
		setCursorPosition(0, y);
		DWORD charsWritten;
		WriteConsoleW(gConsoleHandle, remaining.c_str(), remaining.size(), &charsWritten, nullptr);

		y++; // Move to the next line after finishing this line
	}
}

std::wstring conWstring(std::string inString)
{
	std::wstring wstr(inString.begin(), inString.end());
	return wstr;
}

void WriteToConsole()
{
	while (true)
	{
		std::vector<std::wstring> curVars;
		curVars.push_back(L"Speed: " + std::to_wstring(gSpeed) + L" Gear: " + std::to_wstring(gGear) +
			L" CarName: " + conWstring(gCarName) + L" Clutch: " + std::to_wstring(gShiftValue) + L"?");

		curVars.push_back(L"gGlobalEnable: " + std::to_wstring(gGlobalEnable) + 
			L" IsOnFocus: " + std::to_wstring(IsOnFocus) + L" IsLoading: " + std::to_wstring(IsLoading) +
			L" InGarage: " + std::to_wstring(InGarage) + L" TheGameFlowManager: " + std::to_wstring(TheGameFlowManager) +
			L" gWhineAll: " + std::to_wstring(gWhineAll) + L"gearboxInstruction: " + std::to_wstring(gGearInstruction));
		renderToConsole(curVars);
		Sleep(16.666);
	}
	
}