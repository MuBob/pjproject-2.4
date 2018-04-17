#include "aes_icm.h"
#include "alloc.h"
//#include "cc1lib.h"
//#pragma comment(lib,"cc1lib.lib")
#define PACKAGE_SIZE 38
#define PACKAGE_BUF_NUM 10
err_status_t
lpc_alloc_ismacryp(cipher_t **c, int key_len, int forIsmacryp) {
	extern cipher_type_t lpc_algo;
	uint8_t *pointer;
	int tmp;	/***********************开始*********************/
	/*HANDLE cc1En, cc1De;
	unsigned char Rstar[72];
	unsigned long id = 0x0001;
	unsigned char packBuf[PACKAGE_BUF_NUM][PACKAGE_SIZE];

	for (int i = 0; i<sizeof(Rstar); ++i)
		Rstar[i] = i + 1;

	cc1En = CC_1_CreateContext(id, Rstar);
	cc1De = CC_1_CreateContext(id, Rstar);

	*/
	/****************开始********************/
	/*
	* Ismacryp, for example, uses 16 byte key + 8 byte
	* salt  so this function is called with key_len = 24.
	* The check for key_len = 30 does not apply. Our usage
	* of aes functions with key_len = values other than 30
	* has not broken anything. Don't know what would be the
	* effect of skipping this check for srtp in general.
	*/
	if (!forIsmacryp && key_len != 30)
		return err_status_bad_param;

	/* allocate memory a cipher of type aes_icm */
	tmp = (sizeof(aes_icm_ctx_t)+sizeof(cipher_t));
	pointer = (uint8_t*)crypto_alloc(tmp);
	if (pointer == NULL)
		return err_status_alloc_fail;

	/* set pointers */
	*c = (cipher_t *)pointer;
	(*c)->type = &lpc_algo;
	(*c)->state = pointer + sizeof(cipher_t);

	/* increment ref_count */
	lpc_algo.ref_count++;

	/* set key size        */
	(*c)->key_len = key_len;

	return err_status_ok;
}

err_status_t lpc_alloc(cipher_t **c, int key_len, int forIsmacryp) {
	return lpc_alloc_ismacryp(c, key_len, 0);
}

err_status_t
lpc_dealloc(cipher_t *c) {
	extern cipher_type_t lpc_algo;

	/* zeroize entire state*/
	octet_string_set_to_zero((uint8_t *)c,
		sizeof(aes_icm_ctx_t)+sizeof(cipher_t));

	/* free memory */
	crypto_free(c);

	/* decrement ref_count */
	lpc_algo.ref_count--;

	return err_status_ok;
}


/*
* aes_icm_context_init(...) initializes the aes_icm_context
* using the value in key[].
*
* the key is the secret key
*
* the salt is unpredictable (but not necessarily secret) data which
* randomizes the starting point in the keystream
*/

err_status_t
lpc_context_init(aes_icm_ctx_t *c, const uint8_t *key) {
	v128_t tmp_key;

	/* set counter and initial values to 'offset' value */
	/* FIX!!! this assumes the salt is at key + 16, and thus that the */
	/* FIX!!! cipher key length is 16!  Also note this copies past the
	end of the 'key' array by 2 bytes! */
	v128_copy_octet_string(&c->counter, key + 16);
	v128_copy_octet_string(&c->offset, key + 16);

	/* force last two octets of the offset to zero (for srtp compatibility) */
	c->offset.v8[14] = c->offset.v8[15] = 0;
	c->counter.v8[14] = c->counter.v8[15] = 0;

	/* set tmp_key (for alignment) */
	v128_copy_octet_string(&tmp_key, key);



	/* expand key */
	aes_expand_encryption_key(&tmp_key, c->expanded_key);

	/* indicate that the keystream_buffer is empty */
	c->bytes_in_buffer = 0;

	return err_status_ok;
}

/*
* aes_icm_set_octet(c, i) sets the counter of the context which it is
* passed so that the next octet of keystream that will be generated
* is the ith octet
*/

