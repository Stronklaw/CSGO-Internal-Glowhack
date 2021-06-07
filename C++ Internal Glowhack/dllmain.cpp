// dllmain.cpp : Defines the entry point for the DLL application.
#include <iostream>
#include <Windows.h>
#include "pch.h"
#include "csgo.hpp"

struct offsets
{
	DWORD glowObjectManager = hazedumper::signatures::dwGlowObjectManager;
	DWORD glowIndex = hazedumper::netvars::m_iGlowIndex;
	DWORD team = hazedumper::netvars::m_iTeamNum;
	DWORD localPlayer = hazedumper::signatures::dwLocalPlayer;
	DWORD entityList = hazedumper::signatures::dwEntityList;
	DWORD health = hazedumper::netvars::m_iHealth;
	DWORD isSpotted = hazedumper::netvars::m_bSpotted;
	DWORD isDefusing = hazedumper::netvars::m_bIsDefusing;
}offset;

struct values
{
	DWORD localPlayer;
	DWORD gameModule;
	DWORD glowObject;
	DWORD engineModule;
}val;

struct ClrRender
{
	BYTE red, green, blue;
};

struct GlowStruct
{
	BYTE base[4];
	float red;
	float green;
	float blue;
	float alpha;
	BYTE buffer[16];
	bool renderWhenOccluded;
	bool renderWhenUnOccluded;
	bool fullBloom;
	BYTE buffer1;
	BYTE buffer2[4];
	int glowStyle;
}Glow;

ClrRender clrTeam;
ClrRender clrEnemy;

GlowStruct SetGlowColor(GlowStruct Glow, uintptr_t entity)
{
	bool defusing = *(bool*)(entity + offset.isDefusing);

	if (defusing)
	{
		Glow.red = 1.0f;
		Glow.green = 1.0f;
		Glow.blue = 1.0f;
		Glow.alpha = 1.0f;
	}
	else
	{
		int health = *(int*)(entity + offset.health);

		Glow.red = health * -0.01 + 1;
		Glow.green = health * 0.01;
		Glow.alpha = 1.0f;
	}
	Glow.renderWhenOccluded = true;
	Glow.renderWhenUnOccluded = false;

	return Glow;
}

void SetEnemyGlow(uintptr_t entity, int glowIndex)
{
	GlowStruct EGlow;
	EGlow = *(GlowStruct*)(val.glowObject + (glowIndex * 0x38));
	EGlow = SetGlowColor(EGlow, entity);

	*(GlowStruct*)(val.glowObject + (glowIndex * 0x38)) = EGlow;
}

void SetTeamGlow(uintptr_t entity, int glowIndex)
{
	GlowStruct TGlow;
	TGlow = *(GlowStruct*)(val.glowObject + (glowIndex * 0x38));

	TGlow.blue = 1.0f;
	TGlow.green = 0;
	TGlow.red = 0;
	TGlow.alpha = 1.0f;
	TGlow.renderWhenOccluded = true;
	TGlow.renderWhenUnOccluded = false;
	*(GlowStruct*)(val.glowObject + (glowIndex * 0x38)) = TGlow;
}

void HandleGlow()
{
	val.glowObject = *(DWORD*)(val.gameModule + offset.glowObjectManager);
	int myTeam = *(int*)(val.localPlayer + offset.team);
	//std::cout << "My Team: " << myTeam << std::endl;

	for (short int i = 0; i < 64; i++)
	{
		DWORD entity = *(DWORD*)(val.gameModule + offset.entityList + i * 0x10);
		if (entity != NULL)
		{
			int glowIndx = *(int*)(entity + offset.glowIndex);
			int entityTeam = *(int*)(entity + offset.team);
			//std::cout << "Entity Team: " << entityTeam << std::endl;

			if (myTeam == entityTeam)
			{
				*(ClrRender*)(entity + hazedumper::netvars::m_clrRender) = clrTeam;
				SetTeamGlow(entity, glowIndx);
			}
			else if (myTeam != entityTeam)
			{
				*(ClrRender*)(entity + hazedumper::netvars::m_clrRender) = clrEnemy;
				SetEnemyGlow(entity, glowIndx);
			}

		}
		//Sleep(100);
	}
}

void SetBrightness()
{
	clrEnemy.red = 255;
	clrEnemy.green = 0;
	clrEnemy.blue = 0;

	clrTeam.red = 0;
	clrTeam.green = 0;
	clrTeam.blue = 255;


	float brightness = 5.0f;

	int ptr = *(int*)(val.engineModule + hazedumper::signatures::model_ambient_min);
	int xorptr = *(int*)&brightness ^ ptr;
	*(int*)(val.engineModule + hazedumper::signatures::model_ambient_min) = xorptr;
}

void main()
{
	val.gameModule = (DWORD)GetModuleHandle("client.dll");
	val.engineModule = (DWORD)GetModuleHandle("engine.dll");
	val.localPlayer = *(DWORD*)(val.gameModule + offset.localPlayer);

	SetBrightness();

	while (true)
	{
		HandleGlow();
	}
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		DisableThreadLibraryCalls(hModule);
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)main, NULL, NULL, NULL);
	}

	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
