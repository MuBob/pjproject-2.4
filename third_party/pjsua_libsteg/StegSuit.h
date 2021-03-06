#include "StegRTP.h"
#include <queue>
#include <pj/lock.h>

extern "C" {
#include "iLBC.h"
#include "iLBC_define.h"
#include "iLBC_decode.h"
#include "iLBC_encode.h"
}

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
//	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
//#endif

struct SIABuffer   // 应用层缓冲区结构
{
	BYTE * Storage;   // 缓冲区
	UINT Length;      // 数据长度
	BYTE * Cursor;    // 读指针 
};

struct STMFrame    // STM层帧结构
{
	BYTE * Frame;  // limited by the length of relating field in the head
	UINT Length;   // 帧长度
	UINT Time;     // 等待时间
};
/*
enum STATUS   // 连接状态
{
IDLE,  SYNSENT,  SYNCONF,  SYNRECV,  SYNREPL,  ESTAB
};
*/

class CStegSuit
{
private:
	UINT getSeqFromCursor(unsigned char* cursor);
	UINT getTypeFromCursor(unsigned char* cursor);
	UINT getLenFromCursor(unsigned char* cursor);
	void setLenToCursor(unsigned int len, int i, unsigned int odd, unsigned int seq, unsigned int lastRSeq, unsigned char* &cursor);
public:
	void Create(pj_pool_t * pool);     // 构造类，及初始化
	void Allocate(unsigned int pt);    // 分配内存空间
	void Configure();
	void Clean();      // 释放内存
	void Control(unsigned int pt, UINT Command);
	// 
	// 	UINT Send (void * pSrc, int length, int type);
	// 	UINT Receive (void * pDst, int maxlength, int type);
	// 	UINT CStegSuit::GetSecMsg(BYTE * SecretMessage);
	// 	UINT CStegSuit::Retransmission();
	// 	UINT CStegSuit::STMSdata(int *datatype);		//向SIA申请数据
	// 	UINT CStegSuit::STMSheader(int datatype);
	// 	UINT CStegSuit::Embedheader(void * pCarrier);
	// 	UINT CStegSuit::STMR(void * pCarrier, char SecMsg);	

	UINT Send(void * pSrc, int length, int type);
	UINT Receive(void * pDst, int maxlength, int type);
	UINT Embedding(void * pCarrier, UINT RTPheadlen, pj_size_t dataLen, /*CAudioBase * pACIn*/char* pPcmIn, UINT channel_pt);
	UINT Retransmission();
	UINT STMSdata(int *datatype);		//向SIA申请数据
	UINT SAESdata(void * pCarrier, UINT RTPheadlen, pj_size_t dataLen, /*CAudioBase * pACIn*/char* pPcmIn);
	UINT STMSheader(int datatype);
	UINT SAESheader(void * pCarrier);

	UINT Retriving(void *hdr, void * pCarrier, int pCarrierLength,/*CAudioBase * pACOut*/char* pPcmOut, UINT channel_pt);
	UINT SAER(void *hdr, void * pCarrier, int pCarrierLength,/*CAudioBase * pACOut */char* pPcmOut);
	UINT STMR();

	void Encode(unsigned char *encoded_data,  //编码后的数据
		void *block,//要编码的数据
		pj_size_t dataLen,  //要编码的数据长度
		short bHide,
		void *hdTxt
	);
	void Decode(void *decblock,  //要解码的数据
		unsigned char *bytes,  //解码后的数据
		int bytes_length,
		int mode,
		short bHide,
		char *msg);

	//lock
	//JMutex SLock;
	//std::mutex SLock;
	pj_lock_t *SLock;
	pj_pool_t * pool;
	void lock_init(pj_pool_t * pool);
	void lock();
	void unlock();

public:

	UINT CarrierType;    // 载体类型
	UINT SIADU, Harves;
	UINT m_Seclev;
	int quit_flag;
	//int received;
	//int received_over;
	//	UINT steg_status;
	bool bMessageSent;
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

	SIABuffer SD[2], RC[2]; //SIA层数据发送和接收缓存

#define COUNT_WINDOW_CACHE 0x8  //统一发送方和接收方的缓存窗口数量
#define COUNT_WINDOW  COUNT_WINDOW_CACHE
#define COUNT_CACHE  COUNT_WINDOW_CACHE
	STMFrame m_Window[COUNT_WINDOW], m_Cache[COUNT_CACHE], m_Crt, m_Rcv;
	UINT m_SEQ, m_LastRANN, m_LastRSEQ;
	UINT m_Threshold;
	UINT m_retranstep;

	BYTE * m_FrmS, *m_FrmR;
	BYTE * m_FrmSCursor, *m_FrmRCursor; //当前待发送帧和待接收帧的缓存
	UINT m_FrmSLength, m_RTPSeq;
	BYTE m_CheckTable[256];
	UINT m_ActualByte;			//预演后实际能嵌入的字节数
	std::queue<STMFrame> m_Retrans; //压入重传队列
	STMFrame m_Resend;		//重传指针

	CStegLSB * m_pAudio;
	CStegLSB * m_pRTP;
//	long count_resend, count_window_full;

	//SSW: need to be modified for iLBC
	// ssw iLBC
	//char m_chEmdSecMsg[2];		//嵌入的机密信息
	//char m_chRtrSecMsg[2];		//提取的机密信息
	char m_chEmdSecMsg[35];		//嵌入的机密信息
	char m_chRtrSecMsg[35];		//提取的机密信息
								//ssw iLBC
								//char m_pFrmBuf[24*3];
								//THZ: 缩小编码结果缓冲区到一帧
	char m_pFrmBuf[50];		//ssw: 语音编码结果（带隐藏或不带隐藏）
							//char m_pFrmBuf[50 * 3];		//ssw: 语音编码结果（带隐藏或不带隐藏）
							// 	
							// #if 0
							// 	CStegLSB * m_pAudio;
							// #endif
							// 	CStegLSB * m_pRTP;

	unsigned int m_channel_pt_receive;
	unsigned int m_channel_pt_send;

protected:

	bool Inside(UINT Seq, UINT LastRANN);
	bool Between(UINT Seq, UINT LastRSEQ);
	void MakeCheckTable();
};

