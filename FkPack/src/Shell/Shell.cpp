// Shell.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "stdafx.h"
#include "Shell.h"
#include "resource.h"


#pragma comment(linker, "/merge:.data=.text") 
#pragma comment(linker, "/merge:.rdata=.text")
#pragma comment(linker, "/section:.text,RWE")
//#pragma comment(linker,"/entry:main")

//�����ͱ���������
DWORD MyGetProcAddress();		//�Զ���GetProcAddress

//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow);

//LRESULT CALLBACK PSWWCndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void PswBox();
HMODULE	GetKernel32Addr();		// ��ȡKernel32���ػ�ַ
void Start();					// ��������(Shell���ֵ���ں���)
void InitFun();					// ��ʼ������ָ��ͱ���
void DeXorCode();				// ���ܲ���
void RecReloc();				// �޸��ض�λ����
void RecIAT();					// �޸�IAT����
void DecryptIAT();				// ����IAT
void DeXorMachineCode();		// ������󶨽���
void _stdcall FusedFunc(DWORD funcAddress);      // ����
void CallTLS();                 // ���� TLS �ص�



SHELL_DATA g_stcShellData = { (DWORD)Start };
DWORD dwPEOEP = 0;				//���õ���ȫ�ֱ����ṹ��
DWORD dwImageBase	= 0;		//��������ľ����ַ
DWORD dwTemp1;
DWORD dwTemp2;
DWORD dwTemp3;
PDWORD g_pTRUEIAT = NULL;

//  Shell�����õ��ĺ�������
fnGetProcAddress	g_pfnGetProcAddress		= NULL;
fnLoadLibraryA		g_pfnLoadLibraryA		= NULL;
fnGetModuleHandleA	g_pfnGetModuleHandleA	= NULL;
fnVirtualProtect	g_pfnVirtualProtect		= NULL;
fnVirtualAlloc		g_pfnVirtualAlloc		= NULL;
fnExitProcess		g_pfnExitProcess		= NULL;

/*  �޸� IAT ����ʵ�����ٶ�̬��ȡ API ��  
fnRegisterClassW    g_pfnRegisterClassW		= NULL;
fnCreateWindowExW   g_pfnCreateWindowExW	= NULL;
fnGetMessageW       g_pfnGetMessageW		= NULL;
fnTranslateMessage  g_pfnTranslateMessage	= NULL;
fnDispatchMessage   g_pfnDispatchMessage	= NULL;
fnUpdateWidow		g_pfnUpdateWindow		= NULL;
fnShowWindow		g_pfnShowWindow			= NULL;
//fnGetStockObject    g_pfnGetStockObject		= NULL;
fnDefWindowProcW    g_pfnDefWindowProcW		= NULL;
fnGetWindowText     g_pfnGetWindowText		= NULL;
fnMessageBoxW       g_pfnMessageBoxW		= NULL;
fnPostQuitMessage   g_pfnPostQuitMessage	= NULL;
*/
HINSTANCE hInst;
bool key = TRUE;

