/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCCOM_CMN_H
#define SMCCOM_CMN_H

//------------------------------------------------------------------------------------------------------------------------
//
//	SMCCom共通ヘッダ（内部公開用）
//
//------------------------------------------------------------------------------------------------------------------------

#define CC_DEBUG_MODE		//*******    デバッグ用定義 (正式リリース時は削除すること)   ***********

//---------------------------------------------------------------------------------
//定数定義
//---------------------------------------------------------------------------------
/** URIの基本部分文字列				*/
// ↓ネットワーク接続切替ツールによりセットされた値の取得
#define CC_CMN_XML_BASIC_CENTER_URI				(const char *)(SCC_GetPortalAPI())
#define CC_CMN_XML_BASIC_CENTER_SMS_SP_URI		(const char *)(SCC_GetSmsSpAPI())
#define CC_CMN_XML_BASIC_CENTER_SMS_AUTH_URI	(const char *)(SCC_GetSmsAuthAPI())

/** MUPS系API用のURIの基本部分文字列	*/
// ↓ネットワーク接続切替ツールによりセットされた値の取得
#define CC_CMN_XML_BASIC_MUPS_CENTER_URI	(const char *)(SCC_GetMUPSAPI())

// BASIC認証
#define	SC_CMN_BASIC_AUTH_SMS												// SMSのAPIのBASIC認証(有効)
//#undef	SC_CMN_BASIC_AUTH_SMS												// SMSのAPIのBASIC認証(無効)
#define	SC_CMN_BASIC_AUTH_MUPS												// MUPSのAPIのBASIC認証(有効)
//#undef	SC_CMN_BASIC_AUTH_MUPS												// MUPSのAPIのBASIC認証(無効)
#if (defined(SC_CMN_BASIC_AUTH_SMS) || defined(SC_CMN_BASIC_AUTH_MUPS))
// Basic認証　ネットワーク接続切替ツールによりセットされた値の取得
#define	SC_CMN_BASIC_AUTH_ID		(const char *)(SCC_GetKeyword1())			// BASIC認証ID
#define	SC_CMN_BASIC_AUTH_PWD		(const char *)(SCC_GetKeyword2())			// BASIC認証パスワード
#endif


#define	SCC_MUTEX_INITIALIZER	{}
#ifndef SC_TAG_CC
#define	SC_TAG_CC				"SMCC"
#endif

#define	_STR(x)					#x
#define	_STR2(x)				_STR(x)
#define	__SLINE__				_STR2(__LINE__)
#define	HERE					"FILE : " __FILE__ "(" __SLINE__ ")\n"
#define SCC_LOG_START			(Char*)"[START] %s()\n", __FUNCTION__
#define SCC_LOG_END				(Char*)"[END]   %s()\n", __FUNCTION__

#define USE_IT(x)				(void)(x)	// 未使用引数に対するコンパイル時Warning警告対策

#define	CC_ISCANCEL()			(isCancel)
#define	CC_ISLOGINED()			(isLogined)

#define	CC_ISNULL(val)			((NULL == (val)) ? true : false)
#define CC_ISEOS(val)			((EOS == *(val)) ? true : false)
#define CC_ISINVALIDSTR(val)	(((CC_ISNULL(val)) || (CC_ISEOS(val))) ? true : false)

// API同時実行可能数
#define	CC_CMN_API_CONCURRENCY_MAXNUM		5

#ifdef __SMS_APPLE__
#define	SCC_MAX_PATH						512
#else
#define	SCC_MAX_PATH						260
#endif /* __SMS_APPLE__ */

#define	CC_CMN_USERPOLICY_FILE_NAME			"SMCC_UserPolicy.txt"				// ユーザ規約文章ファイル文字列

#define CC_CMN_XML_NODE_NAME_CIC			"cic_response"				/** <cic_response>					*/
#define CC_CMN_XML_NODE_NAME_ELGG			"elgg"						/** <elgg>							*/
#define CC_CMN_XML_NODE_NAME_STATUS			"status"					/** <status>						*/
#define CC_CMN_XML_NODE_NAME_RESULT			"result"					/** <result>						*/
#define CC_CMN_XML_NODE_NAME_MESSAGE		"message"					/** <message>						*/
#define CC_CMN_XML_NODE_NAME_API_STATUS		"api_status"				/** <api_status>					*/
#define CC_CMN_XML_NODE_NAME_USER_MESSAGE	"user_message"				/** <user_message>					*/
#define CC_CMN_XML_NODE_NAME_ARRAY_ITEM		"array_item"				/** <array_item>					*/
#define CC_CMN_XML_NODE_NAME_TOKEN_REQ		"token_req"					/** <token_req>						*/	// レスポンスのフォーマット仕様変更により不要となるため、削除予定 2014.02.26
#define CC_CMN_XML_NODE_NAME_DEVICEID_REQ	"deviceid_req"				/** <deviceid_req>					*/	// レスポンスのフォーマット仕様変更により不要となるため、削除予定 2014.02.26
#define CC_CMN_XML_NODE_NAME_TERM_REG		"term_reg"					/** <term_reg>						*/	// レスポンスのフォーマット仕様変更により不要となるため、削除予定 2014.02.26
#define CC_CMN_XML_NODE_NAME_USER_REG		"user_reg"					/** <user_reg>						*/	// レスポンスのフォーマット仕様変更により不要となるため、削除予定 2014.02.26
#define CC_CMN_XML_NODE_NAME_AUTH_REQ		"auth_req"					/** <auth_req>						*/	// レスポンスのフォーマット仕様変更により不要となるため、削除予定 2014.02.26
#define CC_CMN_XML_NODE_NAME_TOKEN			"token"						/** <token>							*/
#define CC_CMN_XML_NODE_NAME_DEV_ID			"dev_id"					/** <dev_id>						*/
#define CC_CMN_XML_NODE_NAME_USER_SIG		"user_sig"					/** <user_sig>						*/
#define CC_CMN_XML_NODE_NAME_ACT_STATUS		"act_status"				/** <act_status>					*/
#define CC_CMN_XML_NODE_NAME_GUID			"guid"						/** <guid>							*/
#define CC_CMN_XML_NODE_NAME_POLICY_VER		"policy_ver"				/** <policy_ver>					*/
#define CC_CMN_XML_NODE_NAME_POLICY_LANG	"policy_lang"				/** <policy_lang>					*/
#define CC_CMN_XML_NODE_NAME_POLICY_CONTENT	"policy_content"			/** <policy_content>				*/
#define	CC_CMN_XML_NODE_NAME_GEM_LIKE		"like_cnt"					/** <gem_like>						*/
#define CC_CMN_XML_NODE_NAME_XML			"xml"						/** <xml>							*/
#define CC_CMN_XML_NODE_NAME_ALERT_LIST		"alert_list"				/** <alert_list>					*/
#define CC_CMN_XML_NODE_NAME_ITEM			"item"						/** <item>							*/
#define CC_CMN_XML_NODE_NAME_HIF			"hif"						/** <hif>							*/
#define CC_CMN_XML_NODE_NAME_POLICY_LATE_FLG	"policy_late_flg"		/** <policy_late_flg>				*/
#define CC_CMN_XML_NODE_NAME_LANG			"lang"						/** <lang>							*/
#define CC_CMN_XML_NODE_NAME_DIALOG_FLG		"dialog_flg"				/** <dialog_flg>					*/
#define CC_CMN_XML_NODE_NAME_PKG_FLG		"pkg_flg"					/** <pkg_flg>						*/

#define CC_CMN_APPROVAL_FLG_TRUE			"TRUE"						/** 承認済フラグ：承認済み(TRUE)	*/
#define CC_CMN_APPROVAL_FLG_FALSE			"FALSE"						/** 承認済フラグ：未承認(FALSE)		*/
#define CC_CMN_APPROVAL_FLG_CMP_SIZE		(4)							/** 承認済フラグ文字列比較数("TRUE"の文字数)	*/

