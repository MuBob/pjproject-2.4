//#include "stdafx.h"
//#include <tchar.h>
#include <iostream>
#include "StegSuit.h"
#include "StegiLBC.h"
#include <pj/os.h>

#include <pj/string.h>
//#include <pjsua-lib/pjsua.h>
//#include <pjsua-lib/pjsua_internal.h>
//extern   pjsua_data pjsua_var;
//#include "../../pjmedia/include/pjmedia/rtp.h"
//#include <pjsua_internal.h>

#include <pj/log.h>
#include <pjmedia/alaw_ulaw.h>
#include <pjmedia-codec/types.h>
#include <pjmedia/types.h>
//ssw
//#include "StegTalk.h"

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#endif

iLBC_Enc_Inst_t Enc_Inst;
iLBC_Dec_Inst_t Dec_Inst;
UINT mode20_30;

//extern 
#define THIS_FILE			"StegSuit.c"
void CStegSuit::lock_init(pj_pool_t * pool)
{
	this->pool = pool;
	pj_lock_t *tLock;
	
	pj_status_t s = pj_lock_create_simple_mutex(this->pool, "steglock", &tLock);
	if (s != PJ_SUCCESS)
	{
		return;
	}

	if (SLock)
	{
		pj_lock_destroy(SLock);
	}

	SLock = tLock;
}

void CStegSuit::lock()
{
	pj_lock_acquire(SLock);
}

void CStegSuit::unlock()
{
	pj_lock_release(SLock);
}

void  on_pager_wrapper1(pj_str_t *from, pj_str_t* to, pj_str_t *body, pj_str_t* mimetype);

void CStegSuit::Create(pj_pool_t * pool)
{
	quit_flag = 0;
//	steg_status = NONE;
	//air
	bMessageArrived=false;
	bFileArrived = false;
	bFileSent = false;

	//lock init
	//SLock.Init();
	SLock = NULL;
	lock_init(pool);
	
	initEncode(&Enc_Inst, mode20_30);
	initDecode(&Dec_Inst, mode20_30, 1);

	SD[0].Storage = SD[1].Storage = RC[0].Storage = RC[1].Storage = NULL;
	SD[0].Cursor = SD[1].Cursor = RC[0].Cursor = RC[1].Cursor = NULL;
	SD[0].Length = SD[1].Length = RC[0].Length = RC[1].Length = 0;

	//m_Seclev = 1;
	//m_Status = IDLE;
	m_SEQ = 0;
	m_LastRSEQ = 0;
	m_LastRANN = 1;
	m_ActualByte = 0;

	for (int i=0; i<8; i++)
	{
		m_Window[i].Length = 0;  m_Window[i].Time = 0;  m_Window[i].Frame = NULL;
		m_Cache[i].Length = 0;  m_Cache[i].Time = 0;  m_Window[i].Frame = NULL;
	}
	m_Crt.Frame = NULL; m_Crt.Length = 0; m_Crt.Time = 0;
	m_Rcv.Frame = NULL; m_Rcv.Length = 0; m_Rcv.Time = 0;
	m_Resend.Frame =NULL; m_Resend.Frame=0; m_Resend.Time=0; //重传
	m_retranstep = 0;	//重传步骤

	m_FrmSLength = 0; m_RTPSeq = 0;
	m_FrmS = NULL;  m_FrmR = NULL;
	m_FrmSCursor = NULL;  m_FrmRCursor = NULL;

	//CRuntimeClass * pRun = RUNTIME_CLASS(CStegRTP);
	m_pRTP = (CStegLSB *) (new CStegRTP());

	CarrierType = 97;
	m_nSegment = 1;
	LThreshold = 40;
	HThreshold = 1000;

	m_Threshold = LThreshold;

	MakeCheckTable();
	/*
	int  *msg = new int(3210);
	int *encoded_msg = new int(0);
	int msg_length = sizeof(msg);
	int *decoded_msg = new int(0);
	pj_int16_t *src = (pj_int16_t *)msg;
	pj_uint8_t *dst_encode = (pj_uint8_t *)encoded_msg;
	pj_uint16_t *dst_decode = (pj_uint16_t *)decoded_msg;

	for (size_t i = 0; i < msg_length; ++i, ++dst_encode)
	{
		*dst_encode = pjmedia_linear2alaw(src[i]);  //pcmu
		PJ_LOG(4, (THIS_FILE, "Create: encode src[%d]=%d, dst[%d]=%d", i, src[i], i, *dst_encode));
	}
	PJ_LOG(4, (THIS_FILE, "Create:src=%s, after encoded = %s!\r", msg, encoded_msg));
	dst_encode = (pj_uint8_t *)encoded_msg;
	for (size_t i = 0; i < msg_length; ++i)
	{
		*dst_decode++ = (pj_uint16_t)pjmedia_alaw2linear(*dst_encode++);  //pcmu
		PJ_LOG(4, (THIS_FILE, "Create:test decode src[%d]=%d, dst[%d]=%d", i, *(dst_encode-1), i, *(dst_decode-1)));
	}
	PJ_LOG(4, (THIS_FILE, "Create:src=%s, after decoded = %s!\r", encoded_msg, decoded_msg));
	//delete[] encoded_msg;
	//delete[] decoded_msg;
	*/
	return;
}

