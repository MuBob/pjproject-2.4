#include <math.h>
#include <string.h>

#include "iLBC_define.h"
#include "gainquant.h"
#include "createCB.h"
#include "filter.h"
#include "constants.h"

#include "bitstream.h"
/*----------------------------------------------------------------*
*  Search routine for codebook encoding and gain quantization.
*  激励编码函数。实现码本编码和增益量化的常规搜索。
*---------------------------------------------------------------*/

void iCBSearch(
   iLBC_Enc_Inst_t *iLBCenc_inst,
					   /* (i) the encoder state structure */
   int *index,         /* (o) Codebook indices */
   int *gain_index,/* (o) Gain quantization indices */
   float *intarget,/* (i) Target vector for encoding */
   float *mem,         /* (i) Buffer for codebook construction */
   int lMem,           /* (i) Length of buffer 码本缓存*/
   int lTarget,    /* (i) Length of vector */
   int nStages,    /* (i) Number of codebook stages */
   float *weightDenum, /* (i) weighting filter coefficients */
   float *weightState, /* (i) weighting filter state */
   int block,           /* (i) the sub-block number */
   STE *ste
){
   int i, j, icount, stage, best_index, range, counter;  //range 每阶段的吗本数，
   float max_measure, gain, measure, crossDot, ftmp;	// measure 码本契合度的衡量值,
   float gains[CB_NSTAGES];
   float target[SUBL];							//量化目标
   int base_index, sInd, eInd, base_size;		// base_size 基本码本尺寸(不包括任何扩展)
   int sIndAug=0, eIndAug=0;
   float buf[CB_MEML+SUBL+2*LPC_FILTERORDER];
   float invenergy[CB_EXPAND*128], energy[CB_EXPAND*128]; /*invenergy 码本能量的倒数，energy 码本能量*/
   float *pp, *ppi=0, *ppo=0, *ppe=0;
   float cbvectors[CB_MEML];			//通过滤波构造的新的码本
   float tene, cene, cvec[SUBL];		// cvec编码了的向量 ，
   float aug_vec[SUBL];

   memset(cvec,0,SUBL*sizeof(float));

   /* Determine size of codebook sections 
   *  确定基本码本尺寸，85-22/23 or 147-40  */
   base_size=lMem-lTarget+1;
   if (lTarget==SUBL) {
	   base_size=lMem-lTarget+1+lTarget/2;
   }

   /* setup buffer for weighting 
   *  buf数据分布： 权重滤波初始状态+码本重建缓存+量化目标*/

   memcpy(buf,weightState,sizeof(float)*LPC_FILTERORDER);
   memcpy(buf+LPC_FILTERORDER,mem,lMem*sizeof(float));
   memcpy(buf+LPC_FILTERORDER+lMem,intarget,lTarget*sizeof(float));

   /* weighting 权重滤波*/

   AllPoleFilter(buf+LPC_FILTERORDER, weightDenum,lMem+lTarget, LPC_FILTERORDER);

   /* Construct the codebook and target needed 取出量化目标数据*/
   memcpy(target, buf+LPC_FILTERORDER+lMem, lTarget*sizeof(float));

   /* 计算 量化目标数据的 能量和*/
   tene=0.0;
   for (i=0; i<lTarget; i++) {
	   tene+=target[i]*target[i];
   }

   /* Prepare search over one more codebook section. This section
   *  is created by filtering the original buffer with a filter. 
   *  通过滤波 构造新的码本 cbvectors   */

   filteredCBvecs(cbvectors, buf+LPC_FILTERORDER, lMem);

   /* The Main Loop over stages */

   for (stage=0; stage<nStages; stage++) {

	   /*
	   信息隐藏 ，ChoiceSTE[4] ，动态码本量化，每一阶段可以嵌入 1 比特，总共 3/5 比特
	   */
	   if (ste->ChoiceSTE[4] ==1)
	   {
		   getbit(ste);
	   }


	   /* 确定码本 数*/
	   range = search_rangeTbl[block][stage];

	   /* initialize search measure */

	   max_measure = (float)-10000000.0;
	   gain = (float)0.0;
	   best_index = 0;

	   /* Compute cross dot product between the target and the CB memory */
	   crossDot=0.0;
	   pp=buf+LPC_FILTERORDER+lMem-lTarget;/* 第一个码本向量的开始位置(虽然按照RFC是码本结束的位置) */
	   for (j=0; j<lTarget; j++) {
		   crossDot += target[j]*(*pp++);
	   }


	   /* 计算第一个码本的 契合度 
	   *  对于第一个量化阶段stage==0,需要计算第一个码字的能量
	   *  对于后面的量化阶段stage!=0,可以直接使用已有“第一个码字能量”的结果 */
	   if (stage==0) {

		   /* Calculate energy in the first block of
			 'lTarget' samples. */
		   ppe = energy;
		   ppi = buf+LPC_FILTERORDER+lMem-lTarget-1;/* 第二个码本向量的开始位置*/
		   ppo = buf+LPC_FILTERORDER+lMem-1;		/* 第二个码本向量的结束位置*/

		   /* 计算第一个码本的能量 */
		   *ppe=0.0;
		   pp=buf+LPC_FILTERORDER+lMem-lTarget;
		   for (j=0; j<lTarget; j++) {
			   *ppe+=(*pp)*(*pp++);
		   }

		   if (*ppe>0.0) {
			   invenergy[0] = (float) 1.0 / (*ppe + EPS);
		   } else {
			   invenergy[0] = (float) 0.0;
		   }
		   ppe++;

		   measure=(float)-10000000.0;

		   if (crossDot > 0.0) {
				  measure = crossDot*crossDot*invenergy[0];/* 码本契合度的衡量值*/
		   }
	   }
	   else {
		   measure = crossDot*crossDot*invenergy[0]; /* 后面的量化阶段可以直接利用已有数据invenergy*/
	   }


	   /* check if measure is better */
	   ftmp = crossDot*invenergy[0];// 把 增益 缓存起来

	   if (  ( (ste->ChoiceSTE[4]==0) || ((ste->ChoiceSTE[4]==1) && (ste->curbit == 0)) ) 
		   && (measure>max_measure) && (fabs(ftmp)<CB_MAXGAIN) ) 
	   {
		   best_index = 0;
		   max_measure = measure;
		   gain = ftmp;
	   }

	   /* loop over the main first codebook section, full search
	   *  遍历第一个主要码本(从第二个码字开始) */
	   for (icount=1; icount<range; icount++) {

		   /* calculate measure */

		   crossDot=0.0;
		   pp = buf+LPC_FILTERORDER+lMem-lTarget-icount;//指向 第 icount+1 个码字

		   /* 计算 量化目标 与 当前码字的 点积*/
		   for (j=0; j<lTarget; j++) {
			   crossDot += target[j]*(*pp++);
		   }

		   /* 计算 measure 需要 码字的能量，如果stage==0 则需要计算，否则可以直接使用之前计算好的值*/
		   if (stage==0) {
			   *ppe++ = energy[icount-1] + (*ppi)*(*ppi) - (*ppo)*(*ppo); /* 由上一次的能量计算本次的能量，只需改变两个点*/
			   ppo--;
			   ppi--;

			   if (energy[icount]>0.0) {
				   invenergy[icount] =
					   (float)1.0/(energy[icount]+EPS);
			   } else {
				   invenergy[icount] = (float) 0.0;
			   }
			   measure=(float)-10000000.0;

			   if (crossDot > 0.0) {
				   measure = crossDot*crossDot*invenergy[icount];
			   }
		   }
		   else {
			   measure = crossDot*crossDot*invenergy[icount];
		   }

		   /* check if measure is better */
		   ftmp = crossDot*invenergy[icount];

		   if ((measure>max_measure) && (fabs(ftmp)<CB_MAXGAIN) && 
			   ((ste->ChoiceSTE[4]==0) || ((ste->ChoiceSTE[4]==1) && (icount%2 ==ste->curbit)) )) 
		   {
			   best_index = icount;
			   max_measure = measure;
			   gain = ftmp;
		   }
	   }

	   /* Loop over augmented part in the first codebook section, full search.
		* The vectors are interpolated.
		* 遍历第一个码本的扩展部分，扩展部分是内插来的。 */

	   if (lTarget==SUBL) {

		   /* Search for best possible cb vector and compute the CB-vectors' energy.
			* 在扩展码本中搜索，参数为：
			* 搜索开始序号，结束序号，当前量化阶段，第一个扩展码的序号，
			* 目标向量，指向扩展码本缓存结束位置的指针，当前最大的判断值，
			* 当前最佳序号，当前增益，扩展码本向量的能量，前一值的倒数 */
		   searchAugmentedCB(20, 39, stage, base_size-lTarget/2,
			   target, buf+LPC_FILTERORDER+lMem,
			   &max_measure, &best_index, &gain, energy,
			   invenergy,ste);
	   }

	   /* 开始搜索第二个码本（滤波而成的码本） */

	   /* set search range for following codebook sections 
	   *  记录当前码本的最佳 码字 */
	   base_index=best_index;


	   /* 确定码本搜索范围  */
	   /* unrestricted search */
	   if (CB_RESRANGE == -1) {
		   /* sInd start_index, eInd end_index, 
		   *  sIndAug start_index_augment,eIndAug end_index_augment */
		   sInd=0;
		   eInd=range-1;
		   sIndAug=20;
		   eIndAug=39;
	   }

	   /* restricted search around best index from first codebook section */
	   else {
		   /* Initialize search indices */
		   sIndAug=0;
		   eIndAug=0;
		   sInd=base_index-CB_RESRANGE/2;
		   eInd=sInd+CB_RESRANGE;

		   if (lTarget==SUBL) {

			   if (sInd<0) {
				   /* 如果 开始码字的序号小于零，进行调整 */
				   sIndAug = 40 + sInd;
				   eIndAug = 39;
				   sInd=0;

			   } else if ( base_index < (base_size-20) ) {
				   if (eInd > range) {
					   sInd -= (eInd-range);
					   eInd = range;
				   }
			   } else { /* base_index >= (base_size-20) */

				   if (sInd < (base_size-20)) {
					   sIndAug = 20;
					   sInd = 0;
					   eInd = 0;
					   eIndAug = 19 + CB_RESRANGE;

					   if(eIndAug > 39) {
						   eInd = eIndAug-39;
						   eIndAug = 39;
					   }
				   } else {
					   sIndAug = 20 + sInd - (base_size-20);
					   eIndAug = 39;
					   sInd = 0;
					   eInd = CB_RESRANGE - (eIndAug-sIndAug+1);
				   }
			   }

		   } else { /* lTarget = 22 or 23 */

			   if (sInd < 0) {
				   eInd -= sInd;
				   sInd = 0;
			   }

			   if(eInd > range) {
				   sInd -= (eInd - range);
				   eInd = range;
			   }
		   }
	   }


	   /* search of higher codebook section */

	   /* index search range */
	   counter = sInd;
	   sInd += base_size;
	   eInd += base_size;
	   
	   /* 计算码字能量 */
	   if (stage==0) {
		   ppe = energy+base_size;
		   *ppe=0.0;

		   pp=cbvectors+lMem-lTarget;
		   for (j=0; j<lTarget; j++) {
			   *ppe+=(*pp)*(*pp++);
		   }

		   ppi = cbvectors + lMem - 1 - lTarget;
		   ppo = cbvectors + lMem - 1;

		   for (j=0; j<(range-1); j++) {
			   *(ppe+1) = *ppe + (*ppi)*(*ppi) - (*ppo)*(*ppo);
			   ppo--;
			   ppi--;
			   ppe++;
		   }
	   }

	   /* loop over search range */

	   for (icount=sInd; icount<eInd; icount++) {

		   /* calculate measure */

		   crossDot=0.0;
		   pp=cbvectors + lMem - (counter++) - lTarget;

		   for (j=0;j<lTarget;j++) {
			   crossDot += target[j]*(*pp++);
		   }

		   if (energy[icount]>0.0) {
			   invenergy[icount] =(float)1.0/(energy[icount]+EPS);
		   } else {
			   invenergy[icount] =(float)0.0;
		   }

		   if (stage==0) {

			   measure=(float)-10000000.0;

			   if (crossDot > 0.0) {
				   measure = crossDot*crossDot*
					   invenergy[icount];
			   }
		   }
		   else {
			   measure = crossDot*crossDot*invenergy[icount];
		   }

		   /* check if measure is better */
		   ftmp = crossDot*invenergy[icount];

		   if ((measure>max_measure) && (fabs(ftmp)<CB_MAXGAIN) && 
			  ((ste->ChoiceSTE[4]==0) || ((ste->ChoiceSTE[4]==1) && (icount%2 ==ste->curbit)) )) 
		   {
			   best_index = icount;
			   max_measure = measure;
			   gain = ftmp;
		   }
	   }

	   /* Search the augmented CB inside the limited range. */

	   if ((lTarget==SUBL)&&(sIndAug!=0)) {
		   searchAugmentedCB(sIndAug, eIndAug, stage,
			   2*base_size-20, target, cbvectors+lMem,
			   &max_measure, &best_index, &gain, energy,
			   invenergy,ste);
	   }


	   /*某些情况嵌入出错的处理方法
	    此时找到的最佳码字序号为 0，而如果要嵌入的是1 则发生错误，解决方法是直接将0改写为1
	   */
	   	   if( (ste->ChoiceSTE[4]==1) && (ste->curbit==1) && best_index==0)
		   best_index++;

	   /* record best index 
	   *  已经找到最佳码字匹配，记录结果 */
	   index[stage] = best_index;

	   /* gain quantization */
	   /* 
	   信息隐藏 ，ChoiceSTE[5] ，动态增益量化，每一阶段可以嵌入 1 比特，总共3* 3/5 比特
	   */
	   if (ste->ChoiceSTE[5] ==1)
	   {
		   getbit(ste);
	   }
	   if (stage==0){
		   if (gain<0.0){
			   gain = 0.0;
		   }

		   if (gain>CB_MAXGAIN) {
			   gain = (float)CB_MAXGAIN;
		   }
		   /* 参数为：增益，增益最大值(上级量化结果将用作本次量化尺度调整因子)，
		   *  量化值数量，量化序列*/
		   gain = gainquant(gain, 1.0, 32, &gain_index[stage],ste);
	   }
	   else {
		   if (stage==1) {
			   gain = gainquant(gain, (float)fabs(gains[stage-1]),
				   16, &gain_index[stage],ste);
		   } else {
			   gain = gainquant(gain, (float)fabs(gains[stage-1]),
				   8, &gain_index[stage],ste);
		   }
	   }

	   /* Extract the best (according to measure) codebook vector */

	   /* 如果是开始状态的剩余部分 */
	   if (lTarget==(STATE_LEN-iLBCenc_inst->state_short_len)) {

		   if (index[stage]<base_size) {
			   pp=buf+LPC_FILTERORDER+lMem-lTarget-index[stage];
		   } else {
			   pp=cbvectors+lMem-lTarget-index[stage]+base_size;
		   }
	   } else {/* 如果是一个独立的子帧 */

		   if (index[stage]<base_size) {
			   if (index[stage]<(base_size-20)) {
				   pp=buf+LPC_FILTERORDER+lMem-lTarget-index[stage];/*不在扩展码本中*/
			   } else {
				   createAugmentedVec(index[stage]-base_size+40,
					   buf+LPC_FILTERORDER+lMem,aug_vec);/*在扩展码本中，需要重建该码字*/
				   pp=aug_vec;
			   }
		   } else {
			   int filterno, position;

			   filterno=index[stage]/base_size;/* 有点“取整”的意思，index是base_size的几个整数倍*/
			   position=index[stage]-filterno*base_size;
			   if (position<(base_size-20)) {
				   pp=cbvectors+filterno*lMem-lTarget-index[stage]+filterno*base_size;
			   } else {
				   createAugmentedVec(
					   index[stage]-(filterno+1)*base_size+40,
					   cbvectors+filterno*lMem,aug_vec);
				   pp=aug_vec;
			   }
		   }
	   }

	   /* Subtract the best codebook vector, according
		  to measure, from the target vector */

	   for (j=0;j<lTarget;j++) {
		   cvec[j] += gain*(*pp);/* 编码结果*/
		   target[j] -= gain*(*pp++); /* 减去编码结果，更新目标向量*/
	   }

	   /* record quantized gain */

	   gains[stage]=gain;

   }/* end of Main Loop. for (stage=0;... */

   /* Gain adjustment for energy matching 
   *  第一级量化 增益调整 */
   cene=0.0;
   for (i=0; i<lTarget; i++) {
	   cene+=cvec[i]*cvec[i];
   }
   j=gain_index[0];

   for (i=gain_index[0]; i<32; i++) {
	   ftmp=cene*gain_sq5Tbl[i]*gain_sq5Tbl[i];

	   if ((ftmp<(tene*gains[0]*gains[0])) && (gain_sq5Tbl[j]<(2.0*gains[0])) && 
			((ste->ChoiceSTE[5]==0) || ((ste->ChoiceSTE[5]==1) && (i%2 == j%2)))) 
	   {
		   j=i;
	   }
   }
   gain_index[0]=j;
}