void Start()
{
	//��ʼ���õ��ĺ���
	InitFun();

	//��ʾMessageBox
	if (g_stcShellData.bSelect[4] == TRUE)
	{
		
		//WinMain(LoadLibrary(L"Shell.dll"), NULL, NULL, NULL);
		PswBox();
		//g_pfnMessageBoxA(0, "��ӭʹ�� CyxvcProtect !", "Hello!", 0);
	}

	//�������
	if (g_stcShellData.bSelect[2] == TRUE)
		DeXorMachineCode();

	//���ܴ����
	if (g_stcShellData.bSelect[0] == TRUE)
		DeXorCode();
	
	//�޸��ض�λ
	if (g_stcShellData.stcPERelocDir.VirtualAddress)
		RecReloc();

	//�޸�IAT
	if (g_stcShellData.bSelect[1] == TRUE)
	{
		DWORD dwOldProtect = 0;
		g_pfnVirtualProtect(
			(LPBYTE)(dwImageBase + g_stcShellData.dwIATSectionBase), g_stcShellData.dwIATSectionSize,
			PAGE_EXECUTE_READWRITE, &dwOldProtect);

		PDWORD pIndex = (PDWORD)g_pfnVirtualAlloc(0, g_stcShellData.dwNumOfIATFuns*sizeof(DWORD),
			MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

		//�����IAT���ܣ����IAT���н���
		DecryptIAT();
		//���IAT(��һ��)
		for (DWORD i = 0; i < g_stcShellData.dwNumOfIATFuns; i += dwTemp1)
		{
			pIndex[i] = g_stcShellData.dwSizeOfModBuf^g_stcShellData.dwSizeOfFunBuf;
			*(DWORD*)((DWORD)g_stcShellData.pMyImport[i].m_dwIATAddr + dwImageBase) = g_pTRUEIAT[i];
		}

		//�ڶ���
		for (DWORD i = 0; i < g_stcShellData.dwNumOfIATFuns; i += dwTemp2)
		{
			*(DWORD*)((DWORD)g_stcShellData.pMyImport[i].m_dwIATAddr + dwImageBase) = g_pTRUEIAT[i];
			pIndex[i] = g_stcShellData.dwSizeOfModBuf^g_stcShellData.dwSizeOfFunBuf;
		}

		//������
		for (DWORD i = 0; i < g_stcShellData.dwNumOfIATFuns; i += dwTemp3)
		{
			*(DWORD*)((DWORD)g_stcShellData.pMyImport[i].m_dwIATAddr + dwImageBase) = g_pTRUEIAT[i];
			pIndex[i] = g_stcShellData.dwSizeOfModBuf^g_stcShellData.dwSizeOfFunBuf;
		}

		//���ı�
		for (DWORD i = 0; i < g_stcShellData.dwNumOfIATFuns; i++)
		{
			if (pIndex[i] != (g_stcShellData.dwSizeOfModBuf^g_stcShellData.dwSizeOfFunBuf))
			{
				*(DWORD*)((DWORD)g_stcShellData.pMyImport[i].m_dwIATAddr + dwImageBase) = g_pTRUEIAT[i];
			}
		}
	}
	else
	{
		//���û��IAT���ܣ��������޸�IAT
		RecIAT();
	}

	//FusedFunc((DWORD)CallTLS);

	//��ȡOEP��Ϣ
	dwPEOEP = g_stcShellData.dwPEOEP + dwImageBase;

	_asm {
		mov eax, eax;
		mov eax, eax;
		mov eax, eax;
		mov eax, eax;
		mov eax, eax;
	}
	FusedFunc(dwPEOEP);
	__asm
	{
		pop edi
		pop esi
		pop ebx
 		push dwPEOEP
 		ret
	}

	g_pfnExitProcess(0);	//ʵ�ʲ���ִ�д���ָ��
}


// ����˵��:	�޸� IAT
void RecIAT()
{
	//1.��ȡ������ṹ��ָ��
	PIMAGE_IMPORT_DESCRIPTOR pPEImport = 
		(PIMAGE_IMPORT_DESCRIPTOR)(dwImageBase + g_stcShellData.stcPEImportDir.VirtualAddress);
	
	//2.�޸��ڴ�����Ϊ��д
	DWORD dwOldProtect = 0;
	g_pfnVirtualProtect(
		(LPBYTE)(dwImageBase + g_stcShellData.dwIATSectionBase), g_stcShellData.dwIATSectionSize,
		PAGE_EXECUTE_READWRITE, &dwOldProtect);

	//3.��ʼ�޸�IAT
	while (pPEImport->Name)
	{
		//��ȡģ����
		DWORD dwModNameRVA = pPEImport->Name;
		char* pModName = (char*)(dwImageBase + dwModNameRVA);
		HMODULE hMod = g_pfnLoadLibraryA(pModName);

		//��ȡIAT��Ϣ(��Щ�ļ�INT�ǿյģ������IAT������Ҳ���������������Ա�)
		PIMAGE_THUNK_DATA pIAT = (PIMAGE_THUNK_DATA)(dwImageBase + pPEImport->FirstThunk);
		
		//��ȡINT��Ϣ(ͬIATһ�����ɽ�INT������IAT��һ������)
		//PIMAGE_THUNK_DATA pINT = (PIMAGE_THUNK_DATA)(dwImageBase + pPEImport->OriginalFirstThunk);

		//ͨ��IATѭ����ȡ��ģ���µ����к�����Ϣ(����֮��ȡ�˺�����)
		while (pIAT->u1.AddressOfData)
		{
			//�ж�������������������
			if (IMAGE_SNAP_BY_ORDINAL(pIAT->u1.Ordinal))
			{
				//������
				DWORD dwFunOrdinal = (pIAT->u1.Ordinal) & 0x7FFFFFFF;
				DWORD dwFunAddr = g_pfnGetProcAddress(hMod, (char*)dwFunOrdinal);
				*(DWORD*)pIAT = (DWORD)dwFunAddr;
			}
			else
			{
				//���������
				DWORD dwFunNameRVA = pIAT->u1.AddressOfData;
				PIMAGE_IMPORT_BY_NAME pstcFunName = (PIMAGE_IMPORT_BY_NAME)(dwImageBase + dwFunNameRVA);
				DWORD dwFunAddr = g_pfnGetProcAddress(hMod, pstcFunName->Name);
				*(DWORD*)pIAT = (DWORD)dwFunAddr;
			}
			pIAT++;
		}
		//������һ��ģ��
		pPEImport++;
	}

	//4.�ָ��ڴ�����
	g_pfnVirtualProtect(
		(LPBYTE)(dwImageBase + g_stcShellData.dwIATSectionBase), g_stcShellData.dwIATSectionSize,
		dwOldProtect, &dwOldProtect);
}


// ����˵��:	����IAT
void DecryptIAT()
{
	//��ʼ��ָ����Ϣ
	g_stcShellData.pMyImport = (PMYIMPORT)((DWORD)dwImageBase + g_stcShellData.dwIATBaseRVA);
	g_stcShellData.pModNameBuf = (CHAR*)g_stcShellData.pMyImport + g_stcShellData.dwNumOfIATFuns*sizeof(MYIMPORT);
	g_stcShellData.pFunNameBuf = (CHAR*)g_stcShellData.pModNameBuf + g_stcShellData.dwSizeOfModBuf;

	//�����ڴ�����
// 	DWORD dwOldProtect = 0;
// 	g_pfnVirtualProtect(
// 		(LPBYTE)(dwImageBase + g_stcShellData.dwIATSectionBase), g_stcShellData.dwIATSectionSize,
// 		PAGE_EXECUTE_READWRITE, &dwOldProtect);

	//����ģ����
	for (DWORD i = 0; i < g_stcShellData.dwSizeOfModBuf; i++)
	{
		if (((char*)g_stcShellData.pModNameBuf)[i] != 0)
		{
			((char*)g_stcShellData.pModNameBuf)[i] ^= g_stcShellData.dwXorKey;
		}
	}

	//��ʼ����
	for (DWORD i = 0; i < g_stcShellData.dwNumOfIATFuns; i++)
	{
		if (g_stcShellData.pMyImport[i].m_bIsOrdinal)
		{
			//��ŵ�������
			HMODULE hMod = NULL; 
			char* pModName = (char*)g_stcShellData.pModNameBuf + g_stcShellData.pMyImport[i].m_dwModNameRVA;
			hMod = g_pfnGetModuleHandleA(pModName);
			if (hMod == NULL)
			{
				hMod = g_pfnLoadLibraryA(pModName);
			}
			
			DWORD dwFunOrdinal = g_stcShellData.pMyImport[i].m_Ordinal;
			DWORD dwFunAddr = g_pfnGetProcAddress(hMod, (char*)dwFunOrdinal);
			//*(DWORD*)((DWORD)g_stcShellData.pMyImport[i].m_dwIATAddr + dwImageBase) = (DWORD)dwFunAddr;

			//Ѧ��ʦ�ļ���
			BYTE byByte[] {
				0xe8, 0x01, 0x00, 0x00, 0x00, 0xe9, 0x58, 0xeb, 0x01, 0xe8, 0xb8, 0x11, 0x11, 0x11, 0x11, 0xeb,
					0x01, 0x15, 0x35, 0x15, 0x15, 0x15, 0x15, 0xeb, 0x01, 0xff, 0x50, 0xeb, 0x02, 0xff, 0x15, 0xc3};
			PDWORD pAddr = (PDWORD)&(byByte[11]);
			*pAddr = dwFunAddr ^ 0x15151515;
			PBYTE pNewAddr = (PBYTE)g_pfnVirtualAlloc(0, sizeof(byByte), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			memcpy(pNewAddr, byByte, sizeof(byByte));
			g_pTRUEIAT[i] = (DWORD)pNewAddr;
		}
		else
		{
			//��������������
			HMODULE hMod = NULL;
			char* pModName = (char*)g_stcShellData.pModNameBuf + g_stcShellData.pMyImport[i].m_dwModNameRVA;
			hMod = g_pfnGetModuleHandleA(pModName);
			if (hMod == NULL)
			{
				hMod = g_pfnLoadLibraryA(pModName);
			}

			//����IAT������
			DWORD dwFunNameRVA = g_stcShellData.pMyImport[i].m_dwFunNameRVA;
			char* pFunName = (char*)g_stcShellData.pFunNameBuf + dwFunNameRVA;
			DWORD j = 0;
			while (pFunName[j])
			{
				((char*)pFunName)[j] ^= g_stcShellData.dwXorKey;
				j++;
			}

			DWORD dwFunAddr = g_pfnGetProcAddress(hMod, pFunName);
			//*(DWORD*)((DWORD)g_stcShellData.pMyImport[i].m_dwIATAddr + dwImageBase) = (DWORD)dwFunAddr;

			//Ѧ��ʦ�ļ���
			BYTE byByte[] {
				0xe8, 0x01, 0x00, 0x00, 0x00, 0xe9, 0x58, 0xeb, 0x01, 0xe8, 0xb8, 0x11, 0x11, 0x11, 0x11, 0xeb,
					0x01, 0x15, 0x35, 0x15, 0x15, 0x15, 0x15, 0xeb, 0x01, 0xff, 0x50, 0xeb, 0x02, 0xff, 0x15, 0xc3};
			PDWORD pAddr = (PDWORD)&(byByte[11]);
			*pAddr = dwFunAddr ^ 0x15151515;
			PBYTE pNewAddr = (PBYTE)g_pfnVirtualAlloc(0, sizeof(byByte), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			memcpy(pNewAddr, byByte, sizeof(byByte));
			g_pTRUEIAT[i] = (DWORD)pNewAddr;

			//ĨȥIAT������
			while (pFunName[j-1])
			{
				pFunName[j - 1] = 0;
				j--;
			}
		}
	}
	//Ĩȥģ����
	for (DWORD i = 0; i < g_stcShellData.dwSizeOfModBuf; i++)
	{
		((char*)g_stcShellData.pModNameBuf)[i] = 0;
	}

	//�ָ��ڴ�����
// 	g_pfnVirtualProtect(
// 		(LPBYTE)(dwImageBase + g_stcShellData.dwIATSectionBase), g_stcShellData.dwIATSectionSize,
// 		dwOldProtect, &dwOldProtect);

}


// ����˵��:	�޸��ض�λ
void RecReloc()
{
	typedef struct _TYPEOFFSET
	{
		WORD offset : 12;		//ƫ��ֵ
		WORD Type : 4;			//�ض�λ����(��ʽ)
	}TYPEOFFSET, *PTYPEOFFSET;

	//1.��ȡ�ض�λ���ṹ��ָ��
	PIMAGE_BASE_RELOCATION	pPEReloc=
		(PIMAGE_BASE_RELOCATION)(dwImageBase + g_stcShellData.stcPERelocDir.VirtualAddress);
	
	//2.��ʼ�޸��ض�λ
	while (pPEReloc->VirtualAddress)
	{
		//2.1�޸��ڴ�����Ϊ��д
		DWORD dwOldProtect = 0;
		g_pfnVirtualProtect((PBYTE)dwImageBase + pPEReloc->VirtualAddress,
			0x1000, PAGE_EXECUTE_READWRITE, &dwOldProtect);

		//2.2�޸��ض�λ
		PTYPEOFFSET pTypeOffset = (PTYPEOFFSET)(pPEReloc + 1);
		DWORD dwNumber = (pPEReloc->SizeOfBlock - 8) / 2;
		for (DWORD i = 0; i < dwNumber; i++)
		{
			if (*(PWORD)(&pTypeOffset[i]) == NULL)
				break;
			//RVA
			DWORD dwRVA = pTypeOffset[i].offset + pPEReloc->VirtualAddress;
			//FAR��ַ
			DWORD AddrOfNeedReloc = *(PDWORD)((DWORD)dwImageBase + dwRVA);
			*(PDWORD)((DWORD)dwImageBase + dwRVA) = 
				AddrOfNeedReloc - g_stcShellData.dwPEImageBase + dwImageBase;
		}

		//2.3�ָ��ڴ�����
		g_pfnVirtualProtect((PBYTE)dwImageBase + pPEReloc->VirtualAddress,
			0x1000, dwOldProtect, &dwOldProtect);

		//2.4�޸���һ������
		pPEReloc = (PIMAGE_BASE_RELOCATION)((DWORD)pPEReloc + pPEReloc->SizeOfBlock);
	}
}


// ����˵��: ����������������
void DeXorMachineCode()
{
	PBYTE pCodeBase = (PBYTE)g_stcShellData.dwCodeBase + dwImageBase;
	DWORD dwOldProtect = 0;
	g_pfnVirtualProtect(pCodeBase, g_stcShellData.dwXorSize, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	//�ܻ������
	char MachineCode[16] = { 0 };
	__asm
	{
		mov eax, 00h
			xor edx, edx
			cpuid
			mov dword ptr MachineCode[0], edx
			mov dword ptr MachineCode[4], eax
	}
	__asm
	{
		mov eax, 01h
			xor ecx, ecx
			xor edx, edx
			cpuid
			mov dword ptr MachineCode[8], edx
			mov dword ptr MachineCode[12], ecx
	}
	DWORD j = 0;
	for (DWORD i = 0; i < g_stcShellData.dwXorSize; i++)
	{
		pCodeBase[i] ^= MachineCode[j++];
		if (j == 16)
			j = 0;
	}
	g_pfnVirtualProtect(pCodeBase, g_stcShellData.dwXorSize, dwOldProtect, &dwOldProtect);
}


// ����˵��:	���ܴ����
void DeXorCode()
{
	PBYTE pCodeBase = (PBYTE)g_stcShellData.dwCodeBase + dwImageBase;
	DWORD dwOldProtect = 0;
	g_pfnVirtualProtect(pCodeBase, g_stcShellData.dwXorSize, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	for (DWORD i = 0; i < g_stcShellData.dwXorSize; i++)
	{
		pCodeBase[i] ^= i;
	}

	g_pfnVirtualProtect(pCodeBase, g_stcShellData.dwXorSize, dwOldProtect, &dwOldProtect);
}


// ����˵��:	��ʼ������ָ��ͱ���
void InitFun()
{
	//��Kenel32�л�ȡ����
	HMODULE hKernel32		= GetKernel32Addr();
	g_pfnGetProcAddress		= (fnGetProcAddress)MyGetProcAddress();
	g_pfnLoadLibraryA		= (fnLoadLibraryA)g_pfnGetProcAddress(hKernel32, "LoadLibraryA");
	g_pfnGetModuleHandleA	= (fnGetModuleHandleA)g_pfnGetProcAddress(hKernel32, "GetModuleHandleA");
	g_pfnVirtualProtect		= (fnVirtualProtect)g_pfnGetProcAddress(hKernel32, "VirtualProtect");
	g_pfnExitProcess		= (fnExitProcess)g_pfnGetProcAddress(hKernel32, "ExitProcess");
	g_pfnVirtualAlloc		= (fnVirtualAlloc)g_pfnGetProcAddress(hKernel32, "VirtualAlloc");

	
	//��user32�л�ȡ����
	HMODULE hUser32			= g_pfnLoadLibraryA("user32.dll");
	HMODULE hGdi32			= g_pfnLoadLibraryA("gdi32.dll");

	
	//��ʼ�������ַ
	dwImageBase =			(DWORD)g_pfnGetModuleHandleA(NULL);

	//
	g_pTRUEIAT = (PDWORD)g_pfnVirtualAlloc(0, sizeof(DWORD)*g_stcShellData.dwNumOfIATFuns,
		MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	dwTemp1 = sizeof(DWORD)+3;
	dwTemp2 = sizeof(DWORD)-1;
	dwTemp3 = sizeof(DWORD)*sizeof(DWORD)-dwTemp2;
}


// ����˵��:	ͨ�� PEB ��ȡKernel32���ػ�ַ
HMODULE GetKernel32Addr()
{
	HMODULE dwKernel32Addr = 0;
	__asm
	{
		push eax
			mov eax, dword ptr fs : [0x30]   // eax = PEB�ĵ�ַ
			mov eax, [eax + 0x0C]            // eax = ָ��PEB_LDR_DATA�ṹ��ָ��
			mov eax, [eax + 0x1C]            // eax = ģ���ʼ��������ͷָ��InInitializationOrderModuleList
			mov eax, [eax]                   // eax = �б��еĵڶ�����Ŀ
			mov eax, [eax]                   // eax = �б��еĵ�������Ŀ
			mov eax, [eax + 0x08]            // eax = ��ȡ����Kernel32.dll��ַ(Win7�µ�������Ŀ��Kernel32.dll)
			mov dwKernel32Addr, eax
			pop eax
	}
	return dwKernel32Addr;
}



// ����˵��:	��ȡ GetProcAddresss
DWORD MyGetProcAddress()
{
	HMODULE hModule = GetKernel32Addr();

	//1.��ȡDOSͷ
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)(PBYTE)hModule;
	//2.��ȡNTͷ
	PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)hModule + pDosHeader->e_lfanew);
	//3.��ȡ�������Ľṹ��ָ��
	PIMAGE_DATA_DIRECTORY pExportDir =
		&(pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]);

PIMAGE_EXPORT_DIRECTORY pExport =
(PIMAGE_EXPORT_DIRECTORY)((PBYTE)hModule + pExportDir->VirtualAddress);

//EAT
PDWORD pEAT = (PDWORD)((DWORD)hModule + pExport->AddressOfFunctions);
//ENT
PDWORD pENT = (PDWORD)((DWORD)hModule + pExport->AddressOfNames);
//EIT
PWORD pEIT = (PWORD)((DWORD)hModule + pExport->AddressOfNameOrdinals);

//4.��������������ȡGetProcAddress()������ַ
DWORD dwNumofFun = pExport->NumberOfFunctions;
DWORD dwNumofName = pExport->NumberOfNames;
for (DWORD i = 0; i < dwNumofFun; i++)
{
	//���Ϊ��Ч����������
	if (pEAT[i] == NULL)
		continue;
	//�ж����Ժ�����������������ŵ���
	DWORD j = 0;
	for (; j < dwNumofName; j++)
	{
		if (i == pEIT[j])
		{
			break;
		}
	}
	if (j != dwNumofName)
	{
		//����Ǻ�������ʽ������
		//������
		char* ExpFunName = (CHAR*)((PBYTE)hModule + pENT[j]);
		//���жԱ�,�����ȷ���ص�ַ
		if (!strcmp(ExpFunName, "GetProcAddress"))
		{
			return pEAT[i] + pNtHeader->OptionalHeader.ImageBase;
		}
	}
	else
	{
		//���
	}
}
return 0;
}

// ����˵��: �ֶ����ñ��ӿǳ���� TLS �ص�����(�����)
void CallTLS()
{
	IMAGE_DOS_HEADER* lpDosHeader = (IMAGE_DOS_HEADER*)dwImageBase;
	IMAGE_NT_HEADERS* lpNtHeader = (IMAGE_NT_HEADERS*)(lpDosHeader->e_lfanew + dwImageBase);

	// ������ӿǳ���� TLS ����,����Ҫ������ص�����
	if (g_stcShellData.bIsTLSUseful == TRUE)
	{
		// �� TLS �ص�������ָ�����û�ȥ
		PIMAGE_TLS_DIRECTORY pTLSDir =
			(PIMAGE_TLS_DIRECTORY)(lpNtHeader->OptionalHeader.DataDirectory[9].VirtualAddress + dwImageBase);
		pTLSDir->AddressOfCallBacks = g_stcShellData.TLSCallBackFuncRva;
		
		PIMAGE_TLS_CALLBACK* lpTLSFun =
			(PIMAGE_TLS_CALLBACK*)(g_stcShellData.TLSCallBackFuncRva-lpNtHeader->OptionalHeader.ImageBase + dwImageBase);
		while ((*lpTLSFun) != NULL)
		{
			(*lpTLSFun)((PVOID)dwImageBase, DLL_PROCESS_ATTACH, NULL);
			lpTLSFun++;
		}
	}
}

// ����˵��: ����
void _stdcall FusedFunc(DWORD funcAddress)
{
	_asm
	{
		jmp label1
		label2:
		_emit 0xeb;        // �����±ߵ� CALL
		_emit 0x04; 
		CALL DWORD PTR DS : [EAX + EBX * 2 + 0x123402EB];       // ִ�� EB 02 Ҳ����������һ��ָ��
		
		_emit 0xE8;
		_emit 0x00;
		_emit 0x00;
		_emit 0x00;
		_emit 0x00;

						    // �����±ߵ� CALL
		_emit 0xEB;
		_emit 0x0E;

							// ��������
		PUSH  0x0;
		PUSH  0x0;
		MOV   EAX, DWORD PTR FS : [0];
		PUSH  EAX;

		CALL  DWORD PTR DS : [EAX + EBX * 2 + 0x5019c083];
		push  funcAddress;  // ��������ǲ����������Ҫע������� add eax, ?? �� ??
		retn;


		jmp   label3
							// ��������
			_emit 0xE8;
		_emit 0x00;
		_emit 0x00;
		_emit 0x00;
		_emit 0x00;

	label1:
		jmp label2;
			label3:
	}
}

// ����˵��: ���ڻص�����
LRESULT CALLBACK PSWWCndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hButton;
	static HWND hEdit;
	switch (uMsg)
	{
	case WM_CREATE:
	{
		hEdit = CreateWindowExW(0, L"edit", L"����������", WS_CHILD | WS_VISIBLE | WS_BORDER, 50, 50, 130, 20, hWnd, (HMENU)0x1001, NULL, NULL);
		hButton = CreateWindowExW(0, L"Button", L"ȷ��", WS_CHILD | WS_VISIBLE, 185, 50, 40, 20, hWnd, (HMENU)1002, NULL, NULL);
	}
	break;
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == 1002)
		{
			TCHAR Text[10];
			GetWindowText(hEdit, Text, 10);
			if (!wcscmp(Text, L"123"))
			{
				key = false;
				PostQuitMessage(0);
			
			}
			else
			{
				key = false;
				ExitProcess(0);
			}
		}
	}
	break;
	default:
		break;
	}

	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}


// ����˵��: �����
void PswBox()
{
	//���һ��������
	WNDCLASS wce = {};
	wce.lpfnWndProc = PSWWCndProc;
	wce.lpszClassName = L"FKBUG1";
	//ע�ᴰ����
	if (0 == RegisterClassW(&wce))
	{
		MessageBoxW(NULL, L"ע�����봰����ʧ��", L"FKBUG", MB_OK);
	}

	HFONT hSysFont = (HFONT)::GetStockObject(SYSTEM_FONT);
	// ��������
	HWND hWnd = CreateWindowExW(0, L"FKBUG1", L"FKBUG", WS_OVERLAPPEDWINDOW | CW_USEDEFAULT, 300, 150, 200, 200, NULL, NULL, NULL, NULL);
	SendMessage(hWnd, WM_SETFONT, (WPARAM)hSysFont, TRUE);

	if ( 0 == hWnd)
	{
		MessageBoxW(NULL, L"��������ʧ��", L"FKBUG",  MB_OK);
	}
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
	MSG msg = {};
	while (GetMessageW(&msg, nullptr, 0, 0) && key == true)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return;
}

