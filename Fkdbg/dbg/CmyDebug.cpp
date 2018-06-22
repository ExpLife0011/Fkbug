#include "stdafx.h"
#include "CmyDebug.h"
#include "Cdbg_Info.h"


CmyDebug::CmyDebug()
{
	//m_context.ContextFlags = CONTEXT_ALL;    // �����߳������ķ���Ȩ��
	m_MemStep = false;
	m_ConditionStep = false;
	m_Oep = false;
}


CmyDebug::~CmyDebug()
{
}

//  �������ԻỰ
void CmyDebug::CreateDebug()
{
	printf("exe:");
	// ȡ�ļ�·�����������ԻỰ

	gets_s(m_path, MAX_PATH);

	//�ȱ��滺����
	TCHAR Tea[MAX_PATH] = {};
	MultiByteToWideChar(CP_ACP, NULL, m_path, -1, Tea, MAX_PATH);

	DWORD dwHeight = 0;
	DWORD dwFileSize = 0;

	m_Size = GetFileSize(m_hfile, &dwHeight);
	m_pBuff = new BYTE[dwFileSize]{};

	DWORD rea = 0;

	ReadFile(m_hfile, m_pBuff, dwFileSize, &rea, NULL);
	
	//CloseHandle(m_hfile);

	STARTUPINFOA si = { sizeof(STARTUPINFOA) };
	PROCESS_INFORMATION pi = { 0 };
	BOOL ret = 0;
	ret = CreateProcessA(m_path,
		NULL,
		NULL,
		NULL,
		FALSE,
		DEBUG_ONLY_THIS_PROCESS | CREATE_NEW_CONSOLE,
		NULL,
		NULL,
		&si,
		&pi);

	assert(ret);
	return;
}

// ΢��ĵ��������
void CmyDebug::EnterDebugLoop()
{
	CreateDebug();

	DWORD dwContinueStatus = DBG_CONTINUE;     // exception continuation 
	DEBUG_EVENT dbgEvent = { 0 };
	HANDLE hProcess = { 0 };
	HANDLE hThread = { 0 };

	for (;;)
	{
		WaitForDebugEvent(&dbgEvent, INFINITE);

		m_process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dbgEvent.dwProcessId);
		m_Thread = OpenThread(THREAD_ALL_ACCESS, FALSE, dbgEvent.dwThreadId);
  
		switch (dbgEvent.dwDebugEventCode)
		{
		case EXCEPTION_DEBUG_EVENT:	
			dwContinueStatus = OnExceptionDebugEvent(dbgEvent);
			break;
		case CREATE_THREAD_DEBUG_EVENT:printf(" �̴߳����¼�\n");  break;
		case CREATE_PROCESS_DEBUG_EVENT:
			printf(" ���̴����¼�\n"); 
			m_bpAbout.SetBP_INT3(m_process, (SIZE_T)dbgEvent.u.CreateProcessInfo.lpStartAddress, true);
			printf("���� OEP �ϵ�\n");
			m_Oep = true;

			m_processID = dbgEvent.dwProcessId;
			break;
		case EXIT_THREAD_DEBUG_EVENT:   printf(" �˳��߳��¼�\n"); break;
		case EXIT_PROCESS_DEBUG_EVENT:  printf(" �˳������¼�\n"); break;
		case LOAD_DLL_DEBUG_EVENT:      printf(" ӳ��DLL�¼�\n"); break;
		case UNLOAD_DLL_DEBUG_EVENT:    printf(" ж��DLL�¼�\n"); break;
		case OUTPUT_DEBUG_STRING_EVENT: printf(" �����ַ�������¼�\n"); break;
		case RIP_EVENT:                 printf(" RIP�¼�(�ڲ�����)\n"); break;
		}

		// Resume executing the thread that reported the debugging event. 

		// �����Խ��̽���ʱ�ͷ���Դ
		CloseHandle(hThread);         
		CloseHandle(hProcess);

		ContinueDebugEvent(dbgEvent.dwProcessId,dbgEvent.dwThreadId,dwContinueStatus);
	}


}

void CmyDebug::showDisasm(HANDLE hProc, LPVOID address, int nLen)
{
	// system("cls");

	// 1. �����̵��ڴ��ȡ����
	// 2. ʹ�÷��������ɨ���ڴ�,��ȡ�����
	// 3. ��������

	BYTE  OPcode[512];
	DWORD dwRead = 0;
	DWORD dwOldProtect = 0;        // �洢ԭ����ҳ������
	
	// �޸�ҳ������
	VirtualProtectEx(m_process, (LPVOID)address, 512, PAGE_READWRITE, &dwOldProtect);
	ReadProcessMemory(m_process, (LPVOID)address, OPcode, 512, &dwRead);
	VirtualProtectEx(m_process, (LPVOID)address, 512, dwOldProtect, &dwOldProtect);
	if (dwRead != 512)
	{
		printf("��ȡ�ڴ�ʧ��\n");
		return;
	}


// 	char* pBuff = new char[nLen * 16];
// 	DWORD dwRead = 0;
// 	if (!ReadProcessMemory(hProc, address, pBuff, nLen * 16, &dwRead))
// 		return;

	
	DISASM d = { 0 };
	CString funName = {};              // ������Ϣ
	d.EIP = (UIntPtr)OPcode;
	d.VirtualAddr = (UInt64)address;
	int OPcodeSubscript = 0;          // ��¼ OPCODE λ��   
	

	while (nLen > 0) {
		int len = Disasm(&d);
		if (len == -1) {
			printf("��������\n");
			return;
		}
		printf("%08X |", (DWORD)d.VirtualAddr);
		// һ���ֽ�һ���ֽڵĴ�ӡ����ָ��ȵ� OPCODE
		for (int i = 0; i < len; i++)
		{
			printf("%02X", OPcode[i+OPcodeSubscript]);
		}
		// 7�ֽڲ�� �����ָ��� OPCODE ������
		if (len < 7)
		{
			for (int i = 2*(7 - len); i > 0; i--)
			{
				printf(" ");
			}
		}
		printf(" |");
		printf("%-35s", d.CompleteInstr);
		printf(" |");

		// ����Ǹ� CALL �� JMP �ͳ��Ի�ȡ������Ϣ����ӡ
		if (d.Instruction.Opcode == 0xE8 || d.Instruction.Opcode == 0xEB)
		{
			GetSymFuncName(d.Instruction.AddrValue, funName);
			wprintf(L"%s", funName);
		}
		printf("\n");

		d.EIP += len;
		OPcodeSubscript += len;
		d.VirtualAddr += len;
		--nLen;
	}

	ShowRegister();
	ShowStack();
}

