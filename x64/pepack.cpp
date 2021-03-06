// Pack.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "pack.h"


//// 这是导出变量的一个示例
//PACK_API int nPack = 0;
//
//// 这是导出函数的一个示例。
//PACK_API int fnPack(void)
//{
//	return 42;
//}

// 这是已导出类的构造函数。
// 有关类定义的信息，请参阅 Pack.h
//CPack::CPack()
//{
//	return;
//}

#include <Psapi.h>
#pragma comment(lib,"psapi.lib")
#include "stub.h"
#ifdef DEBUG 
#ifdef _WIN64
	#pragma comment(lib,"x64/Debug/Stub.lib")
#else
   #pragma comment(lib,"Debug/Stub.lib")
#endif
#endif // DEBUG
#ifdef NDEBUG

#ifdef _WIN64
#pragma comment(lib,"x64/Release/Stub.lib")
#else
#pragma comment(lib,"Release/Stub.lib")
#endif

#endif // NDEBUG

#include "PE.h"
//extern DWORD dwPEVersion;
BOOL Pack(CString strPath, BYTE byXor)
{
	// 1.获取目标文件PE信息
	CPE objPE;
	BOOL bRet = FALSE;
	bRet = objPE.InitPE(strPath);

	DWORD dwVirtualAddr = objPE.XorCode(byXor);
	HMODULE hMod;
	//2. 获取Stub文件PE信息,将必要的信息设置到Stub中
	//int a = 0;
#ifdef DEBUG 
#ifdef _WIN64
	hMod = LoadLibrary(L"x64\\Debug\\Stub.dll");
#else
	hMod = LoadLibrary(L"Debug\\Stub.dll");
#endif
#endif // DEBUG
#ifdef NDEBUG

#ifdef _WIN64
	hMod = LoadLibrary(L"x64\\Release\\Stub.dll");
//	a = GetLastError();
#else
	hMod = LoadLibrary(L"Release\\Stub.dll");
#endif
#endif


	PGLOBAL_PARAM pstcParam = (PGLOBAL_PARAM)GetProcAddress(hMod, "g_stcParam");
	pstcParam->dwImageBase = objPE.m_dwImageBase;
	pstcParam->dwCodeSize = objPE.m_dwCodeSize;
	pstcParam->dwOEP = objPE.m_dwOEP;
	pstcParam->byXor = byXor;
	pstcParam->lpStartVA = (PBYTE)dwVirtualAddr;

	// 3. 添加Stub代码段到被加壳程序中
	// 3.1 读取Stub代码段
	MODULEINFO modinfo = { 0 };
	GetModuleInformation(GetCurrentProcess(), hMod, &modinfo, sizeof(MODULEINFO));
	PBYTE  lpMod = new BYTE[modinfo.SizeOfImage];
	memcpy_s(lpMod, modinfo.SizeOfImage, hMod, modinfo.SizeOfImage);
	PBYTE pCodeSection = NULL;
	DWORD dwCodeBaseRVA = 0;
	DWORD dwSize = objPE.GetSectionData(lpMod, 0, pCodeSection, dwCodeBaseRVA);

	// 3.2 修复Stub段中的代码
	objPE.FixReloc(lpMod, pCodeSection, objPE.m_dwNewSectionRVA);

	// 3.3 修改被加壳程序的OEP，指向stub
	DWORD dwStubOEPRVA = pstcParam->dwStart - (DWORD)hMod;
	DWORD dwNewOEP = dwStubOEPRVA - dwCodeBaseRVA;
	//StubOEP = dwStubOEPRVA - 原RVA + 新区段的RVA;
	objPE.SetNewOEP(dwNewOEP);
	objPE.ClearRandBase();
	objPE.ClearBundleImport();//兼容有绑定输入表项的程序
//	MessageBoxW(NULL, L"5", L"nothing", MB_OK);
	// 3.4 读取重定位的信息，修复代码
	char pc[10] = { 0 };
	strcpy_s(pc, 10, "15pack");
	if (objPE.AddSection(pCodeSection, dwSize, pc))
	{
		bRet = TRUE;
	}
	// 释放资源
	delete lpMod;
	lpMod = NULL;
	return bRet;
}