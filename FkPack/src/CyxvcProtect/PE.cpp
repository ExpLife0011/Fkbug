#include "stdafx.h"
#include "PE.h"


CPE::CPE()
{
	InitValue();
}


CPE::~CPE()
{
}


void CPE::InitValue()
{
	m_hFile				= NULL;
	m_pFileBuf			= NULL;
	m_pDosHeader		= NULL;
	m_pNtHeader			= NULL;
	m_pSecHeader		= NULL;
	m_dwFileSize		= 0;
	m_dwImageSize		= 0;
	m_dwImageBase		= 0;
	m_dwCodeBase		= 0;
	m_dwCodeSize		= 0;
	m_dwPEOEP			= 0;
	m_dwShellOEP		= 0;
	m_dwSizeOfHeader	= 0;
	m_dwSectionNum		= 0;
	m_dwFileAlign		= 0;
	m_dwMemAlign		= 0;
	m_PERelocDir		= { 0 };
	m_PEImportDir		= { 0 };
	m_IATSectionBase	= 0;
	m_IATSectionSize	= 0;

	m_pMyImport			= 0;
	m_pModNameBuf		= 0;
	m_pFunNameBuf		= 0;
	m_dwNumOfIATFuns	= 0;
	m_dwSizeOfModBuf	= 0;
	m_dwSizeOfFunBuf	= 0;
	m_dwIATBaseRVA		= 0;
}


// ����˵��:	��ʼ��PE����ȡPE�ļ�������PE��Ϣ
BOOL CPE::InitPE(CString strFilePath)
{
	//���ļ�
	if (OpenPEFile(strFilePath) == FALSE)
		return FALSE;

	//��PE���ļ��ֲ���ʽ��ȡ���ڴ�
	m_dwFileSize = GetFileSize(m_hFile, NULL);
	m_pFileBuf = new BYTE[m_dwFileSize];
	DWORD ReadSize = 0;
	ReadFile(m_hFile, m_pFileBuf, m_dwFileSize, &ReadSize, NULL);	
	CloseHandle(m_hFile);
	m_hFile = NULL;

	//�ж��Ƿ�ΪPE�ļ�
	if (IsPE() == FALSE)
		return FALSE;

	//��PE���ڴ�ֲ���ʽ��ȡ���ڴ�
	//����û�����Сû�ж�������
	m_dwImageSize = m_pNtHeader->OptionalHeader.SizeOfImage;
	m_dwMemAlign = m_pNtHeader->OptionalHeader.SectionAlignment;
	m_dwSizeOfHeader = m_pNtHeader->OptionalHeader.SizeOfHeaders;
	m_dwSectionNum = m_pNtHeader->FileHeader.NumberOfSections;

	if (m_dwImageSize % m_dwMemAlign)
		m_dwImageSize = (m_dwImageSize / m_dwMemAlign + 1) * m_dwMemAlign;
	LPBYTE pFileBuf_New = new BYTE[m_dwImageSize];
	memset(pFileBuf_New, 0, m_dwImageSize);
	//�����ļ�ͷ
	memcpy_s(pFileBuf_New, m_dwSizeOfHeader, m_pFileBuf, m_dwSizeOfHeader);
	//��������
	PIMAGE_SECTION_HEADER pSectionHeader = IMAGE_FIRST_SECTION(m_pNtHeader);
	for (DWORD i = 0; i < m_dwSectionNum; i++, pSectionHeader++)
	{
		memcpy_s(pFileBuf_New + pSectionHeader->VirtualAddress,
			pSectionHeader->SizeOfRawData,
			m_pFileBuf+pSectionHeader->PointerToRawData,
			pSectionHeader->SizeOfRawData);
	}
	delete[] m_pFileBuf;
	m_pFileBuf = pFileBuf_New;
	pFileBuf_New = NULL;

	//��ȡPE��Ϣ
	GetPEInfo();
	
	return TRUE;
}


// ����˵��:	���ļ�
BOOL CPE::OpenPEFile(CString strFilePath)
{
	m_hFile = CreateFile(strFilePath,
		GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, _T("�����ļ�ʧ�ܣ�"), _T("��ʾ"), MB_OK);
		m_hFile = NULL;
		return FALSE;
	}
	return TRUE;
}

