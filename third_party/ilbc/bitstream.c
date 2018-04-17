#include <stdio.h>
#include "iLBC_define.h"
/*
在此文件中定义 比特 流 读入读出 函数
使得在进行隐藏或解码隐藏信息时可以通过调用此函数简化操作
更进一步可以通过此函数实现比特信息的管理(例如 随机化 加密等)
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
   	char *hdTxt;		//嵌入信息所在文件  提取信息将存储到的文件
	int hdTxt_pos;			//当前读/写 到的hdTxt位置
	char buf;		//暂存 隐藏信息的 字节
	int bitpos;				//当前 字节的处理位置 0~7 ,应初始化为 8 方便处理
	char curbit;	//当前秘密比特 0 1
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