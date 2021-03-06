#include "..\includes\stdafx.h"
#include "..\includes\hooking\Hooking.Patterns.h"

HWND hWnd;
bool bDelay;

struct Screen
{
	int Width;
	int Height;
	float fWidth;
	float fHeight;
	float fFieldOfView;
	float fAspectRatio;
	int Width43;
	float fWidth43;
	float fHudScale;
	float fHudPos;
} Screen;

DWORD WINAPI Init(LPVOID)
{
	auto pattern = hook::pattern("64 89 25 00 00 00 00 83 C4 90 53");
	if (!(pattern.size() > 0) && !bDelay)
	{
		bDelay = true;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
		return 0;
	}

	if (bDelay)
	{
		while (!(pattern.size() > 0))
			pattern = hook::pattern("64 89 25 00 00 00 00 83 C4 90 53");
	}

	CIniReader iniReader("");
	Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
	Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
	bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;

	if (!Screen.Width || !Screen.Height) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		Screen.Width = info.rcMonitor.right - info.rcMonitor.left;
		Screen.Height = info.rcMonitor.bottom - info.rcMonitor.top;
	}

	Screen.fWidth = static_cast<float>(Screen.Width);
	Screen.fHeight = static_cast<float>(Screen.Height);
	Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
	Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
	Screen.fWidth43 = static_cast<float>(Screen.Width43);

	pattern = hook::pattern("FF 24 95 ? ? ? ? 8B 45 F8 0F B6 08 85 C9"); //0x7A3B5E
	injector::MakeNOP(pattern.get(0).get<uint32_t>(0), 7, true);

	pattern = hook::pattern("C7 02 ? ? ? ? 8B 45 0C C7 00 ? ? ? ? C6 45 FF 01"); //0x7A3B72
	injector::WriteMemory(pattern.get(0).get<uint32_t>(2), Screen.Width, true);
	injector::WriteMemory(pattern.get(0).get<uint32_t>(11), Screen.Height, true);

	//fmv
	Screen.Width43 = static_cast<uint32_t>(Screen.fWidth * (4.0f / 3.0f));
	pattern = hook::pattern("66 8B 15 ? ? ? ? 66 89 15 ? ? ? ? 66 A1 ? ? ? ? 66 A3"); //0x4BCD2F
	injector::WriteMemory(pattern.get(0).get<uint32_t>(3), &Screen.Width43, true);
	//injector::WriteMemory(0x4BCD49 + 3, &Screen.Width43, true);

	if (bFixHUD)
	{
		Screen.fHudPos = (-1.0f * (4.0f / 3.0f)) / (Screen.fAspectRatio);
		Screen.fHudScale = (2.0f * (4.0f / 3.0f)) / (Screen.fAspectRatio);

		pattern = hook::pattern("68 ? ? ? ? 8D 85 00 FF FF FF 50 E8"); //0x756CF8
		injector::WriteMemory<float>(pattern.get(0).get<uint32_t>(1), Screen.fHudPos, true);

		pattern = hook::pattern("D9 05 ? ? ? ? D8 75 10 51 D9 1C 24 8D 55 80 52"); //0x756D23
		injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &Screen.fHudScale, true);
	}

	return 0;
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init(NULL);
	}
	return TRUE;
}