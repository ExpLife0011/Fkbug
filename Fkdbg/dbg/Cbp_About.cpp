#include "stdafx.h"
#include "Cbp_About.h"

Cbp_About::Cbp_About()
{
}


Cbp_About::~Cbp_About()
{

}


// T ���õ���ִ������    
void Cbp_About::SetBP_TF(HANDLE hTrhead) 
{
	CONTEXT ct = { CONTEXT_CONTROL };
	if (!GetThreadContext(hTrhead, &ct))
	{
		printf("��ȡ�̻߳���ʧ��\n");
	}
	EFLAGS* pEflag = (EFLAGS*)&ct.EFlags;
	pEflag->TF = 1;
	if (!SetThreadContext(hTrhead, &ct))
	{
		printf("�����̻߳���ʧ��\n");
	}
}

void Cbp_About::RemoveBP_TF(HANDLE hThread)
{
	CONTEXT ct = { CONTEXT_CONTROL };
	if (!GetThreadContext(hThread, &ct))
	{
		printf("��ȡ�̻߳���ʧ��\n");
	}
	EFLAGS* pEflag = (EFLAGS*)&ct.EFlags;
	pEflag->TF = 0;
	if (!SetThreadContext(hThread, &ct))
	{
		printf("�����̻߳���ʧ��\n");
	}
}


// Ӳ���ϵ㴦���֣�

// ����Ӳ���ϵ�����
BOOL Cbp_About::SetBP_HARD(HANDLE hThread, ULONG_PTR uAddress, DWORD Type, DWORD Len, BOOL delflag)
{
	CONTEXT ct = { CONTEXT_DEBUG_REGISTERS };
	if (!GetThreadContext(hThread, &ct))
	{
		printf("��ȡ�̻߳���ʧ��\n");
	}

	// ���ݳ��ȶԵ�ַ���ж��봦��(����ȡ��)
	if (Len == 1)    // ����Ϊ 2 ʱ��ַ��Ҫ 2 �ֽڶ���
		uAddress = uAddress - uAddress % 2;
	if (Len == 3)    // ����Ϊ 4 ʱ��ַ��Ҫ 4 �ֽڶ���
		uAddress = uAddress - uAddress % 4;


	// �ҿӶ�, �ĸ��Ӷ�û��ռ��ʹ�ĸ�
	DBG_REG7* pDr7 = (DBG_REG7*)&ct.Dr7;
	if (pDr7->L0 == 0) {  
		
		// RW Ϊ 0 ��ʾΪִ�жϵ�
		pDr7->L0 = 1;
		ct.Dr0 = uAddress;
		pDr7->RW0 = Type;
		pDr7->LEN0 = Len;
		
	}
	else if (pDr7->L1 == 0) {
		pDr7->L1 = 1;
		ct.Dr1 = uAddress;
		pDr7->RW1 = Type;
		pDr7->LEN1 = Len;
	}
	else if (pDr7->L2 == 0) {
		pDr7->L2 = 1;
		ct.Dr2 = uAddress;
		pDr7->RW2 = Type;
		pDr7->LEN2 = Len;
	}
	else if (pDr7->L3 == 0) {
		pDr7->L3 = 1;
		ct.Dr3 = uAddress;
		pDr7->RW3 = Type;
		pDr7->LEN3 = Len;
	}
	else
	{
		return FALSE;
	}
	if (!SetThreadContext(hThread, &ct))
	{
		printf("���벻�Ϸ���δ֪����\n");
	}
	BP_HARD BpHard;
	BpHard.address = uAddress;
	BpHard.delFlag = delflag;
	BpHard.Len = Len;
	BpHard.Type = Type;

	m_Vec_HARD.push_back(BpHard);

	return 0;
}

