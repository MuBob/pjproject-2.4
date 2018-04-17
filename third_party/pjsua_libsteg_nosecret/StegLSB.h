#pragma once
//#include "stdafx.h"
//#ifndef __AFXWIN_H__
//#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
//#endif

#define BYTE unsigned char

#define UINT unsigned int

//#define bool bool
#define MAX_PATH          260

struct PBItem
{
	unsigned int byte;     // 可供替换的位置是载体第几字节，从0开始
	unsigned int bit;      // 是这一字节的第几个比特，最低位为0
	unsigned int rank;     // 用于排序置乱
};

class CStegLSB
{
	//DECLARE_SERIAL(CStegLSB)

public:
	~CStegLSB();  // 释放位表

	virtual void InitPosBook();           // 构造位表
	virtual void PreparePosBook();        // 准备位表，如置乱
	virtual unsigned int GetParam(unsigned int sel);    // 获取隐藏算法的隐藏容量

	bool Embed(unsigned char* Dt, unsigned int Dtlen, bool * Sn, unsigned int Snlen, unsigned char * Carrier);   // 嵌入
	bool Extract(unsigned char* Dt, unsigned int Dtlen, bool * Sn, unsigned int Snlen, unsigned char * Carrier); // 提取

protected:
	PBItem * m_PB;  // 位表指针
	unsigned int m_len;     // 位表长度
	bool m_prepared;   // 位表就绪标志
};