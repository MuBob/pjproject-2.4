//#include "stdafx.h"
#include "StegRTP.h"

//IMPLEMENT_SERIAL( CStegRTP, CStegLSB, 1 )


void CStegRTP::InitPosBook()
{
	m_len = 96;
	m_prepared = false;
	m_PB = new PBItem [m_len];

	////////////////////////////////////////////////////////////////////////////embed msg in time stamp
	UINT embedmark = 0;						
	UINT timestamplen = 24;		//只嵌入3字节的STM头域
	UINT myint = 40;			//嵌入位置，以bit计,从第二个字节开始
	for ( UINT i =embedmark ; i < embedmark+timestamplen; i ++ )
	{
		m_PB[i].byte = myint/8;
		m_PB[i].bit = 7-myint%8;		//最高字节为7
		myint++;
	}					//time域前4字节
	embedmark += timestamplen; 

	/////////////////////////////////////////////////////////////////////////////embed msg in mark
	m_PB[embedmark].byte = 1;
	m_PB[embedmark].bit = 7;
	embedmark ++;
	
	////////////////////////////////////////////////////////////////////////////embed msg in payload type
	UINT ptlen = 7;
	myint = 9;
	for ( UINT i=embedmark; i < embedmark+ptlen; i++)
	{
		m_PB[i].byte =myint/8;
		m_PB[i].bit =7-myint%8;
		myint++;
	}
	embedmark += ptlen;

	////////////////////////////////////////////////////////////////////////////embed msg in SSRC
	UINT ssrclen = 32;
	myint = 64;
	for ( UINT i =embedmark ; i < embedmark+ssrclen; i ++ )
	{
		m_PB[i].byte = myint/8;
		m_PB[i].bit = 7-myint%8;		
		myint++;
	}
	embedmark += ssrclen; 


	/////////////////////////////////////////////////////////////////////////////embed msg in Sequence number
	UINT sqnlen = 16;
	myint = 16;
	for ( UINT i =embedmark ; i < embedmark+sqnlen; i ++ )
	{
		m_PB[i].byte = myint/8;
		m_PB[i].bit = 7-myint%8;		
		myint++;
	}
	embedmark += sqnlen; 
	//TRACE(_T("StegRTP Inited. \n"));
}



void CStegRTP::PreparePosBook()
{
	m_prepared = true; 
}

UINT CStegRTP::GetParam(UINT sel)
{
	switch (sel)
	{
	case 0:
		return 0;
	case 1:
		return 3;	//3个STM头域
	case 2:
		return 4;
	default:
		return 2;
	}
	/*
	switch (sel)
	{
	case 0:
		return 0;
	case 1:
		return 2;
	case 2:
		return 4;
	default:
		return 2;
	}
	*/
}