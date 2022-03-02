// pack.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "stdafx.h"
#include "pack.h"

DWORD dwPEVersion;   //全局变量表示文件32还是64

int main(int argc, char *argv[])
{
	// 获取加密Key的文本
	TCHAR szKey[20] = { 0 };
	_tcscpy_s(szKey, 20, L"15");
	ULONGLONG nKey = 0;
	// 转换key为16进制
	_stscanf_s(szKey, L"%p", &nKey);
	// 调用Pack模块
#ifdef _WIN64
	if (argc != 2) {
		MessageBoxW(NULL, L"无目标文件", L"error", MB_OK);
		return 0;
	}
	CString m_strPath = argv[1];
#else
	CString m_strPath = "F:\\1.exe";
#endif // 

	if (Pack(m_strPath, (BYTE)nKey))
	{
		MessageBoxW(NULL, L"加壳成功", L"输出信息", MB_OK);
	}
}