#define CC_CMN_RECIVE_BUFFER_SIZE			(1024 * 1024 * 1)			/** 受信バッファサイズ				*/
#define CC_CMN_PROBE_RECIVE_BUFFER_SIZE		(1024 * 100 * 1)			/** プローブ専用受信バッファサイズ	*/
#define CC_CMN_ALERT_RECIVE_BUFFER_SIZE		(1024 * 100 * 1)			/** アラート関連機能用受信バッファサイズ	*/
#define CC_CMN_ANALYZE_RECIVE_BUFFER_SIZE	(1024 * 100 * 1)			/** HIF画像解析用受信バッファサイズ	*/
#define CC_CMN_GCM_RECIVE_BUFFER_SIZE		(1024 * 100 * 1)			/** GCM用受信バッファサイズ	*/
#define CC_CMN_USER_MESSAGE_AREA_SIZE		(512)						/** ユーザーメッセージエリアサイズ	*/

#define CC_CMN_URI_STR_MAX_LEN				(1024+1)					//!<URI生成用文字バッファサイズ

#define CC_CMN_AREA_CD_STR_SIZE				(4+1)						//!<地域コード文字列サイズ
#define CC_CMN_TARGET_MAX_STR_SIZE			(CC_CMN_URI_STR_MAX_LEN)	//!<更新ターゲットエリア文字列バッファ最大サイズ
#define CC_CMN_ELEMENT_P_MAX_STR_SIZE		(700)						//!<Optfile.info用のelement_p文字バッファ列最大サイズ

#define CC_CMN_ORDER_NUM_STR_SIZE			(16+1)						//!<注文番号文字列サイズ
#define CC_CMN_LICENCE_KEY_STR_SIZE			(16+1)						//!<ライセンスキー文字列サイズ
#define CC_CMN_OPT_FILE_ID_STR_SIZE			(64+1)						//!<最適化更新データファイルID文字列サイズ

#define CC_CMN_APPROVAL_FLG_STR_SIZE		(8+1)						/** 認証済フラグ文字列サイズ	*/

#define	CC_CMN_XML_PARSE_DATA_SIZE			(256)						// XML解析の1回あたりのデータ最大長
#define	CC_CMN_XML_BUF_SIZE					(2048)						// XML解析に使用するバッファ最大長
#define	CC_CMN_XML_LARGE_BUF_SIZE			(1024 * 1024)				// XML解析に使用するバッファ最大長

/* URIパラメタ文字数(CIC生成) */
#define	CC_CMN_XML_TOKEN_SIZE				(46+1)						// tokenサイズ
//#define	CC_CMN_XML_DEVID_SIZE				(12+1)						// dev_idサイズ
//#define	CC_CMN_XML_GUID_SIZE				(16+1)						// guidサイズ

// アクティベーション関連
#define CC_CMN_ACTIVATION_OK				"1"							// アクティベーション状態：済み (TR3.5以降)

// ユーザプロフィール
#define CC_CMN_PROFILE_STR_SIZE				6148						// 自己紹介
#define CC_CMN_COMMENT_STR_SIZE				388							// ちょっと一言
#define CC_CMN_ADDRESS_STR_SIZE				9004						// 住所・地域
#define CC_CMN_HOBBY_STR_SIZE				1540						// 趣味
#define CC_CMN_SKIL_STR_SIZE				1540						// 特技
#define CC_CMN_AVATART_STR_SIZE				260							// アバター

// チャット機能
#define	SCC_CHAT_MAXUSER_NUM				100							// 最大ユーザ数 (暫定で最大100とする)
#define	SCC_CHAT_MAXROOM_NUM				100							// 最大ルーム数 (暫定で最大100とする)
#define	SCC_CHAT_MAXCHAT_NUM				50							// 最大チャット取得要求数 (暫定で最大50とする)
#define	SCC_CHAT_MAXCHAR_USERNAME			(60+1)						// ユーザ名サイズ
#define	SCC_CHAT_MAXCHAR_ROOMNO				(11+1)						// ルームNOサイズ
#define	SCC_CHAT_MAXCHAR_ROOMNAME			(75+1)						// ルーム名サイズ
#define	SCC_CHAT_MAXCHAR_LASTDATE			(16+1)						// 最終操作日サイズ
#define	SCC_CHAT_MAXCHAR_URL				(260)						// URLサイズ
#define	SCC_CHAT_MAXCHAR_MESSAGE			(512+1)						// メッセージサイズ (チャット発信時用)
#define	SCC_CHAT_MAXCHAR_MSGTYPE			(3+1)						// メッセージ種別サイズ
#define	SCC_CHAT_MAXCHAR_GEMID				(60+1)						// GEM IDサイズ
#define	SCC_CHAT_MAXCHAR_TRANSACTION		(11+1)						// トランザクションサイズ

// GEM
#define	SCC_GEM_MAXNUM						100
#define	SCC_MAX_ID							64
#define	SCC_MAX_URL							260
#define	SCC_MAX_USERNAME					64
#define	SCC_MAX_TEXT						1540
#define	SCC_MAX_GEM_PICTPATH				SCC_MAX_URL
#define	SCC_MAX_GEM_DATETM					11
#define	SCC_MAX_GEM_DATE					CC_MAX_DATE
#define	SCC_MAX_CIRCLE_GUID					65
#define	SCC_MAX_GROUPID						65
#define	SCC_MAX_GROUPNAME					772
#define	SCC_MAX_LIKE_CNT					11
#define	SCC_MAX_KEYWORD						301
#define	SCC_MAX_ACCESS						32
#define	SCC_GEMTIMELINE_MAXNUM				100

// GEM公開範囲
#define	CC_CMN_GEM_ACCESS_FLG_PRIVATE		0										// 非公開
#define	CC_CMN_GEM_ACCESS_FLG_PUBLIC		1										// 公開
// ファイル削除フラグ
#define	CC_CMN_GEM_PICTURE_NODEL			0										// 削除しない
#define	CC_CMN_GEM_PICTURE_DEL				1										// 削除する
// GEMソート順
#define	CC_CMN_GEM_SORT_NEW					0										// 新着順
#define	CC_CMN_GEM_SORT_LIKE				1										// LIKE順
#define	CC_CMN_GEM_SORT_DISTANCE			2										// 距離順

#define	CC_MAX_DATE							20
#define	CC_MAX_ROOMNO						12
#define	CC_MAX_POSINFO_NUM					30
#define	CC_MAX_GPSTIME						12

// 位置情報公開可否フラグ
#define	CC_CMN_POSITION_FLG_PRIVATE			0										// 位置情報公開しない
#define	CC_CMN_POSITION_FLG_PUBLIC			1										// 位置情報公開

// GEM SPOT
#define	CC_CMN_GEMSPOT_SPOT_ID				21
#define	CC_CMN_GEMSPOT_SPOT_NAME			772
#define	CC_CMN_GEMSPOT_TEL					13
#define	CC_CMN_GEMSPOT_ADDRESS				772
#define	CC_CMN_GEMSPOT_CLS1_NAME			772
#define	CC_CMN_GEMSPOT_CLS2_NAME			772
#define	CC_CMN_GEMSPOT_SPOT_INFO			3073
#define	CC_CMN_GEMSPOT_MAXNUM				100

// GCM
#define	CC_CMN_GCM_REGISTRATION_ID			4097						// 通知先(Registration ID)長
#define	CC_CMN_GCM_PROJECT_NO				16							// Google開発者プロジェクトNo長
#define	CC_CMN_GCM_OPERATION_DEL			0							// 操作：削除
#define	CC_CMN_GCM_OPERATION_REG			1							// 操作：登録
#define	CC_CMN_GCM_NOTIFY_TYPE_CHAT_UNREAD	0							// 通知種別：チャット未読通知
#define	CC_CMN_GCM_PLATFORM_TYPE_ANDROID	0							// デバイス種別：Android

