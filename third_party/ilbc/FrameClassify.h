#ifndef __iLBC_FRAMECLASSIFY_H
#define __iLBC_FRAMECLASSIFY_H

int FrameClassify(      /* index to the max-energy sub-frame */
				  iLBC_Enc_Inst_t *iLBCenc_inst, /* (i/o) the encoder state structure */
				  float *residual,     /* (i) lpc residual signal */
				  STE *ste
				  );
#endif