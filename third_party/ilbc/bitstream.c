#include <stdio.h>
#include "iLBC_define.h"
/*
�ڴ��ļ��ж��� ���� �� ������� ����
ʹ���ڽ������ػ����������Ϣʱ����ͨ�����ô˺����򻯲���
����һ������ͨ���˺���ʵ�ֱ�����Ϣ�Ĺ���(���� ����� ���ܵ�)
*/
void getbit(STE *ste)
{
	if( ste->bitpos > 7)
	{
		ste->bitpos=0;
		ste->hdTxt_pos++;
	}
	ste->curbit= ((ste->hdTxt[ste->hdTxt_pos] << ste->bitpos) >>7)&0x1 ;
	ste->bitpos++;


	/*
	if( ste->bitpos < 8 )
	{
		ste->curbit= ((ste->buf << ste->bitpos) >>7)&0x1 ;
		ste->bitpos++;
	}
	else
	{
		ste->hdTxt_pos++;
		ste->buf = ste->hdTxt[ste->hdTxt_pos];
		
		if( ste->hdTxt_pos<=100 )
		{
			ste->bitpos=0;
			ste->curbit= ((ste->buf << ste->bitpos) >>7)&0x1 ;
			ste->bitpos++;
		}
		else
		{
			ste->hdTxt_pos=0;			
			ste->buf = ste->hdTxt[ste->hdTxt_pos];
			ste->bitpos=0;
			ste->curbit= ((ste->buf << ste->bitpos) >>7)&0x1 ;
			ste->bitpos++;
		}
	}
	*/

}

void putbit(STE *ste)
{
	/*
	if( ste->bitpos <8)
	{
		ste->buf = ste->buf | ste->curbit<<(7-ste->bitpos);
		ste->bitpos++;
	}
	else
	{
		ste->hdTxt[ste->hdTxt_pos]=ste->buf ;
		ste->hdTxt_pos++;

		ste->bitpos=0;
		ste->buf=0x0;
		ste->buf = ste->buf | ste->curbit<<(7-ste->bitpos);
		ste->bitpos++;
	}
	*/


	
		  /*
   	char *hdTxt;		//Ƕ����Ϣ�����ļ�  ��ȡ��Ϣ���洢�����ļ�
	int hdTxt_pos;			//��ǰ��/д ����hdTxtλ��
	char buf;		//�ݴ� ������Ϣ�� �ֽ�
	int bitpos;				//��ǰ �ֽڵĴ���λ�� 0~7 ,Ӧ��ʼ��Ϊ 8 ���㴦��
	char curbit;	//��ǰ���ܱ��� 0 1
	*/
	unsigned char all_1;
	all_1=255;
	
	if( ste->bitpos > 7)
	{
		ste->bitpos=0;
		ste->hdTxt_pos++;
	}
	all_1=all_1-1<<(7-ste->bitpos);
	ste->hdTxt[ste->hdTxt_pos] = ((ste->hdTxt[ste->hdTxt_pos])&all_1) | ste->curbit<<(7-ste->bitpos);
	ste->bitpos++;
	

}