err_status_t
lpc_set_octet(aes_icm_ctx_t *c,
uint64_t octet_num) {

#ifdef NO_64BIT_MATH
	int tail_num = low32(octet_num) & 0x0f;
	/* 64-bit right-shift 4 */
	uint64_t block_num = make64(high32(octet_num) >> 4,
		((high32(octet_num) & 0x0f) << (32 - 4)) |
		(low32(octet_num) >> 4));
#else
	int tail_num = octet_num % 16;
	uint64_t block_num = octet_num / 16;
#endif


	/* set counter value */
	/* FIX - There's no way this is correct */
	c->counter.v64[0] = c->offset.v64[0];
#ifdef NO_64BIT_MATH
	c->counter.v64[0] = make64(high32(c->offset.v64[0]) ^ high32(block_num),
		low32(c->offset.v64[0]) ^ low32(block_num));
#else
	c->counter.v64[0] = c->offset.v64[0] ^ block_num;
#endif
	/* fill keystream buffer, if needed */
	if (tail_num) {
		v128_copy(&c->keystream_buffer, &c->counter);
		aes_encrypt(&c->keystream_buffer, c->expanded_key);
		c->bytes_in_buffer = sizeof(v128_t);

	/*  indicate number of bytes in keystream_buffer  */
		c->bytes_in_buffer = sizeof(v128_t)-tail_num;

	}
	else {

		/* indicate that keystream_buffer is empty */
		c->bytes_in_buffer = 0;
	}

	return err_status_ok;
}

/*
* aes_icm_set_iv(c, iv) sets the counter value to the exor of iv with
* the offset
*/

err_status_t
lpc_set_iv(aes_icm_ctx_t *c, void *iv) {
	v128_t *nonce = (v128_t *)iv;


	v128_xor(&c->counter, &c->offset, nonce);

	/* indicate that the keystream_buffer is empty */
	c->bytes_in_buffer = 0;

	return err_status_ok;
}



/*
* aes_icm_advance(...) refills the keystream_buffer and
* advances the block index of the sicm_context forward by one
*
* this is an internal, hopefully inlined function
*/

static inline void
lpc_advance_ismacryp(aes_icm_ctx_t *c, uint8_t forIsmacryp) {
	/* fill buffer with new keystream */
	v128_copy(&c->keystream_buffer, &c->counter);
	aes_encrypt(&c->keystream_buffer, c->expanded_key);
	c->bytes_in_buffer = sizeof(v128_t);



	/* clock counter forward */

	if (forIsmacryp) {
		uint32_t temp;
		//alex's clock counter forward
		temp = ntohl(c->counter.v32[3]);
		c->counter.v32[3] = htonl(++temp);
	}
	else {
		if (!++(c->counter.v8[15]))
			++(c->counter.v8[14]);
	}
}

inline void lpc_advance(aes_icm_ctx_t *c) {
	lpc_advance_ismacryp(c, 0);
}


/*e
* icm_encrypt deals with the following cases:
*
* bytes_to_encr < bytes_in_buffer
*  - add keystream into data
*
* bytes_to_encr > bytes_in_buffer
*  - add keystream into data until keystream_buffer is depleted
*  - loop over blocks, filling keystream_buffer and then
*    adding keystream into data
*  - fill buffer then add in remaining (< 16) bytes of keystream
*/

