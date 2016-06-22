/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_MD5_H
#define SMCCOM_MD5_H

/**
 * @brief MD5ダイジェスト計算クラス(SMCCom_Md5.h)
 */

/* MD5 context. */
typedef struct _T_CC_CMN_MD5_CTX {
	UINT32 state[4];	/* state (ABCD) */
	UINT32 count[2];	/* number of bits, modulo 2^64 (lsb first) */
	UINT8 buffer[64];	/* input buffer */
} T_CC_CMN_MD5_CTX;

void CC_MD5_Init(T_CC_CMN_MD5_CTX *);
void CC_MD5_Update(T_CC_CMN_MD5_CTX *apContext, UINT8 *apInput, UINT32 aInputLen);
void CC_MD5_Final(UINT8 *aDigest, T_CC_CMN_MD5_CTX *apContext);

#endif // #ifndef SMCCOM_MD5_H
