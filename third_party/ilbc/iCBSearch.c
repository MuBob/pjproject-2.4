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
*  �������뺯����ʵ���뱾��������������ĳ���������
*---------------------------------------------------------------*/

void iCBSearch(
   iLBC_Enc_Inst_t *iLBCenc_inst,
					   /* (i) the encoder state structure */
   int *index,         /* (o) Codebook indices */
   int *gain_index,/* (o) Gain quantization indices */
   float *intarget,/* (i) Target vector for encoding */
   float *mem,         /* (i) Buffer for codebook construction */
   int lMem,           /* (i) Length of buffer �뱾����*/
   int lTarget,    /* (i) Length of vector */
   int nStages,    /* (i) Number of codebook stages */
   float *weightDenum, /* (i) weighting filter coefficients */
   float *weightState, /* (i) weighting filter state */
   int block,           /* (i) the sub-block number */
   STE *ste
){
   int i, j, icount, stage, best_index, range, counter;  //range ÿ�׶ε�������
   float max_measure, gain, measure, crossDot, ftmp;	// measure �뱾���϶ȵĺ���ֵ,
   float gains[CB_NSTAGES];
   float target[SUBL];							//����Ŀ��
   int base_index, sInd, eInd, base_size;		// base_size �����뱾�ߴ�(�������κ���չ)
   int sIndAug=0, eIndAug=0;
   float buf[CB_MEML+SUBL+2*LPC_FILTERORDER];
   float invenergy[CB_EXPAND*128], energy[CB_EXPAND*128]; /*invenergy �뱾�����ĵ�����energy �뱾����*/
   float *pp, *ppi=0, *ppo=0, *ppe=0;
   float cbvectors[CB_MEML];			//ͨ���˲�������µ��뱾
   float tene, cene, cvec[SUBL];		// cvec�����˵����� ��
   float aug_vec[SUBL];

   memset(cvec,0,SUBL*sizeof(float));

   /* Determine size of codebook sections 
   *  ȷ�������뱾�ߴ磬85-22/23 or 147-40  */
   base_size=lMem-lTarget+1;
   if (lTarget==SUBL) {
	   base_size=lMem-lTarget+1+lTarget/2;
   }

   /* setup buffer for weighting 
   *  buf���ݷֲ��� Ȩ���˲���ʼ״̬+�뱾�ؽ�����+����Ŀ��*/

   memcpy(buf,weightState,sizeof(float)*LPC_FILTERORDER);
   memcpy(buf+LPC_FILTERORDER,mem,lMem*sizeof(float));
   memcpy(buf+LPC_FILTERORDER+lMem,intarget,lTarget*sizeof(float));

   /* weighting Ȩ���˲�*/

   AllPoleFilter(buf+LPC_FILTERORDER, weightDenum,lMem+lTarget, LPC_FILTERORDER);

   /* Construct the codebook and target needed ȡ������Ŀ������*/
   memcpy(target, buf+LPC_FILTERORDER+lMem, lTarget*sizeof(float));

   /* ���� ����Ŀ�����ݵ� ������*/
   tene=0.0;
   for (i=0; i<lTarget; i++) {
	   tene+=target[i]*target[i];
   }

   /* Prepare search over one more codebook section. This section
   *  is created by filtering the original buffer with a filter. 
   *  ͨ���˲� �����µ��뱾 cbvectors   */

   filteredCBvecs(cbvectors, buf+LPC_FILTERORDER, lMem);

   /* The Main Loop over stages */

   for (stage=0; stage<nStages; stage++) {

	   /*
	   ��Ϣ���� ��ChoiceSTE[4] ����̬�뱾������ÿһ�׶ο���Ƕ�� 1 ���أ��ܹ� 3/5 ����
	   */
	   if (ste->ChoiceSTE[4] ==1)
	   {
		   getbit(ste);
	   }


	   /* ȷ���뱾 ��*/
	   range = search_rangeTbl[block][stage];

	   /* initialize search measure */

	   max_measure = (float)-10000000.0;
	   gain = (float)0.0;
	   best_index = 0;

	   /* Compute cross dot product between the target and the CB memory */
	   crossDot=0.0;
	   pp=buf+LPC_FILTERORDER+lMem-lTarget;/* ��һ���뱾�����Ŀ�ʼλ��(��Ȼ����RFC���뱾������λ��) */
	   for (j=0; j<lTarget; j++) {
		   crossDot += target[j]*(*pp++);
	   }


	   /* �����һ���뱾�� ���϶� 
	   *  ���ڵ�һ�������׶�stage==0,��Ҫ�����һ�����ֵ�����
	   *  ���ں���������׶�stage!=0,����ֱ��ʹ�����С���һ�������������Ľ�� */
	   if (stage==0) {

		   /* Calculate energy in the first block of
			 'lTarget' samples. */
		   ppe = energy;
		   ppi = buf+LPC_FILTERORDER+lMem-lTarget-1;/* �ڶ����뱾�����Ŀ�ʼλ��*/
		   ppo = buf+LPC_FILTERORDER+lMem-1;		/* �ڶ����뱾�����Ľ���λ��*/

		   /* �����һ���뱾������ */
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
				  measure = crossDot*crossDot*invenergy[0];/* �뱾���϶ȵĺ���ֵ*/
		   }
	   }
	   else {
		   measure = crossDot*crossDot*invenergy[0]; /* ����������׶ο���ֱ��������������invenergy*/
	   }


	   /* check if measure is better */
	   ftmp = crossDot*invenergy[0];// �� ���� ��������

	   if (  ( (ste->ChoiceSTE[4]==0) || ((ste->ChoiceSTE[4]==1) && (ste->curbit == 0)) ) 
		   && (measure>max_measure) && (fabs(ftmp)<CB_MAXGAIN) ) 
	   {
		   best_index = 0;
		   max_measure = measure;
		   gain = ftmp;
	   }

	   /* loop over the main first codebook section, full search
	   *  ������һ����Ҫ�뱾(�ӵڶ������ֿ�ʼ) */
	   for (icount=1; icount<range; icount++) {

		   /* calculate measure */

		   crossDot=0.0;
		   pp = buf+LPC_FILTERORDER+lMem-lTarget-icount;//ָ�� �� icount+1 ������

		   /* ���� ����Ŀ�� �� ��ǰ���ֵ� ���*/
		   for (j=0; j<lTarget; j++) {
			   crossDot += target[j]*(*pp++);
		   }

		   /* ���� measure ��Ҫ ���ֵ����������stage==0 ����Ҫ���㣬�������ֱ��ʹ��֮ǰ����õ�ֵ*/
		   if (stage==0) {
			   *ppe++ = energy[icount-1] + (*ppi)*(*ppi) - (*ppo)*(*ppo); /* ����һ�ε��������㱾�ε�������ֻ��ı�������*/
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
		* ������һ���뱾����չ���֣���չ�������ڲ����ġ� */

	   if (lTarget==SUBL) {

		   /* Search for best possible cb vector and compute the CB-vectors' energy.
			* ����չ�뱾������������Ϊ��
			* ������ʼ��ţ�������ţ���ǰ�����׶Σ���һ����չ�����ţ�
			* Ŀ��������ָ����չ�뱾�������λ�õ�ָ�룬��ǰ�����ж�ֵ��
			* ��ǰ�����ţ���ǰ���棬��չ�뱾������������ǰһֵ�ĵ��� */
		   searchAugmentedCB(20, 39, stage, base_size-lTarget/2,
			   target, buf+LPC_FILTERORDER+lMem,
			   &max_measure, &best_index, &gain, energy,
			   invenergy,ste);
	   }

	   /* ��ʼ�����ڶ����뱾���˲����ɵ��뱾�� */

	   /* set search range for following codebook sections 
	   *  ��¼��ǰ�뱾����� ���� */
	   base_index=best_index;


	   /* ȷ���뱾������Χ  */
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
				   /* ��� ��ʼ���ֵ����С���㣬���е��� */
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
	   
	   /* ������������ */
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


	   /*ĳЩ���Ƕ�����Ĵ�����
	    ��ʱ�ҵ�������������Ϊ 0�������ҪǶ�����1 �������󣬽��������ֱ�ӽ�0��дΪ1
	   */
	   	   if( (ste->ChoiceSTE[4]==1) && (ste->curbit==1) && best_index==0)
		   best_index++;

	   /* record best index 
	   *  �Ѿ��ҵ��������ƥ�䣬��¼��� */
	   index[stage] = best_index;

	   /* gain quantization */
	   /* 
	   ��Ϣ���� ��ChoiceSTE[5] ����̬����������ÿһ�׶ο���Ƕ�� 1 ���أ��ܹ�3* 3/5 ����
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
		   /* ����Ϊ�����棬�������ֵ(�ϼ�����������������������߶ȵ�������)��
		   *  ����ֵ��������������*/
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

	   /* ����ǿ�ʼ״̬��ʣ�ಿ�� */
	   if (lTarget==(STATE_LEN-iLBCenc_inst->state_short_len)) {

		   if (index[stage]<base_size) {
			   pp=buf+LPC_FILTERORDER+lMem-lTarget-index[stage];
		   } else {
			   pp=cbvectors+lMem-lTarget-index[stage]+base_size;
		   }
	   } else {/* �����һ����������֡ */

		   if (index[stage]<base_size) {
			   if (index[stage]<(base_size-20)) {
				   pp=buf+LPC_FILTERORDER+lMem-lTarget-index[stage];/*������չ�뱾��*/
			   } else {
				   createAugmentedVec(index[stage]-base_size+40,
					   buf+LPC_FILTERORDER+lMem,aug_vec);/*����չ�뱾�У���Ҫ�ؽ�������*/
				   pp=aug_vec;
			   }
		   } else {
			   int filterno, position;

			   filterno=index[stage]/base_size;/* �е㡰ȡ��������˼��index��base_size�ļ���������*/
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
		   cvec[j] += gain*(*pp);/* ������*/
		   target[j] -= gain*(*pp++); /* ��ȥ������������Ŀ������*/
	   }

	   /* record quantized gain */

	   gains[stage]=gain;

   }/* end of Main Loop. for (stage=0;... */

   /* Gain adjustment for energy matching 
   *  ��һ������ ������� */
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