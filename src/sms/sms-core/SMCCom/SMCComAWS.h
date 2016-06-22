/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_AWS_H
#define SMCCOM_AWS_H

#define	SCC_AWS_BACKETNAME_SIZE						64
#define	SCC_AWS_ACCESSKEY_SIZE						20
#define	SCC_AWS_SECRETKEY_SIZE						40
#define	SCC_AWS_POLICY_SIZE							2048
#define	SCC_AWS_VERSIONDIRNAME_SIZE					20

typedef enum _E_HTTP_METHOD {
	e_HTTP_METHOD_GET = 0,
	e_HTTP_METHOD_POST
} E_HTTP_METHOD;

typedef enum _E_AWS_BUCKET {
	e_AWS_BACKET_PROBE = 0,
	e_AWS_BACKET_MAP,
	e_AWS_BACKET_DRIVE
} E_AWS_BUCKET;

typedef struct _SMAWSINFO {
	Char	backetName[SCC_AWS_BACKETNAME_SIZE + 1];	// 配置先バケット名
	UChar	signature[SC_CAL_SIGNATURE_SIZE + 1];		// シグネチャ
	UChar	policy[SCC_AWS_POLICY_SIZE + 1];			// ポリシー
	Char	accessKey[SCC_AWS_ACCESSKEY_SIZE + 1];		// S3アクセスキー
	Char	secretKey[SCC_AWS_SECRETKEY_SIZE + 1];		// S3シークレットキー
	Char	versionDirName[SCC_AWS_VERSIONDIRNAME_SIZE + 1];	// バージョンディレクトリ名
	Char	reserve[2];
	LONG	expires;									// シグネチャ有効時間
} SMAWSINFO;

E_SC_RESULT CC_GetAWSInfo(SMCAL *smcal, T_CC_CMN_SMS_API_PRM *parm, E_HTTP_METHOD method, E_AWS_BUCKET backet, const Char *filePath, SMAWSINFO *aws);
E_SC_RESULT CC_GetAWSBucketName(SMCAL *smcal, T_CC_CMN_SMS_API_PRM *parm, E_HTTP_METHOD method, E_AWS_BUCKET backet, Char *backetName);
E_SC_RESULT SCC_AWS_GetPolicy(SMCAL *smcal, const Char *backet, const Char *filePath, LONG expires, UChar **policy);
// AWSアクセス情報取得
E_SC_RESULT SCC_GetAWSInfo(SMCAL *smcal, E_HTTP_METHOD method, E_AWS_BUCKET backet, const Char *filePath, SMAWSINFO *aws);

#endif // #ifndef SMCCOM_AWS_H
