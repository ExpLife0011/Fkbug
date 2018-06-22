// dbg.cpp : �������̨Ӧ�ó������ڵ㡣
//
#pragma once
#include "stdafx.h"
#include "CmyDebug.h"



BOOL hookIAT(const char * pszDllName, const char * pszFunc, LPVOID pNewFunc)
{
	HANDLE hProc = GetCurrentProcess();

	PIMAGE_DOS_HEADER			pDosHeader;			// Dosͷ
	PIMAGE_NT_HEADERS			pNtHeader;			// Ntͷ
	PIMAGE_IMPORT_DESCRIPTOR	pImpTable;	// �����
	PIMAGE_THUNK_DATA			pInt;					  // ������еĵ������Ʊ�
	PIMAGE_THUNK_DATA			pIat;		        // ������еĵ����ַ��
	DWORD						dwSize;
	DWORD						hModule;
	char*						pFunctionName;
	DWORD						dwOldProtect;

	hModule = (DWORD)GetModuleHandle(NULL);

	// ��ȡdosͷ
	pDosHeader = (PIMAGE_DOS_HEADER)hModule;

	// ��ȡNtͷ
	pNtHeader = (PIMAGE_NT_HEADERS)(hModule + pDosHeader->e_lfanew);


	// ��ȡ�����
	pImpTable = (PIMAGE_IMPORT_DESCRIPTOR)
		(hModule + pNtHeader->OptionalHeader.DataDirectory[1].VirtualAddress);

	// ���������
	while (pImpTable->FirstThunk != 0 && pImpTable->OriginalFirstThunk != 0) {


		// �ж��Ƿ��ҵ��˶�Ӧ��ģ����
		if (_stricmp((char*)(pImpTable->Name + hModule), pszDllName) != 0) {
			++pImpTable;
			continue;
		}

		// �������Ʊ�,�ҵ�������
		pInt = (PIMAGE_THUNK_DATA)(pImpTable->OriginalFirstThunk + hModule);
		pIat = (PIMAGE_THUNK_DATA)(pImpTable->FirstThunk + hModule);

		while (pInt->u1.AddressOfData != 0) {

			// �ж��������Ƶ��뻹������Ҫ����
			if (pInt->u1.Ordinal & 0x80000000 == 1) {
				// ����ŵ���

				// �ж��Ƿ��ҵ��˶�Ӧ�ĺ������
				if (pInt->u1.Ordinal == ((DWORD)pszFunc) & 0xFFFF) {
					// �ҵ�֮��,�����Ӻ����ĵ�ַд�뵽iat
					VirtualProtect(&pIat->u1.Function,
						4,
						PAGE_READWRITE,
						&dwOldProtect
					);

					pIat->u1.Function = (DWORD)pNewFunc;

					VirtualProtect(&pIat->u1.Function,
						4,
						dwOldProtect,
						&dwOldProtect
					);
					return true;
				}
			}
			else {
				// �����Ƶ���
				pFunctionName = (char*)(pInt->u1.Function + hModule + 2);

				// �ж��Ƿ��ҵ��˶�Ӧ�ĺ�����
				if (strcmp(pszFunc, pFunctionName) == 0) {

					VirtualProtect(&pIat->u1.Function,
						4,
						PAGE_READWRITE,
						&dwOldProtect
					);

					// �ҵ�֮��,�����Ӻ����ĵ�ַд�뵽iat
					pIat->u1.Function = (DWORD)pNewFunc;

					VirtualProtect(&pIat->u1.Function,
						4,
						dwOldProtect,
						&dwOldProtect
					);

					return true;
				}
			}

			++pIat;
			++pInt;
		}


		++pImpTable;
	}

	return false;

}


int main()
{
	CmyDebug debuger;
	debuger.EnterDebugLoop();
}