// ����˵��:	�ж��Ƿ�ΪPE�ļ�
BOOL CPE::IsPE()
{
	//�ж��Ƿ�ΪPE�ļ�
	m_pDosHeader = (PIMAGE_DOS_HEADER)m_pFileBuf;
	if (m_pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	{
		//����PE�ļ�
		MessageBox(NULL, _T("������Ч��PE�ļ���"), _T("��ʾ"), MB_OK);
		delete[] m_pFileBuf;
		InitValue();
		return FALSE;
	}
	m_pNtHeader = (PIMAGE_NT_HEADERS)(m_pFileBuf + m_pDosHeader->e_lfanew);
	if (m_pNtHeader->Signature != IMAGE_NT_SIGNATURE)
	{
		//����PE�ļ�
		MessageBox(NULL, _T("������Ч��PE�ļ���"), _T("��ʾ"), MB_OK);
		delete[] m_pFileBuf;
		InitValue();
		return FALSE;
	}
	return TRUE;
}

// ����˵��: RVA ת�ļ�ƫ��
DWORD CPE::RvaToOffset(DWORD Rva)
{
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)m_pFileBuf;
	PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(pDos->e_lfanew + m_pFileBuf);
	PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNt);
	for (int i = 0; i<pNt->FileHeader.NumberOfSections; i++)
	{
		if (Rva >= pSection->VirtualAddress&&
			Rva <= pSection->VirtualAddress + pSection->Misc.VirtualSize)
		{
			// ����ļ���ַΪ0,���޷����ļ����ҵ���Ӧ������
			if (pSection->PointerToRawData == 0)
			{
				return -1;
			}
			return Rva - pSection->VirtualAddress + pSection->PointerToRawData;
		}
		pSection = pSection + 1;
	}
}


// ����˵��:	��ȡPE��Ϣ
void CPE::GetPEInfo()
{
	m_pDosHeader	= (PIMAGE_DOS_HEADER)m_pFileBuf;
	m_pNtHeader		= (PIMAGE_NT_HEADERS)(m_pFileBuf + m_pDosHeader->e_lfanew);

	m_dwFileAlign	= m_pNtHeader->OptionalHeader.FileAlignment;
	m_dwMemAlign	= m_pNtHeader->OptionalHeader.SectionAlignment;
	m_dwImageBase	= m_pNtHeader->OptionalHeader.ImageBase;
	m_dwPEOEP		= m_pNtHeader->OptionalHeader.AddressOfEntryPoint;
	m_dwCodeBase	= m_pNtHeader->OptionalHeader.BaseOfCode;
	m_dwCodeSize	= m_pNtHeader->OptionalHeader.SizeOfCode;
	m_dwSizeOfHeader= m_pNtHeader->OptionalHeader.SizeOfHeaders;
	m_dwSectionNum	= m_pNtHeader->FileHeader.NumberOfSections;
	m_pSecHeader	= IMAGE_FIRST_SECTION(m_pNtHeader);
	m_pNtHeader->OptionalHeader.SizeOfImage = m_dwImageSize;

	//�����ض�λĿ¼��Ϣ
	m_PERelocDir = 
		IMAGE_DATA_DIRECTORY(m_pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]);

	//���� IAT Ŀ¼��Ϣ
	m_PEImportDir =
		IMAGE_DATA_DIRECTORY(m_pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]);

	//��ȡ IAT ���ڵ����ε���ʼλ�úʹ�С
	PIMAGE_SECTION_HEADER pSectionHeader = IMAGE_FIRST_SECTION(m_pNtHeader);
	for (DWORD i = 0; i < m_dwSectionNum; i++, pSectionHeader++)
	{
		if (m_PEImportDir.VirtualAddress >= pSectionHeader->VirtualAddress&&
			m_PEImportDir.VirtualAddress <= pSectionHeader[1].VirtualAddress)
		{
			//��������ε���ʼ��ַ�ʹ�С
			m_IATSectionBase = pSectionHeader->VirtualAddress;
			m_IATSectionSize = pSectionHeader[1].VirtualAddress - pSectionHeader->VirtualAddress;
			break;
		}
	}
}

// ����˵��:	����μ���
DWORD CPE::XorCode(BYTE byXOR)
{
	PBYTE pCodeBase = (PBYTE)((DWORD)m_pFileBuf + m_dwCodeBase);
	for (DWORD i = 0; i < m_dwCodeSize; i++)
	{
		pCodeBase[i] ^= i;
	}
	return m_dwCodeSize;
}