// �����û����������Ӧ����
void CmyDebug::userInput(EXCEPTION_RECORD& record)
{
	char cmdLine[512];
	while (true) {
		printf(">");
		scanf_s("%s", cmdLine, 512);

		// ��������
		if (_stricmp(cmdLine, "t") == 0) {

			m_bpAbout.SetBP_TF(m_Thread);
			return;   // �˳���������
		}

		// ��������
		else if (_stricmp(cmdLine, "g") == 0) {
			return;
		}

		// ���������
		else if (_stricmp(cmdLine, "u") == 0) {
			printf("����೤��:");
			int a = 0;
			scanf_s("%d", &a);
			showDisasm(m_process, record.ExceptionAddress, a);
		}
		
		// �鿴ģ������
		else if (_stricmp(cmdLine, "module") == 0) {
			TraverseModule(m_processID);
		}

		// dump ת������
		else if (_stricmp(cmdLine, "dump") == 0) {
			Dump("ת������.exe", m_path);
		}
		// ������
		else if (_stricmp(cmdLine, "export") == 0) {
			TraverseModule(m_processID);
			printf("Ҫ������ģ��:");
			TCHAR mod[512];
			wscanf_s(L"%s", mod, 512);
			PaserExportTable(ModuleToAddr(mod, m_processID));
		}
		else if (_stricmp(cmdLine, "import") == 0) {
			TraverseModule(m_processID);
			printf("Ҫ������ģ��:");
			TCHAR mod[512];
			wscanf_s(L"%s", mod, 512);
			PaserImportTable(ModuleToAddr(mod, m_processID));
		}
		// ��������ϵ�����
		else if (_stricmp(cmdLine, "bp") == 0) {
			// ��������ϵ��ַ  
			printf("����ϵ��ַ(Hex):");
			SIZE_T  address = 0;
			scanf_s("%X", &address);
			// �����Ƿ�Ϊ����������ϵ�
			char    forever[10];
			bool    delflag = true;
			printf("�Ƿ������Զϵ�(y/n)��");
			scanf_s("%s", &forever, 10);
			if (_stricmp(forever, "y") == 0)
			{
				delflag = false;
			}
			m_bpAbout.SetBP_INT3(m_process, address, delflag);
		}

		// ����Ӳ��ִ�жϵ�����
		else if (_stricmp(cmdLine, "bae") == 0)
		{
			printf("����ϵ��ַ(Hex):");
			SIZE_T  address = 0;
			scanf_s("%X", &address);
			char    forever[10];
			bool    delflag = true;
			printf("�Ƿ������Զϵ�(y/n)��");
			scanf_s("%s", &forever, 10);
			if (_stricmp(forever, "y") == 0)
			{
				delflag = false;
			}
			m_bpAbout.SetBP_HARD(m_Thread, address, BP_HARD_EXEC, K1_BYTE, delflag);
		}

		// ����Ӳ����/д���ݶϵ�
		else if (_stricmp(cmdLine, "bar") == 0)
		{
			printf("����Ӳ�����ݷ��ʶϵ��ַ(Hex):");
			SIZE_T  address = 0;
			scanf_s("%X", &address);
			char    forever[10];
			bool    delflag = true;
			int     nLen = 0;
			printf("�Ƿ������Զϵ�(y/n)��");
			scanf_s("%s", &forever, 10);
			if (_stricmp(forever, "y") == 0)
			{
				delflag = false;
			}
			printf("���ݶ�д�ϵ㳤��(1/2/4):");
			scanf_s("%d", &nLen);
			if (nLen >= 1)
			{
				if (nLen % 4 == 0)
				{
					m_bpAbout.SetBP_HARD(m_Thread, address, BP_HARD_RWDATA, K4_BYTE, delflag);
				}
				else if (nLen % 2 == 0)
				{
					m_bpAbout.SetBP_HARD(m_Thread, address, BP_HARD_RWDATA, K2_BYTE, delflag);
				}
				else
				{
					m_bpAbout.SetBP_HARD(m_Thread, address, BP_HARD_RWDATA, K1_BYTE, delflag);
				}
			}
	  }
		
		// ����Ӳ��д��ϵ�
		else if (_stricmp(cmdLine, "baw") == 0)
		{
			printf("����д��ϵ��ַ(Hex):");
			SIZE_T  address = 0;
			scanf_s("%X", &address);
			char    forever[10];
			bool    delflag = true;
			int     nLen = 0;
			printf("�Ƿ������Զϵ�(y/n)��");
			scanf_s("%s", &forever, 10);
			if (_stricmp(forever, "y") == 0)
			{
				delflag = false;
			}
			printf("Ӳ��д��ϵ㳤��(1/2/4):");
			scanf_s("%d", &nLen);
			if (nLen >= 1)
			{
				if (nLen % 4 == 0)
				{
					m_bpAbout.SetBP_HARD(m_Thread, address, BP_HARD_WR, K4_BYTE, delflag);
				}
				else if (nLen % 2 == 0)
				{
					m_bpAbout.SetBP_HARD(m_Thread, address, BP_HARD_WR, K2_BYTE, delflag);
				}
				else
				{
					m_bpAbout.SetBP_HARD(m_Thread, address, BP_HARD_WR, K1_BYTE, delflag);
				}
			}
		}

		// ���������ϵ�����
		else if (_stricmp(cmdLine, "bpif") == 0) {
			printf("���������ϵ��ַ(HEX): ");
			SIZE_T  address = 0;
			char    ByTimes[10];
			scanf_s("%X", &address);
			bool    conditionway = false;
			printf("���������ϵ㷽ʽ(y�����д���/n���Ĵ���ֵ): ");
			scanf_s("%s", ByTimes, 10);
			if (_stricmp(ByTimes, "y") == 0)
			{
				conditionway = true;
				printf("�������д���: ");
				DWORD EndTimes;
				scanf_s("%d", &EndTimes);
				m_bpAbout.SetBP_Condition(m_process, address, conditionway, EndTimes, 0, Cbp_About::e_eax);
			}
			else
			{
				conditionway = false;
				Cbp_About::Eregister reg = Cbp_About::e_eax;
				DWORD EndValue = 0;
				char Reg[10];
				printf("Ҫ�жϵļĴ���(eax/ebx/ecx/edx/esp/ebp/esi/edi/cs/ss/ds/es/fs): ");
				scanf_s("%s", &Reg, 10);
				if (_stricmp(Reg, "eax") == 0)
				{
					reg = Cbp_About::e_eax;
				}
				else if (_stricmp(Reg, "ebx") == 0)
				{
					reg = Cbp_About::e_ebx;
				}
				else if (_stricmp(Reg, "ecx") == 0)
				{
					reg = Cbp_About::e_ecx;
				}
				else if (_stricmp(Reg, "edx") == 0)
				{
					reg = Cbp_About::e_edx;
				}
				else if (_stricmp(Reg, "ebx") == 0)
				{
					reg = Cbp_About::e_ebx;
				}
				else if (_stricmp(Reg, "esp") == 0)
				{
					reg = Cbp_About::e_esp;
				}
				else if (_stricmp(Reg, "ebp") == 0)
				{
					reg = Cbp_About::e_ebp;
				}
				else if (_stricmp(Reg, "esi") == 0)
				{
					reg = Cbp_About::e_esi;
				}
				else if (_stricmp(Reg, "edi") == 0)
				{
					reg = Cbp_About::e_edi;
				}
				else if (_stricmp(Reg, "cs") == 0)
				{
					reg = Cbp_About::e_ecs;
				}
				else if (_stricmp(Reg, "ds") == 0)
				{
					reg = Cbp_About::e_eds;
				}
				else if (_stricmp(Reg, "ss") == 0)
				{
					reg = Cbp_About::e_ess;
				}
				else if (_stricmp(Reg, "es") == 0)
				{
					reg = Cbp_About::e_ees;
				}
				else if (_stricmp(Reg, "fs") == 0)
				{
					reg = Cbp_About::e_efs;
				}
				else if (_stricmp(Reg, "gs") == 0)
				{
					reg = Cbp_About::e_egs;
				}
				printf("�Ĵ�������������ֵ(HEX):");
				scanf_s("%x", &EndValue);
				m_bpAbout.SetBP_Condition(m_process, address, conditionway, 0, EndValue, reg);
			}
		}

		// �оٶϵ�����
		else if (_stricmp(cmdLine, "bl") == 0){	
			printf("����ϵ�:\n");
			for (auto& i : m_bpAbout.m_Vec_INT3)
			{
				printf("         ��ַ: %4x     ������:%d\n", i.address, !i.delFlag);
			}
			
			printf("Ӳ���ϵ�:\n");
			for (auto& h : m_bpAbout.m_Vec_HARD)
			{
				printf("         ��ַ: %4x     ������:%d", h.address, !h.delFlag);
				switch (h.Type)
				{
				case BP_HARD_EXEC:
					printf("    Ӳ������:  ִ�жϵ�\n");
					break;
				case BP_HARD_WR:
					printf("    Ӳ������:  д��ϵ�\n");
					break;
				case BP_HARD_RWDATA:
					printf("    Ӳ������:  ���ݶ�д�ϵ�\n");
					break;
				default:
					break;
				}				
			}
			
			printf("�ڴ�ϵ�:\n");
			for (auto& i : m_bpAbout.m_Vec_Mem)
			{
				printf("         ��ַ: %4x     ������:%d\n", i.address, !i.delFlag);
// 				switch (i.dwOldAttri)
// 				{
// 
// 				}
			}
		}

		// �����ڴ�ϵ�����
		else if (_stricmp(cmdLine, "bm") == 0) {

			// ��������ϵ��ַ  
			printf("����ϵ��ַ(Hex):");
			SIZE_T  address = 0;
			scanf_s("%X", &address);
			// �����Ƿ�Ϊ����������ϵ�
			char    forever[10];
			bool    delflag = true;
			printf("�Ƿ������Զϵ�(y/n)��");
			scanf_s("%s", &forever, 10);
			if (_stricmp(forever, "y") == 0)
			{
				delflag = false;
			}
			m_bpAbout.SetBP_MEM(m_process, address, delflag);
		}

		// ɾ����Ӧ�ϵ�����
		else if (_stricmp(cmdLine, "bc") == 0)
		{
			printf("Ҫɾ���Ķϵ��ַ:");
			SIZE_T deladdr = 0;
			scanf_s("%x", &deladdr, sizeof(SIZE_T));
			std::vector<Cbp_About::BP_INT3>::iterator iter;
			for ( iter = m_bpAbout.m_Vec_INT3.begin(); iter != m_bpAbout.m_Vec_INT3.end(); )
			{
				if (iter->address == deladdr)
				{
					DWORD dwWrite;
					if (!WriteProcessMemory(m_process, (LPVOID)iter->address, &iter->oldbyte, 1, &dwWrite)) 
					{
						LOG("��ȡ�����ڴ�ʧ��");
					}
					printf("����ϵ� %x   ��ɾ��\n", iter->address);
					iter = m_bpAbout.m_Vec_INT3.erase(iter);			
				}
				else
				{
						iter++;
				}
			}		

			m_bpAbout.RemoveBP_HARD(m_Thread, deladdr, true);
			

		}

		// ������жϵ�����
		else if (_stricmp(cmdLine, "bc*") == 0) {
			std::vector<Cbp_About::BP_INT3>::iterator iter;
			for (iter = m_bpAbout.m_Vec_INT3.begin(); iter != m_bpAbout.m_Vec_INT3.end(); )
			{
				DWORD dwWrite;
				if (!WriteProcessMemory(m_process, (LPVOID)iter->address, &iter->oldbyte, 1, &dwWrite))
				{
					LOG("��ȡ�����ڴ�ʧ��");
				}
				iter = m_bpAbout.m_Vec_INT3.erase(iter);
			}
			// �������Ӳ���ϵ�
			CONTEXT ct = { 0 };
			ct.ContextFlags = CONTEXT_DEBUG_REGISTERS;

			if (!GetThreadContext(m_Thread, &ct))
			{
				printf("��ȡ�̻߳���ʧ��\n");
			}
			DBG_REG7* pDr7 = (DBG_REG7*)&ct.Dr7;
			pDr7->L0 = 0;
			pDr7->L1 = 0;
			pDr7->L2 = 0;
			pDr7->L3 = 0;
			if (!SetThreadContext(m_Thread, &ct))
			{
				printf("���벻�Ϸ���δ֪����\n");
			}

			m_bpAbout.m_Vec_HARD.clear();

			// ��������ڴ���ʶϵ�
		}

		else
		{
			printf("�������\n");
		}

	}
}