void CStegSuit::Allocate()
{
	maxSAE = 1 + m_pRTP->GetParam(2);
	//changed for 20ms
	if(mode20_30==20)
		maxSAE = iLBC_SAEDU_20 + m_pRTP->GetParam(2);
	else
		maxSAE = iLBC_SAEDU_30 + m_pRTP->GetParam(2);

	maxSTM = maxSAE * m_nSegment;
	//SIADU = 100;
	SIADU = (maxSTM-3)*16*((maxSTM-3)*16>=MAX_PATH*2) + MAX_PATH*2*((maxSTM-3)*16<MAX_PATH*2);

	Harves = SIADU/2;
	
	SD[0].Storage = new BYTE[SIADU];  SD[0].Cursor = SD[0].Storage;
	SD[1].Storage = new BYTE[SIADU];  SD[1].Cursor = SD[1].Storage;
	RC[0].Storage = new BYTE[SIADU];  RC[0].Cursor = RC[0].Storage;
	RC[1].Storage = new BYTE[SIADU];  RC[1].Cursor = RC[1].Storage;

	for (int i=0; i<8; i++)
	{
		m_Window[i].Frame = new BYTE[maxSTM];
		m_Cache[i].Frame = new BYTE[maxSTM];
	}
	m_Crt.Frame = new BYTE[maxSTM];
	m_Rcv.Frame = new BYTE[maxSTM];
	m_Resend.Frame = new BYTE[maxSTM];		//重传

	m_FrmS = new BYTE[maxSTM]; 
	m_FrmSCursor = m_FrmS;		//发送指针
	m_FrmR = new BYTE[maxSTM+maxSAE];  
	m_FrmRCursor = m_FrmR;		//接收指针
	memset( m_FrmR, 0, maxSTM+maxSAE );

	m_pRTP->InitPosBook();
}

void CStegSuit::Configure()
{
}

void CStegSuit::Clean()
{
 	quit_flag = 1;
	if (SD[0].Storage!=NULL) delete [] SD[0].Storage;  SD[0].Storage = NULL;
	if (SD[1].Storage!=NULL) delete [] SD[1].Storage;  SD[1].Storage = NULL;
	if (RC[0].Storage!=NULL) delete [] RC[0].Storage;  RC[0].Storage = NULL;
	if (RC[1].Storage!=NULL) delete [] RC[1].Storage;  RC[1].Storage = NULL;
	for (int i=0; i<8; i++)
	{
		if (m_Window[i].Frame!=NULL) delete [] m_Window[i].Frame;  m_Window[i].Frame = NULL;
		if (m_Cache[i].Frame!=NULL) delete [] m_Cache[i].Frame;  m_Cache[i].Frame = NULL;
	}
	if (m_Crt.Frame!=NULL) delete [] m_Crt.Frame;  m_Crt.Frame = NULL;
	if (m_Rcv.Frame!=NULL) delete [] m_Rcv.Frame;  m_Rcv.Frame = NULL;
	if (m_FrmS!=NULL) delete [] m_FrmS;  m_FrmS = NULL;
	if (m_FrmR!=NULL) delete [] m_FrmR;  m_FrmR = NULL;

	if (m_Resend.Frame!=NULL) delete [] m_Resend.Frame; m_Resend.Frame = NULL;	//重传
	if (thread != NULL) // air
	{
		pj_thread_join(thread);
		pj_thread_destroy(thread);
	}

	thread = NULL;
	if (SLock)
	{
		SLock = NULL;
	}

	delete m_pRTP;
}