//#define	CC_CMN_BACKUPDATA_SIZE				256
#define	CC_CMN_BACKUPDATA_SIZE				(1024*1024)
#define	CC_CMN_NOTICE_SIZE					CC_CMN_XML_LARGE_BUF_SIZE

#define	CC_CMN_XML_CIC_RES_STS_OK			(0)							// status OK
#define	CC_CMN_XML_CIC_RES_STS_NG			(-1)						// status NG
#define	CC_CMN_XML_RES_MUPS_STS_SIZE		(9)							// MUPSステータスサイズ
#define	CC_CMN_XML_RES_STS_SIZE				(11)						// ステータスサイズ
#define	CC_CMN_XML_RES_STS_CODE_SIZE		(12)						// ステータスコードサイズ

// アラート
#define	SCC_MAX_ALERT_ID					64
#define	SCC_MAX_NAME						64
#define	SCC_MAX_PID							11
#define	SCC_MAX_ALERT_DATETM				11
#define	SCC_MAX_LINK						10
#define	SCC_MIN_ALERT_NUM					1
#define	SCC_MAX_ALERT_NUM					50
#define	SCC_MIN_PARCEL_NUM					1
#define	SCC_MAX_PARCEL_NUM					9

// パーソナルお知らせ情報
#define	SCC_CMN_PRSNLINFO_MAXNUM			50							// 取得最大件数
#define	SCC_CMN_PRSNLINFO_STS_ALL			0							// 既読＋未読
#define	SCC_CMN_PRSNLINFO_STS_READ			1							// 既読のみ
#define	SCC_CMN_PRSNLINFO_STS_UNREAD		2							// 未読のみ

#define	SCC_CMN_PRSNLINFO_TYPE_UPD_READ		1							// 既読更新
#define	SCC_CMN_PRSNLINFO_TYPE_DEL_INFO		2							// お知らせ削除

#define	SCC_CMN_PRSNLINFO_MSG_TITLE			200
#define	SCC_CMN_PRSNLINFO_MSG_DETAIL		2048

// サークル機能
#define	SCC_CMN_GROUP_SEARCH_MAXNUM			100							// 取得最大件数
#define	SCC_CMN_GROUP_TYPE_ALL				0							// 全サークル
#define	SCC_CMN_GROUP_TYPE_OWNER			1							// ユーザが管理しているサークル検索
#define	SCC_CMN_GROUP_TYPE_MEMBER			2							// ユーザが参加しているサークル検索
#define	SCC_CMN_GROUP_TYPE_STR_ALL			"all"						// 全サークル
#define	SCC_CMN_GROUP_TYPE_STR_OWNER		"owner"						// ユーザが管理しているサークル検索
#define	SCC_CMN_GROUP_TYPE_STR_MEMBER		"member"					// ユーザが参加しているサークル検索
#define	SCC_CMN_GROUP_ID					SCC_MAX_GROUPID
#define	SCC_CMN_GROUP_NAME					SCC_MAX_GROUPNAME
#define	SCC_CMN_DISCRIPTION_BRIEF			304

// GEMコメント
#define	SCC_CMN_GEM_COMMENTINFO_MAXNUM		500
#define	SCC_CMN_GEM_COMMENT					772

// GEM Like
#define	SCC_CMN_GEM_LIKEINFO_MAXNUM			500

// 規約
#define	SCC_CMN_POLICY_LATE_FLG_NEW			'0'							// 最新を同意済み
#define	SCC_CMN_POLICY_LATE_FLG_OLD			'1'							// 最新を同意済みではない

// お知らせ情報
#define	SCC_CMN_NOTICE_LIST_MAXNUM			100
#define	SCC_CMN_NOTICE_LIST_STR_SIZE		1024
#define	SCC_CMN_NOTICE_STR_SIZE				(1024 * 1024)
#define	SCC_CMN_NOTICE_TARGET				"app"
#define	SCC_CMN_NOTICE_TYPE_LISTNUM			0
#define	SCC_CMN_NOTICE_TYPE_LIST			1

// ユーザ検索
#define	SCC_USERSEARCH_MAXNUM				100

// サインイン
#define	SCC_CMN_LOGIN_NORMAL				0
#define	SCC_CMN_LOGIN_REAGREE_POLICY		1

// アプリ評価
#define	SCC_CMN_RATING_DIALOG_SHOW			0							// アプリ評価ダイアログを表示する
#define	SCC_CMN_RATING_DIALOG_NONSHOW		1							// アプリ評価ダイアログを表示しない
#define	SCC_CMN_RATING_DIALOG_SHOW_INTERNAL			1					// アプリ評価ダイアログを表示する(内部用)
#define	SCC_CMN_RATING_DIALOG_NONSHOW_INTERNAL		0					// アプリ評価ダイアログを表示しない(内部用)

#define	SCC_CMN_RATING_DO					0							// 評価する
#define	SCC_CMN_RATING_FINISHED				1							// 評価済み
#define	SCC_CMN_RATING_LATER				2							// 後で
#define	SCC_CMN_RATING_NEVER				3							// 評価しない(未サポート)

// 通知メッセージ
#define CC_CMN_MESSAGE_ID_SIZE				(21)						/** メッセージID	*/
#define CC_CMN_MESSAGE_SIZE					(2049)						/** メッセージ	*/

// 運転特性診断
#define CC_CMN_DRIVE_CHECK_START_MIN		0							// 星ポイント最小
#define CC_CMN_DRIVE_CHECK_START_MAX		5							// 星ポイント最大

// マッピング
#define	CC_CMN_MAPPING_SEARCH_MAXNUM		500							// 検索結果 最大取得件数
#define	CC_CMN_MAPPING_HISTORY_MAXNUM		100							// 編集履歴 最大取得件数
#define	CC_CMN_MAPPING_COMMENT_MAXNUM		100							// コメント 最大取得件数
#define	CC_CMN_MAPPING_LIKE_MAXNUM			100							// LIKEを付けたユーザ 最大取得件数
#define	CC_CMN_MAPPING_RATE_MAXNUM			100							// 評価 最大取得件数

#define	CC_CMN_MAPPING_ID_SIZE				64							// マッピングIDサイズ
#define CC_CMN_MAPPING_CIRCLE_GUID_SIZE		64							// サークルIDサイズ
#define CC_CMN_MAPPING_CIRCLE_GUIDS_SIZE	((CC_CMN_MAPPING_CIRCLE_GUID_SIZE * 10) + 10)		// サークルIDサイズ
#define CC_CMN_MAPPING_KEYWORD_SIZE			301							// キーワードサイズ
#define CC_CMN_MAPPING_COMMENT_SIZE			772							// コメントサイズ
#define CC_CMN_MAPPING_COMMENT_ID_SIZE		11							// コメントIDサイズ
#define	CC_CMN_MAPPING_GENRE_NUM			9							// マッピングジャンル数
#define	CC_CMN_MAPPING_URL_SIZE				260							// URLサイズ
#define	CC_CMN_MAPPING_TEXT_SIZE			1540						// テキストサイズ
#define	CC_CMN_MAPPING_DATE_SIZE			11							// 日時サイズ
#define	CC_CMN_MAPPING_GROUPID_SIZE			65							// サークルIDサイズ
#define	CC_CMN_MAPPING_GROUPNAME_SIZE		151							// サークル名サイズ
#define	CC_CMN_MAPPING_ACCESS_SIZE			32							// 公開範囲サイズ
#define	CC_CMN_MAPPING_RATING_SIZE			11							// 評価値サイズ

