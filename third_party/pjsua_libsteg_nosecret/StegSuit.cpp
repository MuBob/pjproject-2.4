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


//ssw
//#include "StegTalk.h"

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#endif

iLBC_Enc_Inst_t Enc_Inst;
iLBC_Dec_Inst_t Dec_Inst;
UINT mode20_30;

//extern 

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

/*
static int PJ_THREAD_FUNC foo(void* pM)
{


	CStegSuit *m_pSteg = (CStegSuit *)pM;
	while (m_pSteg->quit_flag == 0){
		//�����ڴ汣����Ϣ
		if (m_pSteg->steg_status == NEW_MESSAGE_ARRIVED){
			char *Msg = new char[m_pSteg->SIADU];
			memset(Msg, 0, sizeof(char)* m_pSteg->SIADU);
			//TCHAR * Msg = new TCHAR[m_pSteg->SIADU];
			//memset(Msg, 0, sizeof(TCHAR)* m_pSteg->SIADU);

			//test
			//printf("when OnSIArrive called ,type = %d\n",type);
			//test
			UINT type = 1;
			if (type == 1)
			{
				//����Ϊ��Ϣ��������ͨ��m_Steg��Receive��������ȡ��Msg��
				m_pSteg->lock();
				m_pSteg->length = m_pSteg->Receive((void *)Msg, m_pSteg->SIADU, 1);
				m_pSteg->unlock();

				//��ʾ��Ϣ
				if (m_pSteg->length != 0)
				{
					
					m_pSteg->msg_1 = pj_str(Msg);
					m_pSteg->lock();
					m_pSteg->steg_status = NEW_MESSAGE_OVER;
					m_pSteg->unlock();
					pj_str_t from = pj_str("ttt");
					pj_str_t to = pj_str("bbb");
					pj_str_t mime = pj_str("Text/Plain");
					std::cout << "�յ�������Ϣ" << std::endl;
					//std::cout << "-----------------------------" << std::endl;
					
					//on_pager_wrapper1(&from, &to,&mime, &msg_1);
					

					//std::cout << "�Է�: " << Msg << std::endl;
				}
				//delete[] Msg;
				
				//return 1;
			}
			
			delete[] Msg;
		
		
		}
		else
			pj_thread_sleep(100);
	}
	return 0;

}
*/
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
	m_Resend.Frame =NULL; m_Resend.Frame=0; m_Resend.Time=0; //�ش�
	m_retranstep = 0;	//�ش�����

	m_FrmSLength = 0; m_RTPSeq = 0;
	m_FrmS = NULL;  m_FrmR = NULL;
	m_FrmSCursor = NULL;  m_FrmRCursor = NULL;

	//CRuntimeClass * pRun = RUNTIME_CLASS(CStegRTP);
	m_pRTP = (CStegLSB *) (new CStegRTP());

	CarrierType = 97;
	m_nSegment = 1;
	LThreshold = 40;
	HThreshold = 1000;

	//INILock.Lock();
	//CarrierType = GetPrivateProfileInt(_T("Core"), _T("PayloadType"), 0, iniPath);
	//FRAME = GetPrivateProfileInt( _T("Core"), _T("Frame"), 90, iniPath);
	//m_nSegment = GetPrivateProfileInt(_T("Steg"), _T("Segment"), 6, iniPath);
	//LThreshold = GetPrivateProfileInt(_T("Steg"), _T("Lowthreshold"), 40, iniPath);
	//HThreshold = GetPrivateProfileInt(_T("Steg"), _T("Highthreshold"), 1000, iniPath);
	//INILock.Unlock();

	m_Threshold = LThreshold;

	MakeCheckTable();
	return;
	//pj_status_t status;

	//status = pj_thread_create(pool, "pjsua", &foo,
	//	this, 0, 0, &thread);
	//if (status != PJ_SUCCESS)
	//	return ;

}