void CStegSuit::Control( UINT Command )
{
	
	//SSW: need to be modified for 20ms ilbc
	SAEDU = 9;	//一个RTP包中三个帧共嵌入一个字节
/*	if( Command == 0 )
		SAEDU=iLBC_SAEDU_20;
	else if( Command == 2 )
		SAEDU = iLBC_SAEDU_30;
	else
	SAEDU=iLBC_SAEDU_20;
*/
	STMDU = SAEDU * m_nSegment + 3;			//.ini中设置m_nSegment = 1, STM包头3字节 (24bit)
}

//0至255的数，在二进制表示下，每位相加的奇偶性;数组用于奇偶校验
void CStegSuit::MakeCheckTable()
{
	for ( int i=0; i<256; i++ )
	{
		int n = 0;
		for ( int k=0; k<8; k++ )
			n = n + ( ( i>>k ) & 0x1 ) ;
		m_CheckTable[i] = n%2;
	}
}

//将界面信息和文件缓存至隐蔽消息应用层缓冲区(SD)
UINT CStegSuit::Send(void * pSrc, int length, int type)
{
	if ( ( (UINT) length > SIADU ) || (type != 1 && type != 2) || pSrc == NULL) return 0;
	if ( type == 1) 
	{
		memset( SD[0].Storage, 0, SIADU );
		memcpy(SD[0].Storage, pSrc, length);
		SD[0].Length = length;
		SD[0].Cursor = SD[0].Storage;
	}
	else
	{
		memcpy(SD[1].Storage, pSrc, length);
		SD[1].Cursor = SD[1].Storage;
		SD[1].Length = length;
	}
	return 1;
}
//接收
UINT CStegSuit::Receive(void * pDst, int maxlength, int type)
{
	if ( type == 1 )
	{
		for ( UINT i=0; i<SIADU-1; i += 1 )
		{
			if (RC[0].Storage[i] == 0)
			{
				strcpy_s((char *)pDst, maxlength, (char *)RC[0].Storage);
				memset((void *)RC[0].Storage, 1, SIADU);
				RC[0].Cursor = RC[0].Storage;
				RC[0].Length = 0;
				return i;
			}
		}
		return 0;
	}

	if ( type == 2 )
	{
		if ( maxlength == -1 && RC[1].Storage[0] == '<' ) 
		{
			//air
			for (int i =0; i!= RC[1].Length; ++i)
			{
				printf("%02x ", RC[1].Storage[i]);
			}
			printf("\r\n");
			for (UINT i=1; i<RC[1].Length; ++i)
				if ( RC[1].Storage[ i ] == '>' && RC[1].Storage[ i+1 ] == '\0')
				{
					memcpy ( pDst, RC[1].Storage+1, i-1 );
					RC[1].Length = RC[1].Length - (i+2);
					BYTE* swap = new BYTE [SIADU];
					memcpy ( swap, RC[1].Storage+i+2, RC[1].Length );
					memcpy ( RC[1].Storage, swap, RC[1].Length );
					RC[1].Cursor =  & RC[1].Storage[ RC[1].Length ];
					delete [] swap;
					return 1;
				}
		}
		if ( maxlength == -2 && RC[1].Length >= 4 )
		{
			memcpy ( pDst, RC[1].Storage, 4 );
			RC[1].Length = RC[1].Length - 4;
			BYTE* swap = new BYTE [SIADU];
			memcpy ( swap, RC[1].Storage+4, RC[1].Length );
			memcpy ( RC[1].Storage, swap, RC[1].Length );
			RC[1].Cursor = & RC[1].Storage[ RC[1].Length ];
			delete [] swap;
			return 2;
		}
		if ( ((UINT)maxlength >= Harves && RC[1].Length >= Harves) || ((UINT)maxlength < Harves && maxlength >0) )
		{
			UINT ret = RC[1].Length;
			memcpy ( pDst, RC[1].Storage, RC[1].Length );
			RC[1].Cursor = RC[1].Storage;
			RC[1].Length = 0;
			return ret;
		}
		return 0;		
	}

	return 0;
}

