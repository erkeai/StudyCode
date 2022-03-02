// 07_加壳项目.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include "Share.h"

//stub信息结构体
struct StubInfo
{
	HMODULE hModule;//
	DWORD dwTextRva; //代码段RVA
	DWORD dwTextSize;//代码段大小

	DWORD dwFunAddr;
	StubConf* sc;
};

//01加载stub
StubInfo LoadStub();

//02添加一个新区段
void AddSection(char*& pTarBuff, int& nTarSize,
	const char* pSecName, int nSecSize);

//03修复stub的重定位
void FixStubReloc(char* hModule,
				  DWORD dwNewBase,//新基址	
				  DWORD dwNewSecRva);//新区段的RVA

//04植入stub
void ImplantStub(char*& pTarBuff, int& nTarSize);

int main(int argc, char *argv[])
{
	
	//要加壳的文件	
	if (argc != 2) {
		MessageBoxW(NULL, L"无目标文件", L"error", MB_OK);
		return 0;
	}
	char szPath[MAX_PATH] = { 0 };
	strcpy_s(szPath, MAX_PATH, argv[1]);
	//char szPath[MAX_PATH] = "F:\\1.exe";
	char* pTarBuff = NULL;
	int nPeSize = 0;
	//读取文件数据
	pTarBuff = GetFileData(szPath, &nPeSize);

	/*
	//加密代码段
    //1.获取代码段首地址
	char* pTarText = GetSecHeader(pTarBuff, ".text")->PointerToRawData + pTarBuff;
	//2.获取代码段实际大小
	int nSize = GetSecHeader(pTarBuff, ".text")->Misc.VirtualSize;
	for (int i = 0; i < nSize; ++i)
	{
		pTarText[i] ^= 0x15;
	}
	*/
	
	//植入stub
	ImplantStub(pTarBuff, nPeSize);

	//去掉随机基址
	GetOptHeader(pTarBuff)->DllCharacteristics &= (~0x40);

	//保存文件
		// 3.1 生成输出文件路径

	char szOutPath[MAX_PATH] = { 0 };
	LPCSTR strSuffix = PathFindExtensionA(szPath);                     // 获取文件的后缀名
	strcpy_s(szOutPath, MAX_PATH, szPath); // 备份目标文件路径到szOutPath
	PathRemoveExtensionA(szOutPath);                                         // 将szOutPath中保存路径的后缀名去掉
	strcat_s(szOutPath, MAX_PATH, "_Pack");                       // 在路径最后附加“_15Pack”
	strcat_s(szOutPath, MAX_PATH, strSuffix);
	SavePeFile(pTarBuff,nPeSize, szOutPath);
	MessageBoxW(NULL, L"加壳成功", L"输出信息", MB_OK);
}

//1.加载stub
StubInfo LoadStub()
{
	StubInfo si = { 0 };
	HMODULE hStubDll = LoadLibraryExA("Stub.dll", 0,
		DONT_RESOLVE_DLL_REFERENCES);
	si.hModule = hStubDll;
	si.dwTextRva = (DWORD)GetSecHeader((char*)hStubDll, ".text")->VirtualAddress;
	si.dwTextSize = (DWORD)GetSecHeader((char*)hStubDll, ".text")->SizeOfRawData;
	si.dwFunAddr = (DWORD)GetProcAddress(hStubDll, "Start");
	si.sc = (StubConf*)GetProcAddress(hStubDll, "g_Sc");
	return si;
}

