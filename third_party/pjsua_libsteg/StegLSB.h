#pragma once
//#include "stdafx.h"
//#ifndef __AFXWIN_H__
//#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
//#endif

#define BYTE unsigned char

#define UINT unsigned int

//#define bool bool
#define MAX_PATH          260

struct PBItem
{
	unsigned int byte;     // �ɹ��滻��λ��������ڼ��ֽڣ���0��ʼ
	unsigned int bit;      // ����һ�ֽڵĵڼ������أ����λΪ0
	unsigned int rank;     // ������������
};

class CStegLSB
{
	//DECLARE_SERIAL(CStegLSB)

public:
	~CStegLSB();  // �ͷ�λ��

	virtual void InitPosBook();           // ����λ��
	virtual void PreparePosBook();        // ׼��λ��������
	virtual unsigned int GetParam(unsigned int sel);    // ��ȡ�����㷨����������

	bool Embed(unsigned char* Dt, unsigned int Dtlen, bool * Sn, unsigned int Snlen, unsigned char * Carrier);   // Ƕ��
	bool Extract(unsigned char* Dt, unsigned int Dtlen, bool * Sn, unsigned int Snlen, unsigned char * Carrier); // ��ȡ

protected:
	PBItem * m_PB;  // λ��ָ��
	unsigned int m_len;     // λ����
	bool m_prepared;   // λ�������־
};