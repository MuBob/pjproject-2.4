/******************************************************************
*  编解码主函数。完成编解码的总体过程，同时计算语音的长度，编解码
*  所用的时间。
******************************************************************/
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "iLBC_define.h"
#include "iLBC_encode.h"
#include "iLBC_decode.h"
#include "iLBC.h"

/* Runtime statistics */
#include <time.h>

/*定义 字 数是字节数的一半，说明一个字16位*/
#define ILBCNOOFWORDS_MAX   (NO_OF_BYTES_30MS/2)         

int main(int argc, char* argv[])
{


   if ( 1 ) {
	   fprintf(stderr,"\n*-----------------------------------------------*\n");
	   fprintf(stderr,"   %s <20,30> input encoded decoded ",argv[0]);
	   fprintf(stderr,"stego_in stego_out stego_method (channel)\n\n");
	   fprintf(stderr,"   mode          : Frame size for the encoding/decoding\n");
	   fprintf(stderr,"                     20 - 20 ms\n");
	   fprintf(stderr,"                     30 - 30 ms\n");
	   fprintf(stderr,"   input         : Speech for encoder (16-bit pcm file)\n");
	   fprintf(stderr,"   encoded       : Encoded bit stream\n");
	   fprintf(stderr,"   decoded       : Decoded speech (16-bit pcm file)\n");
	   fprintf(stderr,"   stego_in      : input secret info,txt file\n");
	   fprintf(stderr,"   stego_out     : output secret info,txt file\n");
	   fprintf(stderr,"   stego_method  : stegonography method, 6 bit \n");
	   fprintf(stderr,"                     0：开始状态位置选择 Bolck Class\n");
	   fprintf(stderr,"                     1：22采样段选择\n");
	   fprintf(stderr,"                     2：开始状态比例因子\n");
	   fprintf(stderr,"                     3：开始状态标量量化\n");
	   fprintf(stderr,"                     4：动态码本量化\n");
	   fprintf(stderr,"                     5：动态增益量化\n");

	   fprintf(stderr,"   channel       : Packet loss pattern, optional (16-bit)\n");
	   fprintf(stderr,"                   1 - Packet received correctly\n");
	   fprintf(stderr,"                   0 - Packet Lost\n");
	   fprintf(stderr,"*-----------------------------------------------*\n\n");

   }
   system("pause");

   return(0);
}