//接收方检验收到包是否在待接收窗口内
//检验Seq与LastRSEQ是否符合，即Seq-LastRSEQ=0~7,Seq为当前帧序号，LastRSEQ为上次处理的帧序号
bool CStegSuit::Between (UINT Seq, UINT LastRSEQ)
{
	bool Hit = false;
	UINT Cnt = 0;
	while ( !Hit )
	{
		if ( Seq == (LastRSEQ+1)%16 ) Hit = true;
		LastRSEQ ++;
		Cnt ++;
		if ( Cnt == 8 ) break;
	}
	return Hit;
}

//发送方检验对方是否收到这个包
//
bool CStegSuit::Inside(UINT Seq, UINT LastRANN)
{
	UINT Cnt = 0;
	bool Hit = false;
	while ( !Hit )
	{
		if ( Seq == (LastRANN+16-1)%16 )
		{
			Hit = true;
		}
		LastRANN = LastRANN + 16 -1;
		Cnt ++;
		if ( Cnt == 8 ) break;
	}
	return Hit;
}

//RTP包，RTP包头长度，语音
UINT CStegSuit::Embedding( void * pCarrier,UINT RTPheadlen, char* pPcmIn, UINT channel_pt)
{
	m_channel_pt_send = channel_pt;
	int datatype = 0;	//数据类型
	char* pPcm = pPcmIn;
	//ssw ilbc
	//Control( 1 );	//设定SAE和STM大小
	//SSW: need to be modified for 20ms ilbc
	Control( this->m_Seclev );

	Retransmission(); //重传检测
	if(STMSdata(&datatype) == 1)	//==1 表示重传
	{
-		SAESdata(pCarrier, RTPheadlen, pPcm);	//嵌入数据
 		if(m_ActualByte == m_Resend.Length - 3 )	//重传成功，要求重传的字节数
 		{
			delete [] m_Retrans.front().Frame;
			m_Retrans.front().Frame = NULL;
			m_Retrans.pop();	//嵌入成功，丢弃
			STMSheader(datatype);	//组装STM包头
			SAESheader(pCarrier);	//嵌入STM包头

			memset(m_Resend.Frame, 0, maxSTM);
			m_Resend.Length = 0;
			m_Resend.Time = 0;	
			return 2;
		}
	}

	//重嵌入失败或正常发送
	memset(m_Resend.Frame, 0 ,maxSTM);
	m_Resend.Length = 0;
	m_Resend.Time = 0;

	SAESdata(pCarrier, RTPheadlen, pPcm);	//嵌入数据
	STMSheader(datatype);	//组装STM包头,并修改SIA缓存
	SAESheader(pCarrier);	//嵌入STM包头

	m_retranstep = 0; //开启重传

	return 3;

}


UINT CStegSuit::Retransmission()
{
	//维护重传状态
	if ( m_Threshold > HThreshold ) 
	{
		m_Threshold = HThreshold; 
	}
	for (UINT i=0; i<8; i++)
		if (m_Window[i].Length != 0) m_Window[i].Time ++;  // Add time

	if ( m_Window[ (m_LastRANN -1 ) & 0x7 ].Length != 0 )
		m_Threshold = LThreshold;	//网络通畅

	for ( UINT i=0; i<8; i++)
	{
		if ( m_Window[i].Length !=0 && Inside( m_Window[i].Frame[2]>>4, m_LastRANN ) )	//对方已处理，滑动窗口
		{
			memset( m_Window[i].Frame, 0, maxSTM);
			m_Window[i].Length = 0;
			m_Window[i].Time = 0;
		}
	}
	UINT delay = 0, pos = 0;
	for (int i = 0; i < 8; i++) // retransmit
	{
		if (m_Window[i].Time > delay)
		{
			pos = i;  
			delay = m_Window[i].Time;		//求最大延迟的包
		}
	}
	if (m_Window[pos].Time > m_Threshold)		//重传
	{

		m_Threshold += LThreshold;		//加大时间门限

		if(m_Retrans.empty() == false)
		{
			//重传队列清空
			delete [] m_Retrans.front().Frame;
			m_Retrans.front().Frame = NULL;
			m_Retrans.pop();	
		}
		//重传,压入队列
		STMFrame ReSTM;
		ReSTM.Frame = new BYTE [STMDU];
		memcpy( ReSTM.Frame, m_Window[pos].Frame, STMDU );
		ReSTM.Length = m_Window[pos].Length ;
		m_Retrans.push(ReSTM);			//压入队列
		return 1;
	}
	return 2;
}