// ����˵��:	�������(��������ͬ����ν������)
void CPE::XorMachineCode(CHAR MachineCode[16])
{
	PBYTE pCodeBase = (PBYTE)((DWORD)m_pFileBuf + m_dwCodeBase);
	DWORD j = 0;
	for (DWORD i = 0; i < m_dwCodeSize; i++)
	{
		pCodeBase[i] ^= MachineCode[j++];
		if (j==16)
			j = 0;
	}
}


// ����˵��:	����Shell���ض�λ��Ϣ
BOOL CPE::SetShellReloc(LPBYTE pShellBuf, DWORD hShell)
{
	typedef struct _TYPEOFFSET
	{
		WORD offset : 12;			//ƫ��ֵ
		WORD Type	: 4;			//�ض�λ����(��ʽ)
	}TYPEOFFSET, *PTYPEOFFSET;

	//1.��ȡ���ӿ�PE�ļ����ض�λĿ¼��ָ����Ϣ
	PIMAGE_DATA_DIRECTORY pPERelocDir =
		&(m_pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]);
	
	//2.��ȡShell���ض�λ��ָ����Ϣ
	PIMAGE_DOS_HEADER		pShellDosHeader = (PIMAGE_DOS_HEADER)pShellBuf;
	PIMAGE_NT_HEADERS		pShellNtHeader = (PIMAGE_NT_HEADERS)(pShellBuf + pShellDosHeader->e_lfanew);
	PIMAGE_DATA_DIRECTORY	pShellRelocDir =
		&(pShellNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]);
	PIMAGE_BASE_RELOCATION	pShellReloc = 
		(PIMAGE_BASE_RELOCATION)((DWORD)pShellBuf + pShellRelocDir->VirtualAddress);
	
	//3.��ԭ�޸��ض�λ��Ϣ
	//����Shell.dll��ͨ��LoadLibrary���صģ�����ϵͳ��������һ���ض�λ
	//������Ҫ��Shell.dll���ض�λ��Ϣ�ָ���ϵͳû����ǰ�����ӣ�Ȼ����д�뱻�ӿ��ļ���ĩβ
	while (pShellReloc->VirtualAddress)
	{		
		PTYPEOFFSET pTypeOffset = (PTYPEOFFSET)(pShellReloc + 1);
		DWORD dwNumber = (pShellReloc->SizeOfBlock - 8) / 2;

		for (DWORD i = 0; i < dwNumber; i++)
		{
			if (*(PWORD)(&pTypeOffset[i]) == NULL)
				break;
			//RVA
			DWORD dwRVA = pTypeOffset[i].offset + pShellReloc->VirtualAddress;
			//FAR��ַ
			//***�µ��ض�λ��ַ=�ض�λ��ַ-ӳ���ַ+�µ�ӳ���ַ+�����ַ
			DWORD AddrOfNeedReloc = *(PDWORD)((DWORD)pShellBuf + dwRVA);
			*(PDWORD)((DWORD)pShellBuf + dwRVA)
				= AddrOfNeedReloc - pShellNtHeader->OptionalHeader.ImageBase + m_dwImageBase + m_dwImageSize;
		}
		//3.1�޸�Shell�ض�λ����.text��RVA
		pShellReloc->VirtualAddress += m_dwImageSize;
		// �޸���һ������
		pShellReloc = (PIMAGE_BASE_RELOCATION)((DWORD)pShellReloc + pShellReloc->SizeOfBlock);
	}

	//4.�޸�PE�ض�λĿ¼ָ�룬ָ��Shell���ض�λ����Ϣ
	pPERelocDir->Size = pShellRelocDir->Size;
	pPERelocDir->VirtualAddress = pShellRelocDir->VirtualAddress + m_dwImageSize;

	return TRUE;
}


