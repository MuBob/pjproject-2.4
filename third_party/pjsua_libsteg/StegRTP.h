#include "StegLSB.h"
class CStegRTP : public CStegLSB
{
	//DECLARE_SERIAL(CStegRTP)

public:
	void InitPosBook();
	void PreparePosBook();
	UINT GetParam(UINT sel);
};