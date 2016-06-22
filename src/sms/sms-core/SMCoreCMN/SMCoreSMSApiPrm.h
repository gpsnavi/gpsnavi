/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCORESMSAPIPRM_H
#define SMCORESMSAPIPRM_H

//------------------------------------------------------------------------------------------------------------------------
//
//	SMCoreSMSApiPrmヘッダ（SMS API関連パラメタ構造体定義：センタ通信(SMCCom)／プローブ情報(SMCorePB)作成用）
//
//------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
//定数定義
//---------------------------------------------------------------------------------
#define	CC_CMN_USERINFO_FILE_NAME			"SMCC_UserInfo.txt"					// ユーザ情報ファイル名文字列

#define CC_CMN_FORMAT_VER_STR_SIZE			(4+1)		// 地図フォーマットバージョン文字列サイズ(メジャーver2桁+マイナーver2桁=4桁)
#define CC_CMN_MAP_VER_STR_SIZE				(8+1)		// 車載機地図バージョン文字列サイズ
#define CC_CMN_APP_VER_STR_SIZE				(16+1)		// 車載機アプリバージョン文字列サイズ
#define CC_CMN_TERMID_STR_SIZE				(12+1)		// 車載機ID文字列サイズ
#define CC_CMN_COMMON_URI_MAX_STR_SIZE		(256)		// センタURI(各APIで共通の前半部分)文字列サイズ
#define CC_CMN_INIT_ITEMID_STR_SIZE			(16+1)		// 初期商品ID文字列サイズ
#define CC_CMN_APPVER_STR_SIZE				(20+1)		// 端末アプリVer文字列サイズ

#define CC_CMN_TOKEN_STR_SIZE				(16+1)		// トークン文字列サイズ
#define CC_CMN_TERM_SIG_STR_SIZE			(16+1)		// センタアクセスキー文字列サイズ
#define CC_CMN_TARGETID_STR_SIZE			(12+1)		// ターゲットID文字列サイズ
#define CC_CMN_USER_SIG_STR_SIZE			(48+1)		// ユーザーアクセスキー文字列サイズ	//文字列サイズは、調整要
// #define CC_CMN_GUID_STR_SIZE				(20+1)		// GUID文字列サイズ					//文字列サイズは、調整要
#define CC_CMN_GUID_STR_SIZE				(64+1)		// GUID文字列サイズ					//guidのhash化に伴う変更。文字列サイズは、調整要
#define CC_CMN_ACT_STATUS_STR_SIZE			(2+1)		// act_status文字列サイズ			//文字列サイズは、調整要

#define CC_CMN_USERNAME_STR_SIZE			(60+1)		// ユーザー名文字列サイズ			//文字列サイズは、調整要
#define CC_CMN_MALEADDR_STR_SIZE			(256+1)		// メールアドレス文字列サイズ		//文字列サイズは、調整要
#define CC_CMN_LOGINID_STR_SIZE				(256+1)		// ログインID文字列サイズ			//文字列サイズは、調整要	// TR3.5 ログインID桁数増加対応 2014.08.25
#define CC_CMN_PASSWORD_STR_SIZE			(512+1)		// パスワード文字列サイズ			//文字列サイズは、調整要
#define CC_CMN_LANG_STR_SIZE				(10+1)		// 言語設定文字列サイズ				//文字列サイズは、調整要
#define CC_CMN_POLICYVER_STR_SIZE			(20+1)		// 利用規約Ver文字列サイズ			//文字列サイズは、調整要
#define CC_CMN_POLICYLANG_STR_SIZE			(6+1)		// 利用規約言語タイプ文字列サイズ	//文字列サイズは、調整要
#define CC_CMN_POLICY_LATE_FLG_STR_SIZE		(1+1)		// policy_late_flg文字列サイズ
#define CC_CMN_AGREEVERSION_STR_SIZE		(20+1)		// 再同意した利用規約のバージョン文字列サイズ
#define CC_CMN_COOKIE_STR_SIZE				(512+1)
#define CC_CMN_RATING_DIALOG_STR_SIZE		(1+1)		// アプリ評価ダイアログ表示フラグ
#define CC_CMN_RATING_PKG_FLG_STR_SIZE		(1+1)		// パッケージフラグ