// ����˵��:	�ϲ����뱻�ӿǳ���
void CPE::MergeBuf(LPBYTE pFileBuf, DWORD pFileBufSize,
	LPBYTE pShellBuf, DWORD pShellBufSize, 
	LPBYTE& pFinalBuf, DWORD& pFinalBufSize)
{
	//��ȡ���һ�����ε���Ϣ
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pFileBuf;
	PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)(pFileBuf + pDosHeader->e_lfanew);
	PIMAGE_SECTION_HEADER pSectionHeader = IMAGE_FIRST_SECTION(pNtHeader);
	PIMAGE_SECTION_HEADER pLastSection =
		&pSectionHeader[pNtHeader->FileHeader.NumberOfSections - 1];

	//1.�޸���������
	pNtHeader->FileHeader.NumberOfSections += 1;

	//2.�༭���α�ͷ�ṹ����Ϣ
	PIMAGE_SECTION_HEADER AddSectionHeader =
		&pSectionHeader[pNtHeader->FileHeader.NumberOfSections - 1];
	memcpy_s(AddSectionHeader->Name, 8, ".FKBUG", 6);

	//VOffset(1000����)
	DWORD dwTemp = 0;
	dwTemp = (pLastSection->Misc.VirtualSize / m_dwMemAlign) * m_dwMemAlign;
	if (pLastSection->Misc.VirtualSize % m_dwMemAlign)
	{
		dwTemp += m_dwMemAlign;
	}
	AddSectionHeader->VirtualAddress = pLastSection->VirtualAddress + dwTemp;

	//Vsize��ʵ����ӵĴ�С��
	AddSectionHeader->Misc.VirtualSize = pShellBufSize;

	//ROffset�����ļ���ĩβ��
	AddSectionHeader->PointerToRawData = pFileBufSize;

	//RSize(200����)
	dwTemp = (pShellBufSize / m_dwFileAlign) * m_dwFileAlign;
	if (pShellBufSize % m_dwFileAlign)
	{
		dwTemp += m_dwFileAlign;
	}
	AddSectionHeader->SizeOfRawData = dwTemp;

	//��־
	AddSectionHeader->Characteristics = 0XE0000040;

	//3.�޸� PE ͷ�ļ���С���ԣ������ļ���С
	dwTemp = (pShellBufSize / m_dwMemAlign) * m_dwMemAlign;
	if (pShellBufSize % m_dwMemAlign)
	{
		dwTemp += m_dwMemAlign;
	}
	pNtHeader->OptionalHeader.SizeOfImage += dwTemp;


	//4.����ϲ�����Ҫ�Ŀռ�
	//4.0.���㱣��IAT���õĿռ��С
	DWORD dwIATSize = 0;
	dwIATSize = m_dwSizeOfModBuf + m_dwSizeOfFunBuf + m_dwNumOfIATFuns*sizeof(MYIMPORT);

	if (dwIATSize % m_dwMemAlign)
	{
		dwIATSize = (dwIATSize / m_dwMemAlign + 1)*m_dwMemAlign;
	}
	pNtHeader->OptionalHeader.SizeOfImage += dwIATSize;
	AddSectionHeader->Misc.VirtualSize += dwIATSize;
	AddSectionHeader->SizeOfRawData += dwIATSize;

	pFinalBuf = new BYTE[pFileBufSize + dwTemp + dwIATSize];
	pFinalBufSize = pFileBufSize + dwTemp + dwIATSize;
	memset(pFinalBuf, 0, pFileBufSize + dwTemp + dwIATSize);
	memcpy_s(pFinalBuf, pFileBufSize, pFileBuf, pFileBufSize);
	memcpy_s(pFinalBuf + pFileBufSize, dwTemp, pShellBuf, dwTemp);

	//����IAT��Ϣ
	if (dwIATSize == 0)
	{
		return;
	}
	DWORD dwIATBaseRVA = pFileBufSize + pShellBufSize;
	m_dwIATBaseRVA = dwIATBaseRVA;

	memcpy_s(pFinalBuf + dwIATBaseRVA,
		dwIATSize, m_pMyImport, m_dwNumOfIATFuns*sizeof(MYIMPORT));

	//����ģ����
	for (DWORD i = 0; i < m_dwSizeOfModBuf; i++)
	{
		if (((char*)m_pModNameBuf)[i] != 0)
		{
			((char*)m_pModNameBuf)[i] ^= 0x15;
		}
	}

	memcpy_s(pFinalBuf + dwIATBaseRVA + m_dwNumOfIATFuns*sizeof(MYIMPORT),
		dwIATSize, m_pModNameBuf, m_dwSizeOfModBuf);

	//IAT����������
	for (DWORD i = 0; i < m_dwSizeOfFunBuf; i++)
	{
		if (((char*)m_pFunNameBuf)[i] != 0)
		{
			((char*)m_pFunNameBuf)[i] ^= 0x15;
		}
	}

	memcpy_s(pFinalBuf + dwIATBaseRVA + m_dwNumOfIATFuns*sizeof(MYIMPORT)+m_dwSizeOfModBuf,
		dwIATSize, m_pFunNameBuf, m_dwSizeOfFunBuf);
}