err_status_t
lpc_encrypt_ismacryp(aes_icm_ctx_t *c,
unsigned char *buf, unsigned int *enc_len,
int forIsmacryp) {
	unsigned int bytes_to_encr = *enc_len;
	unsigned int i;
	uint32_t *b;

	/* check that there's enough segment left but not for ismacryp*/
	if (!forIsmacryp && (bytes_to_encr + htons(c->counter.v16[7])) > 0xffff)
		return err_status_terminus;

	
	if (bytes_to_encr <= (unsigned int)c->bytes_in_buffer) {

		/* deal with odd case of small bytes_to_encr */
		for (i = (sizeof(v128_t)-c->bytes_in_buffer);
			i < (sizeof(v128_t)-c->bytes_in_buffer + bytes_to_encr); i++)
		{
			*buf++ ^= c->keystream_buffer.v8[i];
		}

		c->bytes_in_buffer -= bytes_to_encr;

		/* return now to avoid the main loop */
		return err_status_ok;

	}
	else {

		/* encrypt bytes until the remaining data is 16-byte aligned */
		for (i = (sizeof(v128_t)-c->bytes_in_buffer); i < sizeof(v128_t); i++)
			*buf++ ^= c->keystream_buffer.v8[i];

		bytes_to_encr -= c->bytes_in_buffer;
		c->bytes_in_buffer = 0;

	}

	/* now loop over entire 16-byte blocks of keystream */
	for (i = 0; i < (bytes_to_encr / sizeof(v128_t)); i++) {

		/* fill buffer with new keystream */
		lpc_advance_ismacryp(c, forIsmacryp);

		/*
		* add keystream into the data buffer (this would be a lot faster
		* if we could assume 32-bit alignment!)
		*/

#if ALIGN_32
		b = (uint32_t *)buf;
		*b++ ^= c->keystream_buffer.v32[0];
		*b++ ^= c->keystream_buffer.v32[1];
		*b++ ^= c->keystream_buffer.v32[2];
		*b++ ^= c->keystream_buffer.v32[3];
		buf = (uint8_t *)b;
#else    
		if ((((unsigned long)buf) & 0x03) != 0) {
			*buf++ ^= c->keystream_buffer.v8[0];
			*buf++ ^= c->keystream_buffer.v8[1];
			*buf++ ^= c->keystream_buffer.v8[2];
			*buf++ ^= c->keystream_buffer.v8[3];
			*buf++ ^= c->keystream_buffer.v8[4];
			*buf++ ^= c->keystream_buffer.v8[5];
			*buf++ ^= c->keystream_buffer.v8[6];
			*buf++ ^= c->keystream_buffer.v8[7];
			*buf++ ^= c->keystream_buffer.v8[8];
			*buf++ ^= c->keystream_buffer.v8[9];
			*buf++ ^= c->keystream_buffer.v8[10];
			*buf++ ^= c->keystream_buffer.v8[11];
			*buf++ ^= c->keystream_buffer.v8[12];
			*buf++ ^= c->keystream_buffer.v8[13];
			*buf++ ^= c->keystream_buffer.v8[14];
			*buf++ ^= c->keystream_buffer.v8[15];
		}
		else {
			b = (uint32_t *)buf;
			*b++ ^= c->keystream_buffer.v32[0];
			*b++ ^= c->keystream_buffer.v32[1];
			*b++ ^= c->keystream_buffer.v32[2];
			*b++ ^= c->keystream_buffer.v32[3];
			buf = (uint8_t *)b;
		}
#endif /* #if ALIGN_32 */

	}

	/* if there is a tail end of the data, process it */
	if ((bytes_to_encr & 0xf) != 0) {

		/* fill buffer with new keystream */
		lpc_advance_ismacryp(c, forIsmacryp);

		for (i = 0; i < (bytes_to_encr & 0xf); i++)
			*buf++ ^= c->keystream_buffer.v8[i];

		/* reset the keystream buffer size to right value */
		c->bytes_in_buffer = sizeof(v128_t)-i;
	}
	else {

		/* no tail, so just reset the keystream buffer size to zero */
		c->bytes_in_buffer = 0;

	}

	return err_status_ok;
}

err_status_t
lpc_encrypt(aes_icm_ctx_t *c, unsigned char *buf, unsigned int *enc_len) {
	//return lpc_encrypt_ismacryp(c, buf, enc_len, 0);
	return err_status_ok;
}

err_status_t
lpc_output(aes_icm_ctx_t *c, uint8_t *buffer, int num_octets_to_output) {
	unsigned int len = num_octets_to_output;

	/* zeroize the buffer */
	octet_string_set_to_zero(buffer, num_octets_to_output);

	/* exor keystream into buffer */
	return lpc_encrypt(c, buffer, &len);
}

cipher_test_case_t lpc_algo_test_case_0 = {
	0,                 /* octets in key            */
	NULL,              /* key                      */
	0,                 /* packet index             */
	0,                 /* octets in plaintext      */
	NULL,              /* plaintext                */
	0,                 /* octets in plaintext      */
	NULL,              /* ciphertext               */
	NULL               /* pointer to next testcase */                             /* pointer to next testcase */
};
char
lpc_description[] = "lpc integer counter mode";
cipher_type_t lpc_algo = {
	(cipher_alloc_func_t)lpc_alloc,
	(cipher_dealloc_func_t)lpc_dealloc,
	(cipher_init_func_t)lpc_context_init,
	(cipher_encrypt_func_t)lpc_encrypt,
	(cipher_decrypt_func_t)lpc_encrypt,
	(cipher_set_iv_func_t)lpc_set_iv,
	(char *)lpc_description,
	(int)0,   /* instance count */
	(cipher_test_case_t *)&lpc_algo_test_case_0,
	NULL
};