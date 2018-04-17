#include <string.h>

#include "iLBC_define.h"
#include "helpfun.h"
#include "lsf.h"
#include "constants.h"
/*--------------------------------------------------------------*
*  LPC编码函数。主要功能为LPC分析，计算LSF系数以及LSF系数量化。
*--------------------------------------------------------------*/

/*----------------------------------------------------------------*
*  lpc analysis (subrutine to LPCencode)
*  
*---------------------------------------------------------------*/

void SimpleAnalysis(
   float *lsf,         /* (o) lsf coefficients */
   float *data,    /* (i) new data vector */
   iLBC_Enc_Inst_t *iLBCenc_inst  /* (i/o) the encoder state structure */
){
   int k, is;
   float temp[BLOCKL_MAX], lp[LPC_FILTERORDER + 1];
   float lp2[LPC_FILTERORDER + 1];
   float r[LPC_FILTERORDER + 1]; //自相关系数

   /* 确定 data放在 lpc_buffer的位置，20ms:后160点，30ms：后240点 */
   is=LPC_LOOKBACK+BLOCKL_MAX-iLBCenc_inst->blockl;
   memcpy(iLBCenc_inst->lpc_buffer+is,data,
	   iLBCenc_inst->blockl*sizeof(float));

   /* No lookahead, last window is asymmetric */

   for (k = 0; k < iLBCenc_inst->lpc_n; k++) {

	   is = LPC_LOOKBACK;

	   /*加第一个窗，如果是20ms 对称窗，30ms 非对称窗*/
	   if (k < (iLBCenc_inst->lpc_n - 1)) {
		   window(temp, lpc_winTbl,iLBCenc_inst->lpc_buffer, BLOCKL_MAX);
	   } else {
		   window(temp, lpc_asymwinTbl,iLBCenc_inst->lpc_buffer + is, BLOCKL_MAX);
	   }
	   /*计算自相关，结果为 r */
	   autocorr(r, temp, BLOCKL_MAX, LPC_FILTERORDER);
	   window(r, r, lpc_lagwinTbl, LPC_FILTERORDER + 1);
	   /*通过 LD算法 计算LPC系数，结果为 lp 带宽扩展后为lp2
		*levdurb函数的参数分别为：lpc系数，反射系数，自相关系数，阶数
		*bwexpand函数的参数分别为： 扩展结果，输入lpc系数，扩展参数，阶数*/
	   levdurb(lp, temp, r, LPC_FILTERORDER);
	   bwexpand(lp2, lp, LPC_CHIRP_SYNTDENUM, LPC_FILTERORDER+1);
	   /* LPC 系数转换为 LSF系数 */
	   a2lsf(lsf + k*LPC_FILTERORDER, lp2);
   }
   /* 将lpc_buffer中的旧数据向前移动 */
   is=LPC_LOOKBACK+BLOCKL_MAX-iLBCenc_inst->blockl;
   memmove(iLBCenc_inst->lpc_buffer,iLBCenc_inst->lpc_buffer+LPC_LOOKBACK+BLOCKL_MAX-is,is*sizeof(float));
}

/*----------------------------------------------------------------*
*  lsf interpolator and conversion from lsf to a coefficients
*  (subrutine to SimpleInterpolateLSF)
*  内插两个LSF系数，得到一个LPC系数
*---------------------------------------------------------------*/

void LSFinterpolate2a_enc(
   float *a,       /* (o) lpc coefficients */
   float *lsf1,/* (i) first set of lsf coefficients */
   float *lsf2,/* (i) second set of lsf coefficients */
   float coef,     /* (i) weighting coefficient to use between
						  lsf1 and lsf2 */
   long length      /* (i) length of coefficient vectors */
){
   float  lsftmp[LPC_FILTERORDER];

   interpolate(lsftmp, lsf1, lsf2, coef, length);
   lsf2a(a, lsftmp);
}

/*----------------------------------------------------------------*
*  lsf interpolator (subrutine to LPCencode)
*---------------------------------------------------------------*/

