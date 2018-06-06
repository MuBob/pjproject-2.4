#include "StegLSB.h"

#pragma once

//#ifndef __AFXWIN_H__
//	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
//#endif

class CStegRTP : public CStegLSB
{
	//DECLARE_SERIAL(CStegRTP)

public:
	void InitPosBook();
	void PreparePosBook();
	UINT GetParam(UINT sel);
};