//第一步
//返回：0：全局超时； 1：重传；2：发送窗口满； 3：正常 
UINT CStegSuit::STMSdata(int *datatype)		//向SIA申请数据
{
	//m_Crt为传给SAE层的STM包,初始化
	memset( m_Crt.Frame, 0, maxSTM);
	m_Crt.Length = 0;	//长度置零

	if(m_retranstep == 0 && m_Retrans.empty() == false)	//有重传
	{
		m_retranstep = 1;
		memcpy( m_Resend.Frame, m_Retrans.front().Frame, STMDU );	//读一个重传包
		m_Resend.Length = m_Retrans.front().Length ;
		memcpy( m_Crt.Frame, m_Resend.Frame, m_Resend.Length);	//将重传包形成STM帧
		m_Crt.Length = m_Resend.Length;
		return 1;
	}
	else if( m_Window[ (m_SEQ+1)%8 ].Length != 0 )	//发送窗口满禁止发送
	{
		return 2;  // Window full
	}
	else
	{
		//从SD拉一个最长为STMDU长的数据
		for (int i = 0; i < 2; ++i)
		{
			if (SD[i].Length != 0)
			{
				UINT len;	//帧载荷长度
				
				if ( SD[i].Length >= STMDU - 3 ) 
				{
					len = STMDU - 3;	//最大传输长度
				}
				else  
				{
					len = SD[i].Length;
				}
		
				memcpy(& m_Crt.Frame[3], SD[i].Cursor, len );
				m_Crt.Length = len + 3;
				*datatype = i;	//返回数据类型,0为消息，1为文件
				break;
			}
		}	
		return 3;
	}
}

//第二步，SAE层嵌入数据
UINT CStegSuit::SAESdata( void * pCarrier,UINT RTPheadlen, char* pPcmIn)
{
 	memcpy( m_FrmS, m_Crt.Frame, STMDU);	//取STMDU长度（3+1byte）
	m_FrmSLength = m_Crt.Length;
	m_FrmSCursor = m_FrmS;
	m_ActualByte = 0;	//真实传输字节数

	if ( m_FrmSLength > 0 )	//待发送的STM帧数据非空
	{	
		memcpy(m_chEmdSecMsg, m_Crt.Frame + 3, m_Crt.Length - 3);
		int bitpos[3] = { 0,6,4 };
		int hdTxt_pos[3] = { 0,9,19 };
		for (int i = 0; i < 1; ++i)
		{
			Enc_Inst.ste.bitpos = bitpos[i];
			Enc_Inst.ste.hdTxt_pos = hdTxt_pos[i];
			Encode((unsigned char *)(m_pFrmBuf + 38 * i), (pPcmIn + 320 * i),
				1, m_Crt.Frame + 3);
		}
		m_ActualByte = m_FrmSLength - 3;
		m_FrmSLength = 0;
		
	}
	else
	{
		//THZ: 长度调整为一帧的长度
		for (int i = 0; i < 1; ++i)
		{
			Encode((unsigned char *)(m_pFrmBuf + 38 * i), (pPcmIn + 320 * i),
				0, NULL);
		}
		m_ActualByte = 0;
	}
	switch (m_channel_pt_send)
	{
	case PJMEDIA_RTP_PT_ILBC:
		memcpy((char*)pCarrier + RTPheadlen, m_pFrmBuf, 38);
		break;
	case PJMEDIA_RTP_PT_PCMA:
	case PJMEDIA_RTP_PT_PCMU:
		memcpy((char*)pCarrier + RTPheadlen, m_pFrmBuf, 160);
		break;
	default:
		memcpy((char*)pCarrier + RTPheadlen, m_pFrmBuf, 38);
		break;
	}
	return 1;
}

