
#include "pch.h"
#include <iostream>
#include "../07_加壳项目/Share.h"
//把数据段融入代码段
#pragma comment(linker,"/merge:.data=.text")
//把只读数据段融入代码段
#pragma comment(linker,"/merge:.rdata=.text")
//设置代码段为可读可写可执行
#pragma comment(linker,"/section:.text,RWE")

extern "C" __declspec(dllexport) StubConf g_Sc = { 0 };

typedef FARPROC(WINAPI*FuGetProcAddress)(
	_In_ HMODULE hModule,
	_In_ LPCSTR lpProcName
);
typedef HMODULE(WINAPI*FuLoadLibraryExA)(
	_In_ LPCSTR lpLibFileName,
	_Reserved_ HANDLE hFile,
	_In_ DWORD dwFlags
);
typedef int(WINAPI*FuMessageBoxW)(
	_In_opt_ HWND hWnd,
	_In_opt_ LPCWSTR lpText,
	_In_opt_ LPCWSTR lpCaption,
	_In_ UINT uType);
typedef BOOL(WINAPI *FuVirtualProtect)(LPVOID, SIZE_T, DWORD, PDWORD);
typedef HMODULE(WINAPI *FuGetModuleHandleW)(_In_opt_ LPCTSTR lpModuleName);

HMODULE g_hKernel32 = 0;
HMODULE g_hUser32 = 0;
HMODULE g_hModule = 0;

FuGetProcAddress MyGetProcAddress = 0;
FuLoadLibraryExA MyLoadLibraryExA = 0;
FuMessageBoxW MyMessageBoxW = 0;
FuVirtualProtect   MyVirtualProtect = 0;
FuGetModuleHandleW MyGetModuleHandleW = 0;

//获取内核模块基址
void GetKernel()
{
	__asm 
	{
		push esi;
		mov esi, fs:[0x30];   //得到PEB地址
		mov esi, [esi + 0xc]; //指向PEB_LDR_DATA结构的首地址
		mov esi, [esi + 0x1c];//一个双向链表的地址
		mov esi, [esi];       //得到第2个条目kernelBase的链表
		mov esi, [esi];       //得到第3个条目kernel32的链表(win10系统)
		mov esi, [esi + 0x8]; //kernel32.dll地址
		mov g_hKernel32, esi;
		pop esi;
	}
}

//获取GetProcAddress函数地址
void MyGetFunAddress()
{	
	__asm 
	{
		pushad;		
		mov ebp, esp;
		sub esp, 0xc;
		mov edx, g_hKernel32;
		mov esi, [edx + 0x3c];     //NT头的RVA
		lea esi, [esi + edx];      //NT头的VA
		mov esi, [esi + 0x78];     //Export的Rva		
		lea edi, [esi + edx];      //Export的Va
							       
		mov esi, [edi + 0x1c];     //Eat的Rva
		lea esi, [esi + edx];      //Eat的Va
		mov[ebp - 0x4], esi;       //保存Eat
							       
		mov esi, [edi + 0x20];     //Ent的Rva
		lea esi, [esi + edx];      //Ent的Va
		mov[ebp - 0x8], esi;       //保存Ent
							       
		mov esi, [edi + 0x24];     //Eot的Rva
		lea esi, [esi + edx];      //Eot的Va
		mov[ebp - 0xc], esi;       //保存Eot

		xor ecx, ecx;
		jmp _First;
	_Zero:
		inc ecx;
	_First:
		mov esi, [ebp - 0x8];     //Ent的Va
		mov esi, [esi + ecx * 4]; //FunName的Rva

		lea esi, [esi + edx];     //FunName的Va
		cmp dword ptr[esi], 050746547h;// 47657450 726F6341 64647265 7373;
		jne _Zero;                     // 上面的16进制是GetProcAddress的
		cmp dword ptr[esi + 4], 041636f72h;
		jne _Zero;
		cmp dword ptr[esi + 8], 065726464h;
		jne _Zero;
		cmp word  ptr[esi + 0ch], 07373h;
		jne _Zero;

		xor ebx,ebx
		mov esi, [ebp - 0xc];     //Eot的Va
		mov bx, [esi + ecx * 2];  //得到序号

		mov esi, [ebp - 0x4];     //Eat的Va
		mov esi, [esi + ebx * 4]; //FunAddr的Rva
		lea eax, [esi + edx];     //FunAddr
		mov MyGetProcAddress, eax;	
		add esp, 0xc;
		popad;
	}
}

//自写strcmp
int StrCmpText(const char* pStr, char* pBuff)
{
	int nFlag = 1;
	__asm
	{
		mov esi, pStr;
		mov edi, pBuff;
		mov ecx, 0x6;
		cld;
		repe cmpsb;
		je _end;
		mov nFlag, 0;
	_end:
	}
	return nFlag;
}

//解密
void Decryption()
{
	//获取.text的区段头
	auto pNt = GetNtHeader((char*)g_hModule);
	DWORD dwSecNum = pNt->FileHeader.NumberOfSections;
	auto pSec = IMAGE_FIRST_SECTION(pNt);

	//找到代码区段
	for (size_t i = 0; i < dwSecNum; i++)
	{
		if (StrCmpText(".text", (char*)pSec[i].Name))
		{
			pSec += i;
			break;
		}
	}

	//获取代码段首地址
	char* pTarText = pSec->VirtualAddress + (char*)g_hModule;
	int nSize = pSec->Misc.VirtualSize;
	DWORD old = 0;
	//解密代码段
	MyVirtualProtect(pTarText, nSize, PAGE_READWRITE, &old);
	for (int i = 0; i < nSize; ++i) {
		pTarText[i] ^= 0x15;
	}
	MyVirtualProtect(pTarText, nSize, old, &old);
}
 
//运行函数
void RunFun()
{
	MyLoadLibraryExA = (FuLoadLibraryExA)MyGetProcAddress(g_hKernel32, "LoadLibraryExA");
	g_hUser32 = MyLoadLibraryExA("user32.dll", 0, 0);
	MyMessageBoxW = (FuMessageBoxW)MyGetProcAddress(g_hUser32, "MessageBoxW");
	MyMessageBoxW(0, L"大家好我是一个壳", L"提示", 0);
	g_hModule = (HMODULE)(FuGetModuleHandleW)MyGetModuleHandleW(0);
}


extern "C" __declspec(dllexport) __declspec(naked)
void Start()
{
	GetKernel();
	MyGetFunAddress();
	//RunFun();

	g_Sc.dwOep += 0x400000;
	__asm jmp g_Sc.dwOep;
}


