#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "iLBC_define.h"
#include "LPCencode.h"
#include "FrameClassify.h"
#include "StateSearchW.h"
#include "StateConstructW.h"
#include "helpfun.h"
#include "constants.h"
#include "packing.h"
#include "iCBSearch.h"
#include "iCBConstruct.h"
#include "hpInput.h"
#include "anaFilter.h"
#include "syntFilter.h"

#include "bitstream.h"

/*---------------------------------------------------------------*
*  编码主函数。完成编码的整个过程。
*---------------------------------------------------------------*/

/*----------------------------------------------------------------*
*  Initiation of encoder instance.
*---------------------------------------------------------------*/

short initEncode(                   /* (o) Number of bytes encoded */
   iLBC_Enc_Inst_t *iLBCenc_inst,  /* (i/o) Encoder instance */
   int mode                    /* (i) frame size mode */
){
   iLBCenc_inst->mode = mode;
   if (mode==30) {
	   iLBCenc_inst->blockl = BLOCKL_30MS;
	   iLBCenc_inst->nsub = NSUB_30MS;
	   iLBCenc_inst->nasub = NASUB_30MS;
	   iLBCenc_inst->lpc_n = LPC_N_30MS;
	   iLBCenc_inst->no_of_bytes = NO_OF_BYTES_30MS;
	   iLBCenc_inst->no_of_words = NO_OF_WORDS_30MS;
	   iLBCenc_inst->state_short_len=STATE_SHORT_LEN_30MS;
	   /* ULP init */
	   iLBCenc_inst->ULP_inst=&ULP_30msTbl;
   }
   else if (mode==20) {
	   iLBCenc_inst->blockl = BLOCKL_20MS;
	   iLBCenc_inst->nsub = NSUB_20MS;
	   iLBCenc_inst->nasub = NASUB_20MS;
	   iLBCenc_inst->lpc_n = LPC_N_20MS;
	   iLBCenc_inst->no_of_bytes = NO_OF_BYTES_20MS;
	   iLBCenc_inst->no_of_words = NO_OF_WORDS_20MS;
	   iLBCenc_inst->state_short_len=STATE_SHORT_LEN_20MS;
	   /* ULP init */
	   iLBCenc_inst->ULP_inst=&ULP_20msTbl;
   }
   else {
	   exit(2);
   }

   memset((*iLBCenc_inst).anaMem, 0,LPC_FILTERORDER*sizeof(float));
   memcpy((*iLBCenc_inst).lsfold, lsfmeanTbl,LPC_FILTERORDER*sizeof(float));
   memcpy((*iLBCenc_inst).lsfdeqold, lsfmeanTbl,LPC_FILTERORDER*sizeof(float));
   memset((*iLBCenc_inst).lpc_buffer, 0,(LPC_LOOKBACK+BLOCKL_MAX)*sizeof(float));
   memset((*iLBCenc_inst).hpimem, 0, 4*sizeof(float));

   return (iLBCenc_inst->no_of_bytes);
}

/*----------------------------------------------------------------*
*  main encoder function
*---------------------------------------------------------------*/