//第三步，STM组装包头域 修改SIA缓存
UINT CStegSuit::STMSheader(int datatype)
{
	UINT len = m_ActualByte;	//实际嵌入字节数
	int i = datatype;			//数据来源
	//STM数据包头部
	if(m_Resend.Length > 0)	//重传包
	{
		//重写一次
		memcpy( m_Crt.Frame, m_Resend.Frame, m_Resend.Length);	//将重传包形成STM帧
		m_Crt.Length = m_Resend.Length;
		m_Crt.Frame[2] = (m_Crt.Frame[2] & 0xF0) +((m_LastRSEQ+1)%16 );	//重传仅修改一个地方		
	}
	else	//新包
	{
		UINT odd = 0;
		
		//奇偶校验
		for ( UINT k = 0; k < len; k++ )
		{
			odd = odd + m_CheckTable[ *( SD[i].Cursor+k ) ];	
		}
		odd = odd%2;

		if(len > 0)		//机密信息包
		{
			m_SEQ = (m_SEQ+1) % 16;		//分段标号，数据太长可能会分片发送
			m_Crt.Frame[0] = 0x40 + 8*(i+1) + (len & 0x7) ;
			m_Crt.Frame[1] = ( len >> 3 ) + ( odd << 7 );
			m_Crt.Frame[2] = ( m_SEQ << 4 ) + ( (m_LastRSEQ+1)%16 );
			m_Crt.Length = len + 3;		//对SAE层来说的数据大小，包括STM头
	
			SD[i].Length -= len;		//滑动窗口
			SD[i].Cursor += len;		//
//TODO:Bob 加入发送滑动窗口，暂时关闭	
//			memcpy ( m_Window[ m_SEQ & 0x7 ].Frame, m_Crt.Frame, STMDU );	//加入发送滑动窗口
//			m_Window[ m_SEQ & 0x7 ].Length = m_Crt.Length;					//加入发送滑动窗口
			//TRACE(_T("Send: %d\n"), m_SEQ);

			if (SD[i].Length == 0)				//隐秘信息应用层数据发送完毕
			{
				//应用层数据发送完毕，TODO：对接口层发送提示
				//AfxGetMainWnd()->PostMessage(WM_SIACLEAR, (UINT)(i + 1), 0);
				if (i == 1)
				{
					this->bFileSent = true;
				}
			}
		}
		else	//空包，无机密信息
		{
			UINT seq = ( m_LastRANN + 16 - 1)%16;		//分段标号为对方接收窗口外
			m_Crt.Frame[0] = 0x40 + 8*(i+1) + (len & 0x7) ;
			m_Crt.Frame[1] = ( len >> 3 ) + ( odd << 7 );
			m_Crt.Frame[2] = ( seq << 4 ) + ( (m_LastRSEQ+1)%16 );
			m_Crt.Length = len + 3;		//对SAE层来说的数据大小
		}

	}	
	return 1;
}



//第四步，嵌入STM头域
UINT CStegSuit::SAESheader(void * pCarrier)
{
	memcpy( m_FrmS, m_Crt.Frame, 3);	//取STMDU头域
	m_FrmSLength = 3;
	m_FrmSCursor = m_FrmS;

	m_pRTP->PreparePosBook();

	m_pRTP->Embed( m_FrmSCursor, 3, NULL, 0, (BYTE *) pCarrier);		//STM头域3字节填入RTP

	return 1;
}

//提取机密信息
UINT CStegSuit::Retriving(void *hdr, void * pCarrier, int pCarrierLength, char* pPcmOut, UINT channel_pt)
{
	m_channel_pt_receive = channel_pt;
  	if(SAER(hdr, pCarrier, pCarrierLength, pPcmOut))		//提取 组成STM帧
	{
		STMR();
	}
	return 1;
}

//SAE层提取
UINT CStegSuit::SAER(void *hdr, void * pCarrier, int pCarrierLength, char* pPcmOut)
{
	//rtppacket

	BYTE *DstPacket = new BYTE [12];
	memcpy(DstPacket, hdr, 12);

	BYTE *DstData = new BYTE[pCarrierLength+1];
	memcpy(DstData, pCarrier, pCarrierLength);

	m_pRTP->PreparePosBook();
	m_pRTP->Extract( m_FrmRCursor, 3, NULL, 0, DstPacket );	//从RTP中获取STM头域

	UINT len = ( m_FrmRCursor[0] & 0x7 ) + ( ( m_FrmRCursor[1] & 0x7F ) * 8 ) ;

	if( len > 0 )
	{
		//长度不正确，不是机密信息的包，丢弃
		if( len > SAEDU )
		{
			delete [] DstPacket;
			delete [] DstData;
			return 0;		
		}
		int bitpos[3] = { 0,6,4 };
		int hdTxt_pos[3] = { 0,9,19 };
		for (int i = 0; i < 1; ++i)
		{
			Enc_Inst.ste.bitpos = bitpos[i];
			Enc_Inst.ste.hdTxt_pos = hdTxt_pos[i];
			Decode((pPcmOut + 320 * i), (unsigned char *)(DstData + 38 * i), pCarrierLength,
				1, 1, m_chRtrSecMsg);
		}
		PJ_LOG(4, (THIS_FILE, "SAER:msg=%s!", m_chRtrSecMsg));
		memcpy(m_FrmRCursor + 3, (BYTE*)m_chRtrSecMsg, SAEDU);
	}
	else
	{
		for (int i = 0; i < 1; ++i)
		{
			Decode((pPcmOut + 320 * i), (unsigned char *)(DstData + 38 * i), pCarrierLength, 1, 0, NULL);
		}

	}
	delete [] DstPacket;
	delete [] DstData;
	return 1;
}