#define CC_CMN_MAPPING_GENRE_NONE			0							// マッピングジャンル　指定なし
#define CC_CMN_MAPPING_GENRE_GEM			1							// マッピングジャンル　GEM
#define CC_CMN_MAPPING_GENRE_TUBUYAKI		9							// マッピングジャンル　つぶやき
#define CC_CMN_MAPPING_GENRE_DISASTER		10							// マッピングジャンル　災害情報
#define CC_CMN_MAPPING_MODE_NONE			0							// 気分アイコン種別　指定なし
#define CC_CMN_MAPPING_MODE_ENJOY			1							// 気分アイコン種別　楽しい
#define CC_CMN_MAPPING_MODE_SAD				2							// 気分アイコン種別　悲しい
#define CC_CMN_MAPPING_MODE_QUESTION		3							// 気分アイコン種別　質問
#define CC_CMN_MAPPING_MODE_ANGER			4							// 気分アイコン種別　怒り
#define	CC_CMN_MAPPING_PICTURE_NODEL		0							// ファイル削除フラグ　削除しない
#define	CC_CMN_MAPPING_PICTURE_DEL			1							// ファイル削除フラグ　削除する
#define	CC_CMN_MAPPING_PICTURE_NOUPD		2							// ファイル削除フラグ　更新なし
#define	CC_CMN_MAPPING_ACCESS_FLG_PRIVATE	0							// 公開フラグ　本人
#define	CC_CMN_MAPPING_ACCESS_FLG_PUBLIC	1							// 公開フラグ　公開
#define	CC_CMN_MAPPING_ACCESS_FLG_NONE		0							// 本人投稿　指定なし
#define	CC_CMN_MAPPING_ACCESS_FLG_OWN		1							// 本人投稿　本人が投稿したマッピングを対象とする
#define	CC_CMN_MAPPING_FOLLOWER_FLG_NONE	0							// フォロワー投稿　指定なし
#define	CC_CMN_MAPPING_FOLLOWER_FLG_FOLLOW	1							// フォロワー投稿　フォーロしているユーザが投稿したマッピングを対象とする
#define	CC_CMN_MAPPING_RATING_NONE			0							// 評価値　指定なし
#define	CC_CMN_MAPPING_RATING_NO			-1							// 評価値　NO
#define	CC_CMN_MAPPING_RATING_YES			1							// 評価値　YES
#define	CC_CMN_MAPPING_SORT_NEW				0							// ソート順　新着順
#define	CC_CMN_MAPPING_SORT_LIKE			1							// ソート順　LIKE順
#define	CC_CMN_MAPPING_RATE_NO				-1							// 評価　評価しない
#define	CC_CMN_MAPPING_RATE_YES				1							// 評価　評価する
#define	CC_CMN_MAPPING_RATE_NONE			0							// 評価　未評価
#define	CC_CMN_MAPPING_SRCH_RATING_NONE		-1							// 評価値　指定なし

// 駐車場情報
#define CC_CMN_PARKING_STORENAME			256							// 店舗名称
#define CC_CMN_PARKING_PARKINGNAME			256							// 駐車場名称
#define CC_CMN_PARKING_ENTRANCENAME			256							// 対応入口名称

// パッケージマネージャ
#define CC_CMN_PKGMGR_GROUP_MAXNUM			20							// グループ最大数(通常モード)
#define CC_CMN_PKGMGR_MGRGROUP_MAXNUM		1							// グループ最大数(管理者モード)
#define CC_CMN_PKGMGR_PACKAGE_MAXNUM		10							// パッケージ最大数

#define CC_CMN_PKGMGR_PKG_FLG_ON			1							// パッケージあり(ON)
#define CC_CMN_PKGMGR_PKG_FLG_OFF			0							// パッケージなし(OFF)
#define	CC_CMN_PKGMGR_ACCESSCODE			100							// アクセスコード
#define	CC_CMN_PKGMGR_PATH					180							// パス
#define	CC_CMN_PKGMGR_GROUPID				33							// グループID
#define	CC_CMN_PKGMGR_GROUPNAME				301							// グループ名
#define	CC_CMN_PKGMGR_GROUPDESCRIPTION		766							// グループ説明
#define	CC_CMN_PKGMGR_PACKAGEID				33							// パッケージID
#define	CC_CMN_PKGMGR_PACKAGENAME			301							// パッケージ名
#define	CC_CMN_PKGMGR_PACKAGEDESCRIPTION	766							// パッケージ説明
#define	CC_CMN_PKGMGR_AUTHOR				61							// 作成者
#define	CC_CMN_PKGMGR_ICON					65							// アイコン
#define	CC_CMN_PKGMGR_PCKAGE				SCC_MAX_PATH				// パッケージ
#define	CC_CMN_PKGMGR_VERSION				21							// バージョン
#define	CC_CMN_PKGMGR_BACKETNAME			65							// 配置先バケット名
#define	CC_CMN_PKGMGR_ACCESSKEY				21							// S3アクセスキー
#define	CC_CMN_PKGMGR_SECRETKEY				41							// S3シークレットキー
#define	CC_CMN_PKGMGR_MODE_NORMAL			0							// 通常モード
#define	CC_CMN_PKGMGR_MODE_MANAGER			1							// 管理者モード

#define	CC_CMN_PKGCMN_WHITELIST_API_MAXNUM		100						// ホワイトリストに定義可能なAPI数
#define	CC_CMN_PKGCMN_WHITELIST_API_NAME_SIZE	64						// ポータルAPI名の長さ
#define	CC_CMN_PKGCMN_CHKTAMPERINGLIST_FILE_MAXNUM	100					// パッケージ改ざんチェックリストに定義可能なファイル数
#define	CC_CMN_PKGCMN_DOCHECK											// パッケージの改ざんチェックおよび使用可能なポータルAPIをチェックする
																		// ※チェックしたくない場合はコメントアウトすること

// プローブ情報
#define	CC_CMN_PROBEINFO_MAXNUM				10000						// プローブ情報数
#define	CC_CMN_PROBEINFO_TIME				14							// 日時

// 交通情報
#define	CC_CMN_TRAFFICINFO_PARCEL_MAXNUM	36							// パーセル最大数
#define	CC_CMN_TRAFFICINFO_ROAD_KIND_MAXNUM	13							// 道路種別最大数
#define	CC_CMN_TRAFFICINFO_ROAD_KIND_MIN	0							// 道路種別（最少）
#define	CC_CMN_TRAFFICINFO_ROAD_KIND_MAX	12							// 道路種別（最大）
#define	CC_CMN_TRAFFICINFO_SRCID_PROBE		0							// 交通情報 提供情報源（プローブ）
#define	CC_CMN_TRAFFICINFO_SRCID_VICS		1							// 交通情報 提供情報源（VICS）
#define	CC_CMN_TRAFFICINFO_SRCID_PROBE_VICS	2							// 交通情報 提供情報源（プローブ+VICS）
#define	CC_CMN_TRAFFICINFO_RGN_JP			0							// 交通情報 リージョン（日本）
#define	CC_CMN_TRAFFICINFO_RGN_US			1							// 交通情報 リージョン（北米）
#define	CC_CMN_TRAFFICINFO_RGN_EU			3							// 交通情報 リージョン（欧州）
#define	CC_CMN_TRAFFICINFO_RGN_AU			4							// 交通情報 リージョン（豪州）
#define	CC_CMN_TRAFFICINFO_MAP_VERSION		(8 + 128 + 1)				// 地図バージョン

// OAuth連携
#define	CC_CMN_OAUTH_SERVICE_ID				3
#define	CC_CMN_OAUTH_URL_SIZE				2049
#define	CC_CMN_OAUTH_SESSION_ID_SIZE		65