void CStegSuit::Allocate()
{
	//SSW: need to be modified for iLBC
	//ssw iLBC
	//SSW: need to be modified for 20ms ilbc
	//maxSAE = 1 + m_pRTP->GetParam(2);
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
	m_Resend.Frame = new BYTE[maxSTM];		//�ش�

	m_FrmS = new BYTE[maxSTM]; 
	m_FrmSCursor = m_FrmS;		//����ָ��
	m_FrmR = new BYTE[maxSTM+maxSAE];  
	m_FrmRCursor = m_FrmR;		//����ָ��
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

	if (m_Resend.Frame!=NULL) delete [] m_Resend.Frame; m_Resend.Frame = NULL;	//�ش�
	if (thread != NULL) // air
	{
		pj_thread_join(thread);
		pj_thread_destroy(thread);
	}

	thread = NULL;
	if (SLock)
	{
		//pj_lock_release(SLock);
		//pj_lock_destroy(SLock);
		SLock = NULL;
	}

	delete m_pRTP;
}

void CStegSuit::Control( UINT Command )
{
	
	//SSW: need to be modified for 20ms ilbc
	//SAEDU = 1;	//һ��RTP��������֡��Ƕ��һ���ֽ�
	if( Command == 0 )
		SAEDU=iLBC_SAEDU_20;
	else if( Command == 2 )
		SAEDU = iLBC_SAEDU_30;
	else
	SAEDU=iLBC_SAEDU_20;

	STMDU = SAEDU * m_nSegment + 3;			//.ini������m_nSegment = 1, STM��ͷ3�ֽ� (24bit)
}

//0��255�������ڶ����Ʊ�ʾ�£�ÿλ��ӵ���ż��;����������żУ��
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