// �����쳣�¼�ʱ����������벢�����û������벢�����û������������ж�Ӧ����
DWORD CmyDebug::OnExceptionDebugEvent(DEBUG_EVENT dbgEv)
{
	// �����еĶϵ�������һ��.
	DWORD dwCode = DBG_CONTINUE;

	switch (dbgEv.u.Exception.ExceptionRecord.ExceptionCode) {
	case EXCEPTION_BREAKPOINT:                // ����쳣
	{
		dwCode = OnExceptionINT3(dbgEv.u.Exception.ExceptionRecord);
	}
	break;
	case EXCEPTION_SINGLE_STEP:               // �����쳣��Ӳ���ϵ��쳣
	{
		// tf�ϵ�ÿ���ж�֮�󶼻ᱻ�Զ�����.
		dwCode = OnExceptionSingleStep(dbgEv.u.Exception.ExceptionRecord);
	}
	break;
	case EXCEPTION_ACCESS_VIOLATION:          // �ڴ�����쳣
	{
		dwCode = OnExceptionMem(dbgEv);
	}
	break;
	default:
	{
		dwCode = DBG_EXCEPTION_NOT_HANDLED;
		break;
	}
		
	}

	return dwCode;
}

DWORD CmyDebug::OnExceptionINT3(EXCEPTION_RECORD & record)
{
	static BOOL isSystemBreakpoint = TRUE;
	DWORD dwCode = DBG_CONTINUE;

	// ϵͳ�ϵ������:
	// ��һ���쳣�¼�.����, �쳣������BREAKPOINT
	// �Ƿ���ϵͳ�ϵ�
	if (isSystemBreakpoint) {
		if (record.ExceptionCode == EXCEPTION_BREAKPOINT) {
			// ϵͳ�ϵ�
			dwCode = DBG_CONTINUE;
			printf("����ϵͳ�ϵ�:%08X\n", record.ExceptionAddress);
		}
		CONTEXT ct = { CONTEXT_CONTROL };
		if (!GetThreadContext(m_Thread, &ct))
		{
			printf("��ȡ�̻߳���ʧ��\n");
		}
		isSystemBreakpoint = FALSE;
		showDisasm(m_process, (LPVOID)ct.Eip, 20);
		//showDisasm(m_process, record.ExceptionAddress, 20);
		// �����û�����
		userInput(record);
		return dwCode;
	}

	// ȥ������ϵ����������ϵ��쳣
	std::vector<Cbp_About::BP_INT3>::iterator iter;
	for (iter = m_bpAbout.m_Vec_INT3.begin(); iter != m_bpAbout.m_Vec_INT3.end(); )
	{
		if (iter->address == (SIZE_T)record.ExceptionAddress)
		{
			if (m_Oep)
			{
				printf("���� OEP\n");
				m_Oep = false;
			}

			DWORD dwWrite = 0;
			if (!WriteProcessMemory(m_process, (LPVOID)iter->address, &iter->oldbyte, 1, &dwWrite)) {
				LOG("��ȡ�����ڴ�ʧ��");
				dwCode = DBG_EXCEPTION_NOT_HANDLED;

				return dwCode;
			}
			// ��eip��1
			CONTEXT ct = { CONTEXT_CONTROL };
			if (!GetThreadContext(m_Thread, &ct)) {
				LOG("��ȡ�̻߳���ʧ��");
				dwCode = DBG_EXCEPTION_NOT_HANDLED;
				return dwCode;
			}
			ct.Eip--;

			if (!SetThreadContext(m_Thread, &ct)) {
				LOG("��ȡ�̻߳���ʧ��");
				dwCode = DBG_EXCEPTION_NOT_HANDLED;
				return dwCode;
			}
			// ����������Զϵ㣬����ʾ�귴�����ٰ� INT3 д��ȥ
			if (iter->delFlag == FALSE)
			{
				SIZE_T dwRead;
				showDisasm(m_process, record.ExceptionAddress, 10);

				WriteProcessMemory(m_process, (LPVOID)iter->address, "\xcc", 1, &dwRead);
				// �����û�����
				userInput(record);
				return dwCode;
			}
			// �����һ���Զϵ㣬����Ҫ������ϵ��б����Ƴ�
			else
			{
				printf("����ϵ�  %x   ��ɾ��\n", iter->address);
				iter = m_bpAbout.m_Vec_INT3.erase(iter);	
				showDisasm(m_process, record.ExceptionAddress, 20);
				// �����û�����
				userInput(record);

				return dwCode;
			}
		}
		else
		{
			iter++;
		}
	}
	
	// ȥ�������ϵ����������ϵ��쳣
	std::vector<Cbp_About::BP_CONDITION>::iterator iterCondition;
	for (iterCondition = m_bpAbout.m_Vec_Condition.begin(); iterCondition != m_bpAbout.m_Vec_Condition.end(); )
	{
		if (iterCondition->address == (SIZE_T)record.ExceptionAddress)
		{
			DWORD dwWrite = 0;
			if (!WriteProcessMemory(m_process, (LPVOID)iterCondition->address, &iterCondition->oldbyte, 1, &dwWrite)) {
				LOG("��ȡ�����ڴ�ʧ��");
				dwCode = DBG_EXCEPTION_NOT_HANDLED;
				return dwCode;
			}
			// ��eip��1
			CONTEXT ct = { CONTEXT_CONTROL };
			if (!GetThreadContext(m_Thread, &ct)) {
				LOG("��ȡ�̻߳���ʧ��");
				dwCode = DBG_EXCEPTION_NOT_HANDLED;
				return dwCode;
			}
			ct.Eip--;

			if (!SetThreadContext(m_Thread, &ct)) {
				LOG("��ȡ�̻߳���ʧ��");
				dwCode = DBG_EXCEPTION_NOT_HANDLED;
				return dwCode;
			}
			// ����ǰ����д��������Ķϵ�
			if (iterCondition->ConditionWay == TRUE)
			{
				// ÿ����һ�ξͼ�һ�δ���
				iterCondition->Times++;
				
				// �ﵽ����������ɾ���öϵ�Ȼ����ʾ����ಢ�ӹܵ��û�����
				if (iterCondition->Times == iterCondition->EndTimes)
				{
					printf("�����ϵ�  %x   ������ɾ��\n", iterCondition->address);
					iterCondition = m_bpAbout.m_Vec_Condition.erase(iterCondition);				
					
					showDisasm(m_process, record.ExceptionAddress, 10);
					
					// �����û�����
					userInput(record);
					return dwCode;
				}
				// û�дﵽ������������һ��������Ȼ���ڵ����쳣��߽���������ϵ�ָ�
				else
				{
					m_ConditionStep = true;
					m_bpAbout.SetBP_TF(m_Thread);
					iterCondition++;
					return dwCode;
				}
			}
			// ����ǰ��Ĵ�����ֵ������
			else
			{
				CONTEXT ct = { CONTEXT_CONTROL };
				if (!GetThreadContext(m_Thread, &ct)) {
					LOG("��ȡ�̻߳���ʧ��");
					dwCode = DBG_EXCEPTION_NOT_HANDLED;
					return dwCode;
				}

				DWORD TempRegValue = 0;
				switch (iterCondition->Reg)
				{
					case Cbp_About::e_eax:
						TempRegValue = ct.Eax; break;
					case Cbp_About::e_ebx:
						TempRegValue = ct.Ebx; break;
					case Cbp_About::e_ecx:
						TempRegValue = ct.Ecx; break;
					case Cbp_About::e_edx:
						TempRegValue = ct.Edx; break;
					case Cbp_About::e_ebp:
						TempRegValue = ct.Ebp; break;
					case Cbp_About::e_esp:
						TempRegValue = ct.Esp; break;
					case Cbp_About::e_esi:
						TempRegValue = ct.Esi; break;
					case Cbp_About::e_edi:
						TempRegValue = ct.Edi; break;
					case Cbp_About::e_ecs:
						TempRegValue = ct.SegCs; break;
					case Cbp_About::e_ess:
						TempRegValue = ct.SegSs; break;
					case Cbp_About::e_eds:
						TempRegValue = ct.SegDs; break;
					case Cbp_About::e_ees:
						TempRegValue = ct.SegEs; break;
					case Cbp_About::e_efs:
						TempRegValue = ct.SegFs; break;
					case Cbp_About::e_egs:
						TempRegValue = ct.SegGs; break;				
				}

				if (TempRegValue == iterCondition->EndValue)
				{
					printf("�����ϵ�  %x   ������ɾ��\n", iterCondition->address);
					iterCondition = m_bpAbout.m_Vec_Condition.erase(iterCondition);

					showDisasm(m_process, record.ExceptionAddress, 10);

					// �����û�����
					userInput(record);
					return dwCode;
				}
				else
				{
					m_ConditionStep = true;
					m_bpAbout.SetBP_TF(m_Thread);
					iterCondition++;
					return dwCode;
				}		
			}
		}
		else
		{
			iter++;
		}
	}


	showDisasm(m_process,record.ExceptionAddress,20);
	// �����û�����
	userInput(record);

	return dwCode;
}

