#pragma once
#include <windows.h>
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")
struct  StubConf
{
	DWORD dwOep;

};

//打开PE文件
HANDLE OpenPeFile(const char* pPath)
{
	return CreateFileA(pPath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
}

//保存PE文件
void SavePeFile(const char* pFile, int nSize, const char* pPath)
{
	HANDLE hFile = CreateFileA(pPath, GENERIC_WRITE,
		FILE_SHARE_READ,NULL,CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;
	DWORD dwWrite = 0;
	WriteFile(hFile, pFile, nSize, &dwWrite, NULL);
	CloseHandle(hFile);
}

char* GetFileData(const char* pPath, int* nSize)
{
	HANDLE hFile = OpenPeFile(pPath);
	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;
	DWORD dwSize = GetFileSize(hFile, NULL);
	//改变大小
	*nSize = dwSize;

	char* pFileBuff = new char[dwSize];
	memset(pFileBuff, 0, dwSize);
	DWORD dwRead = 0;
	ReadFile(hFile, pFileBuff, dwSize, &dwRead, NULL);
	CloseHandle(hFile);
	//返回堆内存
	return pFileBuff;
}

//获取DOS头
PIMAGE_DOS_HEADER GetDosHeader(char* pBase)
{
	return (PIMAGE_DOS_HEADER)pBase;
}

//获取NT头
PIMAGE_NT_HEADERS GetNtHeader(char* pBase)
{
	return (PIMAGE_NT_HEADERS)
		(GetDosHeader(pBase)->e_lfanew + (DWORD)pBase);
}

//获取文件头
PIMAGE_FILE_HEADER GetFileHeader(char* pBase)
{
	return &GetNtHeader(pBase)->FileHeader;
}

//获取扩展头
PIMAGE_OPTIONAL_HEADER GetOptHeader(char* pBase)
{
	return &GetNtHeader(pBase)->OptionalHeader;
}

//获取区段头
PIMAGE_SECTION_HEADER GetSectionHeader(char* pBase)
{
	return IMAGE_FIRST_SECTION(GetNtHeader(pBase));
}

//获取指定的名字的区段头
PIMAGE_SECTION_HEADER GetSecHeader(char* pFile, const char* pName)
{
	//区段数目
	DWORD dwSecNum = GetFileHeader(pFile)->NumberOfSections;
	//区段头
	PIMAGE_SECTION_HEADER pSec = GetSectionHeader(pFile);
	char szBuf[10] = { 0 };
	for (size_t i = 0; i < dwSecNum; i++)
	{
		memcpy_s(szBuf, 8, (char*)pSec[i].Name, 8);
		if (!strcmp(pName, szBuf))
			return pSec + i;
	}
	return nullptr;
}

//获取最后一个区段头
PIMAGE_SECTION_HEADER GetLastSecHeader(char* pFile)
{
	//区段数目
	DWORD dwSecNum = GetFileHeader(pFile)->NumberOfSections;
	//区段头
	PIMAGE_SECTION_HEADER pSec = GetSectionHeader(pFile);

	return pSec + (dwSecNum - 1);
}

//对齐粒度
DWORD Aligment(DWORD dwSize, DWORD dwAlig)
{
	return (dwSize%dwAlig == 0) ? dwSize : (dwSize / dwAlig + 1)*dwAlig;
}