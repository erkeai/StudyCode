#pragma once
#include "stdafx.h"
class CPE
{
public:
	CPE();
	~CPE();
	DWORD RVA2OffSet(DWORD dwRVA, PIMAGE_NT_HEADERS  pNt);

	BOOL InitPE(CString strPath);
	ULONGLONG AddSection(LPBYTE pBuffer, DWORD dwSectionSize, PCHAR pszSectionName);
	void SetNewOEP(DWORD dwOEP);
	BOOL IsPEFile();
	void ClearRandBase();
	void ClearBundleImport();
	DWORD XorCode(BYTE byXOR);

	// Stub信息处理
	void FixReloc(PBYTE lpImage, PBYTE lpCode, DWORD dwCodeRVA);// 在内存中重定位Stub
	DWORD GetSectionData(PBYTE lpImage, DWORD dwSectionIndex, PBYTE& lpBuffer, DWORD& dwCodeBaseRVA);
public:
	PBYTE  m_pFileBase;
	DWORD  m_dwFileSize;
	PIMAGE_NT_HEADERS  m_pNT;
	PIMAGE_SECTION_HEADER m_pLastSection;//新添加区段地址,即最后一个区段地址
	DWORD   m_dwFileAlign;
	DWORD   m_dwMemAlign;
#ifdef _WIN64
	ULONGLONG   m_dwImageBase;
#else
	DWORD   m_dwImageBase;
#endif
	DWORD   m_dwOEP;
	DWORD   m_dwCodeBase;
	DWORD   m_dwCodeSize;
	DWORD   m_dwNewOEP;
	DWORD   m_dwNewSectionRVA;
	CFile   m_objFile;
};