/* 処理結果 */
#define	CC_CMN_RESULT_OK					(e_SC_RESULT_SUCCESS)					// 正常終了
#define	CC_CMN_RESULT_PARAM_ERR				(e_SC_RESULT_BADPARAM)					// パラメタエラー
#define	CC_CMN_RESULT_API_CANSEL			(e_SC_RESULT_CANCEL)					// APIキャンセル (ユーザー主導)
#define	CC_CMN_RESULT_COMM_ERR				(e_SC_RESULT_TCP_COMMUNICATION_ERR)		// 通信エラー
#define	CC_CMN_RESULT_MALLOC_ERR			(e_SC_RESULT_MALLOC_ERR)				// メモリ確保エラー
#define	CC_CMN_RESULT_FILE_OPEN_ERR			(e_SC_RESULT_FILE_OPENERR)				// ファイルオープンエラー
#define	CC_CMN_RESULT_PROC_ERR				(e_SC_RESULT_FAIL)						// 処理エラー

#define	CC_CMN_RESULT_SMCAL_OK				(e_SC_CAL_RESULT_SUCCESS)				// センタ通信ライブラリ系処理正常終了

#define	CC_CMN_SEND_RETRY_CNT				3										// 送信リトライカウント
#define	CC_CMN_RECV_RETRY_CNT				3										// 受信リトライカウント

#define	CHECK_API_STATUS(status)	(('I' == (status[6])) ? true : false)		// APIステータスコードチェック
#define	CHECK_API_STATUS2(status)	(('I' == (status[7])) ? true : false)		// APIステータスコードチェック
#define	CC_CMN_NOT_FOUND					404									// NOT FOUND
#define	CC_CMN_SERVER_STOP					503									// サーバメンテナンス中などによる停止中

#define	CC_CMN_API_AUTH_ERR					"SMSAPIE003"						// 認証エラー
#define	CC_CMN_API_OAUTH_TOKEN_INVALID		"SMSAPIE152"						// OAuth認証Token不正
#define	CC_CMN_API_OAUTH_TOKEN_EXPIRATION	"SMSAPIE153"						// OAuth認証Token期限切れ

#define SC_CMN_APPCONF_SIZE			(1024 * 1024)
#define	CC_CMN_MD5					(16)

typedef 	pthread_mutex_t			SCC_MUTEX;
typedef		sem_t					SCC_SEMAPHORE;

// メモリ管理
#define		SCC_MALLOC(size)		SCC_MEM_Alloc((size), CC_MEM_TYPE_DYNAMIC)
#define		SCC_FREE(ptr)			SCC_MEM_Free((ptr), CC_MEM_TYPE_DYNAMIC)
#define		SCC_MALLOC2(size)		SCC_MEM_Alloc((size), CC_MEM_TYPE_DYNAMIC2)
#define		SCC_FREE2(ptr)			SCC_MEM_Free((ptr), CC_MEM_TYPE_DYNAMIC2)

/* 地図ダウンロード */
#define	SCC_MAPDWL_MAXCHAR_RGNCODE			(20)		// 仕向け地コードサイズ
#define	SCC_MAPDWL_MAXCHAR_RGNNAME			(196)		// 仕向け地名称サイズ
#define	SCC_MAPDWL_MAXCHAR_SECTNAME			(196)		// 地方名称サイズ
#define	SCC_MAPDWL_MAXCHAR_AREANAME			(196)		// 地域名称サイズ
#define	SCC_MAPDWL_MAXCHAR_FLDPATH			(260)		// 地図データフォルダフルパスサイズ
#define	SCC_MAPDWL_MAXCHAR_PROGMSG			(260)		// 進捗状況メッセージサイズ
#define	SCC_MAPDWL_MAXCHAR_DATAKIND			(68)		// データ種別サイズ
#define	SCC_MAPDWL_MAXNUM_DLRGN				(10)		// リージョン別データフォルダリスト数
#define	SCC_MAPDWL_MAXNUM_SECTION			(30)		// 地方最大数
#define	SCC_MAPDWL_MAXNUM_AREA				(100)		// 地域最大数
#define	SCC_MAPDWL_DATA_VERSION				(20)		// データバージョン
#define	SCC_MAPDWL_DATA_KIND				(68)		// データバージョン

#define	SCC_MAPDWL_CONFIG_FOLDER_PATH			"Config/"					// Configフォルダパス文字列
#define	SCC_MAPDWL_APPLICATION_INI_FILE_NAME	"application.ini"			// application.iniファイル名文字列

#define	SCC_AREA_CODE_JPN					"JPN"		// 仕向け地コード：日本
#define	SCC_AREA_CODE_NAM					"NAM"		// 仕向け地コード：北米
#define	SCC_AREA_CODE_EUR					"EUR"		// 仕向け地コード：欧州
#define	SCC_AREA_CODE_NONE					"NONE"		// 仕向け地コード：未定

// AWS S3
// フォルダ
#define	SCC_CMN_AWS_DIR_APPDATA				"AppData/"	// データルート
#define	SCC_CMN_AWS_DIR_DATA				"Data/"		// プロパティデータ
#define	SCC_CMN_AWS_DIR_MAP					"Map/"		// 地図データ
#define	SCC_CMN_AWS_DIR_PACKAGE				"Package/"	// パッケージ

// ファイルパスとフォルダパス
#define	SCC_CMN_MAP_DIR						"jp.co.hitachi.smsfv.aa/Map/"
#define	SCC_CMN_DATA_DIR					"jp.co.hitachi.smsfv.aa/Data/"
#define	SCC_CMN_TEMP_DIR					"jp.co.hitachi.smsfv.aa/Temp/"
#define	SCC_CMN_ROOT_DIR					"jp.co.hitachi.smsfv.aa"

#define	SCC_CMN_TARGZ_FILE_REGION_LIST		"regionListInfo.tar.gz"
#define	SCC_CMN_TARGZ_FILE_REGION_LIST_MD5	"regionListInfo.tar.gz.md5"
#define	SCC_CMN_DB_FILE_REGION_LIST			"regionListInfo.db"
#define SCC_CMN_TARGZ_FILE_DATAVERSIONINFO	"dataVersionInfo.tar.gz"
#define SCC_CMN_TARGZ_FILE_DATAVERSIONINFO_MD5	"dataVersionInfo.tar.gz.md5"
#define SCC_CMN_DB_FILE_DATAVERSIONINFO		"dataVersionInfo.db"
#define	SCC_CMN_DATAVERSION					"dataVersion.txt"
#define	SCC_CMN_DATAVERSION_NEW				"dataVersion_new.txt"
#define	SCC_CMN_DB_FILE_MAP					"Polaris.db"

// サイズ
#define	SCC_SIZE_KB							(1024)
#define	SCC_SIZE_MB							(1024 * 1024)
#define	SCC_SIZE_GB							(1024 * 1024 *1024)

#define SCC_VERSION_SIZE					8

// 進捗通知
#define	SCC_CMN_PROGRESS_MSG_CHECK			"更新をチェック中です"							// 処理が開始するまでの間に表示するメッセージ
#define	SCC_CMN_PROGRESS_MSG_APPDATA_DL		"アプリケーション設定をダウンロード中です"		// appData.tar.gzのダウンロード
#define	SCC_CMN_PROGRESS_MSG_APPDATA_TARGZ	"アプリケーション設定を更新中です"				// appData.tar.gzの解凍中
#define	SCC_CMN_PROGRESS_MSG_RGNMAP_DL		"広域地図をダウンロード中です"					// regionalMap.tar.gzのダウンロード
#define	SCC_CMN_PROGRESS_MSG_RGNMAP_TARGZ	"広域地図を更新中です"							// regionalMap.tar.gzの解凍
#define	SCC_CMN_PROGRESS_MSG_RGNMAP_VERSION	"広域地図を更新中です"							// バージョン更新
#define	SCC_CMN_PROGRESS_MSG_AREAMAP_DL		"の道路地図をダウンロード中です"				// localMapXXXの.tar.gzのダウンロード
//#define	SCC_CMN_PROGRESS_MSG_AREAMAP_TARGZ	"の道路地図を更新中です"						// localMapXXXの.tar.gzの解凍
#define	SCC_CMN_PROGRESS_MSG_AREAMAP_UPD	"の道路地図を更新中です"						// パーセルテーブル更新、ベースバージョンとDLフラグ更新
#define	SCC_CMN_PROGRESS_MSG_AREACLSMAP_DL	"の住所情報をダウンロード中です"			// areaClsXXXの.tar.gzのダウンロード
//#define	SCC_CMN_PROGRESS_MSG_AREACLSMAP_TARGZ	"の住所情報をダウンロード中です"			// areaClsXXXの.tar.gzの解凍
#define	SCC_CMN_PROGRESS_MSG_AREACLSMAP_UPD	"の住所情報をダウンロード中です"					// 地域クラステーブル更新、ベースバージョンとDLフラグ更新

