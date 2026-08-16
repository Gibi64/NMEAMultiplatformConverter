#pragma once
#if defined(_WIN32)
#include <WinSock2.h>
#include <windows.h>
#endif
class CNMEATranslator;
class CStacking
{
public:
	static void StackWindData(CNMEATranslator *pTranslator, void* Args);
	static	void StackDepthData(CNMEATranslator *pTranslator, void* Args);
	static void StackNavData(CNMEATranslator* pTranslator, void* Args);
	static void StackSpeedData(CNMEATranslator* pTranslator, void* Args);
	static void StackHeadingData(CNMEATranslator* pTranslator, void* Args);
	static void StackTime(CNMEATranslator* pTranslator, void* Args);

};