DWORD CmyDebug::OnExceptionSingleStep(EXCEPTION_RECORD & record)
{
	printf("�����쳣�¼�\n");
	DWORD dwCode = DBG_CONTINUE;
	SIZE_T addrStart = (SIZE_T)record.ExceptionAddress - ((SIZE_T)record.ExceptionAddress % 0x1000);
	// �¶ϵ�ַ����ҳ��ҳĩ��ַ = ҳ��ʼ��ַ + ҳ��С
	SIZE_T addrEnd = addrStart + 0xFFF;

	// ������ڴ�ϵ���ĵ������´����ĵ����쳣,����Ҫ�ָ��ڴ�ϵ㲢�� m_MemStep ��Ϊ false
	if (m_MemStep)
	{
		for (auto i : m_bpAbout.m_Vec_Mem)
		{
			// �ҵ����õ����쳣������ڴ�ϵ㲢��������ڴ�ϵ�
			if (addrStart <= i.address && i.address <= addrEnd)
			{
				m_bpAbout.SetBP_MEM(m_process, i.address, i.delFlag);
				m_MemStep = false;
				
				return dwCode;
			}	
		}
		
	} 

	// ����������ϵ���ĵ������´����ĵ����쳣,����Ҫ�ָ������ϵ㲢�� m_MemStep ��Ϊ false
	if (m_ConditionStep)
	{
		for (auto i : m_bpAbout.m_Vec_Condition)
		{
				DWORD dwRead = 0;

				// ��int 3ָ��(0xcc)д�뵽�ϵ�ĵ�ַ��
				if (!WriteProcessMemory(m_process, (LPVOID)i.address, "\xcc", 1, &dwRead)) {
					LOG("д���ڴ�ʧ��");
					m_ConditionStep = false;
					return dwCode;
				}
				m_ConditionStep = false;
				return dwCode;
		}

	}


	showDisasm(m_process, record.ExceptionAddress, 20);

	//��Ҫ�õ�DR6 DR7 �Ĵ���,����Dr0
	CONTEXT ct = { 0 };
	ct.ContextFlags = CONTEXT_DEBUG_REGISTERS;

	if (!GetThreadContext(m_Thread, &ct))
	{
		printf("��ȡ�̻߳���ʧ��\n");
	}

	// ��Ϊ�����쳣��ĵ�ַ���ܲ���Ӳ���ϵ��ַ
	DBG_REG6 * pDR6 = (DBG_REG6*)&ct.Dr6;
	DBG_REG7* pDr7 = (DBG_REG7*)&ct.Dr7;

	SIZE_T addr = 0;

	if (pDR6->B0)						  //DR0�����ж�
	{
		addr = ct.Dr0;
		//pDr7->L0 = 0;
	}
	else if (pDR6->B1)				//DR1�����ж�
	{
		addr = ct.Dr1;
		//pDr7->L1 = 0;
	}
	else if (pDR6->B2)				//DR2�����ж�
	{
		addr = ct.Dr2;
		//pDr7->L2 = 0;
	}
	else if (pDR6->B3)				//DR3�����ж�
	{
		addr = ct.Dr3;
		//pDr7->L3 = 0;
	}
	m_bpAbout.RemoveBP_HARD(m_Thread, addr, false);
	
	
	// �����û�����
	userInput(record);

	return dwCode;
}

