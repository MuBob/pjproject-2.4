#include "StegLSB.h"

#pragma once

//#ifndef __AFXWIN_H__
//	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
//#endif

class CStegRTP : public CStegLSB
{
	//DECLARE_SERIAL(CStegRTP)

public:
	void InitPosBook();
	void PreparePosBook();
	UINT GetParam(UINT sel);
};