//2.添加一个新区段
void AddSection(char*& pTarBuff, int& nTarSize,
	const char* pSecName, int nSecSize)
{
	//区段数目加1
	int n = GetFileHeader(pTarBuff)->NumberOfSections++;
	PIMAGE_SECTION_HEADER pSec = GetLastSecHeader(pTarBuff);

	DWORD dwFileAlig = GetOptHeader(pTarBuff)->FileAlignment;
	DWORD dwMemAlig = GetOptHeader(pTarBuff)->SectionAlignment;
	//设置新区段信息
	memcpy(pSec->Name, pSecName, 8);
	pSec->Misc.VirtualSize = nSecSize;
	pSec->SizeOfRawData = Aligment(nSecSize, dwFileAlig);
	//内存的RVA
	pSec->VirtualAddress = (pSec - 1)->VirtualAddress +
		Aligment((pSec - 1)->SizeOfRawData, dwMemAlig);
	//文件偏移要注意，把之前的文件文件对齐；
	pSec->PointerToRawData = Aligment(nTarSize, dwFileAlig);
	pSec->Characteristics = 0xE00000E0;
	//映像大小,注意要进行内存对齐
// 	GetOptHeader(pTarBuff)->SizeOfImage = pSec->VirtualAddress +
// 		Aligment(nSecSize,dwMemAlig);
	GetOptHeader(pTarBuff)->SizeOfImage = 
		Aligment(pSec->VirtualAddress + pSec->SizeOfRawData, dwMemAlig);


	//增加文件大小
	int nNewSize = pSec->SizeOfRawData + pSec->PointerToRawData;
	char* pNewFile = new char[nNewSize];
	memset(pNewFile, '\x90', nNewSize);
	memcpy(pNewFile, pTarBuff, nTarSize);
	delete[] pTarBuff;
	pTarBuff = pNewFile;
	nTarSize = nNewSize;
}

//3.修复stub的重定位
void FixStubReloc(char* hModule,DWORD dwNewBase,DWORD dwNewSecRva)
{
	//获取重定位va
	auto pReloc = (PIMAGE_BASE_RELOCATION)
		(GetOptHeader(hModule)->DataDirectory[5].VirtualAddress
			+ hModule);

	//获取.text区段的Rva
	DWORD dwTextRva = (DWORD)GetSecHeader(hModule, ".text")->VirtualAddress;

	//修复重定位
	while (pReloc->SizeOfBlock)
	{
		struct TypeOffset 
		{
			WORD offset : 12;
			WORD type : 4;
		};
		TypeOffset* pTyOf = (TypeOffset*)(pReloc + 1);
		DWORD dwCount = (pReloc->SizeOfBlock - 8) / 2;
		for (size_t i = 0; i < dwCount; i++)
		{
			if(pTyOf[i].type != 3)
				continue;
			//要修复的Rva
			DWORD dwFixRva = pTyOf[i].offset + pReloc->VirtualAddress;
			//要修复的地址
			DWORD* pFixAddr = (DWORD*)(dwFixRva + (DWORD)hModule);

			DWORD dwOld;
			VirtualProtect(pFixAddr, 4, PAGE_READWRITE, &dwOld);
			*pFixAddr -= (DWORD)hModule; //减去原始基址
			*pFixAddr -= dwTextRva;      //减去原始代码段Rva
			*pFixAddr += dwNewBase;      //加上新基址
			*pFixAddr += dwNewSecRva;    //加上新Rva
			VirtualProtect(pFixAddr, 4, dwOld, &dwOld);
		}
		//指向下一个重定位块
		pReloc = (PIMAGE_BASE_RELOCATION)
			((DWORD)pReloc + pReloc->SizeOfBlock);
	}		
}

//4.植入stub
void ImplantStub(char*& pTarBuff, int& nTarSize)
{
	//加载stub
	StubInfo si = LoadStub();

	//新区段名字
	char NewSecName[] = "15pbPack";

	//添加新区段
	AddSection(pTarBuff, nTarSize, NewSecName, si.dwTextSize);

	//修复stub的重定位
	DWORD dwBase = GetOptHeader(pTarBuff)->ImageBase;
	DWORD dwNewSecRva = GetSecHeader(pTarBuff, NewSecName)->VirtualAddress;
	FixStubReloc((char*)si.hModule, dwBase, dwNewSecRva);
	
	//保存原始的OEPss
	si.sc->dwOep = GetOptHeader(pTarBuff)->AddressOfEntryPoint;

	//把stub的代码段移植到目标文件的新区段中
	char* pNewSecAddr = GetSecHeader(pTarBuff, NewSecName)->PointerToRawData
		+ pTarBuff;
	char* pStubTextAddr = si.dwTextRva + (char*)si.hModule;	
	memcpy(pNewSecAddr, pStubTextAddr, si.dwTextSize);

	//设置新的OEP
	GetOptHeader(pTarBuff)->AddressOfEntryPoint = si.dwFunAddr - (DWORD)si.hModule -
		si.dwTextRva + GetSecHeader(pTarBuff, NewSecName)->VirtualAddress;
}