void iLBC_encode(
   unsigned char *bytes,           /* (o) encoded data bits iLBC */
   float *block,                   /* (o) speech vector to encode */
   iLBC_Enc_Inst_t *iLBCenc_inst,   /* (i/o) the general encoder state */
   STE *ste
){

   float data[BLOCKL_MAX];			//对初始输入高通滤波后的数据
   float residual[BLOCKL_MAX], reverseResidual[BLOCKL_MAX];

   int start, idxForMax, idxVec[STATE_LEN];  // start 开始状态，idxForMax 最大残差量化结果, idxVec 残差量化索引向量
   float reverseDecresidual[BLOCKL_MAX], mem[CB_MEML];
   int n, k, meml_gotten, Nfor, Nback, i, pos;	// Nfor Nback 开始状态前后的子帧数，
   int gain_index[CB_NSTAGES*NASUB_MAX],
	   extra_gain_index[CB_NSTAGES];					// extra_gain_index 当前量化帧的 三级增益位置
   int cb_index[CB_NSTAGES*NASUB_MAX],extra_cb_index[CB_NSTAGES];//  extra_cb_index 当前量化帧的 三级编码位置
   int lsf_i[LSF_NSPLIT*LPC_N_MAX];			//lsf量化索引
   unsigned char *pbytes;	//打包数据时用到的指针
   int diff, start_pos, state_first;		// diff 22or23 ,start_pos 开始状态位置(在语音帧中)，state_first 开始状态前后
   float en1, en2;
   int index, ulp, firstpart;
   int subcount, subframe;
   float weightState[LPC_FILTERORDER];
   float syntdenum[NSUB_MAX*(LPC_FILTERORDER+1)];	//综合滤波器系数(LPC系数)
   float weightdenum[NSUB_MAX*(LPC_FILTERORDER+1)];   // 权重系数(扩展后的LPC系数)
   float decresidual[BLOCKL_MAX];					//开始状态解码结果	

   /* high pass filtering of input signal if such is not done prior to calling this function */
   /* 参数依次为： 输入数据帧 数据长度 滤波后数据帧 滤波器初始状态*/
   hpInput(block, iLBCenc_inst->blockl,data, (*iLBCenc_inst).hpimem);

   /* otherwise simply copy */

   /*memcpy(data,block,iLBCenc_inst->blockl*sizeof(float));*/

   /* LPC of hp filtered input data 滤波后的数据 data 进行LPC分析*/
   /* 参数依次为： 综合滤波器系数(LPC系数)   权重系数(扩展后的LPC系数)   lsf量化索引   要分析的数据   编码实例*/
   LPCencode(syntdenum, weightdenum, lsf_i, data, iLBCenc_inst);


   /* inverse filter to get residual */

   for (n=0; n<iLBCenc_inst->nsub; n++) {
	   /*LPC 分析滤波器
	   *参数依次为，要滤波数据，LPC参数，信号长度，滤波结果，滤波器状态*/
	   anaFilter(&data[n*SUBL], &syntdenum[n*(LPC_FILTERORDER+1)],SUBL, &residual[n*SUBL], iLBCenc_inst->anaMem);
   }

   /* find state location 确定开始状态 start */



   /* 
   信息隐藏 ，ChoiceSTE[0] ，开始状态位置选择 Bolck Class，嵌入 1 比特 
   */
   start = FrameClassify(iLBCenc_inst, residual,ste);
   


   /*
   信息隐藏 ，ChoiceSTE[1] ，22采样段选择，嵌入 1 比特 
   */
				/* check if state should be in first or last part of the two subframes 
				*  确定前后 57/58点 */
   diff = STATE_LEN - iLBCenc_inst->state_short_len;
				/* 计算前57/58 点的能量*/
   en1 = 0;
   index = (start-1)*SUBL;
   for (i = 0; i < iLBCenc_inst->state_short_len; i++) {
	   en1 += residual[index+i]*residual[index+i];
   }
				/* 计算后57/58 点的能量*/
   en2 = 0;
   index = (start-1)*SUBL+diff;
   for (i = 0; i < iLBCenc_inst->state_short_len; i++) {
	   en2 += residual[index+i]*residual[index+i];
   }

				/*state_first = 1 前57/58点  ， =0 后    */
   if (ste->ChoiceSTE[1] == 1)
   {
	   getbit(ste);
	   if (ste->curbit == 1)
	   {
		   state_first = 1;
		   start_pos = (start-1)*SUBL;
	   }
	   else
	   {
		   state_first = 0;
		   start_pos = (start-1)*SUBL + diff;
	   }
   }
   else
   {
	   if (en1 > en2) {
		   state_first = 1;
		   start_pos = (start-1)*SUBL;
	   } else {
		   state_first = 0;
		   start_pos = (start-1)*SUBL + diff;
	   }
   }




   /* 
   信息隐藏 ，ChoiceSTE[2] ，开始状态比例因子，嵌入 1 比特 
   信息隐藏 ，ChoiceSTE[3] ，开始状态标量量化，嵌入 57/58 比特 
   */
   /* scalar quantization of state */
   /* 开始状态编码，参数依次为：
   *  编码实例，要编码的残差，综合滤波器系数，感知权重系数，
   *  最大值得量化序号，量化索引向量，初始状态长度，初始状态前后*/
   StateSearchW(iLBCenc_inst, &residual[start_pos],
	   &syntdenum[(start-1)*(LPC_FILTERORDER+1)],
	   &weightdenum[(start-1)*(LPC_FILTERORDER+1)], &idxForMax,
	   idxVec, iLBCenc_inst->state_short_len, state_first,ste);
   /* 解码开始状态，参数为：最最大值的量化序号，量化索引向量，
   *  LCP综合滤波器系数，-----解码结果-----，数据长度*/
   StateConstructW(idxForMax, idxVec,&syntdenum[(start-1)*(LPC_FILTERORDER+1)],
	   &decresidual[start_pos], iLBCenc_inst->state_short_len);


   /*
   信息隐藏 ，ChoiceSTE[4] ，动态码本量化，每一阶段可以嵌入 1 比特，总共 3* 3/5 比特
   信息隐藏 ，ChoiceSTE[5] ，动态增益量化，每一阶段可以嵌入 1 比特，总共 3* 3/5 比特
   */
				/* predictive quantization in state 
					*  开始状态剩余22/23数据的量化 */

   if (state_first) { /* put adaptive part in the end 22/23点在后面 */

	   /* setup memory */
	   /* 设置 mem 前 147-57/58点为0，后57/58点为解量化后的开始状态，
	   *  即构建好了码本缓存 */
	   memset(mem, 0,(CB_MEML-iLBCenc_inst->state_short_len)*sizeof(float));
	   memcpy(mem+CB_MEML-iLBCenc_inst->state_short_len,decresidual+start_pos,
		   iLBCenc_inst->state_short_len*sizeof(float));
	   memset(weightState, 0, LPC_FILTERORDER*sizeof(float));

	   /* encode sub-frames */
	   /* 编码函数的参数为：
	   *  编码实例，编码好的码本序列，增益序列，量化目标，码本缓存，
	   *  码本长度，量化目标长度，量化级数，权重滤波系数，权重滤波状态，字块数*/
	   iCBSearch(iLBCenc_inst, extra_cb_index, extra_gain_index,
		   &residual[start_pos+iLBCenc_inst->state_short_len],
		   mem+CB_MEML-stMemLTbl,stMemLTbl, diff, CB_NSTAGES,
		   &weightdenum[start*(LPC_FILTERORDER+1)],weightState, 0,ste);

	   /* construct decoded vector */
	   /* 编码重建函数的参数为：
	   *  解码向量，已编好的码本序号，已编码好的增益，码本构建缓存，码本长度，矢量长度，量化阶数 */
	   iCBConstruct(&decresidual[start_pos+iLBCenc_inst->state_short_len],
		   extra_cb_index, extra_gain_index,mem+CB_MEML-stMemLTbl,stMemLTbl, diff, CB_NSTAGES);

   }
   else { /* put adaptive part in the beginning 22/23点在前面 */

	   /* create reversed vectors for prediction */

	   for (k=0; k<diff; k++) {
		   reverseResidual[k] = residual[(start+1)*SUBL-1 -(k+iLBCenc_inst->state_short_len)];
	   }

	   /* setup memory */

	   meml_gotten = iLBCenc_inst->state_short_len;
	   for (k=0; k<meml_gotten; k++) {
		   mem[CB_MEML-1-k] = decresidual[start_pos + k];
	   }
	   memset(mem, 0, (CB_MEML-k)*sizeof(float));
	   memset(weightState, 0, LPC_FILTERORDER*sizeof(float));

	   /* encode sub-frames */

	   iCBSearch(iLBCenc_inst, extra_cb_index, extra_gain_index,
		   reverseResidual, mem+CB_MEML-stMemLTbl, stMemLTbl,diff, CB_NSTAGES,
		   &weightdenum[(start-1)*(LPC_FILTERORDER+1)],weightState, 0,ste);

	   /* construct decoded vector */

	   iCBConstruct(reverseDecresidual, extra_cb_index,
		   extra_gain_index, mem+CB_MEML-stMemLTbl, stMemLTbl,diff, CB_NSTAGES);

	   /* get decoded residual from reversed vector */

	   for (k=0; k<diff; k++) {
		   decresidual[start_pos-1-k] = reverseDecresidual[k];
	   }
   }

   /* counter for predicted sub-frames */
   subcount=0;

   /* forward prediction of sub-frames 
   *  编码开始状态 后面的子帧 ？？？  */

   Nfor = iLBCenc_inst->nsub-start-1;
   if ( Nfor > 0 ) {

	   /* setup memory */

	   memset(mem, 0, (CB_MEML-STATE_LEN)*sizeof(float));
	   memcpy(mem+CB_MEML-STATE_LEN, decresidual+(start-1)*SUBL,STATE_LEN*sizeof(float));
	   memset(weightState, 0, LPC_FILTERORDER*sizeof(float));

	   /* loop over sub-frames to encode */

	   for (subframe=0; subframe<Nfor; subframe++) {

		   /* encode sub-frame */

		   iCBSearch(iLBCenc_inst, cb_index+subcount*CB_NSTAGES,
			   gain_index+subcount*CB_NSTAGES,
			   &residual[(start+1+subframe)*SUBL],
			   mem+CB_MEML-memLfTbl[subcount],
			   memLfTbl[subcount], SUBL, CB_NSTAGES,
			   &weightdenum[(start+1+subframe)*(LPC_FILTERORDER+1)],
			   weightState, subcount+1,ste);

		   /* construct decoded vector */

		   iCBConstruct(&decresidual[(start+1+subframe)*SUBL],
			   cb_index+subcount*CB_NSTAGES,
			   gain_index+subcount*CB_NSTAGES,
			   mem+CB_MEML-memLfTbl[subcount],
			   memLfTbl[subcount], SUBL, CB_NSTAGES);

		   /* update memory */

		   memcpy(mem, mem+SUBL, (CB_MEML-SUBL)*sizeof(float));
		   memcpy(mem+CB_MEML-SUBL,&decresidual[(start+1+subframe)*SUBL],SUBL*sizeof(float));
		   memset(weightState, 0, LPC_FILTERORDER*sizeof(float));

		   subcount++;
	   }
   }


   /* backward prediction of sub-frames */

   Nback = start-1;


   if ( Nback > 0 ) {

	   /* create reverse order vectors */

	   for (n=0; n<Nback; n++) {
		   for (k=0; k<SUBL; k++) {
			   reverseResidual[n*SUBL+k] =  residual[(start-1)*SUBL-1-n*SUBL-k];
			   reverseDecresidual[n*SUBL+k]=decresidual[(start-1)*SUBL-1-n*SUBL-k];
		   }
	   }

	   /* setup memory */

	   meml_gotten = SUBL*(iLBCenc_inst->nsub+1-start);


	   if ( meml_gotten > CB_MEML ) {
		   meml_gotten=CB_MEML;
	   }
	   for (k=0; k<meml_gotten; k++) {
		   mem[CB_MEML-1-k] = decresidual[(start-1)*SUBL + k];
	   }
	   memset(mem, 0, (CB_MEML-k)*sizeof(float));
	   memset(weightState, 0, LPC_FILTERORDER*sizeof(float));

	   /* loop over sub-frames to encode */

	   for (subframe=0; subframe<Nback; subframe++) {

		   /* encode sub-frame */

		   iCBSearch(iLBCenc_inst, cb_index+subcount*CB_NSTAGES,
			   gain_index+subcount*CB_NSTAGES,
			   &reverseResidual[subframe*SUBL],
			   mem+CB_MEML-memLfTbl[subcount],
			   memLfTbl[subcount], SUBL, CB_NSTAGES,
			   &weightdenum[(start-2-subframe)*
						   (LPC_FILTERORDER+1)],
			   weightState, subcount+1,ste);

		   /* construct decoded vector */

		   iCBConstruct(&reverseDecresidual[subframe*SUBL],
			   cb_index+subcount*CB_NSTAGES,
			   gain_index+subcount*CB_NSTAGES,
			   mem+CB_MEML-memLfTbl[subcount],
			   memLfTbl[subcount], SUBL, CB_NSTAGES);

		   /* update memory */

		   memcpy(mem, mem+SUBL, (CB_MEML-SUBL)*sizeof(float));
		   memcpy(mem+CB_MEML-SUBL,&reverseDecresidual[subframe*SUBL], SUBL*sizeof(float));
		   memset(weightState, 0, LPC_FILTERORDER*sizeof(float));

		   subcount++;

	   }

	   /* get decoded residual from reversed vector */

	   for (i=0; i<SUBL*Nback; i++) {
		   decresidual[SUBL*Nback - i - 1] = reverseDecresidual[i];
	   }
   }
   /* end encoding part */

   /* adjust index */
   index_conv_enc(cb_index);


   /* pack bytes */

   pbytes=bytes;
   pos=0;

   /* loop over the 3 ULP classes */

   for (ulp=0; ulp<3; ulp++) {
	   /* LSF */
	   for (k=0; k<LSF_NSPLIT*iLBCenc_inst->lpc_n; k++) {
		   /* packsplit 的参数为：
		   *  原始int，切出的前部分，切出的后部分，前部分的长度，总长度*/
		   packsplit(&lsf_i[k], &firstpart, &lsf_i[k],
			   iLBCenc_inst->ULP_inst->lsf_bits[k][ulp],
			   iLBCenc_inst->ULP_inst->lsf_bits[k][ulp]+
			   iLBCenc_inst->ULP_inst->lsf_bits[k][ulp+1]+
			   iLBCenc_inst->ULP_inst->lsf_bits[k][ulp+2]);
		   /* dopack 的参数为： 打包数据流，要打包的数据，打包的数量，当前流位置*/
		   dopack( &pbytes, firstpart,iLBCenc_inst->ULP_inst->lsf_bits[k][ulp], &pos);
	   }

	   /* Start block info */

	   packsplit(&start, &firstpart, &start,
		   iLBCenc_inst->ULP_inst->start_bits[ulp],
		   iLBCenc_inst->ULP_inst->start_bits[ulp]+
		   iLBCenc_inst->ULP_inst->start_bits[ulp+1]+
		   iLBCenc_inst->ULP_inst->start_bits[ulp+2]);
	   dopack( &pbytes, firstpart,iLBCenc_inst->ULP_inst->start_bits[ulp], &pos);

	   packsplit(&state_first, &firstpart, &state_first,
		   iLBCenc_inst->ULP_inst->startfirst_bits[ulp],
		   iLBCenc_inst->ULP_inst->startfirst_bits[ulp]+
		   iLBCenc_inst->ULP_inst->startfirst_bits[ulp+1]+
		   iLBCenc_inst->ULP_inst->startfirst_bits[ulp+2]);
	   dopack( &pbytes, firstpart,iLBCenc_inst->ULP_inst->startfirst_bits[ulp], &pos);

	   packsplit(&idxForMax, &firstpart, &idxForMax,
		   iLBCenc_inst->ULP_inst->scale_bits[ulp],
		   iLBCenc_inst->ULP_inst->scale_bits[ulp]+
		   iLBCenc_inst->ULP_inst->scale_bits[ulp+1]+
		   iLBCenc_inst->ULP_inst->scale_bits[ulp+2]);
	   dopack( &pbytes, firstpart,iLBCenc_inst->ULP_inst->scale_bits[ulp], &pos);

	   for (k=0; k<iLBCenc_inst->state_short_len; k++) {
		   packsplit(idxVec+k, &firstpart, idxVec+k,
			   iLBCenc_inst->ULP_inst->state_bits[ulp],
			   iLBCenc_inst->ULP_inst->state_bits[ulp]+
			   iLBCenc_inst->ULP_inst->state_bits[ulp+1]+
			   iLBCenc_inst->ULP_inst->state_bits[ulp+2]);
		   dopack( &pbytes, firstpart, iLBCenc_inst->ULP_inst->state_bits[ulp], &pos);
	   }
	   /* 23/22 (20ms/30ms) sample block */

	   for (k=0;k<CB_NSTAGES;k++) {
		   packsplit(extra_cb_index+k, &firstpart,extra_cb_index+k,
			   iLBCenc_inst->ULP_inst->extra_cb_index[k][ulp],
			   iLBCenc_inst->ULP_inst->extra_cb_index[k][ulp]+
			   iLBCenc_inst->ULP_inst->extra_cb_index[k][ulp+1]+
			   iLBCenc_inst->ULP_inst->extra_cb_index[k][ulp+2]);
		   dopack( &pbytes, firstpart, iLBCenc_inst->ULP_inst->extra_cb_index[k][ulp],&pos);
	   }

	   for (k=0;k<CB_NSTAGES;k++) {
		   packsplit(extra_gain_index+k, &firstpart,extra_gain_index+k,
			   iLBCenc_inst->ULP_inst->extra_cb_gain[k][ulp],
			   iLBCenc_inst->ULP_inst->extra_cb_gain[k][ulp]+
			   iLBCenc_inst->ULP_inst->extra_cb_gain[k][ulp+1]+
			   iLBCenc_inst->ULP_inst->extra_cb_gain[k][ulp+2]);
		   dopack( &pbytes, firstpart, iLBCenc_inst->ULP_inst->extra_cb_gain[k][ulp], &pos);
	   }

	   /* The two/four (20ms/30ms) 40 sample sub-blocks */

	   for (i=0; i<iLBCenc_inst->nasub; i++) {
		   for (k=0; k<CB_NSTAGES; k++) {
			   packsplit(cb_index+i*CB_NSTAGES+k, &firstpart,
				   cb_index+i*CB_NSTAGES+k,
				   iLBCenc_inst->ULP_inst->cb_index[i][k][ulp],
				   iLBCenc_inst->ULP_inst->cb_index[i][k][ulp]+
				   iLBCenc_inst->ULP_inst->cb_index[i][k][ulp+1]+
				   iLBCenc_inst->ULP_inst->cb_index[i][k][ulp+2]);
			   dopack( &pbytes, firstpart, iLBCenc_inst->ULP_inst->cb_index[i][k][ulp],&pos);
		   }
	   }

	   for (i=0; i<iLBCenc_inst->nasub; i++) {
		   for (k=0; k<CB_NSTAGES; k++) {
			   packsplit(gain_index+i*CB_NSTAGES+k, &firstpart,
				   gain_index+i*CB_NSTAGES+k,
				   iLBCenc_inst->ULP_inst->cb_gain[i][k][ulp],
				   iLBCenc_inst->ULP_inst->cb_gain[i][k][ulp]+
				   iLBCenc_inst->ULP_inst->cb_gain[i][k][ulp+1]+
				   iLBCenc_inst->ULP_inst->cb_gain[i][k][ulp+2]);
			   dopack( &pbytes, firstpart,iLBCenc_inst->ULP_inst->cb_gain[i][k][ulp], &pos);
		   }
	   }
   }

   /* set the last bit to zero (otherwise the decoder
	  will treat it as a lost frame) */
   dopack( &pbytes, 0, 1, &pos);
}