DWORD CmyDebug::OnExceptionMem(DEBUG_EVENT & Exception)
{
	printf("�ڴ��쳣�¼�\n");
	DWORD dwCode = DBG_CONTINUE;

	// �����ڴ�ϵ�����
	std::vector<Cbp_About::BP_MEM>::iterator iterMem;
	for (iterMem = m_bpAbout.m_Vec_Mem.begin(); iterMem != m_bpAbout.m_Vec_Mem.end(); )
	{
		// �жϴ����ڴ�����쳣��λ���Ƿ������õĶϵ�ҳ�ڣ� ExceptionInformation�ڶ���Ԫ�ر�ʾ�����쳣�ĵ�ַ
		SIZE_T ExceptionAddr = (SIZE_T)Exception.u.Exception.ExceptionRecord.ExceptionInformation[1];
		// �¶ϵ�ַ����ҳ����ʼ��ַ = �¶ϵ�ַ - �¶ϵ�ַҳ����ȡ��
		SIZE_T addrStart = iterMem->address - (iterMem->address % 0x1000);
		// �¶ϵ�ַ����ҳ��ҳĩ��ַ = ҳ��ʼ��ַ + ҳ��С
		SIZE_T addrEnd = addrStart + 0xFFF;

		// ��������쳣��ַ�����¶ϵ�ҳ����Ļ��� �͵��赥��һ�ֽ�һ�ֽ���ֱ���¶ϵ�ַ��
		if (ExceptionAddr >= addrStart && ExceptionAddr <= addrEnd)
		{
			// �Ȼָ��ڴ�����
			DWORD dwOldProtect = 0;
			VirtualProtectEx(m_process, (LPVOID)(iterMem->address), 1, iterMem->dwOldAttri, &dwOldProtect);

			// Ȼ�����õ����ϵ�
			m_bpAbout.SetBP_TF(m_Thread);

			// ���ж��Ƿ����ڴ�ϵ�����ĵ�����־�� true
			m_MemStep = true;

			// �ж��Ƿ������ڴ�ϵ�
			if (ExceptionAddr == iterMem->address)
			{
				
				// �ж��Ƿ���һ�����ڴ�ϵ�
				if (iterMem->delFlag)
				{
					iterMem = m_bpAbout.m_Vec_Mem.erase(iterMem);
					// ������о�ȥ���ո���ĵ����ϵ�
					m_bpAbout.RemoveBP_TF(m_Thread);
				}
				else
				{
					iterMem++;
				}
				// ���жϵ�����ʾ����ಢ�ӹܵ��û�����
				showDisasm(m_process, (LPVOID)ExceptionAddr, 20);    // ��������Ӧ�÷�����̵߳�
				userInput(Exception.u.Exception.ExceptionRecord);
				
				return dwCode;
			}
			else
			{
				iterMem++;
			}
		}
		else
		{
			iterMem++;
		}
	}

	return dwCode;
}

void CmyDebug::ShowRegister()
{
	CONTEXT ct = { 0 };
	ct.ContextFlags = CONTEXT_ALL;
	if (!GetThreadContext(m_Thread, &ct))
	{
		LOG("��ȡ�̻߳�����ʧ��");
		return;
	}
	WriteChar(13, 2, "EAX  ", (int)ct.Eax, F_H_GREEN);
	WriteChar(13, 3, "ECX  ", (int)ct.Ecx, F_H_GREEN);
	WriteChar(13, 4, "EDX  ", ct.Edx, F_H_GREEN);
	WriteChar(13, 5, "EBX  ", ct.Ebx, F_H_GREEN);
	WriteChar(13, 6, "ESP  ", ct.Esp, F_H_GREEN);
	WriteChar(13, 7, "EBP  ", ct.Ebp, F_H_GREEN);
	WriteChar(13, 8, "ESI  ", ct.Esi, F_H_GREEN);
	WriteChar(13, 9, "EDI  ", ct.Edi, F_H_GREEN);
	WriteChar(13, 11, "EIP  ",ct.Eip, F_H_GREEN);
	WriteChar(24, 4, "CS   ", ct.SegCs, F_H_YELLOW);
	WriteChar(24, 5, "SS   ", ct.SegSs, F_H_YELLOW);
	WriteChar(24, 6, "DS   ", ct.SegDs, F_H_YELLOW);
	WriteChar(24, 7, "ES   ", ct.SegFs, F_H_YELLOW);
	WriteChar(24, 8, "FS   ", ct.SegGs, F_H_YELLOW);
}