// ����˵��:	�޸��µ�OEPΪShell��Start����
void CPE::SetNewOEP(DWORD dwShellOEP)
{
	m_dwShellOEP = dwShellOEP + m_dwImageSize;
	m_pNtHeader->OptionalHeader.AddressOfEntryPoint = m_dwShellOEP;
}



// ����˵��:	ĨȥIAT(�����)����
void CPE::ClsImportTab()
{
	if (m_PEImportDir.VirtualAddress == 0)
	{
		return;
	}
	//1.��ȡ�����ṹ��ָ��
	PIMAGE_IMPORT_DESCRIPTOR pPEImport =
		(PIMAGE_IMPORT_DESCRIPTOR)(m_pFileBuf + m_PEImportDir.VirtualAddress);

	//2.��ʼѭ��ĨȥIAT(�����)����
	//ÿѭ��һ��Ĩȥһ��Dll�����е�����Ϣ
	while (pPEImport->Name)
	{
		//2.1.Ĩȥģ����
		DWORD dwModNameRVA = pPEImport->Name;
		char* pModName = (char*)(m_pFileBuf + dwModNameRVA);
		memset(pModName, 0, strlen(pModName));

		PIMAGE_THUNK_DATA pIAT = (PIMAGE_THUNK_DATA)(m_pFileBuf + pPEImport->FirstThunk);
		PIMAGE_THUNK_DATA pINT = (PIMAGE_THUNK_DATA)(m_pFileBuf + pPEImport->OriginalFirstThunk);

		//2.2.ĨȥIAT��INT�ͺ������������
		while (pIAT->u1.AddressOfData)
		{
			//�ж�������������������
			if (IMAGE_SNAP_BY_ORDINAL(pIAT->u1.Ordinal))
			{
				//Ĩȥ��ž��ǽ�pIAT���
			}
			else
			{
				//���������
				DWORD dwFunNameRVA = pIAT->u1.AddressOfData;
				PIMAGE_IMPORT_BY_NAME pstcFunName = (PIMAGE_IMPORT_BY_NAME)(m_pFileBuf + dwFunNameRVA);
				//����������ͺ������
				memset(pstcFunName, 0, strlen(pstcFunName->Name) + sizeof(WORD));
			}
			memset(pINT, 0, sizeof(IMAGE_THUNK_DATA));
			memset(pIAT, 0, sizeof(IMAGE_THUNK_DATA));
			pINT++;
			pIAT++;
		}

		//2.3.Ĩȥ�����Ŀ¼��Ϣ
		memset(pPEImport, 0, sizeof(IMAGE_IMPORT_DESCRIPTOR));

		//������һ��ģ��
		pPEImport++;
	}
}


// ����˵��: ���� TLS �ص�����
DWORD CPE::DealWithTLS(PSHELL_DATA & pPackInfo)
{
	if (m_pNtHeader->OptionalHeader.DataDirectory[9].VirtualAddress == 0)
	{
		return false;
	}
	else
	{
		PIMAGE_TLS_DIRECTORY32 g_lpTlsDir =
			(PIMAGE_TLS_DIRECTORY32)
			(RvaToOffset(m_pNtHeader->OptionalHeader.DataDirectory[9].VirtualAddress) + m_pFileBuf);

		// ��ȡ TLSIndex �� �ļ�ƫ��
		DWORD IndexOfOffset = RvaToOffset(g_lpTlsDir->AddressOfIndex - m_dwImageBase);

		// ���� TLSIndex ��ֵ
		pPackInfo->TLSIndex = 0;
		if (IndexOfOffset != -1)
		{
			pPackInfo->TLSIndex = *(DWORD*)(IndexOfOffset + m_pFileBuf);
		}

		// ���� TLS ���е���Ϣ
		m_StartOfDataAddress = g_lpTlsDir->StartAddressOfRawData;
		m_EndOfDataAddresss = g_lpTlsDir->EndAddressOfRawData;
		m_CallBackFuncAddress = g_lpTlsDir->AddressOfCallBacks;

		// �� TLS �ص����� rva ���õ�������Ϣ�ṹ��
		pPackInfo->TLSCallBackFuncRva = m_CallBackFuncAddress;
		return true;
	}
}

