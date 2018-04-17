#ifndef __iLBC_GAINQUANT_H
#define __iLBC_GAINQUANT_H

float gainquant(/* (o) quantized gain value */
				float in,       /* (i) gain value */
				float maxIn,/* (i) maximum of gain value */
				int cblen,      /* (i) number of quantization indices */
				int *index,      /* (o) quantization index */
				STE *ste
				);

float gaindequant(  /* (o) quantized gain value */
				  int index,      /* (i) quantization index */
				  float maxIn,/* (i) maximum of unquantized gain */
				  int cblen       /* (i) number of quantization indices */
				  );

#endif