void CmyDebug::ShowStack()
{
	CONTEXT ct = { 0 };
	ct.ContextFlags = CONTEXT_ALL;
	if (!GetThreadContext(m_Thread, &ct))
	{
		printf("��ȡ�̻߳���ʧ��");
		return;
	}

	BYTE buff[512];
	DWORD dwRead = 0;

	ReadProcessMemory(m_process, (LPVOID)ct.Esp, buff, 512, &dwRead);

	for (int i = 0; i < 10; i++)
	{
		WriteChar(35, 2 + i, "", ((DWORD*)buff)[i], F_H_YELLOW);
	}
}

SIZE_T CmyDebug::GetSymFuncName(SIZE_T Address, CString & strName)
{
	SymInitialize(m_process, NULL, TRUE);
// 	if (!SymInitialize(m_process, NULL, TRUE))
// 	{
// 		printf("��ȡ���Է��ų���\n");
// 	}
	DWORD64 dwDisplacement = 0;
	char    buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYM_NAME;         //(2000)

	// ���ݵ�ַ��ȡ������Ϣ
	if (!SymFromAddr(m_process, Address, &dwDisplacement, pSymbol))
	{
		return false;
	}
	strName.Format(L"%S", pSymbol->Name);
	return SIZE_T();
}

void CmyDebug::WriteChar(int x, int y, char * pszChar, int _number, WORD nColor)
{
		//��ȡ��ǰ���λ�ã�׼�����û�ȥ
		CONSOLE_SCREEN_BUFFER_INFO  rea = { 0 };
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &rea);

		CONSOLE_CURSOR_INFO Cci = { 1, false };			//������Խṹ
		SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &Cci);
		COORD loc = { x * 2, y };
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), nColor);
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), loc);
		printf("%s", pszChar);
		printf("%08X", _number);

		//���û�ȥ������ɫ
		loc.X = rea.dwCursorPosition.X;
		loc.Y = rea.dwCursorPosition.Y;
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), F_H_WHITE);
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), loc);
}





void CmyDebug::TraverseModule(DWORD PID)
{
	//��������
	HANDLE hThreadSnap = 0;
	hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, PID);
	if (hThreadSnap == INVALID_HANDLE_VALUE)
		return;

	//�ṹ��4
	MODULEENTRY32 stcPe32 = { sizeof(MODULEENTRY32) };

	//������վ�� �� ���̽ṹ��
	Module32First(hThreadSnap, &stcPe32);

	printf("ģ����	                 |	ģ���С	|	ģ���ַ	|	ģ��·��\n");
	do
	{
		wprintf(L"%-23s	 |	", stcPe32.szModule);
		wprintf(L"%08X	|	", stcPe32.modBaseSize);
		printf("%08X	|	", stcPe32.modBaseAddr);
		wprintf(L"%s\n", stcPe32.szExePath);

	} while (Module32Next(hThreadSnap, &stcPe32));
}

void CmyDebug::Dump(char * Name, char * path)
{
	HANDLE hFile = CreateFile(L"����123.exe", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return;
	}

	IMAGE_DOS_HEADER* pDosHdr = (IMAGE_DOS_HEADER*)m_pBuff;
	IMAGE_NT_HEADERS* pNtHdr = (IMAGE_NT_HEADERS*)((ULONG_PTR)m_pBuff + pDosHdr->e_lfanew);
	IMAGE_OPTIONAL_HEADER* pOptHdr = &(pNtHdr->OptionalHeader);		//ȡ��ַ����
	IMAGE_DATA_DIRECTORY* pDataDir = pOptHdr->DataDirectory;		  //���ﲻ��ȡ

	//pDataDir[9].VirtualAddress = 0;
	//pDataDir[9].Size = 0;

	//д���ļ�
	DWORD dwBytesWrite = 0;
	WriteFile(hFile, m_pBuff, m_Size, &dwBytesWrite, NULL);

	//CloseHandle(m_hfile);

}

SIZE_T CmyDebug::ModuleToAddr(TCHAR * _Name, DWORD _PID)
{
	//��������
	HANDLE hThreadSnap = 0;
	hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, _PID);
	if (hThreadSnap == INVALID_HANDLE_VALUE)
		return 0;

	//�ṹ��4
	MODULEENTRY32 stcPe32 = { sizeof(MODULEENTRY32) };

	//������վ�� �� ���̽ṹ��
	Module32First(hThreadSnap, &stcPe32);

	do
	{
		if (!_wcsicmp(_Name, stcPe32.szModule))
			return (SIZE_T)stcPe32.modBaseAddr;

	} while (Module32Next(hThreadSnap, &stcPe32));
}