//��������Ϣ���ļ�������������ϢӦ�ò㻺����(SD)
UINT CStegSuit::Send(void * pSrc, int length, int type)
{
	if ( ( (UINT) length > SIADU ) || (type != 1 && type != 2) || pSrc == NULL) return 0;
	if ( type == 1) 
	{
		memset( SD[0].Storage, 0, SIADU );
		memcpy(SD[0].Storage, pSrc, length);
//		strcpy_s((char *)SD[0].Storage, SIADU, (char *)pSrc);
		//_tcscpy_s((TCHAR *)SD[0].Storage, SIADU, (TCHAR *)pSrc);
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
//����
UINT CStegSuit::Receive(void * pDst, int maxlength, int type)
{
	if ( type == 1 )
	{
		for ( UINT i=0; i<SIADU-1; i += 1 )
		{
			if (RC[0].Storage[i] == 0)
			{
				//_tcscpy_s((TCHAR *)pDst, maxlength, (TCHAR *)RC[0].Storage);
				strcpy_s((char *)pDst, maxlength, (char *)RC[0].Storage);
				memset((void *)RC[0].Storage, 1, SIADU);
				RC[0].Cursor = RC[0].Storage;
				RC[0].Length = 0;
				return i;
			}

			//if ( RC[0].Storage[i] == 0 && RC[0].Storage[i+1] == 0 && (i+1)/2 <= (UINT) maxlength) 
			//{
			//	_tcscpy_s((TCHAR *)pDst, maxlength, (TCHAR *)RC[0].Storage);
			//	memset ((void *)RC[0].Storage, 1, SIADU);
			//	RC[0].Cursor = RC[0].Storage;
			//	RC[0].Length = 0;
			//	return (i+1)/2;
			//}
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

//���շ������յ����Ƿ��ڴ����մ�����
//����Seq��LastRSEQ�Ƿ���ϣ���Seq-LastRSEQ=0~7,SeqΪ��ǰ֡��ţ�LastRSEQΪ�ϴδ����֡���
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

//���ͷ�����Է��Ƿ��յ������
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

//RTP����RTP��ͷ���ȣ�����
UINT CStegSuit::Embedding( void * pCarrier,UINT RTPheadlen, char* pPcmIn )
{
	int datatype = 0;	//��������
	char* pPcm = pPcmIn;
	//ssw ilbc
	//Control( 1 );	//�趨SAE��STM��С
	//SSW: need to be modified for 20ms ilbc
	Control( this->m_Seclev );

	Retransmission(); //�ش����
	if(STMSdata(&datatype) == 1)	//==1 ��ʾ�ش�
	{
		SAESdata(pCarrier, RTPheadlen, pPcm);	//Ƕ������
 		if(m_ActualByte == m_Resend.Length - 3 )	//�ش��ɹ���Ҫ���ش����ֽ���
 		{
			delete [] m_Retrans.front().Frame;
			m_Retrans.front().Frame = NULL;
			m_Retrans.pop();	//Ƕ��ɹ�������
			STMSheader(datatype);	//��װSTM��ͷ
			SAESheader(pCarrier);	//Ƕ��STM��ͷ

			memset(m_Resend.Frame, 0, maxSTM);
			m_Resend.Length = 0;
			m_Resend.Time = 0;	
			return 2;
		}
	}

	//��Ƕ��ʧ�ܻ���������
	memset(m_Resend.Frame, 0 ,maxSTM);
	m_Resend.Length = 0;
	m_Resend.Time = 0;

	SAESdata(pCarrier, RTPheadlen, pPcm);	//Ƕ������
	STMSheader(datatype);	//��װSTM��ͷ,���޸�SIA����
	SAESheader(pCarrier);	//Ƕ��STM��ͷ

	m_retranstep = 0; //�����ش�

	return 3;

}


UINT CStegSuit::Retransmission()
{
	//ά���ش�״̬
	if ( m_Threshold > HThreshold ) 
	{
		m_Threshold = HThreshold; 
		//TRACE("m_Threshold > HThreshold");
		//return 0; // Global failure
	}
	for (UINT i=0; i<8; i++)
		if (m_Window[i].Length != 0) m_Window[i].Time ++;  // Add time

	if ( m_Window[ (m_LastRANN -1 ) & 0x7 ].Length != 0 )
		m_Threshold = LThreshold;	//����ͨ��

	for ( UINT i=0; i<8; i++)
	{
		if ( m_Window[i].Length !=0 && Inside( m_Window[i].Frame[2]>>4, m_LastRANN ) )	//�Է��Ѵ�����������
		{
			memset( m_Window[i].Frame, 0, maxSTM);
			m_Window[i].Length = 0;
			m_Window[i].Time = 0;
		}
		//printf("winTime: %04d-", m_Window[i].Time);
	}
	//printf("\n");

	UINT delay = 0, pos = 0;
	for (int i = 0; i < 8; i++) // retransmit
	{
		if (m_Window[i].Time > delay)
		{
			pos = i;  
			delay = m_Window[i].Time;		//������ӳٵİ�
		}
	}
	if (m_Window[pos].Time > m_Threshold)		//�ش�
	{

		m_Threshold += LThreshold;		//�Ӵ�ʱ������

		if(m_Retrans.empty() == false)
		{
			//�ش��������
			delete [] m_Retrans.front().Frame;
			m_Retrans.front().Frame = NULL;
			m_Retrans.pop();	
		}
		//�ش�,ѹ�����
		STMFrame ReSTM;
		ReSTM.Frame = new BYTE [STMDU];
		memcpy( ReSTM.Frame, m_Window[pos].Frame, STMDU );
		ReSTM.Length = m_Window[pos].Length ;
		m_Retrans.push(ReSTM);			//ѹ�����
		return 1;
	}
	return 2;
}

//��һ��
//���أ�0��ȫ�ֳ�ʱ�� 1���ش���2�����ʹ������� 3������ 
UINT CStegSuit::STMSdata(int *datatype)		//��SIA��������
{
	//m_CrtΪ����SAE���STM��,��ʼ��
	memset( m_Crt.Frame, 0, maxSTM);
	m_Crt.Length = 0;	//��������

	if(m_retranstep == 0 && m_Retrans.empty() == false)	//���ش�
	{
		m_retranstep = 1;
		memcpy( m_Resend.Frame, m_Retrans.front().Frame, STMDU );	//��һ���ش���
		m_Resend.Length = m_Retrans.front().Length ;
		memcpy( m_Crt.Frame, m_Resend.Frame, m_Resend.Length);	//���ش����γ�STM֡
		m_Crt.Length = m_Resend.Length;
		return 1;
	}
	else if( m_Window[ (m_SEQ+1)%8 ].Length != 0 )	//���ʹ�������ֹ����
	{
		//TRACE(_T("	Window full\n"));
		return 2;  // Window full
	}
	else
	{
		//��SD��һ���ΪSTMDU��������
		for (int i = 0; i < 2; ++i)
		{
			if (SD[i].Length != 0)
			{
				UINT len;	//֡�غɳ���
				
				if ( SD[i].Length >= STMDU - 3 ) 
				{
					len = STMDU - 3;	//����䳤��
				}
				else  
				{
					len = SD[i].Length;
				}
		
				memcpy(& m_Crt.Frame[3], SD[i].Cursor, len );
				m_Crt.Length = len + 3;
				*datatype = i;	//������������,0Ϊ��Ϣ��1Ϊ�ļ�
				break;
			}
		}	
		return 3;
	}
}

//�ڶ�����SAE��Ƕ������
UINT CStegSuit::SAESdata( void * pCarrier,UINT RTPheadlen, char* pPcmIn)
{
 	memcpy( m_FrmS, m_Crt.Frame, STMDU);	//ȡSTMDU���ȣ�3+1byte��
	m_FrmSLength = m_Crt.Length;
	m_FrmSCursor = m_FrmS;
	m_ActualByte = 0;	//��ʵ�����ֽ���

	if ( m_FrmSLength > 0 )	//�����͵�STM֡���ݷǿ�
	{	
		memcpy(m_chEmdSecMsg, m_Crt.Frame + 3, m_Crt.Length - 3);
		//memcpy(m_chEmdSecMsg, m_FrmSCursor + 3, 1);
		//changed for 20ms


		//THZ: ���ȵ���Ϊһ֡�ĳ���
		int bitpos[3]={0,6,4};
		int hdTxt_pos[3]={0,9,19};
		if(mode20_30==30)
		{
			bitpos[0]=0;
			bitpos[1]=3;
			bitpos[2]=6;
			hdTxt_pos[0]=0;
			hdTxt_pos[1]=11;
			hdTxt_pos[2]=22;

		}
		//THZ: ���ȵ���Ϊһ֡�ĳ���
		for (int i = 0; i < 1; ++i)
		{
			//iLBC  ssw
			//SSW: need to be modified for 20ms ilbc
			//changed for 20ms
			Enc_Inst.ste.bitpos=bitpos[i];
			Enc_Inst.ste.hdTxt_pos=hdTxt_pos[i];
			if(mode20_30==20)
				iLBCEncode((unsigned char *)(m_pFrmBuf + 38 * i), 
				(float *)(pPcmIn + 320 * i), &Enc_Inst, 1, m_chEmdSecMsg);
			else
				iLBCEncode((unsigned char *)(m_pFrmBuf + 50 * i), (float *)(pPcmIn + 480 * i), &Enc_Inst, 1, m_chEmdSecMsg);
		}
		//m_chEmdSecMsg ǰ 34 B Ϊ��������
		//m_ActualByte = 1    /*m_FrmSLength - 3;
		m_ActualByte = m_FrmSLength - 3;
		m_FrmSLength = 0;
		
	}
	else
	{
		//THZ: ���ȵ���Ϊһ֡�ĳ���
		for (int i = 0; i < 1; ++i)
		{
			//iLBC ssw
			//SSW: need to be modified for 20ms ilbc
			//changed for 20ms
			if(mode20_30==20)
				iLBCEncode((unsigned char *)(m_pFrmBuf + 38 * i), 
				(float *)(pPcmIn + 320 * i), &Enc_Inst, 0, NULL);
			else
				iLBCEncode((unsigned char *)(m_pFrmBuf + 50 * i), (float *)(pPcmIn + 480 * i), &Enc_Inst, 0, NULL);

		}
		m_ActualByte = 0;
	}
	//ssw iLBC
	//SSW: need to be modified for 20ms ilbc
	//changed for 20ms
	//THZ: ���ȵ���Ϊһ֡�ĳ���
	if(mode20_30==20)
		memcpy((char*)pCarrier + RTPheadlen, m_pFrmBuf, 38);
	else
		memcpy((char*)pCarrier + RTPheadlen, m_pFrmBuf, 50);
	//if (mode20_30 == 20)
	//	memcpy((char*)pCarrier + RTPheadlen, m_pFrmBuf, 38 * 3);
	//else
	//	memcpy((char*)pCarrier + RTPheadlen, m_pFrmBuf, 50 * 3);
	
	return 1;
}

//��������STM��װ��ͷ�� �޸�SIA����
UINT CStegSuit::STMSheader(int datatype)
{
	UINT len = m_ActualByte;	//ʵ��Ƕ���ֽ���
	int i = datatype;			//������Դ
	//STM���ݰ�ͷ��
	if(m_Resend.Length > 0)	//�ش���
	{
		//��дһ��
		memcpy( m_Crt.Frame, m_Resend.Frame, m_Resend.Length);	//���ش����γ�STM֡
		m_Crt.Length = m_Resend.Length;
		m_Crt.Frame[2] = (m_Crt.Frame[2] & 0xF0) +((m_LastRSEQ+1)%16 );	//�ش����޸�һ���ط�		
	}
	else	//�°�
	{
		UINT odd = 0;
		
		//��żУ��
		for ( UINT k = 0; k < len; k++ )
		{
			odd = odd + m_CheckTable[ *( SD[i].Cursor+k ) ];	
		}
		odd = odd%2;

		if(len > 0)		//������Ϣ��
		{
			m_SEQ = (m_SEQ+1) % 16;		//�ֶα�ţ�����̫�����ܻ��Ƭ����
			m_Crt.Frame[0] = 0x40 + 8*(i+1) + (len & 0x7) ;
			m_Crt.Frame[1] = ( len >> 3 ) + ( odd << 7 );
			m_Crt.Frame[2] = ( m_SEQ << 4 ) + ( (m_LastRSEQ+1)%16 );
			m_Crt.Length = len + 3;		//��SAE����˵�����ݴ�С������STMͷ
	
			SD[i].Length -= len;		//��������
			SD[i].Cursor += len;		//
	
			memcpy ( m_Window[ m_SEQ & 0x7 ].Frame, m_Crt.Frame, STMDU );	//���뷢�ͻ�������
			m_Window[ m_SEQ & 0x7 ].Length = m_Crt.Length;					//���뷢�ͻ�������
			//TRACE(_T("Send: %d\n"), m_SEQ);

			if (SD[i].Length == 0)				//������ϢӦ�ò����ݷ������
			{
				//Ӧ�ò����ݷ�����ϣ�TODO���Խӿڲ㷢����ʾ
				//AfxGetMainWnd()->PostMessage(WM_SIACLEAR, (UINT)(i + 1), 0);
				if (i == 1)
				{
					this->bFileSent = true;
				}
			}
		}
		else	//�հ����޻�����Ϣ
		{
			UINT seq = ( m_LastRANN + 16 - 1)%16;		//�ֶα��Ϊ�Է����մ�����
			m_Crt.Frame[0] = 0x40 + 8*(i+1) + (len & 0x7) ;
			m_Crt.Frame[1] = ( len >> 3 ) + ( odd << 7 );
			m_Crt.Frame[2] = ( seq << 4 ) + ( (m_LastRSEQ+1)%16 );
			m_Crt.Length = len + 3;		//��SAE����˵�����ݴ�С
		}

	}	
	return 1;
}



//���Ĳ���Ƕ��STMͷ��
UINT CStegSuit::SAESheader(void * pCarrier)
{
	memcpy( m_FrmS, m_Crt.Frame, 3);	//ȡSTMDUͷ��
	m_FrmSLength = 3;
	m_FrmSCursor = m_FrmS;

	m_pRTP->PreparePosBook();

	m_pRTP->Embed( m_FrmSCursor, 3, NULL, 0, (BYTE *) pCarrier);		//STMͷ��3�ֽ�����RTP

	return 1;
}

//��ȡ������Ϣ
UINT CStegSuit::Retriving(void *hdr, void * pCarrier, char* pPcmOut)
{
  	if(SAER(hdr, pCarrier, pPcmOut))		//��ȡ ���STM֡
	{
		STMR();
	}
	return 1;
}

//SAE����ȡ
UINT CStegSuit::SAER(void *hdr, void * pCarrier, char* pPcmOut)
{
	//rtppacket
	//pjmedia_rtp_hdr packet;
	//RTPPacket *Pack = (RTPPacket *) pCarrier;
	BYTE *DstPacket = new BYTE [12];
	//ssw iLBC
	//SSW: need to be modified for 20ms ilbc
	//THZ: ���ȵ���Ϊһ֡�ĳ���
	BYTE *DstData = new BYTE [50];
	//BYTE *DstData = new BYTE[50 * 3];
	//BYTE *DstData = new BYTE [24*3];

	memcpy(DstPacket, hdr, 12);
	//ssw iLBC
	//SSW: need to be modified for 20ms ilbc
	//changed for 20ms
	//THZ: ���ȵ���Ϊһ֡�ĳ���
	if(mode20_30==20)
		memcpy(DstData, pCarrier, 38);
	else
		memcpy(DstData, pCarrier, 50);
	//if (mode20_30 == 20)
	//	memcpy(DstData, Pack->GetPayloadData(), 38 * 3);
	//else
	//	memcpy(DstData, Pack->GetPayloadData(), 50 * 3);

	m_pRTP->PreparePosBook();
	m_pRTP->Extract( m_FrmRCursor, 3, NULL, 0, DstPacket );	//��RTP�л�ȡSTMͷ��

	UINT len = ( m_FrmRCursor[0] & 0x7 ) + ( ( m_FrmRCursor[1] & 0x7F ) * 8 ) ;
	/*printf("len = %d\n",len);*/

	if( len > 0 )
	{
		//changed for 20ms
		//if(len > iLBC_SAEDU_30)	//���Ȳ���ȷ�����ǻ�����Ϣ�İ�������
		if( (len > iLBC_SAEDU_20 && mode20_30==20) ||(len > iLBC_SAEDU_30 && mode20_30==30) )
		{
			delete [] DstPacket;
			delete [] DstData;
			return 0;		
		}

		//iLBC ssw
		//SSW: need to be modified for 20ms ilbc
		//changed for 20ms
		//THZ: ���ȵ���Ϊһ֡�ĳ���
		int bitpos[3]={0,6,4};
		int hdTxt_pos[3]={0,9,19};
		if(mode20_30==30)
		{
			bitpos[0]=0;
			bitpos[1]=3;
			bitpos[2]=6;
			hdTxt_pos[0]=0;
			hdTxt_pos[1]=11;
			hdTxt_pos[2]=22;

		}
		//int bitpos[3]={0,3,6};
		//int hdTxt_pos[3]={0,11,22};
		//THZ: ���ȵ���Ϊһ֡�ĳ���
		for (int i = 0; i < 1; ++i)
		{
			//iLBC  ssw
			Dec_Inst.ste.bitpos=bitpos[i];
			Dec_Inst.ste.hdTxt_pos=hdTxt_pos[i];
			//changed for 20ms
			if(mode20_30==20)
				iLBCDecode((float *)(pPcmOut + 320 * i), (unsigned char *)(DstData + 38 * i), &Dec_Inst,1, 1, m_chRtrSecMsg );
			else
				iLBCDecode((float *)(pPcmOut + 480 * i), (unsigned char *)(DstData + 50 * i), &Dec_Inst, 1, 1, m_chRtrSecMsg);
		}
		//changed for 20ms

		if(mode20_30==20)
			memcpy(m_FrmRCursor + 3, (BYTE*)m_chRtrSecMsg, iLBC_SAEDU_20);
		else
			memcpy(m_FrmRCursor + 3, (BYTE*)m_chRtrSecMsg, iLBC_SAEDU_30);
	}
	else
	{
		//THZ: ���ȵ���Ϊһ֡�ĳ���
		for (int i = 0; i < 1; ++i)
		{
			//iLBC ssw 
			//SSW: need to be modified for 20ms ilbc
			//changed for 20ms
			if (mode20_30 == 20)
				iLBCDecode((float *)(pPcmOut + 320 * i), (unsigned char *)(DstData + 38 * i), &Dec_Inst, 1, 0, NULL);
			else
				iLBCDecode((float *)(pPcmOut + 480 * i), (unsigned char *)(DstData + 50 * i), &Dec_Inst, 1, 0, NULL);
		}

	}
	delete [] DstPacket;
	delete [] DstData;
	return 1;
}

UINT PrintMessage(CStegSuit* m_pSteg)
{
	//�����ڴ汣����Ϣ
	char *Msg = new char[m_pSteg->SIADU];
	memset(Msg, 0, sizeof(char)* m_pSteg->SIADU);
	//TCHAR * Msg = new TCHAR[m_pSteg->SIADU];
	//memset(Msg, 0, sizeof(TCHAR)* m_pSteg->SIADU);

	//test
	//printf("when OnSIArrive called ,type = %d\n",type);
	//test
	UINT type = 1;
	if (type == 1)
	{
		//����Ϊ��Ϣ��������ͨ��m_Steg��Receive��������ȡ��Msg��
		m_pSteg->lock();
		UINT length = m_pSteg->Receive((void *)Msg, m_pSteg->SIADU, 1);
		m_pSteg->unlock();

		//��ʾ��Ϣ
		if (length != 0)
		{
			std::cout << "�յ�������Ϣ" << std::endl;
			std::cout << "-----------------------------" << std::endl;
			std::cout << "�Է�: " << Msg << std::endl;
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
	memcpy(m_Rcv.Frame, m_FrmR, STMDU);		//��ȡ������Ϣ	m_FrmRCursor = m_FrmR;
	memset( m_FrmR, 0, maxSTM+maxSAE );

	//�����ͷ��
	UINT Syn = m_Rcv.Frame[0] & 0xE0;
	UINT Seq = ( m_Rcv.Frame[2] >> 4 ) & 0xF; // cache window sequence
	UINT len = ( m_Rcv.Frame[0] & 0x7 ) + ( ( m_Rcv.Frame[1] & 0x7F ) * 8 ) ;
	UINT odd = 0;

	if( (Syn & 0xE0) == 0x40 )		//��ֹ���յ��ǻ�����Ϣ��
	{
		if(len == 0)	//�հ���������ȷ��
			m_LastRANN = m_Rcv.Frame[2] & 0xF;		//Ϊ���Ͷ���ʼ���
		else
		{
			for ( UINT i = 0; i < len; i++ )
			{
				odd = odd + m_CheckTable[ m_Rcv.Frame[3+i] ];
			}
			if ( (odd%2) != ( ( m_Rcv.Frame[1]>>7 ) & 0x1 ) ) return 0;	//��żУ��


			//Seq��LastRSEQ���Ͼʹ��������Ϣ,SeqΪ��ǰ֡��ţ�LastRSEQΪ�ϴδ����֡���
			if ( Between( Seq, m_LastRSEQ) )
			{
				//��ȡ������Ϣ�����ջ������ڣ����ﲢ������˳��
				memcpy ( m_Cache[ Seq & 0x7 ].Frame, m_Rcv.Frame, STMDU );
				m_Cache[ Seq & 0x7 ].Length = 1; // denote there is a valid frame
				m_LastRANN = m_Rcv.Frame[2] & 0xF;		//Ϊ���Ͷ���ʼ���
			}	//����Ϊ�ط��İ�,ȷ�Ϻ�û��ʱЧ��
			
		}		
	}

	//�����ύ����
	bool NewRequst1 = false, NewRequst2 = false; 
	while ( m_Cache[ (m_LastRSEQ+1) & 0x7 ].Length != 0 )		//��֤˳��
	{
		memcpy ( m_Rcv.Frame, m_Cache[ (m_LastRSEQ+1) & 0x7 ].Frame, STMDU );	//������һ������
		UINT Seq = ( m_Rcv.Frame[2] >> 4 ) & 0xF;
		UINT len = ( m_Rcv.Frame[0] & 0x7 ) + ( ( m_Rcv.Frame[1] & 0x7F ) * 8 );
		UINT type = ( m_Rcv.Frame[0] >> 3 ) & 0x3;

		m_Cache[ (m_LastRSEQ+1) & 0x7 ].Length = 0;		//����������ǰ�ƶ�

		m_LastRSEQ = Seq;		//LastRSEQ��ʾ�����Ѿ���������

		if ( len != 0 )	//�����ύ
		{
			if ( type == 1 ) NewRequst1 = true;
			if ( type == 2 ) NewRequst2 = true;
			//TRACE(_T("		Receive : %d, taking Ann = %d \n"), Seq, m_LastRANN );

			memcpy( RC[type-1].Cursor, & m_Rcv.Frame[3], len );
			RC[type-1].Cursor += len;
			RC[type-1].Length += len;
		}
	}
	//һ���ύһ�߾Ϳ�����ʾ
	//������Ϣ��ʾUI����������ʾ
	if (NewRequst1)
	{
		//�¿�һ���̣߳�����PrintMessage������ֱ�Ӱ�PrintMessage�ĺ������ݿ���ȥ

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