UINT PrintMessage(CStegSuit* m_pSteg)
{
	//分配内存保存信息
	char *Msg = new char[m_pSteg->SIADU];
	memset(Msg, 0, sizeof(char)* m_pSteg->SIADU);
	//TCHAR * Msg = new TCHAR[m_pSteg->SIADU];
	//memset(Msg, 0, sizeof(TCHAR)* m_pSteg->SIADU);

	UINT type = 1;
	if (type == 1)
	{
		//类型为消息，将内容通过m_Steg的Receive函数，获取到Msg中
		m_pSteg->lock();
		UINT length = m_pSteg->Receive((void *)Msg, m_pSteg->SIADU, 1);
		m_pSteg->unlock();

		//显示消息
		if (length != 0)
		{
			std::cout << "收到隐蔽消息" << std::endl;
			std::cout << "-----------------------------" << std::endl;
			std::cout << "对方: " << Msg << std::endl;
		}
		delete[] Msg;
		return 1;
	}

	delete[] Msg;
	return 0;
	//return 1;
}
UINT CStegSuit::STMR()
{
	memcpy(m_Rcv.Frame, m_FrmR, STMDU);		//获取机密信息	m_FrmRCursor == m_FrmR;
	memset( m_FrmR, 0, maxSTM+maxSAE );

	//处理包头域
	UINT Syn = m_Rcv.Frame[0] & 0xE0;
	UINT Seq = ( m_Rcv.Frame[2] >> 4 ) & 0xF; // cache window sequence
	UINT len = ( m_Rcv.Frame[0] & 0x7 ) + ( ( m_Rcv.Frame[1] & 0x7F ) * 8 ) ;
	UINT odd = 0;

	if( (Syn & 0xE0) == 0x40 )		//防止接收到非机密信息包
	{
		if(len == 0)	//空包的作用是确认
			m_LastRANN = m_Rcv.Frame[2] & 0xF;		//为发送端起始序号
		else
		{
			PJ_LOG(4, (THIS_FILE, "frame[0] = %d, frame[1] = %d, len = %d!", m_Rcv.Frame[0], m_Rcv.Frame[1], len));
			for ( UINT i = 0; i < len; i++ )
			{
				odd = odd + m_CheckTable[ m_Rcv.Frame[3+i] ];
			}
			if ( (odd%2) != ( ( m_Rcv.Frame[1]>>7 ) & 0x1 ) ) return 0;	//奇偶校验


			//Seq与LastRSEQ符合就处理接收信息,Seq为当前帧序号，LastRSEQ为上次处理的帧序号
			if ( Between( Seq, m_LastRSEQ) )
			{
				//获取机密信息至接收滑动窗口，这里并不考虑顺序
				memcpy ( m_Cache[ Seq & 0x7 ].Frame, m_Rcv.Frame, STMDU );
				m_Cache[ Seq & 0x7 ].Length = 1; // denote there is a valid frame
				m_LastRANN = m_Rcv.Frame[2] & 0xF;		//为发送端起始序号
			}	//其他为重发的包,确认号没有时效性
			
		}		
	}

	//按序提交数据
	bool NewRequst1 = false, NewRequst2 = false; 
	while ( m_Cache[ (m_LastRSEQ+1) & 0x7 ].Length != 0 )		//保证顺序
	{
		memcpy ( m_Rcv.Frame, m_Cache[ (m_LastRSEQ+1) & 0x7 ].Frame, STMDU );	//按序处理一个数据
		UINT Seq = ( m_Rcv.Frame[2] >> 4 ) & 0xF;
		UINT len = ( m_Rcv.Frame[0] & 0x7 ) + ( ( m_Rcv.Frame[1] & 0x7F ) * 8 );
		UINT type = ( m_Rcv.Frame[0] >> 3 ) & 0x3;

		m_Cache[ (m_LastRSEQ+1) & 0x7 ].Length = 0;		//滑动窗口向前移动

		m_LastRSEQ = Seq;		//LastRSEQ表示己方已经处理的序号

		if ( len != 0 )	//按序提交
		{
			if ( type == 1 ) NewRequst1 = true;
			if ( type == 2 ) NewRequst2 = true;
			//TRACE(_T("		Receive : %d, taking Ann = %d \n"), Seq, m_LastRANN );

			memcpy( RC[type-1].Cursor, & m_Rcv.Frame[3], len );
			RC[type-1].Cursor += len;
			RC[type-1].Length += len;
		}
	}
	//一边提交一边就可以显示
	//发送消息提示UI界面做出显示
	if (NewRequst1)
	{
		//新开一个线程，调用PrintMessage，或者直接把PrintMessage的函数内容拷过去

		this->bMessageArrived = true;
		
		//HANDLE handle = CreateThread(NULL, 0, foo, this, 0, NULL);
		//WaitForSingleObject(handle, INFINITE);  
		//std::thread t1(PrintMessage, this);
		//t1.detach();
		//PrintMessage(1);
	}
	if (NewRequst2) this->bFileArrived = true;
	//if (NewRequst2)
	//	PrintMessage(2);
	//if (NewRequst1) AfxGetMainWnd() -> PostMessage( WM_SIARRIVE, 1, 0 );
	//if (NewRequst2) AfxGetMainWnd() -> PostMessage( WM_SIARRIVE, 2, 0 );
	return 1;
}