void CmyDebug::PaserExportTable(SIZE_T _Address)
{
	SIZE_T toAddress = _Address;

	//�ҵ�DOSͷ��NTͷƫ��
	BYTE  buff[sizeof(IMAGE_DOS_HEADER)];
	DWORD dwRead = 0;
	DWORD dwOldProtect = 0;	//�洢ԭ��ҳ������					//�޸�ҳ������
	VirtualProtectEx(m_process, (LPVOID)(toAddress), sizeof(IMAGE_DOS_HEADER), PAGE_READWRITE, &dwOldProtect);
	ReadProcessMemory(m_process, (LPVOID)toAddress, buff, sizeof(IMAGE_DOS_HEADER), &dwRead);
	VirtualProtectEx(m_process, (LPVOID)(toAddress), sizeof(IMAGE_DOS_HEADER), dwOldProtect, &dwOldProtect);

	IMAGE_DOS_HEADER* DosHed = (IMAGE_DOS_HEADER*)buff;
	DosHed->e_lfanew;

	//�ҵ�NTͷ ����չͷ
	(ULONG_PTR)toAddress += DosHed->e_lfanew;
	(ULONG_PTR)toAddress += 4;
	VirtualProtectEx(m_process, (LPVOID)(toAddress), sizeof(IMAGE_FILE_HEADER), PAGE_READWRITE, &dwOldProtect);
	ReadProcessMemory(m_process, (LPVOID)toAddress, buff, sizeof(IMAGE_FILE_HEADER), &dwRead);
	VirtualProtectEx(m_process, (LPVOID)(toAddress), sizeof(IMAGE_FILE_HEADER), dwOldProtect, &dwOldProtect);

	IMAGE_FILE_HEADER* FileHed = (IMAGE_FILE_HEADER*)buff;
	FileHed->SizeOfOptionalHeader;

	toAddress += sizeof(IMAGE_FILE_HEADER);			//�õ���չͷ��ַ

	char* optbuff[sizeof(IMAGE_OPTIONAL_HEADER)] = {};		//��չͷһ���СΪE0
	VirtualProtectEx(m_process, (LPVOID)(toAddress), 0xE0, PAGE_READWRITE, &dwOldProtect);
	ReadProcessMemory(m_process, (LPVOID)toAddress, optbuff, 0xE0, &dwRead);
	VirtualProtectEx(m_process, (LPVOID)(toAddress), 0xE0, dwOldProtect, &dwOldProtect);

	//0x00CA595A ����δ��������쳣(�� ConsoleApplication2.exe ��):  0xC0000005:  ��ȡλ�� 0x00001008 ʱ�������ʳ�ͻ��
	//��չͷ��DOSͷ��,Խ��

	IMAGE_OPTIONAL_HEADER* OptHed = (IMAGE_OPTIONAL_HEADER*)optbuff;

	//�ҵ�NTͷ������Ŀ¼����ĵ�����
	OptHed->DataDirectory[0];

	//Ĭ�ϼ��ػ�ַ �� RVA ���ڵ������ƫ��
	toAddress = (ULONG_PTR)_Address + (ULONG_PTR)OptHed->DataDirectory[0].VirtualAddress;

	optbuff;		//������Ĵ�СΪ40���ֽ�
	VirtualProtectEx(m_process, (LPVOID)(toAddress), 40, PAGE_READWRITE, &dwOldProtect);
	ReadProcessMemory(m_process, (LPVOID)toAddress, optbuff, 40, &dwRead);
	VirtualProtectEx(m_process, (LPVOID)(toAddress), 40, dwOldProtect, &dwOldProtect);

	//������
	IMAGE_EXPORT_DIRECTORY* EXPORTTable = (IMAGE_EXPORT_DIRECTORY*)optbuff;


	//�ȰѺ�����ַ,�������,������ �ֱ����������� ������,Ȼ�������ӡ���
	DWORD* FunionAddressTable = new DWORD[EXPORTTable->NumberOfFunctions];
	WORD* BaseTable = new WORD[EXPORTTable->NumberOfNames];
	char** FunionNameTable = new char*[EXPORTTable->NumberOfNames];

	//�ȸ㺯����ַ
	DWORD FunionAddress = (ULONG_PTR)EXPORTTable->AddressOfFunctions + (ULONG_PTR)_Address;
	for (int i = 0; i < EXPORTTable->NumberOfFunctions; i++)
	{
		VirtualProtectEx(m_process, (LPVOID)((DWORD*)FunionAddress + i), 4, PAGE_READWRITE, &dwOldProtect);
		ReadProcessMemory(m_process, (LPVOID)((DWORD*)FunionAddress + i), &(FunionAddressTable[i]), 4, &dwRead);
		VirtualProtectEx(m_process, (LPVOID)((DWORD*)FunionAddress + i), 4, dwOldProtect, &dwOldProtect);
	}

	//�ٸ����,������
	DWORD FunionOrd = (ULONG_PTR)EXPORTTable->AddressOfNameOrdinals + (ULONG_PTR)_Address;
	DWORD FuntionName = (ULONG_PTR)EXPORTTable->AddressOfNames + (ULONG_PTR)_Address;
	for (int i = 0; i < EXPORTTable->NumberOfNames; i++)
	{
		VirtualProtectEx(m_process, (LPVOID)((DWORD*)FunionOrd + i), 2, PAGE_READWRITE, &dwOldProtect);
		ReadProcessMemory(m_process, (LPVOID)((DWORD*)FunionOrd + i), &((BaseTable)[i]), 2, &dwRead);
		VirtualProtectEx(m_process, (LPVOID)((DWORD*)FunionOrd + i), 2, dwOldProtect, &dwOldProtect);

		//�������Ƚ��鷳
		//�ȵõ�������ַRVA
		DWORD FuntionRVA = 0;
		VirtualProtectEx(m_process, (LPVOID)((DWORD*)FuntionName + i), 4, PAGE_READWRITE, &dwOldProtect);
		ReadProcessMemory(m_process, (LPVOID)((DWORD*)FuntionName + i), &FuntionRVA, 4, &dwRead);
		VirtualProtectEx(m_process, (LPVOID)((DWORD*)FuntionName + i), 4, dwOldProtect, &dwOldProtect);

		//����RVA ������,��50���ֽ�,��'\0',�ɷ���
		//�������ĵ�ַ��Ҫ����Ĭ�ϻ�ַ
		FuntionRVA += (ULONG_PTR)_Address;
		char* cName = new char[50]();
		VirtualProtectEx(m_process, (LPVOID)((DWORD*)FuntionRVA + i), 50, PAGE_READWRITE, &dwOldProtect);
		ReadProcessMemory(m_process, (LPVOID)((DWORD*)FuntionRVA + i), cName, 50, &dwRead);
		VirtualProtectEx(m_process, (LPVOID)((DWORD*)FuntionRVA + i), 50, dwOldProtect, &dwOldProtect);
		FunionNameTable[i] = cName;
	}

	printf("������ַ	|	�������	|	������\n");

	for (int i = 0; i < EXPORTTable->NumberOfFunctions; i++)
	{
		//������������,�� �����Ĺ���
		//��ӡ������ַ		//��ӡ���

		printf("%08X	|	", FunionAddressTable[i]);
		printf("%08X	|	", i);

		//������ű�,��ӡ������
		for (int j = 0; j < EXPORTTable->NumberOfNames; j++)
		{
			if (BaseTable[j] == i)
			{
				printf("%s", FunionNameTable[j]);
			}
		}

		printf("\n");
	}

	delete FunionAddressTable;
	FunionAddressTable = NULL;
	delete BaseTable;
	BaseTable = NULL;
	//delete ��������
	delete[]FunionNameTable;
}

