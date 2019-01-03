//#include "stdafx.h"
#include "StegLSB.h"
#include <string.h>
#include <stdio.h>

//IMPLEMENT_SERIAL(CStegLSB, CObject, 1)

CStegLSB::~CStegLSB()
{
	if ( m_PB != NULL )  delete [] m_PB;
}

void CStegLSB::InitPosBook() {}
void CStegLSB::PreparePosBook() {}
unsigned int CStegLSB::GetParam( unsigned int sel ) {return 0;}

//Dtlen为数据字节数，Snlen为标志位数
bool CStegLSB::Embed(unsigned char* Dt, unsigned int Dtlen, bool * Sn, unsigned int Snlen, unsigned char * Carrier)
{
	if ( ! m_prepared || ( Dtlen==0 && Snlen==0 ) ) return false;
	if ( Dtlen*8 + Snlen > m_len ) return false;
	unsigned int Start = 40;
	unsigned int Count = Start;
	//嵌入指示标记
	if ( Sn != NULL && Snlen > 0 )
	{
		for ( unsigned int i = 0; i < Snlen; i++ )
		{
			unsigned char t = 1 << m_PB[ Count ].bit;
			if ( Sn[i] == false )
			{	
				t = 0xFF - t;
				Carrier[ m_PB[ Count ].byte ] &= t;
			}
			else
			{
				Carrier[m_PB[Count].byte] |= t;
			}
			Count ++;
		}
	}
	//嵌入数据,Dtlen为数据字节数
	if ( Dt != NULL && Dtlen >0 )
	{
		for ( unsigned int i = 0; i < Dtlen; i ++ )
		{
			//TRACE(_T("%X : "), Dt[i]);
			for ( unsigned int k = 0; k < 8; k ++ )
			{
				bool s = ( Dt[i] >> (7-k) ) & 1;
				unsigned char t = 1 << m_PB[ Count ].bit;
				if (s == false)
				{	
					t = 0xFF - t;
					Carrier[m_PB[Count].byte] &= t;
				}
				else
				{
					Carrier[m_PB[Count].byte] |= t;
				}
				//TRACE(_T("%X "), Carrier[ m_PB[ Count ].unsigned char ]);
				Count ++;
			}
			//TRACE(_T("\n"));
		}
	}
	m_prepared = false;
	return true;
}

bool CStegLSB::Extract(unsigned char* Dt, unsigned int Dtlen, bool * Sn, unsigned int Snlen, unsigned char * Carrier)
{
	if ( ! m_prepared || ( Dtlen == 0 && Snlen==0 ) ) return false;
	if ( Dtlen*8 + Snlen > m_len ) return false;
	memset ( Dt, 0, Dtlen );
	//提取标记
	unsigned int Start = 40;
	unsigned int Count = Start;
	if ( Snlen != 0 )
	{
		for ( unsigned int i = 0; i < Snlen; i++ )
		{
			Sn[i] = (Carrier[m_PB[Count].byte] >> m_PB[Count].bit) & 1;
			Count ++;
		}
	}
	//提取数据
	if ( Dt != NULL )
	{
		for ( unsigned int i = 0; i < Dtlen; i ++ )
		{
			for ( unsigned int k = 0; k < 8; k ++ )
			{
				bool s = (Carrier[m_PB[Count].byte] >> m_PB[Count].bit) & 1;
				Dt[i] += s*(1<<(7-k));
				//TRACE(_T("%X "), Carrier[ m_PB[ Count ].unsigned char ]);
				Count ++;
			}
			//TRACE(_T(" : %X \n"),Dt[i]);
		}
	}
	m_prepared = false;
	return true;
}