/******************************************************************

   iLBC Speech Coder ANSI-C Source Code

   iLBC_define.h

   Copyright (C) The Internet Society (2004).
   All Rights Reserved.

******************************************************************/
#include <string.h>
#include <stdio.h>

#ifndef __iLBC_ILBCDEFINE_H
#define __iLBC_ILBCDEFINE_H

/* general codec settings */

#define FS                      (float)8000.0
#define BLOCKL_20MS             160				//块长度
#define BLOCKL_30MS             240
#define BLOCKL_MAX              240
#define NSUB_20MS               4				//字块数
#define NSUB_30MS               6
#define NSUB_MAX				6
#define NASUB_20MS              2				//除去开始状态的子快数
#define NASUB_30MS              4
#define NASUB_MAX               4
#define SUBL					40				//子块长度
#define STATE_LEN               80				//开始状态的2个字块的长度
#define STATE_SHORT_LEN_30MS    58				//初始状态长度
#define STATE_SHORT_LEN_20MS    57				

/* LPC settings */

#define LPC_FILTERORDER         10				//LPC滤波器阶数
#define LPC_CHIRP_SYNTDENUM     (float)0.9025	//语音谱扩展银子
#define LPC_CHIRP_WEIGHTDENUM   (float)0.4222	//感知权重滤波器参数
#define LPC_LOOKBACK        60					//LPC分析时用到的上一帧的数据 数量(80 for 20ms 60 for 30 ms)
#define LPC_N_20MS              1				//进行LPC的次数
#define LPC_N_30MS              2
#define LPC_N_MAX               2
#define LPC_ASYMDIFF        20					//?????????????????
#define LPC_BW                  (float)60.0		//LCP频谱平滑用到的参数 (后面各点)
#define LPC_WN                  (float)1.0001	//LCP频谱平滑用到的参数 (第一个点)
#define LSF_NSPLIT              3				//LSF系数 分块数
#define LSF_NUMBER_OF_STEPS     4				//??????????????????
#define LPC_HALFORDER           (LPC_FILTERORDER/2)	//

/* cb settings */

#define CB_NSTAGES              3				//量化级数
#define CB_EXPAND               2				//扩展码本数
#define CB_MEML                 147				//码本存储 长度
#define CB_FILTERLEN			2*4				//码本滤波器长度？？？？	
#define CB_HALFFILTERLEN		4				//
#define CB_RESRANGE             34				//第二个码本(滤波形成的码本)的搜索范围大小
#define CB_MAXGAIN              (float)1.3		//最大增益限制

/* enhancer */

#define ENH_BLOCKL              80  /* block length */
#define ENH_BLOCKL_HALF         (ENH_BLOCKL/2)
#define ENH_HL                  3   /* 2*ENH_HL+1 is number blocks in said second sequence */
#define ENH_SLOP            2   /* max difference estimated and correct pitch period */
#define ENH_PLOCSL              20  /* pitch-estimates and pitch-locations buffer length */
#define ENH_OVERHANG        2
#define ENH_UPS0            4   /* upsampling rate */
#define ENH_FL0                 3   /* 2*FLO+1 is the length of each filter */
#define ENH_VECTL               (ENH_BLOCKL+2*ENH_FL0)

#define ENH_CORRDIM             (2*ENH_SLOP+1)
#define ENH_NBLOCKS             (BLOCKL_MAX/ENH_BLOCKL)
#define ENH_NBLOCKS_EXTRA       5
#define ENH_NBLOCKS_TOT         8   /* ENH_NBLOCKS + ENH_NBLOCKS_EXTRA */
#define ENH_BUFL            (ENH_NBLOCKS_TOT)*ENH_BLOCKL
#define ENH_ALPHA0              (float)0.05

/* Down sampling */

#define FILTERORDER_DS          7
#define DELAY_DS            3
#define FACTOR_DS               2

/* bit stream defs */

#define NO_OF_BYTES_20MS    38		//20ms字节数
#define NO_OF_BYTES_30MS    50
#define NO_OF_WORDS_20MS    19
#define NO_OF_WORDS_30MS    25
#define STATE_BITS              3	//状态位？
#define BYTE_LEN            8		//比特长度
#define ULP_CLASSES             3	//重要性级别( uneven level protection)

/* help parameters */

