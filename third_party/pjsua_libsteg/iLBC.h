#include "iLBC_define.h"
#ifndef ilbc__H
#define ilbc__H


short iLBCEncode(   /* (o) Number of bytes encoded */
	unsigned char *encoded_data,    /* (o) The encoded bytes 编码后数据*/
	float *block,                 /* (i) The signal block to encode 要编码的数据*/
	iLBC_Enc_Inst_t *Enc_Inst,
	short bHide,
	char *hdTxt
	);
short iLBCDecode(    /* (o) Number of decoded samples */
	float *decblock,  /* (o) Decoded signal block*/
	unsigned char *encoded_data,  /* (i) Encoded bytes */
	iLBC_Dec_Inst_t *Dec_Inst,
	int mode,
	short bHide,
	char *hdTxt
	);

#endif