#define	CC_CMN_TEMP_DIR_PATH				"Temp/"
#define	CC_CMN_DATA_DIR_PATH				"Data/"
#define	CC_CMN_MAP_DIR_PATH					"Map/"
#define	CC_CMN_TEMP_DIR_NAME				"Temp"
#define	CC_CMN_DATA_DIR_NAME				"Data"
#define	CC_CMN_MAP_DIR_NAME					"Map"

/******************   バージョン情報関連   ******************/
#define	CC_CMN_DATA_VER_INF_DB_PATH			"AppData/%s/dataVersionInfo.tar.gz"		// バージョン情報DBファイルパス

//#define	CC_CMN_MAP_DATA_VER_NUM			10				// 地図データバージョン数
#define	CC_CMN_DL_MAP_DATA_VER_NUM		10				// ダウンロード地図データバージョン数
//#define	CC_CMN_APP_DATA_VER_NUM			5				// APPデータバージョン数
#define	CC_CMN_DL_APP_DATA_VER_NUM		5				// ダウンロードAPPデータバージョン数


//---------------------------------------------------------------------------------
//列挙型定義
//---------------------------------------------------------------------------------
/*  */
enum RespStatus {
	NODE_NONE,
	NODE_ELGG,
	NODE_ELGG_CHILD,
	NODE_STATUS,
	NODE_RESULT,
	NODE_RESULT_CHILD,
	NODE_MESSAGE,
	NODE_API_STATUS,
	NODE_USER_MESSAGE,
	NODE_ARRAY_ITEM,
	NODE_TOKENREQ,						// レスポンスのフォーマット仕様変更により不要となるため、削除予定 2014.02.26
	NODE_DEVICEIDREQ,					// レスポンスのフォーマット仕様変更により不要となるため、削除予定 2014.02.26
	NODE_TERMREG,						// レスポンスのフォーマット仕様変更により不要となるため、削除予定 2014.02.26
	NODE_USERREG,						// レスポンスのフォーマット仕様変更により不要となるため、削除予定 2014.02.26
	NODE_AUTHREQ,						// レスポンスのフォーマット仕様変更により不要となるため、削除予定 2014.02.26
	NODE_TOKEN,
	NODE_DEV_ID,
	NODE_USER_SIG,
	NODE_ACT_STATUS,
	NODE_GUID,
	NODE_POLICY_VER,
	NODE_POLICY_LANG,
	NODE_POLICY_CONTENT,
	NODE_POLICY_LATE_FLG
};

/**  地図/APPデータダウンロード可否ステータス(更新可能なデータの有無)  **/
enum MapDwldStates {
	CC_CMN_DT_UPDATE_NONE = 0,				// 更新可能なデータなし
	CC_CMN_DT_UPDATE_POSSIBLE,				// 更新可能なデータあり(更新任意)
	CC_CMN_DT_UPDATE_NEED,					// 更新可能なデータあり(更新必須)
	CC_CMN_DT_UPDATE_APPUPDATE_NEED			// ナビアプリの更新が必要(ナビアプリが古い)
};


//---------------------------------------------------------------------------------
//構造体定義
//---------------------------------------------------------------------------------
/**
* @brief SMSレスポンス情報
*/
typedef struct _T_CC_CMN_SMS_RESPONSE_INFO {
	INT32	sts;				// ステータス(OK=0、NG=-1)
	Char	*apiSts;			// APIステータス
	Char	*user_msg;
} T_CC_CMN_SMS_RESPONSE_INFO;

/**
* @brief アプリバージョン
*/
typedef struct _T_CC_CMN_APP_VERSION {
	Char	app_version[20];				// APP_VERSION
	Char	st_version[20];					// VERSION_S
	Char	ed_version[20];					// VERSION_E
} T_CC_CMN_APP_VERSION;

/**
* @brief 端末DBデータバージョン
*/
typedef struct _T_CC_CMN_DEVICE_DATA_VERSION {
	Char	version[20];					// VERSION
} T_CC_CMN_DEVICE_DATA_VERSION;

/**
* @brief バージョン情報 Config
*/
typedef struct _T_CC_CMN_VERSION_INFO_DB {
	T_CC_CMN_APP_VERSION			appDT_app_ver[CC_CMN_DL_APP_DATA_VER_NUM][2];	// アプリデータのアプリバージョン (センタDB)
	Bool							appDT_dwld_flg[CC_CMN_DL_APP_DATA_VER_NUM];		// アプリデータのダウンロード可否フラグ (true:可  false:否)
	Bool							appDT_newDT_flg[CC_CMN_DL_APP_DATA_VER_NUM];	// アプリデータの新規追加データフラグ (true:新規追加  false:既存差換え)
	T_CC_CMN_DEVICE_DATA_VERSION	dev_appDT_ver[CC_CMN_DL_APP_DATA_VER_NUM];		// アプリデータのデータバージョン (端末DB)
	T_CC_CMN_APP_VERSION			mapDT_app_ver[CC_CMN_DL_MAP_DATA_VER_NUM][2];	// 地図データのアプリバージョン (センタDB)
	Bool							mapDT_dwld_flg[CC_CMN_DL_MAP_DATA_VER_NUM];		// 地図データのダウンロード可否フラグ (true:可  false:否)
	Bool							mapDT_newDT_flg[CC_CMN_DL_MAP_DATA_VER_NUM];	// 地図データの新規追加データフラグ (true:新規追加  false:既存差換え)
	T_CC_CMN_DEVICE_DATA_VERSION	dev_mapDT_ver[CC_CMN_DL_MAP_DATA_VER_NUM];		// 地図データのデータバージョン (端末DB)
#if 0
	T_CC_CMN_MAP_DATA_VERSION		map_ver[CC_CMN_MAP_DATA_VER_NUM];			// 地図データバージョン
	T_CC_CMN_DL_MAP_DATA_VERSION	dl_map_ver[CC_CMN_DL_MAP_DATA_VER_NUM];		// ダウンロード地図データバージョン
	T_CC_CMN_APP_DATA_VERSION		app_ver[CC_CMN_APP_DATA_VER_NUM];			// APPデータバージョン
	T_CC_CMN_DL_APP_DATA_VERSION	dl_app_ver[CC_CMN_DL_APP_DATA_VER_NUM];		// ダウンロードAPPデータバージョン
#endif
} T_CC_CMN_VERSION_INFO_DB;

// 規約情報
typedef struct _SMTERMSINFO {
//	Bool		isApproved;
	Char		termsFileName[SCC_MAX_PATH];
	Char		termsLang[CC_CMN_LANG_STR_SIZE];
	Char		termsVersion[CC_CMN_POLICYVER_STR_SIZE];
} SMTERMSINFO;

