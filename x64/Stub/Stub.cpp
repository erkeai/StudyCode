// Stub.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "stdafx.h"
#include "Stub.h"


#pragma comment(linker, "/merge:.data=.text") 
#pragma comment(linker, "/merge:.rdata=.text")
#pragma comment(linker, "/section:.text,RWE")
extern "C" __declspec(dllexport) GLOBAL_PARAM g_stcParam = { (DWORD)(Start) };

typedef void(*FUN)();
FUN g_oep;
ULONGLONG GetKernel32Addr()
{
	ULONGLONG dwKernel32Addr = 0;

	// ��ȡTEB�ĵ�ַ
	_TEB* pTeb = NtCurrentTeb();
	// ��ȡPEB�ĵ�ַ
	PULONGLONG pPeb = (PULONGLONG) * (PULONGLONG)((ULONGLONG)pTeb + 0x60);
	// ��ȡPEB_LDR_DATA�ṹ�ĵ�ַ
	PULONGLONG pLdr = (PULONGLONG) * (PULONGLONG)((ULONGLONG)pPeb + 0x18);
	//ģ���ʼ��������ͷָ��InInitializationOrderModuleList
	PULONGLONG pInLoadOrderModuleList = (PULONGLONG)((ULONGLONG)pLdr + 0x10);
	// ��ȡ�����е�һ��ģ����Ϣ��exeģ��
	PULONGLONG pModuleExe = (PULONGLONG)*pInLoadOrderModuleList;
	// ��ȡ�����еڶ���ģ����Ϣ��ntdllģ��
	PULONGLONG pModuleNtdll = (PULONGLONG)*pModuleExe;
	// ��ȡ�����е�����ģ����Ϣ��Kernel32ģ��
	PULONGLONG pModuleKernel32 = (PULONGLONG)*pModuleNtdll;
	// ��ȡkernel32��ַ
	dwKernel32Addr = pModuleKernel32[6];
	return dwKernel32Addr;
}


ULONGLONG MyGetProcAddress()
{
	ULONGLONG dwBase = GetKernel32Addr();
	// 1. ��ȡDOSͷ
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)dwBase;
	// 2. ��ȡNTͷ
#ifdef _WIN64
	PIMAGE_NT_HEADERS64  pNt = (PIMAGE_NT_HEADERS64)(dwBase + pDos->e_lfanew);
#else
	PIMAGE_NT_HEADERS32  pNt = (PIMAGE_NT_HEADERS32)(dwBase + pDos->e_lfanew);
#endif
	// 3. ��ȡ����Ŀ¼��
	PIMAGE_DATA_DIRECTORY pExportDir = pNt->OptionalHeader.DataDirectory;
	pExportDir = &(pExportDir[IMAGE_DIRECTORY_ENTRY_EXPORT]);
	DWORD dwOffset = pExportDir->VirtualAddress;
	// 4. ��ȡ��������Ϣ�ṹ
	PIMAGE_EXPORT_DIRECTORY pExport = (PIMAGE_EXPORT_DIRECTORY)(dwBase + dwOffset);
	DWORD dwFunCount = pExport->NumberOfFunctions;
	DWORD dwFunNameCount = pExport->NumberOfNames;
	DWORD dwModOffset = pExport->Name;

	// Get Export Address Table
	PDWORD pEAT = (PDWORD)(dwBase + pExport->AddressOfFunctions);
	// Get Export Name Table
	PDWORD pENT = (PDWORD)(dwBase + pExport->AddressOfNames);
	// Get Export Index Table
	PWORD  pEIT = (PWORD)(dwBase + pExport->AddressOfNameOrdinals);

	for (DWORD dwOrdinal = 0; dwOrdinal < dwFunCount; dwOrdinal++)
	{
		if (!pEAT[dwOrdinal]) // Export Address offset
			continue;

		// 1. ��ȡ���
		DWORD dwID = pExport->Base + dwOrdinal;
		// 2. ��ȡ����������ַ
		ULONGLONG dwFunAddrOffset = pEAT[dwOrdinal];

		for (DWORD dwIndex = 0; dwIndex < dwFunNameCount; dwIndex++)
		{
			// ����ű��в��Һ��������
			if (pEIT[dwIndex] == dwOrdinal)
			{
				// ��������������������Ʊ��е�����
				ULONGLONG dwNameOffset = pENT[dwIndex];
				char* pFunName = (char*)((ULONGLONG)dwBase + dwNameOffset);
				if (!strcmp(pFunName, "GetProcAddress"))
				{// ���ݺ������Ʒ��غ�����ַ
					return dwBase + dwFunAddrOffset;
				}
			}
		}
	}
	return 0;
}