// ����˵��: ���� TLS �ص�����
// 1. �� ���ӿǳ���� TLS ����Ŀ¼��ָ��ǵ�
// 2. ͨ�� TLS ���е� StartAddressOfRawData ��������Ѱ��
// 3. �� Index �����������ӿ���֮�佻�������ݽṹ,������������� RVA(���ض�λ������Ϊ Rva-����λ�ַ+FKBUG�λ�ַ+���ӿǳ�����ػ�ַ)
// 4. �ǵ� TLS ��ǰ����ͬ ���ӿǳ���� TLS ��, ����ֵ����Ҫ���ض�λ֮������Ϊ�ͱ��ӿǳ���� TLS ������ͬ
// 5. �ǵ� AddressOfFunc ͬ���ӿǳ���� TLS ��, ����ֵ��Ҳ����Ҫ�ض�λ֮����Ϊ�뱻�ӿǳ�����ͬ
void CPE::SetTLS(DWORD NewSectionRva, PCHAR pStubBuf, PSHELL_DATA pPackInfo)
{
	PIMAGE_DOS_HEADER pStubDos = (PIMAGE_DOS_HEADER)pStubBuf;
	PIMAGE_NT_HEADERS pStubNt = (PIMAGE_NT_HEADERS)
		(pStubDos->e_lfanew + pStubBuf);

	PIMAGE_DOS_HEADER pPeDos = (PIMAGE_DOS_HEADER)m_pFileBuf;
	PIMAGE_NT_HEADERS pPeNt = (PIMAGE_NT_HEADERS)(pPeDos->e_lfanew + m_pFileBuf);

	PIMAGE_TLS_DIRECTORY32 pITD =
		(PIMAGE_TLS_DIRECTORY32)(RvaToOffset(pPeNt->OptionalHeader.DataDirectory[9].VirtualAddress) + m_pFileBuf);

	// �����ӿǳ���� TLS ��ָ��ǵ� TLS��
	pPeNt->OptionalHeader.DataDirectory[9].VirtualAddress =
		(pStubNt->OptionalHeader.DataDirectory[9].VirtualAddress - 0x1000) + NewSectionRva;
	
	// ��ȡ�������ݽṹ�� TLSIndex �� Rva
	DWORD IndexRva = ((DWORD)pPackInfo - (DWORD)pStubBuf + 4) - 0x1000 + NewSectionRva + pPeNt->OptionalHeader.ImageBase;
	pITD->AddressOfIndex = IndexRva;
	pITD->StartAddressOfRawData = m_StartOfDataAddress;
	pITD->EndAddressOfRawData = m_EndOfDataAddresss;

	// Ȼ����ȡ�� TLS �Ļص�����, �򽻻����ݽṹ���д��� TLS �ص�����ָ��, �ڿ�����ֶ����� TLS�ص�����,������û�ȥ
	pITD->AddressOfCallBacks = 0;

}