void SimpleInterpolateLSF(
   float *syntdenum,   /* (o) the synthesis filter denominator
							  resulting from the quantized
							  interpolated lsf */
   float *weightdenum, /* (o) the weighting filter denominator
							  resulting from the unquantized
							  interpolated lsf */
   float *lsf,         /* (i) the unquantized lsf coefficients */
   float *lsfdeq,      /* (i) the dequantized lsf coefficients */
   float *lsfold,      /* (i) the unquantized lsf coefficients of
							  the previous signal frame */
   float *lsfdeqold, /* (i) the dequantized lsf coefficients of
							  the previous signal frame */
   int length,         /* (i) should equate LPC_FILTERORDER */
   iLBC_Enc_Inst_t *iLBCenc_inst
					   /* (i/o) the encoder state structure */
){
   int    i, pos, lp_length;
   float  lp[LPC_FILTERORDER + 1], *lsf2, *lsfdeq2;

   lsf2 = lsf + length;
   lsfdeq2 = lsfdeq + length;
   lp_length = length + 1;

   if (iLBCenc_inst->mode==30) {
	   /* sub-frame 1: Interpolation between old and first
		  set of lsf coefficients */

	   /*内插 当前的LSF系数和原有的LSF系数（解量化后的），得到一组LPC系数：lp 
	   * 参数为： lp,旧LSF系数，新LSF系数，内插权重表，数组长度*/
	   LSFinterpolate2a_enc(lp, lsfdeqold, lsfdeq,lsf_weightTbl_30ms[0], length);
	   /* 复制结果 */
	   memcpy(syntdenum,lp,lp_length*sizeof(float));

	   /*内插 当前的LSF系数和原有的LSF系数（未量化的），得到一组LPC系数：lp 
	   * 参数为： lp,旧LSF系数，新LSF系数，内插权重表，数组长度*/
	   LSFinterpolate2a_enc(lp, lsfold, lsf,lsf_weightTbl_30ms[0], length);
	   /*带宽扩展*/
	   bwexpand(weightdenum, lp, LPC_CHIRP_WEIGHTDENUM, lp_length);

	   /* sub-frame 2 to 6: Interpolation between first
		  and second set of lsf coefficients */

	   pos = lp_length;
	   for (i = 1; i < iLBCenc_inst->nsub; i++) {
		   LSFinterpolate2a_enc(lp, lsfdeq, lsfdeq2,lsf_weightTbl_30ms[i], length);
		   memcpy(syntdenum + pos,lp,lp_length*sizeof(float));

		   LSFinterpolate2a_enc(lp, lsf, lsf2,lsf_weightTbl_30ms[i], length);
		   bwexpand(weightdenum + pos, lp,LPC_CHIRP_WEIGHTDENUM, lp_length);
		   pos += lp_length;
	   }
   }
   else {
	   pos = 0;
	   for (i = 0; i < iLBCenc_inst->nsub; i++) {
		   LSFinterpolate2a_enc(lp, lsfdeqold, lsfdeq,lsf_weightTbl_20ms[i], length);
		   memcpy(syntdenum+pos,lp,lp_length*sizeof(float));
		   LSFinterpolate2a_enc(lp, lsfold, lsf,lsf_weightTbl_20ms[i], length);
		   bwexpand(weightdenum+pos, lp,LPC_CHIRP_WEIGHTDENUM, lp_length);
		   pos += lp_length;
	   }
   }

   /* update memory */

   if (iLBCenc_inst->mode==30) {
	   memcpy(lsfold, lsf2, length*sizeof(float));
	   memcpy(lsfdeqold, lsfdeq2, length*sizeof(float));
   }
   else {
	   memcpy(lsfold, lsf, length*sizeof(float));
	   memcpy(lsfdeqold, lsfdeq, length*sizeof(float));
   }
}

/*----------------------------------------------------------------*
*  lsf quantizer (subrutine to LPCencode)
*---------------------------------------------------------------*/

void SimplelsfQ(
   float *lsfdeq,    /* (o) dequantized lsf coefficients
						  (dimension FILTERORDER) */
   int *index,     /* (o) quantization index */
   float *lsf,      /* (i) the lsf coefficient vector to be
						  quantized (dimension FILTERORDER ) */
   int lpc_n     /* (i) number of lsf sets to quantize */
){
   /* Quantize first LSF with memoryless split VQ 
   *  参数依次为： 量化后的LSF系数，量化索引，要量化的LSF系数，
   *               量化码本，量化分段数，每段长度，码本大小   */
   SplitVQ(lsfdeq, index, lsf, lsfCbTbl, LSF_NSPLIT,
	   dim_lsfCbTbl, size_lsfCbTbl);

   /*  如果是30ms格式要进行第二次LSF系数的量化 */
   if (lpc_n==2) {
	   /* Quantize second LSF with memoryless split VQ */
	   SplitVQ(lsfdeq + LPC_FILTERORDER, index + LSF_NSPLIT,
		   lsf + LPC_FILTERORDER, lsfCbTbl, LSF_NSPLIT,
		   dim_lsfCbTbl, size_lsfCbTbl);
   }
}

/*----------------------------------------------------------------*
*  lpc encoder
*---------------------------------------------------------------*/

void LPCencode(
   float *syntdenum, /* (i/o) synthesis filter coefficients before/after encoding */
   float *weightdenum, /* (i/o) weighting denumerator coefficients before/after encoding */
   int *lsf_index,     /* (o) lsf quantization index */
   float *data,    /* (i) lsf coefficients to quantize */
   iLBC_Enc_Inst_t *iLBCenc_inst /* (i/o) the encoder state structure */
){
   float lsf[LPC_FILTERORDER * LPC_N_MAX];
   float lsfdeq[LPC_FILTERORDER * LPC_N_MAX];
   int change=0;
   
   /* 计算LSF系数
   *  三个参数为： 计算好的LSF系数 输入的数据 编码实例 */
   SimpleAnalysis(lsf, data, iLBCenc_inst); 
   /* LSF系数的量化
   *  参数分别为： 量化后的LSF系数，量化索引，要量化的LSF系数，量化几次*/
   SimplelsfQ(lsfdeq, lsf_index, lsf, iLBCenc_inst->lpc_n);
   /* 检测LSF系数的稳定性 change=1说明调整过*/
   change = LSF_check(lsfdeq, LPC_FILTERORDER, iLBCenc_inst->lpc_n);
   /* 内插得到 LCP系数和 带宽扩展的LPC系数，分别为第一个和第二个参数
   *  第三个参数开始为： 未量化的LSF系数，解量化的LSF系数，
   *  之前的LSF系数，之前解量化的LSF系数，阶数，编码实例 */
   SimpleInterpolateLSF(syntdenum, weightdenum,
	   lsf, lsfdeq, iLBCenc_inst->lsfold,
	   iLBCenc_inst->lsfdeqold, LPC_FILTERORDER, iLBCenc_inst);
}