// GEM情報
typedef struct _SMGEMINFO {
	DOUBLE		lat;
	DOUBLE		lon;
	DOUBLE		alt;
	Char		id[SCC_MAX_ID];
	Char		url[SCC_MAX_URL];
	Char		user[SCC_MAX_USERNAME];
	Char		text[SCC_MAX_TEXT];
	Char		gemPict[SCC_MAX_GEM_PICTPATH];
	Char		gemDatetm[SCC_MAX_GEM_DATE];
	Char		rgstDatetm[SCC_MAX_GEM_DATE];
	Char		altDatetm[SCC_MAX_GEM_DATE];
	Char		lastDatetm[SCC_MAX_GEM_DATE];
	LONG		likeCnt;
	INT32		likeFlg;
	Char		groupId[SCC_MAX_GROUPID];
	Char		groupName[SCC_MAX_GROUPNAME];
	LONG		commentCnt;
	Char		access[SCC_MAX_ACCESS];
	INT32		gemOwner;
	INT32		canDelete;
	Char		guid[CC_CMN_GUID_STR_SIZE];
} SMGEMINFO;

// GEMタイムライン情報
typedef struct _SMGEMTIMELINEINFO {
	DOUBLE		lat;
	DOUBLE		lon;
	DOUBLE		alt;
	Char		id[SCC_MAX_ID];
	Char		url[SCC_MAX_URL];
	Char		user[SCC_MAX_USERNAME];
	Char		text[SCC_MAX_TEXT];
	Char		gemPict[SCC_MAX_GEM_PICTPATH];
	Char		gemDatetm[SCC_MAX_GEM_DATE];
	Char		rgstDatetm[SCC_MAX_GEM_DATE];
	Char		altDatetm[SCC_MAX_GEM_DATE];
	Char		lastDatetm[SCC_MAX_GEM_DATE];
	LONG		likeCnt;
	INT32		likeFlg;
	Char		groupId[SCC_MAX_GROUPID];
	Char		groupName[SCC_MAX_GROUPNAME];
	LONG		commentCnt;
	Char		access[SCC_MAX_ACCESS];
	INT32		gemOwner;
	INT32		canDelete;
	Char		guid[CC_CMN_GUID_STR_SIZE];
} SMGEMTIMELINEINFO;

// SMPOSINFO
typedef struct _SMPOSITIONINFO {
	Char		*roomNo;
	INT32		positionFlg;
	Bool		latFlg;
	Bool		lonFlg;
	Char		reserve[2];
	DOUBLE		lat;
	DOUBLE		lon;
	Char		guid[CC_CMN_GUID_STR_SIZE];
	Char		userName[CC_CMN_USERNAME_STR_SIZE];
	Char		lastAction[CC_MAX_DATE];
} SMPOSITIONINFO;

// 画像のMIME TYPE
typedef enum _CC_IMAGE_MIMETYPE {
	CC_IMAGE_MIMETYPE_NONE = 0,
	CC_IMAGE_MIMETYPE_JPG,
	CC_IMAGE_MIMETYPE_PNG,
	CC_IMAGE_MIMETYPE_GIF
} CC_IMAGE_MIMETYPE;

// ダウンロード異常情報
typedef enum _CC_DLERROR {
#if 0
	CC_DLERROR_NONE = 0,				// 異常情報なし
	CC_DLERROR_DLSTOPNG,				// ダウンロードの中止不可(リトライ必須)
	CC_DLERROR_DLSTOPNG2,			// ダウンロードの中止不可(ただし、最初から更新はやり直し)
	CC_DLERROR_DLSTOPOK,				// ダウンロードの中止可(リトライ任意)
	CC_DLERROR_CANCEL,				// キャンセルによる終了(リトライ任意)
	CC_DLERROR_CANCEL2,				// キャンセルによる終了(リトライ必須)
	CC_DLERROR_OTHERERR				// その他エラー(パラメータエラー、内部エラーなど)
#else
	CC_DLERROR_OK = 0,				// 異常情報なし
	CC_DLERROR_NG					// 異常情報あり
#endif
} CC_DLERROR;

typedef struct _SMDLERRORINFO {
	CC_DLERROR	dlErrInfo;
} SMDLERRORINFO;

/**
* @brief ネットワーク接続先情報
*/
typedef struct _T_CC_CMN_CONNECT_INFO {
	Char portalAPI[256];	// ポータル APIリクエスト送付先
	Char smsSpAPI[256];		// ポータル APIリクエスト送付先
	Char smsAuthAPI[256];	// ポータル APIリクエスト送付先
	Char MUPSAPI[256];		// MUPS APIリクエスト送付先
	Char keyword1[256];		// BASIC認証 ID
	Char keyword2[256];		// BASIC認証 PW
} T_CC_CMN_CONNECT_INFO;

extern T_CC_CMN_CONNECT_INFO connectInfo;

typedef struct _SCC_AUTHINFO {
	Char	*term_id;		// センター採番新規端末ID (ターゲットID)
	Char	*term_sig;		// センタアクセスキー(token等から生成する)
	Char	*user_sig;		// ユーザーアクセスキー
	Char	*guid;			// GUID
} SCC_AUTHINFO;

// マッピング情報
typedef struct _SMMAPPINGINFO {
	DOUBLE	lat;
	DOUBLE	lon;
	LONG	parcelId;
	INT32	x;
	INT32	y;
	INT32	genre;
	Char	mappingId[CC_CMN_MAPPING_ID_SIZE];
	Char	*text;
	Char	*pic;
	INT32	mode;
	Char	*circleGuid;
	INT32	accessFlg;
	INT32	pictureDel;
	INT32	cause;
	Bool	pictureUpd;
	Char	reserve[3];
} SMMAPPINGINFO;

// パッケージ情報の構造体
typedef struct _SMPACKAGEINFO {
	Char	packageId[CC_CMN_PKGMGR_PACKAGEID];					// パッケージID
	Char	packageName[CC_CMN_PKGMGR_PACKAGENAME];				// パッケージ名
	Char	packageDescription[CC_CMN_PKGMGR_PACKAGEDESCRIPTION];	// パッケージ説明
	Char	author[CC_CMN_PKGMGR_AUTHOR];						// 作成者
	Char	contact[CC_CMN_MALEADDR_STR_SIZE];					// 連絡先
	Char	iconFileName[CC_CMN_PKGMGR_ICON];					// アイコンファイル名
	Char	packageFileName[CC_CMN_PKGMGR_PCKAGE];				// パッケージファイル名
	Char	version[CC_CMN_PKGMGR_VERSION];						// バージョン
	INT32	startDatetm;										// 公開開始日
	INT32	endDatetm;											// 公開終了日
	Char	backetName[CC_CMN_PKGMGR_BACKETNAME];				// 配置先バケット名
	Char	accessKey[CC_CMN_PKGMGR_ACCESSKEY];					// S3アクセスキー
	Char	secretKey[CC_CMN_PKGMGR_SECRETKEY];					// S3シークレットキー
	Char	reserve;
} SMPACKAGEINFO;

// パッケージグループ情報の構造体
typedef struct _SMPACKAGEGROUPINFO {
	Char	groupId[CC_CMN_PKGMGR_GROUPID];						// グループID
	Char	groupName[CC_CMN_PKGMGR_GROUPNAME];					// グループ名
	Char	groupDescription[CC_CMN_PKGMGR_GROUPDESCRIPTION];	// グループ説明
	INT32	type;												// パッケージ種別
	INT32	startDatetm;										// パッケージグループ公開開始日
	INT32	endDatetm;											// パッケージグループ公開終了日
	INT32	pkgInfoNum;											// パッケージ数
	SMPACKAGEINFO	pkgInfo[CC_CMN_PKGMGR_PACKAGE_MAXNUM];	// パッケージ一覧
} SMPACKAGEGROUPINFO;

