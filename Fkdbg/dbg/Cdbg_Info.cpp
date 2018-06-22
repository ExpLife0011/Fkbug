#include "stdafx.h"
#include "Cdbg_Info.h"


Cdbg_Info::Cdbg_Info()
{
}


Cdbg_Info::~Cdbg_Info()
{
}




void Cdbg_Info::showDebugInfor(HANDLE hProc, HANDLE hThread, LPVOID address)
{

	CONTEXT ct = { CONTEXT_ALL };
	GetThreadContext(hThread, &ct);

	// ��ȡ�Ĵ�����Ϣ
	printf("EIP:%08X EAX:%X ECX:%X EDX:%X\n",
		ct.Eip,
		ct.Eax,
		ct.Ecx,
		ct.Edx);


	char opcode[200];
	DWORD dwWrite = 0;
	ReadProcessMemory(hProc, address, opcode, 200, &dwWrite);

 
  // ��ȡ�������Ϣ
	DISASM da = { 0 };
	da.Archi = 0;// 32λ���
	da.EIP = (UIntPtr)opcode; // ����opcode�Ļ�������ַ
	da.VirtualAddr = (UInt64)address;// opcodeԭ�����ڵĵ�ַ

	int nLen = 0;

	int nCount = 20;
	while (nCount-- && -1 != (nLen = Disasm(&da))) {
		printf("%llx | %s\n", da.VirtualAddr, da.CompleteInstr);
		da.VirtualAddr += nLen;
		da.EIP += nLen;
	}
}