void XorCode()
{
	// ��ȡ�����ַ
#ifdef _WIN64
	PBYTE pBase = (PBYTE)((ULONGLONG)g_stcParam.dwImageBase64 + g_stcParam.lpStartVA);
#else
	PBYTE pBase = (PBYTE)((DWORD)g_stcParam.dwImageBase32 + g_stcParam.lpStartVA);
#endif  
	// ������
	for (DWORD i = 0; i < g_stcParam.dwCodeSize; i++)
	{
		pBase[i] ^= g_stcParam.byXor;
	}
}

void  Start()
{
	// ��ȡkernel32��ַ
	fnGetProcAddress pfnGetProcAddress = (fnGetProcAddress)MyGetProcAddress();
	ULONGLONG dwBase = GetKernel32Addr();
	// ��ȡAPI��ַ
	fnLoadLibraryA pfnLoadLibraryA = (fnLoadLibraryA)pfnGetProcAddress((HMODULE)dwBase, "LoadLibraryA");
	fnGetModuleHandleA pfnGetModuleHandleA = (fnGetModuleHandleA)pfnGetProcAddress((HMODULE)dwBase, "GetModuleHandleA");
	fnVirtualProtect pfnVirtualProtect = (fnVirtualProtect)pfnGetProcAddress((HMODULE)dwBase, "VirtualProtect");
	HMODULE hUser32 = (HMODULE)pfnGetModuleHandleA("user32.dll");
	fnMessageBox pfnMessageBoxA = (fnMessageBox)pfnGetProcAddress(hUser32, "MessageBoxA");
	HMODULE hKernel32 = (HMODULE)pfnGetModuleHandleA("kernel32.dll");
	fnExitProcess pfnExitProcess = (fnExitProcess)pfnGetProcAddress(hKernel32, "ExitProcess");

    // ������Ϣ��
	//int nRet = pfnMessageBoxA(NULL, "��ӭʹ�����64λ�ӿǳ����Ƿ�����������", "Hello PEDIY", MB_YESNO);
	//if (nRet == IDYES)
	//{
	    // �޸Ĵ��������
#ifdef _WIN64
	ULONGLONG dwCodeBase = g_stcParam.dwImageBase64 + (DWORD)g_stcParam.lpStartVA;
#else
	DWORD dwCodeBase = g_stcParam.dwImageBase32 + (DWORD)g_stcParam.lpStartVA;
#endif 

	   
	    DWORD dwOldProtect = 0;
	    pfnVirtualProtect((LPBYTE)dwCodeBase, g_stcParam.dwCodeSize, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	    XorCode(); // ���ܴ���
	    pfnVirtualProtect((LPBYTE)dwCodeBase, g_stcParam.dwCodeSize, dwOldProtect, &dwOldProtect);
#ifdef _WIN64
		g_oep = (FUN)(g_stcParam.dwImageBase64 + g_stcParam.dwOEP);
#else
		g_oep = (FUN)(g_stcParam.dwImageBase32 + g_stcParam.dwOEP);
#endif 
	
		g_oep(); // ����ԭʼOEP
	//}
	// �˳�����
	pfnExitProcess(0);
}
