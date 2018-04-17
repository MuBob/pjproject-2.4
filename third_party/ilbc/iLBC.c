/******************************************************************
*  编解码主函数。完成编解码的总体过程，同时计算语音的长度，编解码
*  所用的时间。
******************************************************************/

#include "iLBC_define.h"
#include "iLBC_encode.h"
#include "iLBC_decode.h"
#include "iLBC.h"


/*定义 字 数是字节数的一半，说明一个字16位*/
//#define ILBCNOOFWORDS_MAX   (NO_OF_BYTES_30MS/2)
/*----------------------------------------------------------------*
*  Encoder interface function
*---------------------------------------------------------------*/

short iLBCEncode(   /* (o) Number of bytes encoded */
	unsigned char *encoded_data,    /* (o) The encoded bytes 编码后数据*/
	float *block,                 /* (i) The signal block to encode 要编码的数据*/
   iLBC_Enc_Inst_t *Enc_Inst,
   short bHide,
   char *hdTxt
){
   //float block[BLOCKL_MAX];
   int k;

   
   for(k=0;k<6;k++)
   {
	   if(bHide==1)
			Enc_Inst->ste.ChoiceSTE[k]=1;
	   else
		    Enc_Inst->ste.ChoiceSTE[k]=0;
   }
   /*
   Enc_Inst->ste.ChoiceSTE[3]=0;
   Enc_Inst->ste.ChoiceSTE[4]=0;
   Enc_Inst->ste.ChoiceSTE[5]=0;
   */
   Enc_Inst->ste.hdTxt=hdTxt;		//嵌入信息所在文件  提取信息将存储到的文件
   //Enc_Inst->ste.hdTxt_pos=0;			//当前读/写 到的hdTxt位置
   //Enc_Inst->ste.bitpos=0;
   Enc_Inst->ste.buf=0;
   Enc_Inst->ste.curbit=0;

   /* convert signal to float */

   //for (k=0; k<Enc_Inst->blockl; k++)
	  // block[k] = (float)data[k];

   /* do the actual encoding */

   iLBC_encode((unsigned char *)encoded_data, block, Enc_Inst, &Enc_Inst->ste);

   return (Enc_Inst->no_of_bytes);
}

/*----------------------------------------------------------------*
*  Decoder interface function
*---------------------------------------------------------------*/

short iLBCDecode(float *decblock,
				 unsigned char *encoded_data,
				 iLBC_Dec_Inst_t *Dec_Inst,
				 int mode,
				 short bHide,
				 char *hdTxt				 
				 )
{
   int k;
   //float decblock[BLOCKL_MAX], dtmp;  //解码后数据，用于数据交换的temp
   float dtmp;
   
   int good = mode;
   for(k=0;k<6;k++)
   {
	   if(bHide==1)
		   Dec_Inst->ste.ChoiceSTE[k]=1;
	   else
		   Dec_Inst->ste.ChoiceSTE[k]=0;
   }
   /*
   Dec_Inst->ste.ChoiceSTE[3]=0;
   Dec_Inst->ste.ChoiceSTE[4]=0;
   Dec_Inst->ste.ChoiceSTE[5]=0;
   */
   //Dec_Inst->ste.bitpos=0;
   Dec_Inst->ste.buf=0;
   Dec_Inst->ste.curbit=0;

   Dec_Inst->ste.hdTxt=hdTxt;
   //Dec_Inst->ste.hdTxt_pos=0; //?


   /* do actual decoding of block */
   
   iLBC_decode(decblock, (unsigned char *)encoded_data,Dec_Inst, good,&Dec_Inst->ste);

   /* convert to short */
   //for (k=0; k<Dec_Inst->blockl; k++){
	  // dtmp=decblock[k];
	  // if (dtmp<MIN_SAMPLE)
		 //  dtmp=MIN_SAMPLE;
	  // else if (dtmp>MAX_SAMPLE)
		 //  dtmp=MAX_SAMPLE;
	  // decoded_data[k] = (short) dtmp;
   //}
   return (Dec_Inst->blockl);
}