void CmyDebug::PaserImportTable(SIZE_T _Address)
{
	SIZE_T toAddress = _Address;

	//�ҵ�DOSͷ��NTͷƫ��
	BYTE  buff[sizeof(IMAGE_DOS_HEADER)];
	DWORD dwRead = 0;
	DWORD dwOldProtect = 0;	//�洢ԭ��ҳ������					//�޸�ҳ������
	VirtualProtectEx(m_process, (LPVOID)(toAddress), sizeof(IMAGE_DOS_HEADER), PAGE_READWRITE, &dwOldProtect);
	ReadProcessMemory(m_process, (LPVOID)toAddress, buff, sizeof(IMAGE_DOS_HEADER), &dwRead);
	VirtualProtectEx(m_process, (LPVOID)(toAddress), sizeof(IMAGE_DOS_HEADER), dwOldProtect, &dwOldProtect);

	IMAGE_DOS_HEADER* DosHed = (IMAGE_DOS_HEADER*)buff;
	DosHed->e_lfanew;

	//�ҵ�NTͷ ����չͷ
	(ULONG_PTR)toAddress += DosHed->e_lfanew;
	(ULONG_PTR)toAddress += 4;
	VirtualProtectEx(m_process, (LPVOID)(toAddress), sizeof(IMAGE_FILE_HEADER), PAGE_READWRITE, &dwOldProtect);
	ReadProcessMemory(m_process, (LPVOID)toAddress, buff, sizeof(IMAGE_FILE_HEADER), &dwRead);
	VirtualProtectEx(m_process, (LPVOID)(toAddress), sizeof(IMAGE_FILE_HEADER), dwOldProtect, &dwOldProtect);

	IMAGE_FILE_HEADER* FileHed = (IMAGE_FILE_HEADER*)buff;
	FileHed->SizeOfOptionalHeader;

	toAddress += sizeof(IMAGE_FILE_HEADER);			//�õ���չͷ��ַ

	char* optbuff[sizeof(IMAGE_OPTIONAL_HEADER)] = {};		//��չͷһ���СΪE0
	VirtualProtectEx(m_process, (LPVOID)(toAddress), 0xE0, PAGE_READWRITE, &dwOldProtect);
	ReadProcessMemory(m_process, (LPVOID)toAddress, optbuff, 0xE0, &dwRead);
	VirtualProtectEx(m_process, (LPVOID)(toAddress), 0xE0, dwOldProtect, &dwOldProtect);

	//0x00CA595A ����δ��������쳣(�� ConsoleApplication2.exe ��):  0xC0000005:  ��ȡλ�� 0x00001008 ʱ�������ʳ�ͻ��
	//��չͷ��DOSͷ��,Խ��

	IMAGE_OPTIONAL_HEADER* OptHed = (IMAGE_OPTIONAL_HEADER*)optbuff;

	//�ҵ�NTͷ������Ŀ¼����ĵ����
	OptHed->DataDirectory[1];

	//Ĭ�ϼ��ػ�ַ �� RVA ���ڵ�����ƫ��
	toAddress = (ULONG_PTR)_Address + (ULONG_PTR)OptHed->DataDirectory[1].VirtualAddress;

	IMAGE_IMPORT_DESCRIPTOR* IMPORTTable = 0;

	//�����ж�������
	//�������׵�ַ
	do
	{
		//ֱ�Ӷ�ȡ������ڴ�
		char* Imabuff[sizeof(IMAGE_IMPORT_DESCRIPTOR)] = {};
		VirtualProtectEx(m_process, (LPVOID)(toAddress), sizeof(IMAGE_IMPORT_DESCRIPTOR), PAGE_READWRITE, &dwOldProtect);
		ReadProcessMemory(m_process, (LPVOID)toAddress, Imabuff, sizeof(IMAGE_IMPORT_DESCRIPTOR), &dwRead);
		VirtualProtectEx(m_process, (LPVOID)(toAddress), sizeof(IMAGE_IMPORT_DESCRIPTOR), dwOldProtect, &dwOldProtect);

		//����while������Ҫ���������ⲿ����
		IMPORTTable = (IMAGE_IMPORT_DESCRIPTOR*)Imabuff;

		if (!IMPORTTable->Name)
			break;

		//DLL��VA
		DWORD NameAddress = (ULONG_PTR)IMPORTTable->Name + (ULONG_PTR)_Address;

		//��ӡ����			//�����ɹ�
		char DllName[50] = {};
		VirtualProtectEx(m_process, (LPVOID)(NameAddress), 50, PAGE_READWRITE, &dwOldProtect);
		ReadProcessMemory(m_process, (LPVOID)NameAddress, DllName, 50, &dwRead);
		VirtualProtectEx(m_process, (LPVOID)(NameAddress), 50, dwOldProtect, &dwOldProtect);
		//printf("DLL����%s\n\n", DllName);
		//
		//��һ������ɫ��DLL��
		//��ȡ��ǰ���λ�ã�׼�����û�ȥ
		CONSOLE_SCREEN_BUFFER_INFO  rea = { 0 };
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &rea);

		CONSOLE_CURSOR_INFO Cci = { 1, false };			//������Խṹ
		SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &Cci);
		COORD loc = { rea.dwCursorPosition.X + 15 , rea.dwCursorPosition.Y };
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), F_H_GREEN);
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), loc);
		printf("%s\n\n", DllName);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), F_H_WHITE);



		//IAT��ַ
		IMAGE_THUNK_DATA * ThunkAddress = (IMAGE_THUNK_DATA*)((ULONG_PTR)IMPORTTable->FirstThunk + (ULONG_PTR)_Address);
		//INT��ַ
		IMAGE_THUNK_DATA * INTAddress = (IMAGE_THUNK_DATA*)((ULONG_PTR)IMPORTTable->OriginalFirstThunk + (ULONG_PTR)_Address);
		do
		{

			//����IAT���õ���ַ,�ٽ���INT���ó���ź����֣�

			//������
			IMAGE_THUNK_DATA  ThunkDataAddress = {};
			VirtualProtectEx(m_process, (LPVOID)(ThunkAddress), sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &dwOldProtect);
			ReadProcessMemory(m_process, (LPVOID)ThunkAddress, &ThunkDataAddress, sizeof(IMAGE_THUNK_DATA), &dwRead);
			VirtualProtectEx(m_process, (LPVOID)(ThunkAddress), sizeof(IMAGE_THUNK_DATA), dwOldProtect, &dwOldProtect);

			//��ַΪ�գ��ṹ��Ϊ��ʱ����ѭ��
			if (!ThunkDataAddress.u1.Function)
				break;

			//��ӡ��ַ
			printf("%08X	|	", ThunkDataAddress.u1.Function);



			///�ȶ���
			//��š�����
			//�õ���һ��IMAGE_THUNK_DATA�ṹ���ַ		//�������
			//������
			IMAGE_THUNK_DATA  ThunkData = {};
			VirtualProtectEx(m_process, (LPVOID)(INTAddress), sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &dwOldProtect);
			ReadProcessMemory(m_process, (LPVOID)INTAddress, &ThunkData, sizeof(IMAGE_THUNK_DATA), &dwRead);
			VirtualProtectEx(m_process, (LPVOID)(INTAddress), sizeof(IMAGE_THUNK_DATA), dwOldProtect, &dwOldProtect);

			//�������ŵ����
			if (IMAGE_SNAP_BY_ORDINAL(ThunkData.u1.Ordinal))
			{
				//ֱ�Ӵ�ӡ���
				printf("%08X	|	", LOWORD(ThunkData.u1.Ordinal));
			}
			else
			{
				//�ҵ���� ����������ӡ
				//�õ�IMAGE_IMPORT_BY_NAME�ṹ��
				IMAGE_IMPORT_BY_NAME * ImportByName = (IMAGE_IMPORT_BY_NAME*)((ULONG_PTR)ThunkData.u1.AddressOfData + (ULONG_PTR)_Address);

				//�ٶ�����						//IMAGE_IMPORT_BY_NAME���̶���С����û�£��õ��׵�ֱַ�Ӷ�
				IMAGE_IMPORT_BY_NAME  byName = {};
				VirtualProtectEx(m_process, (LPVOID)(ImportByName), sizeof(IMAGE_IMPORT_BY_NAME), PAGE_READWRITE, &dwOldProtect);
				ReadProcessMemory(m_process, (LPVOID)ImportByName, &byName, sizeof(IMAGE_IMPORT_BY_NAME), &dwRead);
				VirtualProtectEx(m_process, (LPVOID)(ImportByName), sizeof(IMAGE_IMPORT_BY_NAME), dwOldProtect, &dwOldProtect);

				//��ӡ���
				printf("%08X	|	", byName.Hint);

				//����������
				char _FuntionName[50] = {};
				VirtualProtectEx(m_process, (LPVOID)(&ImportByName->Name), 50, PAGE_READWRITE, &dwOldProtect);
				ReadProcessMemory(m_process, (LPVOID)&ImportByName->Name, _FuntionName, 50, &dwRead);
				VirtualProtectEx(m_process, (LPVOID)(&ImportByName->Name), 50, dwOldProtect, &dwOldProtect);

				//��ӡ������
				printf("%s	|	", _FuntionName);
			}

			printf("\n");
			//����һ����ѭ��
			ThunkAddress++;
			INTAddress++;
		} while (1);


		printf("\n\n\n");

		(ULONG_PTR)toAddress += sizeof(IMAGE_IMPORT_DESCRIPTOR);
		//�ж��Ƿ�Ϊ��
	} while (1);		//�ṹ��Ϊ��ʱ����			//while ������ı����(NameAddress)��ֵ)
}