void CStegSuit::Encode(unsigned char *encoded_data, void *block, short bHide, void *hdTxt)
{
	int byte_length = 80;
	//pcmu
	pj_int16_t *samples = (pj_int16_t *)block;
	pj_uint8_t *dst = (pj_uint8_t *)encoded_data;
	switch (m_channel_pt_send)
	{
	case PJMEDIA_RTP_PT_ILBC:
		iLBCEncode(encoded_data, (float *)block, &Enc_Inst, bHide, (char *)hdTxt);
		break;
	case PJMEDIA_RTP_PT_PCMA:
	case PJMEDIA_RTP_PT_PCMU:
		for (size_t i = 0; i < byte_length/2; ++i, ++dst)
		{
				*dst = pjmedia_linear2ulaw(samples[i]);
		}
		if (bHide != 0)
		{
			int length = sizeof(hdTxt);
			for (size_t i = 0; i < length; ++i, dst++)
			{
				*dst = ((char *)hdTxt)[i];
			}
			PJ_LOG(4, (THIS_FILE, "Encode: src=%d, dst=%d, length=%d", (pj_int16_t *)block, *encoded_data, length));
		}
		break;
	default:
		break;
	}

	
	

}

void CStegSuit::Decode(void *decblock, unsigned char *bytes, int bytes_length, int mode, short bHide, char *msg)
{
	pj_uint8_t *src = (pj_uint8_t*)bytes;
	pj_uint16_t *dst;
	switch (m_channel_pt_receive)
	{
	case PJMEDIA_RTP_PT_ILBC:
		//ilbc
		iLBCDecode((float *)decblock, bytes, &Dec_Inst, mode, bHide, msg);
		break;
	case PJMEDIA_RTP_PT_PCMA:
	case PJMEDIA_RTP_PT_PCMU:
		//pcmu
		//if (msg == NULL)
		//	{
		dst = (pj_uint16_t *)decblock;
		//	}
		//	else
		//	{
		//		dst = (pj_uint16_t *)msg;
		//	}
		for (size_t i = 0; i < bytes_length; ++i, ++dst)
		{
			*dst = (pj_uint16_t)pjmedia_ulaw2linear(src[i]);  //pcmu
		}
		if (bHide != 0)
		{
			char * buffer = msg;
			size_t length = 0;
			for (size_t i = 0; ((char *)bytes)[i] != '\0'; ++i, buffer++)
			{
				*buffer = ((char *)bytes)[i];
				++length;
			}
			PJ_LOG(4, (THIS_FILE, "Decode:decoded block = %d, src byte = %d, msg=%s, length=%d!", (pj_uint16_t *)decblock, *bytes, msg, length));
		}
		break;
	default:
		break;
	}
	
}