// �Ƴ�Ӳ���ϵ�����, ���һ��������ʾ�Ǵ����쳣ʱȥ��һ���Զϵ㻹���û�����ֱ���Ƴ��ϵ�
void Cbp_About::RemoveBP_HARD(HANDLE hThread, SIZE_T Address, bool bc)
{
	//��Ҫ�õ�DR6 DR7 �Ĵ���,����Dr0
	CONTEXT ct = { 0 };
	ct.ContextFlags = CONTEXT_ALL;

	if (!GetThreadContext(hThread, &ct))
	{
		printf("��ȡ�̻߳���ʧ��\n");
	}
	DBG_REG7* pDr7 = (DBG_REG7*)&ct.Dr7;


	std::vector<Cbp_About::BP_HARD>::iterator iterHard;
	for (iterHard = m_Vec_HARD.begin(); iterHard != m_Vec_HARD.end(); )
	{
		// ���û�����Ҫɾ���Ķϵ㣬�Ҷ��˾�ֱ��ɾ��
		if (bc)
		{
			if (Address == iterHard->address)
			{
				iterHard = m_Vec_HARD.erase(iterHard);
				printf("Ӳ���ϵ� %x   ��ɾ��\n", iterHard->address);
				if (ct.Dr0 == Address)
				{
					pDr7->L0 = 0;
				}
				else if (ct.Dr1 == Address)
				{
					pDr7->L1 = 0;
				}
				else if (ct.Dr2 == Address)
				{
					pDr7->L2 = 0;
				}
				else if (ct.Dr3 == Address)
				{
					pDr7->L3 = 0;
				}
			}
			else
			{
				iterHard++;
			}
		}
		// ���쳣�����öϵ㣬Ϊһ���Զϵ���Ƴ�
		else
		{
			// �ҵ������쳣�Ķϵ�
			if (Address == iterHard->address )
			{
				// �����Ӳ�����ݷ��ʶϵ㣬����ʱ�Ѿ�ִ��������ָ���ˣ�������Ϊû�취��������ָ��ĳ��ȣ����Ժ���ͨ��ֱ�Ӳ���eip�ķ�ʽ���˻�ȥ
				// ����ֻ��ֱ����һ������
  				if (iterHard->Type == BP_HARD_RWDATA)
  				{
 						
 					}
				// һ���Զϵ��ɾ�����������жϼĴ���
				if (iterHard->delFlag)
				{
					printf("Ӳ���ϵ� %x   ��ɾ��\n", iterHard->address);
					iterHard = m_Vec_HARD.erase(iterHard);
					if (ct.Dr0 == Address)
					{
						pDr7->L0 = 0;
					}
					else if (ct.Dr1 == Address)
					{
						pDr7->L1 = 0;
					}
					else if (ct.Dr2 == Address)
					{
						pDr7->L2 = 0;
					}
					else if (ct.Dr3 == Address)
					{
						pDr7->L3 = 0;
					}
				}	
				else
				{
					iterHard++;
				}
			}
			else
			{
				iterHard++;
			}
		}
		
	}

	if (!SetThreadContext(hThread, &ct))
	{
		printf("���벻�Ϸ���δ֪����\n");
	}

	return;
}

void Cbp_About::SetBP_MEM(HANDLE hProcess, SIZE_T Address, BOOL delflag)
{
	// �洢ԭ����ҳ������
	DWORD dwOldProtect = 0;

	// �޸��¶ϵ�ַ����ҳ����
	VirtualProtectEx(hProcess, (LPVOID)Address, 1, PAGE_NOACCESS, &dwOldProtect);

	// ���ڴ���ʶϵ�浽�ڴ�ϵ�������
	BP_MEM bpMem = { 0 };
	bpMem.address = Address;
	bpMem.dwOldAttri = dwOldProtect;
	bpMem.delFlag = delflag;
	
	m_Vec_Mem.push_back(bpMem);
}

BOOL Cbp_About::SetBP_Condition(HANDLE hProcess, SIZE_T Address, BOOL ConditionWay, DWORD EndTimes, DWORD EndVaule, Eregister Reg)
{
	// 1. ��ȡ�ϵ����ڵ��ڴ��1�ֽڵ����ݱ���
	BP_CONDITION bp_Condition = { 0 };
	DWORD dwRead = 0;
	if (!ReadProcessMemory(hProcess, (LPVOID)Address, &bp_Condition.oldbyte, 1, &dwRead)) {
		LOG("��ȡ�ڴ�ʧ��");
		return false;
	}
	// 2. ��int 3ָ��(0xcc)д�뵽�ϵ�ĵ�ַ��
	if (!WriteProcessMemory(hProcess, (LPVOID)Address, "\xcc", 1, &dwRead)) {
		LOG("д���ڴ�ʧ��");
		return false;
	}
	bp_Condition.address = Address;
	bp_Condition.ConditionWay = ConditionWay;
	bp_Condition.Times = 0;
	bp_Condition.EndTimes = EndTimes;
	bp_Condition.Reg = Reg;
	bp_Condition.EndValue = EndVaule;

	// 3. ����ϵ���Ϣ
	m_Vec_Condition.push_back(bp_Condition);
	return 0;
}





// ����ϵ㴦���֣�


// ��������ϵ�
BOOL Cbp_About::SetBP_INT3(HANDLE hProc, SIZE_T address, bool delflag)
{
	// 1. ��ȡ�ϵ����ڵ��ڴ��1�ֽڵ����ݱ���
	BP_INT3 bp = { 0 };
	DWORD dwRead = 0;
	if (!ReadProcessMemory(hProc, (LPVOID)address, &bp.oldbyte, 1, &dwRead)) {
		LOG("��ȡ�ڴ�ʧ��");
		return false;
	}
	// 2. ��int 3ָ��(0xcc)д�뵽�ϵ�ĵ�ַ��
	if (!WriteProcessMemory(hProc, (LPVOID)address, "\xcc", 1, &dwRead)) {
		LOG("д���ڴ�ʧ��");
		return false;
	}
	bp.address = address;
	bp.delFlag = delflag;
	// 3. ����ϵ���Ϣ
	m_Vec_INT3.push_back(bp);
	
	return true;
}


