#pragma once

//#include "StegLSB.h"
//#include "Steg711.h"
#include "StegRTP.h"
//#include "Steg723.h"
//#include "Steg729.h"
//#include "rtp/rtppacket.h"
//#include "rtp/jmutex.h"
//#include "UsefulTools.h"
//#include "../Voice/AC7231.h"
#include <queue>
extern "C"
{
	#include "iLBC.h"
	#include "iLBC_define.h"
	#include "iLBC_decode.h"
	#include "iLBC_encode.h"
}

#include <pj/types.h>
#include <pj/lock.h>
//#include <pjsip/sip_types.h>

extern iLBC_Enc_Inst_t Enc_Inst;
extern iLBC_Dec_Inst_t Dec_Inst;
extern UINT mode20_30;
//enum steg_state
//{
//	NONE					= (int)0x00000, 
//	NEW_MESSAGE_ARRIVED		= (int)0x00001,
//	NEW_MESSAGE_OVER		= (int)0x00002,
//	SEND_FILE_STEP1			= (int)0x00004,
//	NEW_FILE_ARRIVED		= (int)0x00008,
//};
//#ifndef __AFXWIN_H__
//	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
//#endif

struct SIABuffer   // Ӧ�ò㻺�����ṹ
{
	BYTE * Storage;   // ������
	UINT Length;      // ���ݳ���
	BYTE * Cursor;    // ��ָ�� 
};

struct STMFrame    // STM��֡�ṹ
{
	BYTE * Frame;  // limited by the length of relating field in the head
	UINT Length;   // ֡����
	UINT Time;     // �ȴ�ʱ��
};
/*
enum STATUS   // ����״̬
{
	IDLE,  SYNSENT,  SYNCONF,  SYNRECV,  SYNREPL,  ESTAB
};
*/

class CStegSuit
{
public:
	void Create(pj_pool_t * pool);     // �����࣬����ʼ��
	void Allocate();    // �����ڴ�ռ�
	void Configure();
	void Clean();      // �ͷ��ڴ�
	void Control( UINT Command );
// 
// 	UINT Send (void * pSrc, int length, int type);
// 	UINT Receive (void * pDst, int maxlength, int type);
// 	UINT CStegSuit::GetSecMsg(BYTE * SecretMessage);
// 	UINT CStegSuit::Retransmission();
// 	UINT CStegSuit::STMSdata(int *datatype);		//��SIA��������
// 	UINT CStegSuit::STMSheader(int datatype);
// 	UINT CStegSuit::Embedheader(void * pCarrier);
// 	UINT CStegSuit::STMR(void * pCarrier, char SecMsg);	

	UINT Send (void * pSrc, int length, int type);
	UINT Receive (void * pDst, int maxlength, int type);
	UINT Embedding( void * pCarrier,UINT RTPheadlen, /*CAudioBase * pACIn*/char* pPcmIn );
	UINT Retransmission();
	UINT STMSdata(int *datatype);		//��SIA��������
	UINT SAESdata( void * pCarrier,UINT len, /*CAudioBase * pACIn*/char* pPcmIn);
	UINT STMSheader(int datatype);
	UINT SAESheader(void * pCarrier);

	UINT Retriving(void *hdr, void * pCarrier,/*CAudioBase * pACOut*/char* pPcmOut);
	UINT SAER(void *hdr, void * pCarrier,/*CAudioBase * pACOut */char* pPcmOut);
	UINT STMR();
	//UINT CStegSuit::PrintMessage(UINT type);

	//lock
	//JMutex SLock;
	//std::mutex SLock;
	pj_lock_t *SLock;
	pj_pool_t * pool;
	void lock_init(pj_pool_t * pool);
	void lock();
	void unlock();

public:

	UINT CarrierType;    // ��������
	UINT SIADU, Harves;
	UINT m_Seclev;
	int quit_flag;
	//int received;
	//int received_over;
//	UINT steg_status;
	bool bMessageArrived;
	bool bFileArrived;
	bool bFileSent;
	pj_str_t msg_1;
	UINT length;
	pj_thread_t		*thread;
protected:
	UINT m_ReqSec;

	UINT m_nSegment;
	UINT LThreshold, HThreshold;
	UINT STMDU, SAEDU, maxSTM, maxSAE;

	SIABuffer SD[2], RC[2];

	STMFrame m_Window[8], m_Cache[8], m_Crt, m_Rcv;
	UINT m_SEQ, m_LastRANN, m_LastRSEQ;
	UINT m_Threshold;
	UINT m_retranstep;

	BYTE * m_FrmS, * m_FrmR;
	BYTE * m_FrmSCursor, * m_FrmRCursor;
	UINT m_FrmSLength, m_RTPSeq;
	BYTE m_CheckTable[256];
	UINT m_ActualByte;			//Ԥ�ݺ�ʵ����Ƕ����ֽ���
	std::queue<STMFrame> m_Retrans; //ѹ���ش�����
	STMFrame m_Resend;		//�ش�ָ��
	
	CStegLSB * m_pAudio;
	CStegLSB * m_pRTP;


	//SSW: need to be modified for iLBC
	// ssw iLBC
	//char m_chEmdSecMsg[2];		//Ƕ��Ļ�����Ϣ
	//char m_chRtrSecMsg[2];		//��ȡ�Ļ�����Ϣ
	char m_chEmdSecMsg[35];		//Ƕ��Ļ�����Ϣ
	char m_chRtrSecMsg[35];		//��ȡ�Ļ�����Ϣ
	//ssw iLBC
	//char m_pFrmBuf[24*3];
	//THZ: ��С��������������һ֡
	char m_pFrmBuf[50];		//ssw: �����������������ػ򲻴����أ�
	//char m_pFrmBuf[50 * 3];		//ssw: �����������������ػ򲻴����أ�
// 	
// #if 0
// 	CStegLSB * m_pAudio;
// #endif
// 	CStegLSB * m_pRTP;

protected:

	bool Inside (UINT Seq, UINT LastRANN);
	bool Between(UINT Seq, UINT LastRSEQ);
	void MakeCheckTable();
};

