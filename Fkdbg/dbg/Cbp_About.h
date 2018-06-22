#pragma once
#include "Cdbg_Info.h"
#include <vector>
#include "debugRegisters.h"

#define BP_HARD_EXEC     0 
#define BP_HARD_WR			 1
#define BP_HARD_RWDATA   3
#define K1_BYTE          0
#define K2_BYTE          1
#define K4_BYTE          3       


// �ϵ������
class Cbp_About
{
public:
	Cbp_About();
	~Cbp_About();

// �����ϵ����:
	void SetBP_TF(HANDLE hTrhead);
	void RemoveBP_TF(HANDLE hThread);

        

// ����ϵ����:
	typedef struct _BP_INT3 {
		BOOL   delFlag;        // ɾ����־:һ���Զϵ�Ϊ true
		BYTE   oldbyte;        // ԭ����һ�ֽ�����
		SIZE_T address;        // �ϵ��ַ
	}BP_INT3;
	std::vector <BP_INT3> m_Vec_INT3;
	BOOL SetBP_INT3(HANDLE hProc, SIZE_T address, bool delflag);


// Ӳ���ϵ����
	typedef struct _BP_HARD {
		BOOL   delFlag;        // ɾ����־:һ���Զϵ�Ϊ true
		SIZE_T address;        // �ϵ��ַ
		DWORD Type;						 // �ϵ�����
		DWORD Len;             // �ϵ��������
	}BP_HARD;

	std::vector<BP_HARD> m_Vec_HARD;

	BOOL SetBP_HARD(HANDLE hThread, ULONG_PTR uAddress, DWORD Type, DWORD Len, BOOL delflag);
	void RemoveBP_HARD(HANDLE hThread, SIZE_T Address, bool bc);    

// �ڴ�ϵ����
	typedef struct _BP_MEM {
		BOOL   delFlag;          // ɾ����־:һ���Զϵ�Ϊ true
		SIZE_T address;          // �ϵ��ַ
		DWORD  dwOldAttri;       // �ڴ�ϵ�ԭ�����ڴ��ҳ����
	}BP_MEM;
	std::vector<BP_MEM> m_Vec_Mem;
	void  SetBP_MEM(HANDLE hProcess, SIZE_T Address, BOOL defflag);


// �����ϵ����
//�Ĵ�������ö��
	enum Eregister
	{
		e_eax,
		e_ecx,
		e_edx,
		e_ebx,
		e_esp,
		e_ebp,
		e_esi,
		e_edi,
		e_ecs,
		e_ess,
		e_eds,
		e_ees,
		e_efs,
		e_egs
	};


	typedef struct _BP_CONDITION {
		BYTE   oldbyte;          // ԭ����һ�ֽ�����
		SIZE_T address;          // �ϵ��ַ
		BOOL   ConditionWay;     // ������ʽ
		DWORD  Times;            // �����ϵ����д���
		DWORD  EndTimes;         // �����ϵ���������
		DWORD  EndValue;         // �����ϵ������Ĵ���ֵ
		Eregister Reg;           // �����ϵ���Ҫ�жϵļĴ���
	}BP_CONDITION;



	std::vector<BP_CONDITION> m_Vec_Condition;

	BOOL SetBP_Condition(HANDLE hProcess, SIZE_T Address, BOOL ConditionWay, DWORD EndTimes, DWORD EndValue, Eregister Reg);

};

