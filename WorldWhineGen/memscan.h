#include <windows.h>
#include <vector>
#include <string>
#include <sstream>
#include <psapi.h>

std::vector<int> PatternToBytes(const char* pattern)
{
    std::vector<int> bytes;
    const char* current = pattern;

    while (*current)
    {
        if (*current == ' ')
        {
            current++;
            continue;
        }

        if (*current == '?')
        {
            bytes.push_back(-1);
            current++;

            if (*current == '?')
                current++;
        }
        else
        {
            bytes.push_back(strtoul(current, const_cast<char**>(&current), 16));
        }
    }

    return bytes;
}

uintptr_t FindPattern(uintptr_t base, size_t size, const char* pattern)
{
    std::vector<int> patternBytes = PatternToBytes(pattern);
    size_t patternSize = patternBytes.size();

    for (size_t i = 0; i < size - patternSize; i++)
    {
        bool found = true;

        for (size_t j = 0; j < patternSize; j++)
        {
            if (patternBytes[j] != -1 &&
                patternBytes[j] != *(uint8_t*)(base + i + j))
            {
                found = false;
                break;
            }
        }

        if (found)
            return base + i;
    }

    return 0;
}

bool GetModuleInfoData(const char* moduleName, uintptr_t& base, size_t& size)
{
    HMODULE hModule = GetModuleHandleA(moduleName);

    if (!hModule)
        return false;

    MODULEINFO info{};
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &info, sizeof(info)))
        return false;

    base = reinterpret_cast<uintptr_t>(info.lpBaseOfDll);
    size = static_cast<size_t>(info.SizeOfImage);

    return true;
}