//---------------------------------------------------------------------------------
//構造体定義
//---------------------------------------------------------------------------------
/**
* @brief 車載機の情報を基とするパラメータ
*/
typedef struct _T_CC_CMN_SMS_API_PRM_NAVI {
	Char	reg_term_id[CC_CMN_TERMID_STR_SIZE];		// 登録用端末ID
	Char	common_uri[CC_CMN_COMMON_URI_MAX_STR_SIZE];	// センタURI(各APIで共通の前半部分)
	Char	sms_sp_uri[CC_CMN_COMMON_URI_MAX_STR_SIZE];	// センタURI(各APIで共通の前半部分)
#if	0	// アラート機能無効
	Char	alert_uri[CC_CMN_COMMON_URI_MAX_STR_SIZE];	// アラート機能用のセンタURI(各APIで共通の前半部分)
	Char	analyze_uri[CC_CMN_COMMON_URI_MAX_STR_SIZE];	// HIF画像解析用のセンタURI
#endif	// アラート機能無効
	Char	init_item_id[CC_CMN_INIT_ITEMID_STR_SIZE];	// 初期商品ID
	Char	appVer[CC_CMN_APPVER_STR_SIZE];				// 端末アプリVer
} T_CC_CMN_SMS_API_PRM_NAVI;

/**
* @brief MUPSの情報を基とするパラメータ
*/
typedef struct _T_CC_CMN_SMS_API_PRM_MUPS {
	Char	common_uri[CC_CMN_COMMON_URI_MAX_STR_SIZE];	// MUPS系API用センタURI(各APIで共通の前半部分)
	Char	token[CC_CMN_TOKEN_STR_SIZE];				// トークン
	Char	term_sig[CC_CMN_TERM_SIG_STR_SIZE];			// センタアクセスキー(token等から生成する)
	Char	new_term_id[CC_CMN_TARGETID_STR_SIZE];		// センター採番新規端末ID (ターゲットID)
	Char	user_sig[CC_CMN_USER_SIG_STR_SIZE];			// ユーザーアクセスキー
	Char	guid[CC_CMN_GUID_STR_SIZE];					// GUID
	Char	act_status[CC_CMN_ACT_STATUS_STR_SIZE];		// アクティベーション状態
} T_CC_CMN_SMS_API_PRM_MUPS;

/**
* @brief ユーザー情報
*/
typedef struct _T_CC_CMN_SMS_API_PRM_USER{
	Char	user_name[CC_CMN_USERNAME_STR_SIZE];		// ユーザー名
	Char	mail_addr[CC_CMN_MALEADDR_STR_SIZE];		// メールアドレス
	Char	login_id[CC_CMN_LOGINID_STR_SIZE];			// ログインID
	Char	password[CC_CMN_PASSWORD_STR_SIZE];			// パスワード
	Char	lang[CC_CMN_LANG_STR_SIZE];					// 言語設定
	Char	policyver[CC_CMN_POLICYVER_STR_SIZE];		// 利用規約Ver
	Char	policylang[CC_CMN_LANG_STR_SIZE];			// 利用規約言語タイプ
	Char	policyLateFlg[CC_CMN_POLICY_LATE_FLG_STR_SIZE];	// 利用規約最新フラグ
	Char	cookie[CC_CMN_COOKIE_STR_SIZE];
	INT32	showRatingDialog;							// アプリ評価依頼ダイアログの表示有無
	INT32	packageFlg;									// パッケージフラグ
} T_CC_CMN_SMS_API_PRM_USER;

/**
* @brief SMS API関連パラメタ
*/
typedef struct _T_CC_CMN_SMS_API_PRM{
	T_CC_CMN_SMS_API_PRM_NAVI ApiPrmNavi;				// 車載機の情報を基とするパラメタ
	T_CC_CMN_SMS_API_PRM_MUPS ApiPrmMups;				// MUPSの情報を基とするパラメタ
	T_CC_CMN_SMS_API_PRM_USER ApiPrmUser;				// ユーザー情報を基にするパラメタ
} T_CC_CMN_SMS_API_PRM;


#endif // #ifndef SMCORESMSAPIPRM_H