// ����˵��: �ӿǵ�ʱ��� IAT(�����) �������
void CPE::SaveImportTab()
{
	if (m_PEImportDir.VirtualAddress == 0)
	{
		return;
	}
	//0.��ȡ�����ṹ��ָ��
	PIMAGE_IMPORT_DESCRIPTOR pPEImport =
		(PIMAGE_IMPORT_DESCRIPTOR)(m_pFileBuf + m_PEImportDir.VirtualAddress);

	//1.��һ��ѭ��ȷ�� m_pModNameBuf �� m_pFunNameBuf �Ĵ�С
	DWORD dwSizeOfModBuf = 0;
	DWORD dwSizeOfFunBuf = 0;
	m_dwNumOfIATFuns = 0;
	while (pPEImport->Name)
	{
		DWORD dwModNameRVA = pPEImport->Name;
		char* pModName = (char*)(m_pFileBuf + dwModNameRVA);
		dwSizeOfModBuf += (strlen(pModName) + 1);

		PIMAGE_THUNK_DATA pIAT = (PIMAGE_THUNK_DATA)(m_pFileBuf + pPEImport->FirstThunk);

		while (pIAT->u1.AddressOfData)
		{
			if (IMAGE_SNAP_BY_ORDINAL(pIAT->u1.Ordinal))
			{
				m_dwNumOfIATFuns++;
			}
			else
			{
				m_dwNumOfIATFuns++;
				DWORD dwFunNameRVA = pIAT->u1.AddressOfData;
				PIMAGE_IMPORT_BY_NAME pstcFunName = (PIMAGE_IMPORT_BY_NAME)(m_pFileBuf + dwFunNameRVA);
				dwSizeOfFunBuf += (strlen(pstcFunName->Name) + 1);
			}
			pIAT++;
		}
		pPEImport++;
	}

	//2.�ڶ���ѭ��������Ϣ���Լ���������ݽṹ���
	m_pModNameBuf = new CHAR[dwSizeOfModBuf];
	m_pFunNameBuf = new CHAR[dwSizeOfFunBuf];
	m_pMyImport = new MYIMPORT[m_dwNumOfIATFuns];
	memset(m_pModNameBuf, 0, dwSizeOfModBuf);
	memset(m_pFunNameBuf, 0, dwSizeOfFunBuf);
	memset(m_pMyImport, 0, sizeof(MYIMPORT)*m_dwNumOfIATFuns);

	pPEImport =	(PIMAGE_IMPORT_DESCRIPTOR)(m_pFileBuf + m_PEImportDir.VirtualAddress);
	DWORD TempNumOfFuns = 0;
	DWORD TempModRVA = 0;
	DWORD TempFunRVA = 0;
	while (pPEImport->Name)
	{
		DWORD dwModNameRVA = pPEImport->Name;
		char* pModName = (char*)(m_pFileBuf + dwModNameRVA);
		memcpy_s((PCHAR)m_pModNameBuf + TempModRVA, strlen(pModName) + 1, 
			pModName, strlen(pModName) + 1);

		PIMAGE_THUNK_DATA pIAT = (PIMAGE_THUNK_DATA)(m_pFileBuf + pPEImport->FirstThunk);

		while (pIAT->u1.AddressOfData)
		{
			if (IMAGE_SNAP_BY_ORDINAL(pIAT->u1.Ordinal))
			{
				//��������ŵ��뷽ʽ�ĺ�����Ϣ
				m_pMyImport[TempNumOfFuns].m_dwIATAddr = (DWORD)pIAT - (DWORD)m_pFileBuf;
				m_pMyImport[TempNumOfFuns].m_bIsOrdinal = TRUE;
				m_pMyImport[TempNumOfFuns].m_Ordinal = pIAT->u1.Ordinal & 0x7FFFFFFF;
				m_pMyImport[TempNumOfFuns].m_dwModNameRVA = TempModRVA;
			}
			else
			{
				//�������Ƶ��뷽ʽ�ĺ�����Ϣ
				m_pMyImport[TempNumOfFuns].m_dwIATAddr = (DWORD)pIAT - (DWORD)m_pFileBuf;

				DWORD dwFunNameRVA = pIAT->u1.AddressOfData;
				PIMAGE_IMPORT_BY_NAME pstcFunName = (PIMAGE_IMPORT_BY_NAME)(m_pFileBuf + dwFunNameRVA);
				memcpy_s((PCHAR)m_pFunNameBuf + TempFunRVA, strlen(pstcFunName->Name) + 1,
					pstcFunName->Name, strlen(pstcFunName->Name) + 1);

				m_pMyImport[TempNumOfFuns].m_dwFunNameRVA = TempFunRVA;
				m_pMyImport[TempNumOfFuns].m_dwModNameRVA = TempModRVA;
				TempFunRVA += (strlen(pstcFunName->Name) + 1);
			}
			TempNumOfFuns++;
			pIAT++;
		}
		TempModRVA += (strlen(pModName) + 1);
		pPEImport++;
	}

	//�������� m_pMyImport
	MYIMPORT stcTemp = { 0 };
	DWORD dwTempNum = m_dwNumOfIATFuns / 2;
	for (DWORD i = 0; i < dwTempNum; i++)
	{
		m_pMyImport[i];
		m_pMyImport[m_dwNumOfIATFuns - i - 1];
		memcpy_s(&stcTemp, sizeof(MYIMPORT), &m_pMyImport[i], sizeof(MYIMPORT));
		memcpy_s(&m_pMyImport[i], sizeof(MYIMPORT), &m_pMyImport[m_dwNumOfIATFuns - i - 1], sizeof(MYIMPORT));
		memcpy_s(&m_pMyImport[m_dwNumOfIATFuns - i - 1], sizeof(MYIMPORT), &stcTemp, sizeof(MYIMPORT));
	}

	//������Ϣ
	m_dwSizeOfModBuf = dwSizeOfModBuf;
	m_dwSizeOfFunBuf = dwSizeOfFunBuf;
}







