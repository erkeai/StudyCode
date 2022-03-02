#pragma once
#include <windows.h>
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")
struct  StubConf
{
	DWORD dwOep;

};

//��PE�ļ�
HANDLE OpenPeFile(const char* pPath)
{
	return CreateFileA(pPath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
}

//����PE�ļ�
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
	//�ı��С
	*nSize = dwSize;

	char* pFileBuff = new char[dwSize];
	memset(pFileBuff, 0, dwSize);
	DWORD dwRead = 0;
	ReadFile(hFile, pFileBuff, dwSize, &dwRead, NULL);
	CloseHandle(hFile);
	//���ض��ڴ�
	return pFileBuff;
}

//��ȡDOSͷ
PIMAGE_DOS_HEADER GetDosHeader(char* pBase)
{
	return (PIMAGE_DOS_HEADER)pBase;
}

//��ȡNTͷ
PIMAGE_NT_HEADERS GetNtHeader(char* pBase)
{
	return (PIMAGE_NT_HEADERS)
		(GetDosHeader(pBase)->e_lfanew + (DWORD)pBase);
}

//��ȡ�ļ�ͷ
PIMAGE_FILE_HEADER GetFileHeader(char* pBase)
{
	return &GetNtHeader(pBase)->FileHeader;
}

//��ȡ��չͷ
PIMAGE_OPTIONAL_HEADER GetOptHeader(char* pBase)
{
	return &GetNtHeader(pBase)->OptionalHeader;
}

//��ȡ����ͷ
PIMAGE_SECTION_HEADER GetSectionHeader(char* pBase)
{
	return IMAGE_FIRST_SECTION(GetNtHeader(pBase));
}

//��ȡָ�������ֵ�����ͷ
PIMAGE_SECTION_HEADER GetSecHeader(char* pFile, const char* pName)
{
	//������Ŀ
	DWORD dwSecNum = GetFileHeader(pFile)->NumberOfSections;
	//����ͷ
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

//��ȡ���һ������ͷ
PIMAGE_SECTION_HEADER GetLastSecHeader(char* pFile)
{
	//������Ŀ
	DWORD dwSecNum = GetFileHeader(pFile)->NumberOfSections;
	//����ͷ
	PIMAGE_SECTION_HEADER pSec = GetSectionHeader(pFile);

	return pSec + (dwSecNum - 1);
}

//��������
DWORD Aligment(DWORD dwSize, DWORD dwAlig)
{
	return (dwSize%dwAlig == 0) ? dwSize : (dwSize / dwAlig + 1)*dwAlig;
}