#define FLOAT_MAX               (float)1.0e37
#define EPS                     (float)2.220446049250313e-016
#define PI                      (float)3.14159265358979323846
#define MIN_SAMPLE              -32768
#define MAX_SAMPLE              32767
#define TWO_PI                  (float)6.283185307
#define PI2                     (float)0.159154943

/* type definition encoder instance */
typedef struct iLBC_ULP_Inst_t_ {
   int lsf_bits[6][ULP_CLASSES+2];
   int start_bits[ULP_CLASSES+2];
   int startfirst_bits[ULP_CLASSES+2];
   int scale_bits[ULP_CLASSES+2];
   int state_bits[ULP_CLASSES+2];
   int extra_cb_index[CB_NSTAGES][ULP_CLASSES+2];
   int extra_cb_gain[CB_NSTAGES][ULP_CLASSES+2];
   int cb_index[NSUB_MAX][CB_NSTAGES][ULP_CLASSES+2];
   int cb_gain[NSUB_MAX][CB_NSTAGES][ULP_CLASSES+2];
} iLBC_ULP_Inst_t;


/*信息隐藏增加 */
typedef struct STE_ {
	int ChoiceSTE[6];		/*选择隐藏方法 每个单元表示 是否采用该种隐藏方法
							隐藏方法顺序为：
							0：开始状态位置选择 Bolck Class
							1：22采样段选择
							2：开始状态比例因子
							3：开始状态标量量化
							4：动态码本量化
							5：动态增益量化
							*/
	char *hdTxt;		//嵌入信息所在文件  提取信息将存储到的文件
	int hdTxt_pos;			//当前读/写 到的hdTxt位置
	char buf;		//暂存 隐藏信息的 字节
	int bitpos;				//当前 字节的处理位置 0~7 ,应初始化为 8 方便处理
	char curbit;	//当前秘密比特 0 1

} STE;



/* type definition encoder instance 定义 编码实例*/

typedef struct iLBC_Enc_Inst_t_ {

   int mode;	/* flag for frame size mode */

   /* basic parameters for different frame sizes */
   int blockl;				//块长度
   int nsub;				//子块数
   int nasub;				//除去开始状态的子块数
   int no_of_bytes, no_of_words;  //字节数，字数
   int lpc_n;				//LPC滤波器 数量 1/2
   int state_short_len;		//初始状态长度 57/58
   const iLBC_ULP_Inst_t *ULP_inst; 

   /* analysis filter state    分析滤波器状态？  */
   float anaMem[LPC_FILTERORDER];

   /* old lsf parameters for interpolation 用于内插的旧LSF系数   */
   float lsfold[LPC_FILTERORDER];
   float lsfdeqold[LPC_FILTERORDER];

   /* signal buffer for LP analysis    LPC分析缓存 lookback+current  */
   float lpc_buffer[LPC_LOOKBACK + BLOCKL_MAX];

   /* state of input HP filter  高通滤波器状态？*/
   float hpimem[4];
   //ssw ilbc
   STE ste;


} iLBC_Enc_Inst_t;

/* type definition decoder instance */
typedef struct iLBC_Dec_Inst_t_ {

   int mode;		/* flag for frame size mode */   

   /* basic parameters for different frame sizes */
   int blockl;
   int nsub;
   int nasub;
   int no_of_bytes, no_of_words;
   int lpc_n;
   int state_short_len;
   const iLBC_ULP_Inst_t *ULP_inst;

   /* synthesis filter state  综合滤波器状态？  */
   float syntMem[LPC_FILTERORDER];

   /* old LSF for interpolation */

   float lsfdeqold[LPC_FILTERORDER];

   /* pitch lag estimated in enhancer and used in PLC “增强功能”中使用的峰值滞后估计  */
   int last_lag;

   /* PLC state information    丢包隐藏 状态信息*/
   int prevLag, consPLICount, prevPLI, prev_enh_pl;
   float prevLpc[LPC_FILTERORDER+1];
   float prevResidual[NSUB_MAX*SUBL];
   float per;
   unsigned long seed;

   /* previous synthesis filter parameters */
   float old_syntdenum[(LPC_FILTERORDER + 1)*NSUB_MAX];

   /* state of output HP filter */
   float hpomem[4];

   /* enhancer state information */
   int use_enhancer;
   float enh_buf[ENH_BUFL];
   float enh_period[ENH_NBLOCKS_TOT];
   
   //ssw ilbc
   STE ste;

} iLBC_Dec_Inst_t;









#endif