// パッケージ情報の構造体(内部用)
typedef struct _SMPACKAGEINFO_INTERNAL {
//	Char	groupId[CC_CMN_PKGMGR_GROUPID];						// グループID
//	Char	packageId[CC_CMN_PKGMGR_PACKAGEID];					// パッケージID
	Char	accessCode[CC_CMN_PKGMGR_ACCESSCODE+1];				// アクセスコード
	Char	iconFileName[CC_CMN_PKGMGR_ICON];					// アイコンファイル名
	Char	packageFileName[CC_CMN_PKGMGR_PCKAGE];				// パッケージファイル名
	Char	backetName[CC_CMN_PKGMGR_BACKETNAME];				// 配置先バケット名
	Char	accessKey[CC_CMN_PKGMGR_ACCESSKEY];					// S3アクセスキー
	Char	secretKey[CC_CMN_PKGMGR_SECRETKEY];					// S3シークレットキー
	Char	packageVer[CC_CMN_PKGMGR_VERSION];					// パッケージバージョン
	Char	reserve[2];
} SMPACKAGEINFO_INTERNAL;

// 更新データ種別
typedef enum _UPDATE_DATA_KIND {
	SCC_MAPDWL_KIND_MAP = 0,						// 広域地図データ
	SCC_MAPDWL_KIND_DATA,							// プロパティデータ

	SCC_MAPDWL_KIND_NUM = 10						// 将来の拡張用に最大10個にしておく
} UPDATE_DATA_KIND;

// 更新データ有無チェックステータス
typedef enum _upDTcheck_States {
	SCC_MAPDWL_CHECK_NONE = 0,						// 更新可能な地図データ無（端末内地図データが最新）
	SCC_MAPDWL_CHECK_POSSIBLE,						// 更新可能な地図データ有（更新任意）
	SCC_MAPDWL_CHECK_NEED,							// 更新可能な地図データ有（更新必須）
	SCC_MAPDWL_CHECK_RGN_AREA_DWLNONE,				// リージョンデータ／地域データ未ダウンロード
	SCC_MAPDWL_CHECK_NO_DATA,						// 地図データが存在しない
	SCC_MAPDWL_CHECK_APPUPDATE_NEED					// アプリケーションの更新必須
} upDTcheck_States;

// ダウンロード済み仕向け情報
typedef struct _SMDLRGN {
	Char Region[SCC_MAPDWL_MAXCHAR_RGNCODE];			// 仕向け地コード
	Char folder_Path[SCC_MAPDWL_MAXCHAR_FLDPATH];		// 地図データフォルダフルパス
} SMDLRGN;

// 仕向け設定情報構造体
typedef struct _SMRGNSETTING {
	Char    now_Region[SCC_MAPDWL_MAXCHAR_RGNCODE];		// 現在の仕向け
	UINT16	folder_num;									// リージョン別データフォルダ数
	SMDLRGN dt_Folder[SCC_MAPDWL_MAXNUM_DLRGN];			// リージョン別データフォルダリスト
} SMRGNSETTING;

// データ更新情報構造体
typedef struct _SMUPDINFO {
	Char	regionCode[SCC_MAPDWL_MAXCHAR_RGNCODE];		// 仕向け地コード
	Char	installDirPath[SCC_MAX_PATH];				// 地図データフォルダパス
	Char	propertyPath[SCC_MAX_PATH];					// プロパティファイルフォルダパス
	Bool	rgnMapUpdate;								// 広域地図ダウンロード有無フラグ
	Char	reserve[3];
	INT32	updateAreaNum;								// 更新地域数
	INT32	updateArea[SCC_MAPDWL_MAXNUM_AREA];			// ダウンロード地域リスト
} SMUPDINFO;

// リージョン情報構造体
typedef struct _SMRGNINFO {
	Char	regionCode[SCC_MAPDWL_MAXCHAR_RGNCODE];
	Char	regionName[SCC_MAPDWL_MAXCHAR_RGNNAME];
	INT32	tempFileSize;
	INT32	importSize;
} SMRGNINFO;

// 地域情報構造体
typedef struct _SMAREAINFO {
	INT32	sectionCode;
	INT32	areaCode;
	Char	areaName[SCC_MAPDWL_MAXCHAR_AREANAME];
	INT32	tempFileSize;
	INT32	importSize;
	upDTcheck_States	updateStatus;
} SMAREAINFO;

// 地方情報構造体
typedef struct _SMSECINFO {
	INT32	sectionCode;
	Char	sectionName[SCC_MAPDWL_MAXCHAR_SECTNAME];
	INT32	area_num;
	SMAREAINFO	areaInfo[SCC_MAPDWL_MAXNUM_AREA];
} SMSECINFO;

// ダウンロード進捗構造体
typedef struct _SMDLPROGRESS {
	UINT32	doneSize;
	UINT32	totalSize;
	UINT32	completeCount;
	UINT32	totalCount;
	Char	msg[SCC_MAPDWL_MAXCHAR_PROGMSG];
} SMDLPROGRESS;

// ダウンロードデータ情報構造体
typedef struct _SMDWLDATAINFO {
	INT32	areaCode;
	INT32	dataVer;
	INT32	dataType;
	Char	dataKind[SCC_MAPDWL_MAXCHAR_DATAKIND];
	INT32	fileType;
	Char	filePath[SCC_MAX_PATH];
	INT32	pressSize;
	INT32	nonPressSize;
} SMDWLDATAINFO;

// 進捗情報
typedef struct _SMPROGRESSINFO {
	INT64	doneSize;
	INT64	totalSize;
	UINT32	completeCount;
	UINT32	totalCount;
	Char	msg[SCC_MAPDWL_MAXCHAR_PROGMSG];
} SMPROGRESSINFO;

// コールバック関数
typedef Bool (*SCC_CANCEL_FNC)(void);
typedef void (*SCC_PROGRESS_FNC)(UINT32);
typedef struct _SMPROGRESSCBFNC {
	SCC_CANCEL_FNC			cancel;
	SCC_PROGRESS_FNC		progress;
} SMPROGRESSCBFNC;
typedef void (*SCC_PROGRESSINFO_FNC)(const SMDLPROGRESS*);
typedef struct _SMMAPDLCBFNC {
	SCC_CANCEL_FNC			cancel;
	SCC_PROGRESSINFO_FNC	progress;
} SMMAPDLCBFNC;

// アプリバージョン
typedef struct _CC_APP_VERSION {
	INT32	ver1;
	INT32	ver2;
	INT32	ver3;
	INT32	ver4;
} CC_APP_VERSION;

// 交通情報検索条件
typedef struct _SMTRAFFICSRCH {
	UINT32			parcelIdNum;
	UINT32			parcelId[CC_CMN_TRAFFICINFO_PARCEL_MAXNUM];
	UINT32			linkLv;
	UINT32			roadKindNum;
	UChar			roadKind[CC_CMN_TRAFFICINFO_ROAD_KIND_MAXNUM];
	UChar			srcId;
	UChar			rgn;
	Bool			jam;
	Bool			sapa;
	Bool			park;
	Bool			reg;
	Char			reserve[2];
	UINT32			time;
	Char			mapVer[CC_CMN_TRAFFICINFO_MAP_VERSION];
} SMTRAFFICSRCH;

// 交通情報
typedef struct _SMTRAFFICINFO {
	UINT32			fileSize;
	Char			filePath[SCC_MAX_PATH];
} SMTRAFFICINFO;
// パッケージから使用可能なポータルAPIのリスト（ホワイトリスト）
typedef struct _SMPKGWHITELIST {
	Char	apiName[CC_CMN_PKGCMN_WHITELIST_API_NAME_SIZE];
	UINT32	resType;
} SMPKGWHITELIST;
// パッケージのリスト（改ざんチェックリスト）
typedef struct _SMCHKTAMPERING {
	Char	fileName[SCC_MAX_PATH];
	UINT8	md5[CC_CMN_MD5];
} SMCHKTAMPERING;

extern Char	rootDirPath[SCC_MAX_PATH];
extern Char	dataDirPath[SCC_MAX_PATH];
extern Char	configDirPath[SCC_MAX_PATH];
extern Bool isCancel;
extern Bool isCancel__Polling;
extern Bool isLogined;
#endif // #ifndef SMCCOM_CMN_H

