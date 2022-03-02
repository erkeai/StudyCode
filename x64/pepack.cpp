// Pack.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "stdafx.h"
#include "pack.h"


//// ���ǵ���������һ��ʾ��
//PACK_API int nPack = 0;
//
//// ���ǵ���������һ��ʾ����
//PACK_API int fnPack(void)
//{
//	return 42;
//}

// �����ѵ�����Ĺ��캯����
// �й��ඨ�����Ϣ������� Pack.h
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
	// 1.��ȡĿ���ļ�PE��Ϣ
	CPE objPE;
	BOOL bRet = FALSE;
	bRet = objPE.InitPE(strPath);

	DWORD dwVirtualAddr = objPE.XorCode(byXor);
	HMODULE hMod;
	//2. ��ȡStub�ļ�PE��Ϣ,����Ҫ����Ϣ���õ�Stub��
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

	// 3. ���Stub����ε����ӿǳ�����
	// 3.1 ��ȡStub�����
	MODULEINFO modinfo = { 0 };
	GetModuleInformation(GetCurrentProcess(), hMod, &modinfo, sizeof(MODULEINFO));
	PBYTE  lpMod = new BYTE[modinfo.SizeOfImage];
	memcpy_s(lpMod, modinfo.SizeOfImage, hMod, modinfo.SizeOfImage);
	PBYTE pCodeSection = NULL;
	DWORD dwCodeBaseRVA = 0;
	DWORD dwSize = objPE.GetSectionData(lpMod, 0, pCodeSection, dwCodeBaseRVA);

	// 3.2 �޸�Stub���еĴ���
	objPE.FixReloc(lpMod, pCodeSection, objPE.m_dwNewSectionRVA);

	// 3.3 �޸ı��ӿǳ����OEP��ָ��stub
	DWORD dwStubOEPRVA = pstcParam->dwStart - (DWORD)hMod;
	DWORD dwNewOEP = dwStubOEPRVA - dwCodeBaseRVA;
	//StubOEP = dwStubOEPRVA - ԭRVA + �����ε�RVA;
	objPE.SetNewOEP(dwNewOEP);
	objPE.ClearRandBase();
	objPE.ClearBundleImport();//�����а��������ĳ���
//	MessageBoxW(NULL, L"5", L"nothing", MB_OK);
	// 3.4 ��ȡ�ض�λ����Ϣ���޸�����
	char pc[10] = { 0 };
	strcpy_s(pc, 10, "15pack");
	if (objPE.AddSection(pCodeSection, dwSize, pc))
	{
		bRet = TRUE;
	}
	// �ͷ���Դ
	delete lpMod;
	lpMod = NULL;
	return bRet;
}