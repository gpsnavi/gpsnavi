/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCALInternal.h"

#define SC_HTTP_DEFAULT_PORTNO			80
#define SC_HTTPS_DEFAULT_PORTNO			443
#define SC_HTTP_PROTOCOL				"http://"
#define SC_HTTPS_PROTOCOL				"https://"
#define SC_SENDBUF_SIZE					(1024 * 1024)
#define SC_RECVBUF_SIZE					(512 * 1024)
#define SC_UPLOAD_MAXNUM				5
#define SC_SEND_DATA_SIZE				SC_SENDBUF_SIZE
#define SC_HEADER_SIZE					1024

#define CHECK_STATUS(status)			(((200 <= (status)) && (300 > (status))) ? true : false)	// ステータスコードチェック
#define GET_HTTP_VERSION(uver, lver)	((10 * (uver)) + (lver))						// HTTPバージョン取得
#define CHECK_HTTP_VERSION(ver)			((10 <= (ver) && 11 >= (ver)) ? true : false)	// HTTPバージョンチェック(1.0～1.1はOK)

#define SC_HTTP_HEAD_ACCEPT				"Accept: text/html,*/*\r\n"
#define SC_HTTP_HEAD_ACCEPT_JSON		"Accept: application/json\r\n"
#define SC_HTTP_HEAD_ACCEPT_LNG			"Accept-Language: ja\r\n"
#define SC_HTTP_HEAD_ACCEPT_ENC			"Accept-Encoding: identity\r\n"
#define SC_HTTP_HEAD_CACHE_CTR			"Cache-Control: no-cache\r\n"
#define SC_HTTP_HEAD_USER_AGENT			"User-Agent: SocialMap_Android\r\n"
#define SC_HTTP_HEAD_BASIC_AUTH			"Authorization: Basic "
#define SC_HTTP_HEAD_USER_CONNECT		"Connection: close\r\n"
#ifdef PROXY_ENABLE
#define	SC_HTTP_HEAD_PROXY_AUTH			"Proxy-Authorization: Basic xxxxxxxxxxxxxxxxxxxxxxxx\r\n"	// basic認証用ヘッダ
																//  ^^^^^^^^^^^^^^^^^^^^^^^^←"id:pass"でbase64エンコードした値(必要に応じて変えること)
#endif
#define SC_HTTP_HEAD_CONTENT_TYPE		"Content-Type: "
#define SC_HTTP_HEAD_CONTENT_TYPE_XML	"text/xml"
#define SC_HTTP_HEAD_CONTENT_TYPE_XML2	"application/xml"
#define SC_HTTP_HEAD_CONTENT_TYPE_BIN	"application/binary"
#define SC_HTTP_HEAD_CONTENT_TYPE_OCTET	"octet-stream"
#define SC_HTTP_HEAD_CONTENT_TYPE_TEXT	"text"
#define SC_HTTP_HEAD_CONTENT_TYPE_ENC	"application/x-www-form-urlencoded"
#define SC_HTTP_HEAD_CONTENT_TYPE_MULTI	"multipart/form-data; boundary="
#define	SC_HTTP_BOUNDARY_HEAD			"-----1842288301\r\n"
#define	SC_HTTP_BOUNDARY				"-------1842288301\r\n"
#define	SC_HTTP_BOUNDARY_END			"\r\n-------1842288301--\r\n"
#define	SC_HTTP_MULTIPART_FORM_DATA		"Content-Disposition: form-data; name="
#define SC_HTTP_HEAD_CONTENT_LEN		"Content-Length: "
#define SC_HTTP_RECV_HEAD_CONTENT_TYPE	"content-type: "
#define SC_HTTP_RECV_HEAD_CONTENT_LEN	"content-length: "
#define SC_HTTP_RECV_HEAD_CHUNKED		"transfer-encoding: chunked"
#define SC_HTTP_RECV_HEAD_COOKIE		"set-cookie: "
#define SC_HTTP_RECV_HEAD_COOKIE_ID		"phplaravelsession="
#define SC_HTTP_RECV_HEAD_COOKIE_ID_MAXLEN	512			// NULL終端文字は含まないサイズ

// AWS
#define SC_HTTP_HEAD_AWS_AUTH			"Authorization: AWS "
#define SC_HTTP_HEAD_AWS_KEY			"key: "						// ファイル名
#define SC_HTTP_HEAD_AWS_ACL			"acl: "						// acl
#define SC_HTTP_HEAD_AWS_POLICY			"policy: "					// ポリシー
#define SC_HTTP_HEAD_AWS_SIGNATURE		"signature: "				// シグネチャ
#define SC_HTTP_HEAD_AWS_ACL_PRIVATE	"private"					// private
#define SC_HTTP_HEAD_AWS_DATE			"Date: "

// ソケット
#define SC_INVALID_SOCKET				-1
#define SC_SOCKET_ERROR					-1

#ifdef	OUTPUT_SEND_RECV_LOG
#define SC_SEND_FILE					"httpsend%05d.bin"//!<HTTP送信データ用ファイル
#define SC_RECV_FILE					"httprecv%05d.bin"//!<HTTP受信データ用ファイル
#define SC_BODY_FILE					"httpbody%05d.bin"//!<HTTP受信データ用ファイル(bodyのみ)
#endif

#define SC_CAL_CONNECT_TIMEOUT			30
#define SC_CAL_DEFAULT_TIMEOUT			60

#define ASCII_ALPHA1_START				0x41		//A
#define ASCII_ALPHA1_END				0x5A		//Z
#define ASCII_ALPHA2_START				0x61		//a
#define ASCII_ALPHA2_END				0x7A		//z
#define ASCII_DIGIT_START				0x30		//0
#define ASCII_DIGIT_END					0x39		//9
#define ASCII_HYPHEN					0x2D		//-
#define ASCII_PERIOD					0x2E		//.
#define ASCII_UNDERSCORE				0x5F		//_
#define ASCII_TILDA						0x7E		//~


//-----------------------------------
// マクロ定義
//-----------------------------------
#ifdef ANDROID
#define LOG_PRINT_START(tag)															\
{																						\
	__android_log_print(ANDROID_LOG_DEBUG, tag,											\
						"[START] %s() \n", __FUNCTION__);								\
}
#define LOG_PRINT_END(tag)																\
{																						\
	__android_log_print(ANDROID_LOG_DEBUG, tag,											\
						"[END]   %s() \n", __FUNCTION__);								\
}
#define LOG_PRINT_ERROR(tag, log)														\
{																						\
	__android_log_print(ANDROID_LOG_ERROR, tag,											\
						"### ERROR ### %s(), %s, FILE : %s(%d) \n",						\
						__FUNCTION__, log, __FILE__, __LINE__ );	\
}
#define LOG_PRINT_ERRORNO(tag, log, errorno)											\
{																						\
	__android_log_print(ANDROID_LOG_ERROR, tag,											\
						"### ERROR ### %s(), %s(0x%08x), FILE : %s(%d) \n",				\
						__FUNCTION__, log, errorno, __FILE__, 							\
						__LINE__ );														\
}
#else
// ANDROID以外は何もしない
#define LOG_PRINT_START(tag)
#define LOG_PRINT_END(tag)
#define LOG_PRINT_ERROR(tag, log)
#define LOG_PRINT_ERRORNO(tag, log, errorno)
#endif	// ANDROID

// ファイルディスクリプタタイプ
typedef enum _E_SC_CAL_FD_TYPE {
	e_SC_CAL_FD_CONNECT,
	e_SC_CAL_FD_READ,
	e_SC_CAL_FD_WRITE
} E_SC_CAL_FD_TYPE;

static E_SC_CAL_RESULT SC_CAL_CreateSocket(SMCAL *cal);
static E_SC_CAL_RESULT SC_CAL_ChkFd(SMCAL *cal, E_SC_CAL_FD_TYPE fdType, INT32 timeout, const SMCALOPT *opt);
static E_SC_CAL_RESULT SC_CAL_GetExpiresGMT(SMCAL *cal, Char *gmt);
static E_SC_CAL_RESULT SC_CAL_CreateHttpGetReq(SMCAL *cal, const Char *url, const Char *hostName, Char *req, UINT32 *reqLen, const SMCALOPT *opt);
static E_SC_CAL_RESULT SC_CAL_CreateHttpPostReq(SMCAL *cal, const Char *url, const Char *hostName, Char *req, UINT32 *reqLen, const Char *data, UINT32 dataLen, const SMCALOPT *opt);
static E_SC_CAL_RESULT SC_CAL_CreateHttpPostReq_Multipart(SMCAL *cal, const Char *url, const Char *hostName, Char *req, UINT32 *reqLen, const SMCALPOSTPARAM *param, UINT32 paramNum, const SMCALOPT *opt);
static E_SC_CAL_RESULT SC_CAL_SendHttpReq(SMCAL *cal, Char *req, UINT32 reqLen, const SMCALOPT *opt);
static E_SC_CAL_RESULT SC_CAL_SendHttpReq_Multipart(SMCAL *cal, Char *req, UINT32 reqLen, SMCALPOSTPARAM *param, UINT32 paramNum, const SMCALOPT *opt);
static E_SC_CAL_RESULT SC_CAL_RecvHttpRes(SMCAL *cal, Char *res, UINT32 resBufSize, UINT32 *resSize, const SMCALOPT *opt);
static INT32 SC_CAL_ValueFromHexChar(Char hex);
static Char SC_CLA_HexCharFromValue(UINT32 value);
static E_SC_CAL_RESULT SC_CAL_GetRequestInternal(SMCAL *cal, const Char *url, const Char *hostName, INT32 portNo, Char *res, UINT32 resBufSize, UINT32 *resSize, const SMCALOPT *opt);
static E_SC_CAL_RESULT SC_CAL_PostRequestInternal(SMCAL *cal, const Char *url, const Char *hostName, INT32 portNo, const void *param1, UINT32 param2, Char *res, UINT32 resBufSize, UINT32 *resSize, const SMCALOPT *opt, Bool isMultipart);
static Bool SC_CAL_IsChunked(const Char *res);
static E_SC_CAL_RESULT SC_CAL_ConnectSSL(SMCAL *cal, const Char *hostName, INT32 portNo);
static E_SC_CAL_RESULT SC_CAL_CreateMultipartBody(SMCAL *cal, SMCALPOSTPARAM *param, UINT32 paramNum, UINT32 *paramIdx, UINT32 *remainSize, Char *body, INT32 bodyBufSize, INT32 *bodyLen, const SMCALOPT *opt);
static UINT32 SC_CAL_GetMultipartBodySize(SMCAL *cal, const SMCALPOSTPARAM *param, UINT32 paramNum);
static void SC_CAL_ToLowerString(Char *s);

/**
 * @brief OpenSSL初期化
 */
void SC_CAL_Initialize_OpenSSL() {
	// 初期化
	ERR_load_BIO_strings();
	SSL_load_error_strings();
	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();

	// SSLの初期化(戻り値は常に1)
	SSL_library_init();
}

/**
 * @brief 初期化
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_Initialize(SMCAL *cal, const Char *logFile)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;

	LOG_PRINT_START(SC_CAL_TAG);

	do {
		// パラメータチェック
		if (NULL == cal) {
			LOG_PRINT_ERROR(SC_CAL_TAG, "param err");
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}

		// 初期化
		memset(cal, 0, sizeof(SMCAL));

		if (NULL != logFile && EOS != logFile[0]) {
			// ディレクトリ作成
			SC_CAL_MakeDir(logFile);
			strcpy(cal->log.logFile, logFile);
		} else {
			cal->log.logFile[0] = EOS;
		}
		cal->log.logType = SC_CAL_LOG_TYPE_BOTH;
		cal->log.logLvMin = SC_CAL_LOG_LV_DEBUG;

		// ログ初期化
		ret = SC_CAL_LOG_Initialize(&cal->log);
		if (e_SC_CAL_RESULT_SUCCESS != ret) {
			LOG_PRINT_ERROR(SC_CAL_TAG, "SC_CAL_LOG_Finalize err");
			break;
		}

		// メモリ確保
		cal->sendBuf = (Char*)malloc(SC_SENDBUF_SIZE + 1);
		if (NULL == cal->sendBuf) {
			LOG_PRINT_ERROR(SC_CAL_TAG, "malloc err[sendBuf]");
			ret = e_SC_CAL_RESULT_MALLOC_ERR;
			break;
		}
		cal->recvBuf = (Char*)malloc(SC_RECVBUF_SIZE + 1);
		if (NULL == cal->recvBuf) {
			LOG_PRINT_ERROR(SC_CAL_TAG, "malloc err[recvBuf]");
			ret = e_SC_CAL_RESULT_MALLOC_ERR;
			break;
		}
	} while (0);

	LOG_PRINT_END(SC_CAL_TAG);

	return (ret);
}

/**
 * @brief 終了化
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_Finalize(SMCAL *cal)
{
	//SMCALLOG	log;
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;

	LOG_PRINT_START(SC_CAL_TAG);

	// メモリ解放
	if (NULL != cal->sendBuf) {
		free(cal->sendBuf);
	}
	if (NULL != cal->recvBuf) {
		free(cal->recvBuf);
	}

	// ログ終了化
	SC_CAL_LOG_Finalize(&cal->log);

	LOG_PRINT_END(SC_CAL_TAG);

	return (ret);
}

/**
 * @brief ホスト名取得
 * @param[in]  url      URL
 * @param[out] hostName ホスト名
 * @param[out] portNo   ポート番号
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_GetHostName(SMCAL *cal, const Char *url, Char *hostName, INT32 *port)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	const Char	*buf = NULL;
	Char	*chr = NULL;
	INT32	len = 0;

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	do {
		// パラメータチェック
		if (NULL == url) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "param err[url], " HERE);
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}
		if (NULL == port) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "param err[port], " HERE);
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}

		// http:// または https:// で始まるかをチェック
		if (0 == strncmp((char*)url, SC_HTTP_PROTOCOL, strlen(SC_HTTP_PROTOCOL)) ||
			0 == strncmp((char*)url, SC_HTTPS_PROTOCOL, strlen(SC_HTTPS_PROTOCOL))) {
			if (0 == strncmp((char*)url, SC_HTTP_PROTOCOL, strlen(SC_HTTP_PROTOCOL))) {
				buf = &url[strlen(SC_HTTP_PROTOCOL)];
			} else {
				buf = &url[strlen(SC_HTTPS_PROTOCOL)];
			}

			chr = strchr(buf, '/');
			if (NULL == chr) {
				chr = (Char*)&buf[strlen(buf)];
			}

			len = (INT32)(chr - buf);
			strncpy((char*)hostName, (char*)buf, len);
			hostName[len] = EOS;
		} else {
			strcpy(hostName, url);
		}

		chr = strchr(hostName, ':');
		if (NULL != chr) {
			*chr = EOS;
			*port = atoi(chr + 1);
		} else {
			if (0 == strncmp((char*)url, SC_HTTPS_PROTOCOL, strlen(SC_HTTPS_PROTOCOL))) {
				*port = SC_HTTPS_DEFAULT_PORTNO;
			} else {
				*port = SC_HTTP_DEFAULT_PORTNO;
			}
		}
	} while (0);

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}

/**
 * @brief 回線接続
 * @param[out] cal      ファイルディスクリプタ(ソケット)
 * @param[in] hostName  HTTPリクエスト先IPアドレス(ドット付10進表記 or ホスト名)
 * @param[in] portNo    ポート番号
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_Connect(SMCAL *cal, const Char *hostName, INT32 portNo, const SMCALOPT *opt)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	struct sockaddr_in	srv = {};
	INT32	val = 1;
	UINT32	**addr = NULL;
	INT32	timeout = 0;

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	do {
		// パラメータチェック
		if (NULL == cal) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "param err[cal], " HERE);
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}
		if (NULL == hostName) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "param err[hostName], " HERE);
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}

		// タイムアウト時間設定
		timeout = SC_CAL_CONNECT_TIMEOUT;

		// ソケット作成
		ret = SC_CAL_CreateSocket(cal);
		if(e_SC_CAL_RESULT_SUCCESS != ret){
			cal->sock = SC_INVALID_SOCKET;
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_CreateSocket err, " HERE);
			break;
		}

		if ((NULL != opt) && (NULL != opt->cancel) && (opt->cancel())) {
			SC_CAL_LOG_WarnPrint(&cal->log, SC_CAL_TAG, "cancel, " HERE);
			ret = e_SC_CAL_RESULT_CANCEL;
			break;
		}

		// 接続先指定用構造体設定
		srv.sin_family = AF_INET;
		srv.sin_addr.s_addr = inet_addr((char*)hostName);
		srv.sin_port = htons(portNo);

		// ノンブロックに変更
		if (-1 == ioctl(cal->sock, FIONBIO, &val)) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "ioctl err, " HERE);
			ret = e_SC_CAL_RESULT_TCP_CONNECT_ERROR;
			break;
		}

		// hostNameがドット付10進表記解析に失敗(ホスト名として扱う)
		if (INADDR_NONE == srv.sin_addr.s_addr) {
			struct hostent	*host = NULL;

			// ホスト名変換
			host = gethostbyname((char*)hostName);
			if (NULL == host) {
				SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "gethostbyname err[%s], " HERE, hostName);
				ret = e_SC_CAL_RESULT_TCP_CONNECT_ERROR;
				break;
			}
			addr = (UINT32 **)host->h_addr_list;

			while (NULL != *addr) {
				if ((NULL != opt) && (NULL != opt->cancel) && (opt->cancel())) {
					SC_CAL_LOG_WarnPrint(&cal->log, SC_CAL_TAG, "cancel, " HERE);
					ret = e_SC_CAL_RESULT_CANCEL;
					break;
				}

				srv.sin_addr.s_addr = *(*addr);

				// 接続
				connect(cal->sock, (struct sockaddr*)&srv, sizeof(srv));
				// fdチェック
				ret = SC_CAL_ChkFd(cal, e_SC_CAL_FD_CONNECT, timeout, opt);
				if (e_SC_CAL_RESULT_SUCCESS == ret) {
					SC_CAL_LOG_InfoPrint(&cal->log, SC_CAL_TAG, (Char*)"connect succuess, " HERE);
					if (cal->isHttps) {
						// SSL接続
						ret = SC_CAL_ConnectSSL(cal, hostName, portNo);
						if (e_SC_CAL_RESULT_SUCCESS != ret) {
							SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, (Char*)"SC_CAL_ConnectSSL err, " HERE);
						}
					}
					break;
				} else if (e_SC_CAL_RESULT_CANCEL == ret) {
					SC_CAL_LOG_WarnPrint(&cal->log, SC_CAL_TAG, (Char*)"connect cancel, " HERE);
					break;
				} else {
					// エラーの場合は他のIPでも試してみる
					// ホスト情報の次のIPアドレス用ポインタ加算
					addr++;
				}
			}
		} else {
			// 接続
			connect(cal->sock, (struct sockaddr*)&srv, sizeof(srv));
			// fdチェック
			ret = SC_CAL_ChkFd(cal, e_SC_CAL_FD_CONNECT, timeout, opt);
			if (e_SC_CAL_RESULT_SUCCESS == ret) {
				SC_CAL_LOG_InfoPrint(&cal->log, SC_CAL_TAG, (Char*)"connect succuess, " HERE);
			} else if (e_SC_CAL_RESULT_CANCEL == ret) {
				SC_CAL_LOG_InfoPrint(&cal->log, SC_CAL_TAG, (Char*)"connect cancel, " HERE);
				break;
			} else {
				SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "connect err, " HERE);
				break;
			}

			if (cal->isHttps) {
				// SSL接続
				ret = SC_CAL_ConnectSSL(cal, hostName, portNo);
				if (e_SC_CAL_RESULT_SUCCESS != ret) {
					SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, (Char*)"SC_CAL_ConnectSSL err, " HERE);
					break;
				}
			}
		}
	} while (0);

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}

/**
 * @brief 回線接続(SSL)
 * @param[out] cal      ファイルディスクリプタ(ソケット)
 * @param[in] hostName  HTTPリクエスト先IPアドレス(ドット付10進表記 or ホスト名)
 * @param[in] portNo    ポート番号
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_ConnectSSL(SMCAL *cal, const Char *hostName, INT32 portNo)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	Char	host[256] = {};
	INT32	sslFd = 0;
	INT32	val = 1;

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	do {
		// SSL_CTX オブジェクト生成
		cal->ssl.ctx = SSL_CTX_new(SSLv23_method());
		if (NULL == cal->ssl.ctx) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, (Char*)"SSL_CTX_new err, " HERE);
			ret = e_SC_CAL_RESULT_TCP_CONNECT_ERROR;
			break;
		}

		// 証明書ロード
//		if (!SSL_CTX_load_verify_locations((SSL_CTX*)cal->ssl.ctx, "証明書ファイルパス", NULL)) {
//			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, (Char*)"SSL_CTX_load_verify_locations err, " HERE);
//			ret = e_SC_CAL_RESULT_TCP_CONNECT_ERROR;
//			break;
//		}

		// 接続
		cal->ssl.bio = BIO_new_ssl_connect((SSL_CTX*)cal->ssl.ctx);
		BIO_get_ssl((BIO*)cal->ssl.bio, (SSL**)&cal->ssl.ssl);
		SSL_set_mode((SSL*)cal->ssl.ssl, SSL_MODE_AUTO_RETRY);

		sprintf(host, "%s:%d", hostName, portNo);

		// ホスト名設定
		BIO_set_conn_hostname((BIO*)cal->ssl.bio, host);
		if (0 >= BIO_do_connect((BIO*)cal->ssl.bio)) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, (Char*)"BIO_do_connect err, " HERE);
			ret = e_SC_CAL_RESULT_TCP_CONNECT_ERROR;
			break;
		}

		// ファイルディスクリプタ取得
		sslFd = SSL_get_fd((SSL*)cal->ssl.ssl);

		if (-1 == ioctl(sslFd, FIONBIO, &val)) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "ioctl err, " HERE);
			ret = e_SC_CAL_RESULT_TCP_CONNECT_ERROR;
			break;
		}

		// 証明書が有効か確認
//		if (X509_V_OK != SSL_get_verify_result((SSL*)cal->ssl.ssl)) {
//			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, (Char*)"SSL_get_verify_result err, " HERE);
//			ret = e_SC_CAL_RESULT_TCP_CONNECT_ERROR;
//			break;
//		}
	} while (0);

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}

/**
 * @brief 回線切断
 * @param[out] cal     ファイルディスクリプタ(ソケット)
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_DisConnect(SMCAL *cal)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	if (NULL != cal) {
		if (cal->isHttps) {
			// SSLの終了
			if (NULL != cal->ssl.bio) {
				BIO_free_all((BIO*)cal->ssl.bio);
			}
			if (NULL != cal->ssl.ctx) {
				SSL_CTX_free((SSL_CTX*)cal->ssl.ctx);
			}
			ERR_free_strings();

			cal->ssl.ssl = NULL;
			cal->ssl.ctx = NULL;
			cal->ssl.bio = NULL;
			cal->isHttps = false;
		}
		// socketの終了
		close(cal->sock);
		cal->sock = 0;
	}

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}

/**
 * @brief GETリクエスト
 * @param[in]  cal
 * @param[in]  url      URL
 * @param[out] res      センタからのレスポンス
 * @param[in]  opt      通信オプション
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_GetRequest(SMCAL *cal,
								  const Char *url,
								  Char *res,
								  UINT32 resBufSize,
								  UINT32 *resSize,
								  const SMCALOPT *opt)
{
	return (SC_CAL_GetRequestInternal(cal, url, NULL, 0, res, resBufSize, resSize, opt));
}

/**
 * @brief GETリクエスト(内部関数)
 * @param[in]  cal
 * @param[in]  url      URL
 * @param[in]  hostName ホスト名(NULLの場合は、URLから取得する。)
 * @param[in]  portNo   ポート番号(ホスト名がNULLの場合は、URLから取得する。取得できない場合は80固定)
 * @param[out] res      センタからのレスポンスバッファ
 * @param[in]  resBufSize   センタからのレスポンスバッファサイズ
 * @param[out] resSize  センタからのレスポンスサイズ
 * @param[in]  opt      通信オプション
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_GetRequestInternal(SMCAL *cal,
										  const Char *url,
										  const Char *hostName,
										  INT32 portNo,
										  Char *res,
										  UINT32 resBufSize,
										  UINT32 *resSize,
										  const SMCALOPT *opt)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	E_SC_CAL_RESULT	ret2 = e_SC_CAL_RESULT_SUCCESS;
	Char	*host = NULL;
	INT32	port = 0;

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	do {
		// パラメータチェック
		if (NULL == cal) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "param err[cal], " HERE);
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}
		if (NULL == url) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "param err[url], " HERE);
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}
		if (NULL == res) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "param err[res], " HERE);
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}

		// 初期化
		memset(cal->sendBuf, 0, SC_SENDBUF_SIZE);

		if ((NULL == hostName) || (EOS == *hostName)) {
			// メモリ確保
			host = (Char*)malloc(1024);
			if (NULL == host) {
				SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "hostname malloc err, " HERE);
				ret = e_SC_CAL_RESULT_MALLOC_ERR;
				break;
			}
			// ホスト名取得
			ret = SC_CAL_GetHostName(cal, url, host, &port);
			if (e_SC_CAL_RESULT_SUCCESS != ret) {
				SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_GetHostName err, " HERE);
				break;
			}
		} else {
			host = (Char*)hostName;
			port = portNo;
		}

		if (0 == strncmp((char*)url, SC_HTTP_PROTOCOL, strlen(SC_HTTP_PROTOCOL))) {
			cal->isHttps = false;
		} else {
			cal->isHttps = true;
		}

		// 回線接続
		ret = SC_CAL_Connect(cal, host, port, opt);
		if (e_SC_CAL_RESULT_SUCCESS != ret) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_Connect err, " HERE);
			break;
		}

		// タイムアウト時間設定
		if ((NULL == opt) || (0 >= opt->timeout)) {
			cal->comTimeout = SC_CAL_DEFAULT_TIMEOUT;
		} else {
			cal->comTimeout = opt->timeout;
		}

		// HTTPリクエスト(GET)ヘッダ生成
		ret = SC_CAL_CreateHttpGetReq(cal, url, host, cal->sendBuf, &cal->sendLen, opt);
		if (e_SC_CAL_RESULT_SUCCESS != ret) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_CreateHttpGetReq err, " HERE);
			break;
		}

		// HTTPリクエスト送信
		ret = SC_CAL_SendHttpReq(cal, cal->sendBuf, cal->sendLen, opt);
		if (e_SC_CAL_RESULT_SUCCESS != ret) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_SendHttpReq err, " HERE);
			break;
		}

		// HTTPレスポンス受信
		ret = SC_CAL_RecvHttpRes(cal, res, resBufSize, resSize, opt);
		if (e_SC_CAL_RESULT_SUCCESS != ret) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_RecvHttpRes err, " HERE);
			break;
		}
	} while(0);

	// 回線切断
	ret2 = SC_CAL_DisConnect(cal);
	if (e_SC_CAL_RESULT_SUCCESS != ret2) {
		SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_DisConnect err, " HERE);
		if (e_SC_CAL_RESULT_SUCCESS == ret) {
			ret = ret2;
		}
	}

	// メモリ解放
	if (NULL != host) {
		free(host);
	}

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}

/**
 * @brief POSTリクエスト
 * @param[in]  cal
 * @param[in]  url          URL
 * @param[in]  hostName     ホスト名
 * @param[in]  portNo       ポート番号
 * @param[in]  data         bodyデータ
 * @param[in]  dataLen      bodyデータ長
 * @param[out] res          センタからのレスポンス
 * @param[out] resBufSize   レスポンス格納バッファサイズ
 * @param[out] resSize      レスポンスサイズ
 * @param[in]  opt          通信オプション
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_PostRequest(SMCAL *cal,
								   const Char *url,
								   const Char *data,
								   UINT32 dataLen,
								   Char *res,
								   UINT32 resBufSize,
								   UINT32 *resSize,
								   const SMCALOPT *opt)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	ret = SC_CAL_PostRequestInternal(cal, url, NULL, 0, data, dataLen, res, resBufSize, resSize, opt, false);

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}

/**
 * @brief POSTリクエスト(Multipart)
 * @param[in]  cal
 * @param[in]  url          URL
 * @param[in]  hostName     ホスト名
 * @param[in]  portNo       ポート番号
 * @param[in]  param        bodyパラメータ
 * @param[in]  paramNum     bodyパラメータ数
 * @param[out] res          センタからのレスポンス
 * @param[out] resBufSize   レスポンス格納バッファサイズ
 * @param[out] resSize      レスポンスサイズ
 * @param[in]  opt          通信オプション
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_PostRequest_Multipart(SMCAL *cal,
											 const Char *url,
											 const SMCALPOSTPARAM *param,
											 UINT32 paramNum,
											 Char *res,
											 UINT32 resBufSize,
											 UINT32 *resSize,
											 const SMCALOPT *opt)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	ret = SC_CAL_PostRequestInternal(cal, url, NULL, 0, param, paramNum, res, resBufSize, resSize, opt, true);

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}

/**
 * @brief POSTリクエスト
 * @param[in]  cal
 * @param[in]  url          URL
 * @param[in]  hostName     ホスト名
 * @param[in]  portNo       ポート番号
 * @param[in]  param        bodyパラメータ
 * @param[in]  paramNum     bodyパラメータ数
 * @param[out] res          センタからのレスポンス
 * @param[out] resBufSize   レスポンス格納バッファサイズ
 * @param[out] resSize      レスポンスサイズ
 * @param[in]  opt          通信オプション
 * @param[in]  isMultipart  Multipartか否か
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_PostRequestInternal(SMCAL *cal,
										   const Char *url,
										   const Char *hostName,
										   INT32 portNo,
										   const void *param1,
										   UINT32 param2,
										   Char *res,
										   UINT32 resBufSize,
										   UINT32 *resSize,
										   const SMCALOPT *opt,
										   Bool isMultipart)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	E_SC_CAL_RESULT	ret2 = e_SC_CAL_RESULT_SUCCESS;
	Char	*host = NULL;
	INT32	port = 0;
	UINT32	num = 0;

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	do {
		// パラメータチェック
		if (NULL == cal) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "param err[cal], " HERE);
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}
		if (NULL == url) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "param err[url], " HERE);
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}
		if (NULL == param1) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "param err[data], " HERE);
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}
		if (0 == param2) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "param err[dataLen], " HERE);
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}
		if (NULL == res) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "param err[res], " HERE);
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}

		// 初期化
		memset(cal->sendBuf, 0, SC_SENDBUF_SIZE);

		if ((NULL == hostName) || (EOS == *hostName)) {
			// メモリ確保
			host = (Char*)malloc(1024);
			if (NULL == host) {
				SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "hostname malloc err, " HERE);
				ret = e_SC_CAL_RESULT_MALLOC_ERR;
				break;
			}
			// ホスト名取得
			ret = SC_CAL_GetHostName(cal, url, host, &port);
			if (e_SC_CAL_RESULT_SUCCESS != ret) {
				SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_GetHostName err, " HERE);
				break;
			}
		} else {
			host = (Char*)hostName;
			port = portNo;
		}

		if (0 == strncmp((char*)url, SC_HTTP_PROTOCOL, strlen(SC_HTTP_PROTOCOL))) {
			cal->isHttps = false;
		} else {
			cal->isHttps = true;
		}

		// 回線接続
		ret = SC_CAL_Connect(cal, host, port, opt);
		if (e_SC_CAL_RESULT_SUCCESS != ret) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_Connect err, " HERE);
			break;
		}

		// タイムアウト時間設定
		if ((NULL == opt) || (0 >= opt->timeout)) {
			cal->comTimeout = SC_CAL_DEFAULT_TIMEOUT;
		} else {
			cal->comTimeout = opt->timeout;
		}

		if (isMultipart) {
			// 初期化
			for (num = 0; num < param2; num++) {
				((SMCALPOSTPARAM*)param1)[num].fp = NULL;
			}

			// HTTPリクエスト(POST)生成(Multipart)
			ret = SC_CAL_CreateHttpPostReq_Multipart(cal, url, host, cal->sendBuf, &cal->sendLen, (SMCALPOSTPARAM*)param1, param2, opt);
			if (e_SC_CAL_RESULT_SUCCESS != ret) {
				SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_CreateHttpPostReq_Multipart err, " HERE);
				break;
			}

			// HTTPリクエスト送信
			ret = SC_CAL_SendHttpReq_Multipart(cal, cal->sendBuf, cal->sendLen, (SMCALPOSTPARAM*)param1, param2, opt);
			if (e_SC_CAL_RESULT_SUCCESS != ret) {
				SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_SendHttpReq_Multipart err, " HERE);
				break;
			}
		} else {
			// HTTPリクエスト(POST)生成
			ret = SC_CAL_CreateHttpPostReq(cal, url, host, cal->sendBuf, &cal->sendLen, (Char*)param1, param2, opt);
			if (e_SC_CAL_RESULT_SUCCESS != ret) {
				SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_CreateHttpPostReq err, " HERE);
				break;
			}

			// HTTPリクエスト送信
			ret = SC_CAL_SendHttpReq(cal, cal->sendBuf, cal->sendLen, opt);
			if (e_SC_CAL_RESULT_SUCCESS != ret) {
				SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_SendHttpReq err, " HERE);
				break;
			}
		}

		// HTTPレスポンス受信
		ret = SC_CAL_RecvHttpRes(cal, res, resBufSize, resSize, opt);
		if (e_SC_CAL_RESULT_SUCCESS != ret) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_RecvHttpRes err, " HERE);
			break;
		}
	} while(0);

	// 回線切断
	ret2 = SC_CAL_DisConnect(cal);
	if (e_SC_CAL_RESULT_SUCCESS != ret2) {
		SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_DisConnect err, " HERE);
		if (e_SC_CAL_RESULT_SUCCESS == ret) {
			ret2 = ret;
		}
	}

	// メモリ解放
	if (NULL != host) {
		free(host);
	}

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}

/**
 * @brief HTTPレスポンスデータ解析
 * @param[in]  res          HTTPレスポンス
 * @param[out] body         HTTPレスポンスボディ
 * @param[out] contextType  Content-Type
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_AnalyzeResponse(SMCAL *cal,
									   const Char *res,
									   UINT32 resSize,
									   const Char **body,
									   UINT32 *bodyLen,
									   E_CONTEXT_TYPE *contextType)
{
	return (SC_CAL_AnalyzeResponseStatus(cal, res, resSize, body, bodyLen, contextType, NULL));
}

/**
 * @brief HTTPレスポンスデータ解析
 * @param[in]  res          HTTPレスポンス
 * @param[out] body         HTTPレスポンスボディ
 * @param[out] contextType  Content-Type
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_AnalyzeResponseStatus(SMCAL *cal,
											 const Char *res,
											 UINT32 resSize,
											 const Char **body,
											 UINT32 *bodyLen,
											 E_CONTEXT_TYPE *contextType,
											 INT32 *httpStatus)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	Char	*str = NULL;
	Char	*resBody = NULL;
	UINT32	bodySize = 0;
	INT32	majorVer = 0;
	INT32	minorVer = 0;
	INT32	status = 0;
	UINT32	i = 0;
	//Bool 	isChunked = false;
	Char	*header = NULL;

	*body = EOS;

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	do {
		// 受信HTTPヘッダ解析
		// レスポンス行の解析
		if (3 != sscanf((char*)res, " HTTP/%d.%d %3d", &majorVer, &minorVer, &status)) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "http res err, " HERE);
			ret = e_SC_CAL_RESULT_TCP_COMMUNICATION_ERR;
			break;
		}
		// ステータスの解析
		if (NULL != httpStatus) {
			*httpStatus = status;
		}
		if (!CHECK_STATUS(status)) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "response status err[%d], " HERE, status);
			ret = e_SC_CAL_RESULT_TCP_COMMUNICATION_ERR;
			break;
		}
		// HTTPバージョンの解析
		if (!CHECK_HTTP_VERSION(GET_HTTP_VERSION(majorVer, minorVer))) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "response http version err, " HERE);
			ret = e_SC_CAL_RESULT_TCP_COMMUNICATION_ERR;
			break;
		}
		// メモリ確保
		header = (Char*)malloc(SC_HEADER_SIZE);
		if (NULL == header) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "malloc err, " HERE);
			ret = e_SC_CAL_RESULT_MALLOC_ERR;
			break;
		}

		// BODY部開始位置を探索
		for (i = 0 ; i < resSize; i++) {
			// 改行2連続の直後がBODY部(CRLFが改行)
			if (('\r' == res[i]) && ('\n' == res[i+1]) && ('\r' == res[i+2]) && ('\n' == res[i+3])) {
				resBody = (Char*)&res[i + 4];
				bodySize = resSize - (i + 4);
				memcpy(header, res, i);
				header[i] = EOS;
				break;
			}
			// 改行2連続の直後がBODY部(LFが改行)
			if (('\n' == res[i]) && ('\n' == res[i+1])) {
				resBody = (Char*)&res[i + 2];
				bodySize = resSize - (i + 2);
				memcpy(header, res, i);
				header[i] = EOS;
				break;
			}
		}

		// 大文字を小文字に変換
		SC_CAL_ToLowerString(header);

		// Content-Typeの解析
		str = strstr(header, (Char*)SC_HTTP_RECV_HEAD_CONTENT_TYPE);
		if (NULL != str) {
			str += (INT32)strlen(SC_HTTP_RECV_HEAD_CONTENT_TYPE);
			if (0 == strncmp((char*)str, (char*)SC_HTTP_HEAD_CONTENT_TYPE_XML, strlen(SC_HTTP_HEAD_CONTENT_TYPE_XML))) {
				// text/xml
				*contextType = E_TEXT_XML;
			} else if (0 == strncmp((char*)str, (char*)SC_HTTP_HEAD_CONTENT_TYPE_XML2, strlen(SC_HTTP_HEAD_CONTENT_TYPE_XML2))) {
				// application/xml
				*contextType = E_TEXT_XML;
			} else if ((0 == strncmp((char*)str, (char*)SC_HTTP_HEAD_CONTENT_TYPE_BIN, strlen(SC_HTTP_HEAD_CONTENT_TYPE_BIN))) ||
					   (0 == strncmp((char*)str, (char*)SC_HTTP_HEAD_CONTENT_TYPE_OCTET, strlen(SC_HTTP_HEAD_CONTENT_TYPE_OCTET)))) {
				// application/binary or octet-stream
				*contextType = E_APP_BIN;
			} else {
				*contextType = E_OTHER;
			}
		} else {
			*contextType = E_OTHER;
		}

		// Content-Lengthの解析
		str = strstr(header, (Char*)SC_HTTP_RECV_HEAD_CONTENT_LEN);
		if (NULL != str) {
			// Content-Lengthがある場合のみ、BODY部の長さをチェックする
			str += (INT32)strlen(SC_HTTP_RECV_HEAD_CONTENT_LEN);
			if (bodySize != atol(str)) {
				SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "Content-Length err(bodySize=%u, Content-Length=%ld), " HERE, bodySize, atol(str));
				ret = e_SC_CAL_RESULT_TCP_COMMUNICATION_ERR;
				break;
			}
		}

		*body = resBody;
		*bodyLen = bodySize;
	} while (0);

	if (NULL != header) {
		free(header);
	}

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}

/**
 * @brief ソケット作成
 * @param[out] cal      ファイルディスクリプタ(ソケット)
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_CreateSocket(SMCAL *cal)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	do {
		// ソケット作成
		cal->sock = socket(AF_INET, SOCK_STREAM, 0);
		if (SC_INVALID_SOCKET == cal->sock) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "socket err, " HERE);
			ret = e_SC_CAL_RESULT_TCP_CONNECT_ERROR;
			break;
		}
	} while (0);

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}

/**
 * @brief ファイルディスクリプタチェック
 * @param[in] cal      ファイルディスクリプタ(ソケット)
 * @param[in] fdType    ファイルディスクリプタの種類
 * @param[in] timeout   タイムアウト秒数
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_ChkFd(SMCAL *cal, E_SC_CAL_FD_TYPE fdType, INT32 timeout, const SMCALOPT *opt)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_TCP_TIMEOUT;
	INT32	cnt = 0;
	struct timeval	tv = {};
	fd_set	fds = {};
	INT32	n = 0;
	INT32	sslFd = 0;

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	// ディスクリプタのチェック
	for (cnt = 0; cnt < timeout; cnt++) {
		if ((NULL != opt) && (NULL != opt->cancel) && (opt->cancel())) {
			SC_CAL_LOG_WarnPrint(&cal->log, SC_CAL_TAG, "cancel, " HERE);
			ret = e_SC_CAL_RESULT_CANCEL;
			break;
		}

		if ((e_SC_CAL_FD_CONNECT == fdType) || (!cal->isHttps)) {
			// 使用可否判定処理
			FD_ZERO(&fds);
			FD_SET(cal->sock, &fds);
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			if (e_SC_CAL_FD_READ == fdType) {
				n = select(cal->sock + 1, &fds, NULL, NULL, &tv);
			} else {
				n = select(cal->sock + 1, NULL, &fds, NULL, &tv);
			}
			if ((0 < n) && (0 != FD_ISSET(cal->sock, &fds))) {
				// 使用可能な場合
				ret = e_SC_CAL_RESULT_SUCCESS;
				break;
			}
		} else {
			// ファイルディスクリプタ取得
			sslFd = SSL_get_fd(cal->ssl.ssl);
			FD_ZERO(&fds);
			FD_SET(sslFd, &fds);
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			if (e_SC_CAL_FD_READ == fdType) {
				n = select(sslFd + 1, &fds, NULL, NULL, &tv);
			} else {
				n = select(sslFd + 1, NULL, &fds, NULL, &tv);
			}
			if ((0 < n) &&
				((0 < BIO_pending(cal->ssl.bio)) ||
				 (0 != FD_ISSET(sslFd, &fds)))) {
				// 使用可能な場合
				ret = e_SC_CAL_RESULT_SUCCESS;
				break;
			}
		}
	}

	// タイムアウトで抜けた場合
	if (e_SC_CAL_RESULT_TCP_TIMEOUT == ret) {
		SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "timeout err, " HERE);
	}

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}

/**
 * @brief GET送信HTTPヘッダ生成
 * @param[in]  url      URL
 * @param[in]  hostName ホスト名
 * @param[out] req      リクエスト
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_CreateHttpGetReq(SMCAL *cal,
										const Char *url,
										const Char *hostName,
										Char *req,
										UINT32 *reqLen,
										const SMCALOPT *opt)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	Char	gmt[64];
	UChar	*baseStr = NULL;
	UChar	*base64 = NULL;
	INT32	len = 0;
	const Char	*url2 = NULL;
	const Char	*str = NULL;

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	// GMT時刻取得
	if (e_SC_CAL_RESULT_SUCCESS != SC_CAL_GetExpiresGMT(cal, gmt)) {
		SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_GetExpiresGMT err, " HERE);
		return (e_SC_CAL_RESULT_FAIL);
	}

	// ホスト名検索
	str = strstr(url, hostName);
	if (NULL != str) {
		url2 = str + strlen(hostName);
	} else {
		url2 = url;
	}

	if ((NULL != opt) && (opt->isResAcceptJson)) {
		sprintf((char*)req,
				"GET %s HTTP/1.1\r\n%s%s%s%sExpires: %s GMT\r\n",
				url2,
				SC_HTTP_HEAD_ACCEPT_JSON,
				SC_HTTP_HEAD_ACCEPT_LNG,
				SC_HTTP_HEAD_ACCEPT_ENC,
				SC_HTTP_HEAD_CACHE_CTR,
				gmt									// GMT
		);
	} else {
		sprintf((char*)req,
				"GET %s HTTP/1.1\r\n%s%s%s%sExpires: %s GMT\r\n",
				url2,
				SC_HTTP_HEAD_ACCEPT,
				SC_HTTP_HEAD_ACCEPT_LNG,
				SC_HTTP_HEAD_ACCEPT_ENC,
				SC_HTTP_HEAD_CACHE_CTR,
				gmt									// GMT
		);
	}

	if ((NULL != opt) && (opt->isBasicAuth)) {
		// Basic認証
		// Base64エンコードの文字列長
		len = strlen(opt->basic.basicAuthId) + strlen(opt->basic.basicAuthPwd) + 1 + 1;
		// Base64エンコードの文字列格納メモリ確保
		baseStr = (UChar*)malloc(len);
		if (NULL == baseStr) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "malloc err, " HERE);
			return (e_SC_CAL_RESULT_MALLOC_ERR);
		}
		sprintf(baseStr, "%s:%s", opt->basic.basicAuthId, opt->basic.basicAuthPwd);

		// Base64エンコード
		ret = SC_CAL_Base64Encode(cal, baseStr, strlen((char *)baseStr), &base64);
		if (e_SC_CAL_RESULT_SUCCESS != ret) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_Base64Encode err, " HERE);
			free(baseStr);
			if (NULL != base64) {
				free(base64);
			}
			return (ret);
		}
		sprintf((char*)&req[strlen(req)],
				"%s%s\r\n",
				SC_HTTP_HEAD_BASIC_AUTH,
				base64
		);
		free(baseStr);
		free(base64);
	}

	// AWS
	if ((NULL != opt) && (opt->isAws)) {
		sprintf((char*)&req[strlen(req)],
				"%s%s\r\n%s%s GMT\r\n",
				SC_HTTP_HEAD_AWS_ACL,
				SC_HTTP_HEAD_AWS_ACL_PRIVATE,
				SC_HTTP_HEAD_AWS_DATE,
				gmt
		);
	}

	if ((NULL != opt) && (opt->isResume)) {
		sprintf((char*)&req[strlen(req)],
				"Range: bytes=%u-%u\r\n",
				opt->resumeStratPos,
				opt->resumeEndPos
		);
	}

	sprintf((char*)&req[strlen(req)],
#ifdef PROXY_ENABLE
			"%s%sHost: %s\r\n%s\r\n",
#else
			"%sHost: %s\r\n%s\r\n",
#endif
#ifdef PROXY_ENABLE
			SC_HTTP_HEAD_PROXY_AUTH,
#endif
			SC_HTTP_HEAD_USER_AGENT,
			hostName,							// ホスト名
			SC_HTTP_HEAD_USER_CONNECT
	);

	*reqLen = strlen(req);

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}

/**
 * @brief POST送信HTTPヘッダ生成
 * @param[in]  url      URL
 * @param[in]  hostName ホスト名
 * @param[out] req      リクエスト
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_CreateHttpPostReq(SMCAL *cal,
										 const Char *url,
										 const Char *hostName,
										 Char *req,
										 UINT32 *reqLen,
										 const Char *data,
										 UINT32 dataLen,
										 const SMCALOPT *opt)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	Char	gmt[64];
	UChar	*baseStr = NULL;
	UChar	*base64 = NULL;
	INT32	len = 0;
	const Char	*url2 = NULL;
	const Char	*str = NULL;

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	do {
		// GMT時刻取得
		if (e_SC_CAL_RESULT_SUCCESS != SC_CAL_GetExpiresGMT(cal, gmt)) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_GetExpiresGMT err, " HERE);
			ret = e_SC_CAL_RESULT_FAIL;
			break;
		}

		// ホスト名検索
		str = strstr(url, hostName);
		if (NULL != str) {
			url2 = str + strlen(hostName);
		} else {
			url2 = url;
		}

		// 送信バッファに送信HTTPデータを設定
		if ((NULL != opt) && (opt->isResAcceptJson)) {
			sprintf((char*)req,
					"POST %s HTTP/1.1\r\nHost: %s\r\n%s%s%s%sExpires: %s GMT\r\n",
					url2,
					hostName,							// ホスト名
					SC_HTTP_HEAD_ACCEPT_JSON,
					SC_HTTP_HEAD_ACCEPT_LNG,
					SC_HTTP_HEAD_ACCEPT_ENC,
					SC_HTTP_HEAD_CACHE_CTR,
					gmt);								// GMT
		} else {
			sprintf((char*)req,
					"POST %s HTTP/1.1\r\nHost: %s\r\n%s%s%s%sExpires: %s GMT\r\n",
					url2,
					hostName,							// ホスト名
					SC_HTTP_HEAD_ACCEPT,
					SC_HTTP_HEAD_ACCEPT_LNG,
					SC_HTTP_HEAD_ACCEPT_ENC,
					SC_HTTP_HEAD_CACHE_CTR,
					gmt);								// GMT
		}

		// Basic認証
		if ((NULL != opt) && (opt->isBasicAuth)) {
			// Base64エンコードの文字列長
			len = strlen(opt->basic.basicAuthId) + strlen(opt->basic.basicAuthPwd) + 1 + 1;
			// Base64エンコードの文字列格納メモリ確保
			baseStr = (UChar*)malloc(len);
			if (NULL == baseStr) {
				SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "malloc err, " HERE);
				ret = e_SC_CAL_RESULT_MALLOC_ERR;
				break;
			}
			sprintf(baseStr, "%s:%s", opt->basic.basicAuthId, opt->basic.basicAuthPwd);

			// Base64エンコード
			ret = SC_CAL_Base64Encode(cal, baseStr, strlen((char *)baseStr), &base64);
			if (e_SC_CAL_RESULT_SUCCESS != ret) {
				SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_Base64Encode err, " HERE);
				break;
			}

			sprintf((char*)&req[strlen(req)],
					"%s%s\r\n",
					SC_HTTP_HEAD_BASIC_AUTH,
					base64
			);
		}

		sprintf((char*)&req[strlen(req)],
#ifdef PROXY_ENABLE
				"%s%s%s%s%s\r\n%s%d\r\n\r\n",
#else
				"%s%s%s%s\r\n%s%d\r\n\r\n",
#endif
#ifdef PROXY_ENABLE
				SC_HTTP_HEAD_PROXY_AUTH,
#endif
				SC_HTTP_HEAD_USER_AGENT,
				SC_HTTP_HEAD_USER_CONNECT,
				SC_HTTP_HEAD_CONTENT_TYPE,
				SC_HTTP_HEAD_CONTENT_TYPE_ENC,
				SC_HTTP_HEAD_CONTENT_LEN,
				dataLen
		);

		// アップロードデータ
		len = strlen(req);
		memcpy(&req[len], data, dataLen);

		*reqLen = (len + dataLen);
	} while (0);

	if (NULL != baseStr) {
		free(baseStr);
	}
	if (NULL != base64) {
		free(base64);
	}
	return (ret);

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}

/**
 * @brief POST送信HTTPヘッダ生成(Multipart)
 * @param[in]  url      URL
 * @param[in]  hostName ホスト名
 * @param[out] req      リクエスト
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_CreateHttpPostReq_Multipart(SMCAL *cal,
										 		   const Char *url,
										 		   const Char *hostName,
										 		   Char *req,
										 		   UINT32 *reqLen,
										 		   const SMCALPOSTPARAM *param,
										 		   UINT32 paramNum,
										 		  const SMCALOPT *opt)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	Char	gmt[64];
	UChar	*baseStr = NULL;
	UChar	*base64 = NULL;
	UINT32	len = 0;
	UINT32	bodyLen = 0;
	//UINT32	num = 0;
	const Char	*url2 = NULL;
	const Char	*str = NULL;

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	do {
		// GMT時刻取得
		if (e_SC_CAL_RESULT_SUCCESS != SC_CAL_GetExpiresGMT(cal, gmt)) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_GetExpiresGMT err, " HERE);
			ret = e_SC_CAL_RESULT_FAIL;
			break;
		}

		// bodyサイズ
		bodyLen = SC_CAL_GetMultipartBodySize(cal, param, paramNum);

		// ホスト名検索
		str = strstr(url, hostName);
		if (NULL != str) {
			url2 = str + strlen(hostName);
		} else {
			url2 = url;
		}

		// 送信バッファに送信HTTPデータを設定
		if ((NULL != opt) && (opt->isResAcceptJson)) {
			sprintf((char*)req,
					"POST %s HTTP/1.1\r\nHost: %s\r\n%s%s%s%sExpires: %s GMT\r\n",
					url2,
					hostName,							// ホスト名
					SC_HTTP_HEAD_ACCEPT_JSON,
					SC_HTTP_HEAD_ACCEPT_LNG,
					SC_HTTP_HEAD_ACCEPT_ENC,
					SC_HTTP_HEAD_CACHE_CTR,
					gmt);								// GMT
		} else {
			sprintf((char*)req,
					"POST %s HTTP/1.1\r\nHost: %s\r\n%s%s%s%sExpires: %s GMT\r\n",
					url2,
					hostName,							// ホスト名
					SC_HTTP_HEAD_ACCEPT,
					SC_HTTP_HEAD_ACCEPT_LNG,
					SC_HTTP_HEAD_ACCEPT_ENC,
					SC_HTTP_HEAD_CACHE_CTR,
					gmt);								// GMT
		}

		// Basic認証
		if ((NULL != opt) && (opt->isBasicAuth)) {
			// Base64エンコードの文字列長
			len = strlen(opt->basic.basicAuthId) + strlen(opt->basic.basicAuthPwd) + 1 + 1;
			// Base64エンコードの文字列格納メモリ確保
			baseStr = (UChar*)malloc(len);
			if (NULL == baseStr) {
				SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "malloc err, " HERE);
				ret = e_SC_CAL_RESULT_MALLOC_ERR;
				break;
			}
			sprintf(baseStr, "%s:%s", opt->basic.basicAuthId, opt->basic.basicAuthPwd);

			// Base64エンコード
			ret = SC_CAL_Base64Encode(cal, baseStr, strlen((char *)baseStr), &base64);
			if (e_SC_CAL_RESULT_SUCCESS != ret) {
				SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_Base64Encode err, " HERE);
				break;
			}
			sprintf((char*)&req[strlen(req)],
					"%s%s\r\n",
					SC_HTTP_HEAD_BASIC_AUTH,
					base64
			);
		}

		sprintf((char*)&req[strlen(req)],
#ifdef PROXY_ENABLE
				"%s%s%s%s%s%s%s%d\r\n\r\n",
#else
				"%s%s%s%s%s%s%d\r\n\r\n",
#endif
#ifdef PROXY_ENABLE
				SC_HTTP_HEAD_PROXY_AUTH,
#endif
				SC_HTTP_HEAD_USER_AGENT,
				SC_HTTP_HEAD_USER_CONNECT,
				SC_HTTP_HEAD_CONTENT_TYPE,
				SC_HTTP_HEAD_CONTENT_TYPE_MULTI,
				SC_HTTP_BOUNDARY_HEAD,
				SC_HTTP_HEAD_CONTENT_LEN,
				bodyLen
		);

		*reqLen = strlen(req) + bodyLen;
	} while (0);

	if (NULL != baseStr) {
		free(baseStr);
	}
	if (NULL != base64) {
		free(base64);
	}

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}

/**
 * @brief 世界標準時刻取得処理
 * @param[out] gmt  世界標準時刻(文字列)
 * @return 処理結果(E_SC_CAL_RESULT)
 * @pre  gmtには世界標準時刻(文字列)を格納するのに十分な領域が確保済みであること
 * @note gmtの書式はMUPS通信基本仕様のサンプルに従っている。
 * @note なおgmtに改行文字は付加しない。
 * @warning 本関数はtime関数群のプラットフォーム依存の問題が解決されない場合
 * @warning 2038年以降のGMT算出においては別途対応が必要となる可能性がある。
*/
E_SC_CAL_RESULT SC_CAL_GetExpiresGMT(SMCAL *cal, Char *gmt)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	time_t	rTime;
	time_t	aTime;
	struct tm	s_time = {};

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	do {
		// time_t型の取得
		rTime= time(&aTime);
		// tm構造体の取得
		gmtime_r(&rTime, &s_time);

		// 書式を指定してGMT文字列生成
		if (0 == strftime((char*)gmt, 64, "%a, %d %b %Y %H:%M:%S", &s_time)) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "strftime err, " HERE);
			ret = e_SC_CAL_RESULT_FAIL;
			break;
		}
	} while (0);

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}

/**
 * @brief HTTPリクエスト送信
 * @param[in] cal      ファイルディスクリプタ(ソケット)
 * @param[in] hostName  ホスト名
 * @param[in] req       リクエスト
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_SendHttpReq(SMCAL *cal, Char *req, UINT32 reqLen, const SMCALOPT *opt)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	INT32	size = 0;
	UINT32 allSendSize = 0;		// 送信対象データ総サイズ
	UINT32 sendSize = 0;		// 送信済みデータサイズ

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	do {
		// パラメータチェック
		if (NULL == req) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "param err[req], " HERE);
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}

		// 送信対象データ総サイズ
		allSendSize = reqLen;

		while (sendSize < allSendSize) {
			if ((NULL != opt) && (NULL != opt->cancel) && (opt->cancel())) {
				SC_CAL_LOG_WarnPrint(&cal->log, SC_CAL_TAG, "cancel, " HERE);
				ret = e_SC_CAL_RESULT_CANCEL;
				break;
			}

			// fdチェック(送信側)
			ret = SC_CAL_ChkFd(cal, e_SC_CAL_FD_WRITE, cal->comTimeout, opt);
			if (e_SC_CAL_RESULT_SUCCESS != ret) {
				SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_ChkFd err, " HERE);
				break;
			}

			if (!cal->isHttps) {
				// データ送信処理
				size = send(cal->sock, (char*)&(req[sendSize]), (allSendSize - sendSize), 0);
				if (0 > size) {
					// 送信失敗
					SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "send err(0x%08x), " HERE, errno);
					ret = e_SC_CAL_RESULT_TCP_COMMUNICATION_ERR;
					break;
				}
			} else {
				// データ送信処理
				size = BIO_write((BIO*)cal->ssl.bio, (char*)&(req[sendSize]), (allSendSize - sendSize));
				if (0 > size) {
					size = BIO_should_retry((BIO*)cal->ssl.bio);
					if (!size) {
						// 送信失敗
						SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "BIO_write err, " HERE);
						ret = e_SC_CAL_RESULT_TCP_COMMUNICATION_ERR;
						break;
					}
					SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, "BIO_write retry, " HERE);
					continue;
				}
			}

			//送信済みサイズの更新
			sendSize += size;
		}
		if (e_SC_CAL_RESULT_SUCCESS != ret) {
			break;
		}

		// デバッグ用に送信結果を標準出力へ表示
		SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, (Char*)"send=%s", req);

#ifdef OUTPUT_SEND_RECV_LOG
		// 送信データをファイル保存
		if (EOS != cal->log.logFile[0]) {
			FILE	*fp = NULL;

			sprintf((char*)cal->workBuf, "%s.send%05d", cal->log.logFile, cal->sendIdx++);

			// ファイルオープン
			fp = fopen((char*)cal->workBuf, (char*)"wb");
			if (NULL == fp) {
				SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "fopen err(0x%08x) %s, " HERE, errno, cal->workBuf);
			} else {
				// ファイル書き込み
				if (allSendSize != (INT32)fwrite((char*)req, sizeof(char), allSendSize, fp)) {
					SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "fwrite err, " HERE);
				}
				fclose(fp);
			}
		}
#endif
	} while (0);

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}

/**
 * @brief HTTPリクエスト送信
 * @param[in] cal      ファイルディスクリプタ(ソケット)
 * @param[in] hostName  ホスト名
 * @param[in] req       リクエスト
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_SendHttpReq_Multipart(SMCAL *cal,
											 Char *req,
											 UINT32 reqLen,
											 SMCALPOSTPARAM *param,
											 UINT32 paramNum,
											 const SMCALOPT *opt)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	INT32	size = 0;
	Char	*sendData = NULL;
	INT32	bodyBufSize = 0;
	INT32	bodyLen = 0;
	UINT32	allSendSize = 0;	// 送信対象データ総サイズ
	UINT32	sendSize = 0;		// 送信済みデータサイズ
	UINT32	headerSize = 0;
	UINT32	dataSize = 0;
	UINT32	num = 0;
	//UINT32	idx = 0;
	UINT32	remainSize = 0;
	UINT32	remainSendSize = 0;
	//INT32	cnt = 0;
	Bool	isRetry = false;
#ifdef OUTPUT_SEND_RECV_LOG
	FILE	*fp = NULL;
#endif

//	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	do {
		// パラメータチェック
		if (NULL == req) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "param err[req], " HERE);
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}

		// 送信対象データ総サイズ
		allSendSize = reqLen;

		// ヘッダサイズ
		headerSize = strlen(req);

#ifdef OUTPUT_SEND_RECV_LOG
		// 送信データをファイル保存
		if (EOS != cal->log.logFile[0]) {

			sprintf((char*)cal->workBuf, "%s.send%05d", cal->log.logFile, cal->sendIdx++);

			// ファイルオープン
			fp = fopen((char*)cal->workBuf, (char*)"wb");
			if (NULL == fp) {
				SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "fopen err(0x%08x) %s, " HERE, errno, cal->workBuf);
			}
		}
#endif

		num = 0;
		while (sendSize < allSendSize) {
			if (!isRetry) {
				if ((NULL != opt) && (NULL != opt->cancel) && (opt->cancel())) {
					SC_CAL_LOG_WarnPrint(&cal->log, SC_CAL_TAG, "cancel, " HERE);
					ret = e_SC_CAL_RESULT_CANCEL;
					break;
				}

				dataSize = 0;
				bodyLen = 0;
				sendData = req;
				if (sendSize < headerSize) {
					// ヘッダ
					dataSize = headerSize;
					bodyBufSize = (SC_SEND_DATA_SIZE - dataSize);
				} else {
					memset(sendData, 0, SC_SEND_DATA_SIZE);
					bodyBufSize = (SC_SEND_DATA_SIZE - dataSize);
				}

				if (0 < bodyBufSize) {
					// ボディ(Multipart)生成
					ret = SC_CAL_CreateMultipartBody(cal, param, paramNum, &num, &remainSize, &sendData[dataSize], bodyBufSize, &bodyLen, opt);
					if (e_SC_CAL_RESULT_SUCCESS != ret) {
						SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_CreateMultipartBody err(0x%08x), " HERE, ret);
						ret = e_SC_CAL_RESULT_TCP_COMMUNICATION_ERR;
						break;
					}
					dataSize += bodyLen;
				}
			}

			do {
				if ((NULL != opt) && (NULL != opt->cancel) && (opt->cancel())) {
					SC_CAL_LOG_WarnPrint(&cal->log, SC_CAL_TAG, "cancel, " HERE);
					ret = e_SC_CAL_RESULT_CANCEL;
					break;
				}

				// fdチェック(送信側)
				ret = SC_CAL_ChkFd(cal, e_SC_CAL_FD_WRITE, cal->comTimeout, opt);
				if (e_SC_CAL_RESULT_CANCEL == ret) {
					SC_CAL_LOG_InfoPrint(&cal->log, SC_CAL_TAG, (Char*)"SC_CAL_ChkFd cancel, " HERE);
					break;
				} else if (e_SC_CAL_RESULT_SUCCESS != ret) {
					SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_ChkFd err, " HERE);
					break;
				}

				if (!cal->isHttps) {
					// データ送信処理
					size = send(cal->sock, (char*)sendData, dataSize, 0);
					if (0 >= size) {
						// 送信失敗
						SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "send err(0x%08x), " HERE, errno);
						ret = e_SC_CAL_RESULT_TCP_COMMUNICATION_ERR;
						break;
					}
				} else {
					// データ送信処理
					size = BIO_write((BIO*)cal->ssl.bio, (char*)sendData, dataSize);
					if (0 >= size) {
						size = BIO_should_retry((BIO*)cal->ssl.bio);
						if (!size) {
							// 送信失敗
							SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "BIO_write err, " HERE);
							ret = e_SC_CAL_RESULT_TCP_COMMUNICATION_ERR;
							break;
						}
						SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, "BIO_write retry, " HERE);
						isRetry = true;
						continue;
					}
					isRetry = false;
				}
				// デバッグ用に送信結果を標準出力へ表示
//				SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, (Char*)"sendSize=%d", size);
				SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, (Char*)"send=%s", sendData);


#ifdef OUTPUT_SEND_RECV_LOG
				if (NULL != fp) {
					// ファイル書き込み
					if (dataSize != (INT32)fwrite((char*)sendData, sizeof(char), dataSize, fp)) {
						SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "fwrite err, " HERE);
					}
				}
#endif

				//送信済みサイズの更新
				sendSize += size;
				remainSendSize = (dataSize - size);
				sendData += size;
				dataSize -= size;
			} while (remainSendSize);
			if (e_SC_CAL_RESULT_SUCCESS != ret) {
				break;
			}
		}
		if (e_SC_CAL_RESULT_SUCCESS != ret) {
			break;
		}
	} while (0);

#ifdef OUTPUT_SEND_RECV_LOG
	if (NULL != fp) {
		fclose(fp);
	}
#endif

//	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}

/**
 * @brief HTTPレスポンス受信
 * @param[in] cal      ファイルディスクリプタ(ソケット)
 * @param[in] res       レスポンスバッファ
 * @param[in] resSize   レスポンスバッファサイズ
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_RecvHttpRes(SMCAL *cal,
								   Char *res,
								   UINT32 resBufSize,
								   UINT32 *resSize,
								   const SMCALOPT *opt)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	UINT32	recvSize = 0;
	INT32	i = 0;
	INT32	n = 0;
	FILE	*resFp = NULL;
	Bool	isHeader = true;
	INT32	headerSize = 0;
	Char	*body = NULL;
	Char	*bodyTop = NULL;
	UINT32	bodySize = 0;
	UINT32	recvBodyLen = 0;
	UINT32	contentLen = 0;
	UINT32	recvDataSize = 0;
	Char	*str = NULL;
	UINT32	writeSize = 0;
	Bool	isChunked = false;
	INT32	remainSize = 0;
	Char	chunkSize[16] = {};
	Char	*header = NULL;
	Char	mode[3] = {};
	Bool	isRecvLoop = true;

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	// レスポンスをファイルに出力する
	if ((NULL != opt) && (true == opt->isResOutputFile)) {
		if (!opt->isResume) {
			// 上書き
			strcpy(mode, "wb");
		} else {
			// 追加
			strcpy(mode, "ab");
		}
		// ファイルオープン
		resFp = fopen((char*)opt->resFilePath, (char*)mode);
		if (NULL == resFp) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "res file open err[file=%s, mode=%s], " HERE, opt->resFilePath, mode);
			SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);
			return (e_SC_CAL_RESULT_FILE_ACCESSERR);
		}
	}

	// メモリ確保
	header = malloc(SC_HEADER_SIZE);
	if (NULL == header) {
		SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "malloc error, " HERE);
		SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);
		return (e_SC_CAL_RESULT_MALLOC_ERR);
	}

	while (isRecvLoop) {
		if ((NULL != opt) && (NULL != opt->cancel) && (opt->cancel())) {
			SC_CAL_LOG_WarnPrint(&cal->log, SC_CAL_TAG, "cancel, " HERE);
			ret = e_SC_CAL_RESULT_CANCEL;
			break;
		}

		writeSize = 0;
		memset(cal->recvBuf, 0, SC_RECVBUF_SIZE);

		// fdチェック(受信側)
		ret = SC_CAL_ChkFd(cal, e_SC_CAL_FD_READ, cal->comTimeout, opt);
		if (e_SC_CAL_RESULT_SUCCESS != ret) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_ChkFd err, " HERE);
			break;
		}

		if (!cal->isHttps) {
			// データ受信
			n = recv(cal->sock, (char*)cal->recvBuf, SC_RECVBUF_SIZE, 0);
			if (0 > n) {
				SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "recv err, " HERE);
				ret = e_SC_CAL_RESULT_TCP_COMMUNICATION_ERR;
				break;
			} else if (0 == n) {
				// 受信完了
				// TODO:受信データログ出力
				break;
			}
		} else {
			// データ受信
			n = BIO_read((BIO*)cal->ssl.bio, (char*)cal->recvBuf, SC_RECVBUF_SIZE);
			if (0 > n) {
				n = BIO_should_retry((BIO*)cal->ssl.bio);
				if (!n) {
					SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "BIO_read err, " HERE);
					ret = e_SC_CAL_RESULT_TCP_COMMUNICATION_ERR;
					break;
				}
				continue;
				SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, "BIO_read retry, " HERE);
			} else if (0 == n) {
				// 受信完了
				// TODO:受信データログ出力
				break;
			}
		}
#ifdef OUTPUT_SEND_RECV_LOG
		SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, "recv size=[%d], " HERE, n);
		SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, "recv data=[%s], " HERE, cal->recvBuf);
		// 受信データをファイル保存
		if (EOS != cal->log.logFile[0]) {
			FILE	*fp = NULL;

			sprintf((char*)cal->workBuf, "%s.recv%05d", cal->log.logFile, cal->recvIdx++);

			// ファイルオープン
			fp = fopen((char*)cal->workBuf, (char*)"wb");
			if (NULL == fp) {
				SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "fopen err(0x%08x) %s, " HERE, errno, cal->workBuf);
			} else {
				// ファイル書き込み
				if (n != (INT32)fwrite((char*)cal->recvBuf, sizeof(char), n, fp)) {
					SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "fwrite err, " HERE);
				}
				fclose(fp);
			}
		}
#endif

		if (true == isHeader) {
			headerSize = n;
			// BODY部開始位置を探索
			for (i = 0 ; i < n; i++) {
				// 改行2連続の直後がBODY部(CRLFが改行)
				if (('\r' == cal->recvBuf[i])   && ('\n' == cal->recvBuf[i+1]) &&
					('\r' == cal->recvBuf[i+2]) && ('\n' == cal->recvBuf[i+3])) {
					// ヘッダサイズ
					headerSize = (i + 4);
					// ボディ
					body = &cal->recvBuf[i + 4];
					isHeader = false;
					break;
				}
				// 改行2連続の直後がBODY部(LFが改行)
				if (('\n' == cal->recvBuf[i]) && ('\n' == cal->recvBuf[i+1])) {
					// ヘッダサイズ
					headerSize = n - (i + 2);
					// ボディ
					body = &cal->recvBuf[i + 2];
					isHeader = false;
					break;
				}
			}
			recvDataSize += headerSize;

			// 受信したヘッダを出力先バッファに格納
			memcpy(res, cal->recvBuf, headerSize);
			res[headerSize] = EOS;
			strcpy(header, res);

			// 大文字を小文字に変換
			SC_CAL_ToLowerString(header);

			// チャンク形式のデータか
			isChunked = SC_CAL_IsChunked(header);
			if (!isChunked) {
				// Content-Lengthの解析
				str = strstr(header, (Char*)SC_HTTP_RECV_HEAD_CONTENT_LEN);
				if (NULL != str) {
					// Content-Lengthがある場合のみ、BODY部の長さをチェックする
					str += (INT32)strlen(SC_HTTP_RECV_HEAD_CONTENT_LEN);
					contentLen = atol(str);
				}
			}
		} else {
			// ボディ
			body = cal->recvBuf;
		}
		bodyTop = body;

		// ボディはファイルに出力する
		if (0 < (n - headerSize)) {
			if (true == isChunked) {
				while(1) {
					if ((NULL != opt) && (NULL != opt->cancel) && (opt->cancel())) {
						SC_CAL_LOG_WarnPrint(&cal->log, SC_CAL_TAG, "cancel, " HERE);
						ret = e_SC_CAL_RESULT_CANCEL;
						break;
					}

					// チャンクデータの続きか？
					if ((0 == remainSize) && (EOS == chunkSize[0])) {
						// 続きではない
						// チャンクデータのサイズ取得
						str = strchr(body, '\n');
						if (NULL == str) {
							memcpy(chunkSize, body, ((cal->recvBuf + n) - body));
							chunkSize[(cal->recvBuf + n) - body] = EOS;
							break;
						}
						bodySize = strtoul(body, NULL, 16);
						if (0 == bodySize) {
							isRecvLoop = false;
							break;
						}
						remainSize = bodySize;
						str++;
						writeSize = (n - headerSize) - (UINT32)(str - bodyTop);
						if (writeSize > bodySize) {
							writeSize = bodySize;
						}
						body = str;
					} else {
						// 続き
						if (EOS != chunkSize[0]) {
							// チャンクデータのサイズ取得
							str = strchr(body, '\n');
							if (NULL == str) {
								SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "chuncked err, " HERE);
								ret = e_SC_CAL_RESULT_TCP_COMMUNICATION_ERR;
								break;
							}
							memcpy(&chunkSize[strlen(chunkSize)], body, (str - body));
							bodySize = strtoul(body, NULL, 16);
							if (0 == bodySize) {
								isRecvLoop = false;
								break;
							}
							chunkSize[0] = EOS;
							remainSize = bodySize;
							str++;
							writeSize = (n - headerSize) - (UINT32)(str - bodyTop);
							if (writeSize) {
								writeSize = bodySize;
							}
							body = str;
						} else {
							if ((n - headerSize) >= remainSize) {
								bodySize = remainSize;
							} else {
								bodySize = (n - headerSize);
							}
							writeSize = bodySize;
						}
					}
					if (0 >= bodySize) {
						SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "chuncked size err, " HERE);
						ret = e_SC_CAL_RESULT_TCP_COMMUNICATION_ERR;
						break;
					}

					// レスポンスをファイルに出力する
					if ((NULL != opt) && (true == opt->isResOutputFile)) {
						// ファイル書き込み
						if (writeSize != (INT32)fwrite((char*)body, sizeof(char), writeSize, resFp)) {
							SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "res file write err, " HERE);
							ret = e_SC_CAL_RESULT_FILE_ACCESSERR;
							break;
						}
					} else {
						// データ連結
						if (resBufSize < (recvDataSize + writeSize)) {
							SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "recv maxsize over, " HERE);
							ret = e_SC_CAL_RESULT_TCP_COMMUNICATION_ERR;
							break;
						}
						// 受信した分を出力先バッファに追記
						memcpy((res + recvDataSize), body, writeSize);
						recvDataSize += writeSize;
					}
					// 進捗通知
					if ((NULL != opt) && (NULL != opt->progress)) {
						opt->progress(writeSize);
					}

					remainSize -= writeSize;
					if (0 < (n - ((body + writeSize) - cal->recvBuf))) {
						body += writeSize;
						if (0 == remainSize) {
							// 改行分
							body += 2;
						}
					} else {
						break;
					}
				}
				if (e_SC_CAL_RESULT_SUCCESS != ret) {
					break;
				}
			} else {
				writeSize = (n - headerSize);
				// レスポンスをファイルに出力する
				if ((NULL != opt) && (true == opt->isResOutputFile)) {
					// ファイル書き込み
					if (writeSize != (INT32)fwrite((char*)body, sizeof(char), writeSize, resFp)) {
						SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "res file write err, " HERE);
						ret = e_SC_CAL_RESULT_FILE_ACCESSERR;
						break;
					}
				} else {
					// データ連結
					if (resBufSize < (recvDataSize + writeSize)) {
						SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "recv maxsize over, " HERE);
						ret = e_SC_CAL_RESULT_TCP_COMMUNICATION_ERR;
						break;
					}
					// 受信した分を出力先バッファに追記
					memcpy((res + recvDataSize), body, writeSize);
					recvDataSize += writeSize;
				}
				// 進捗通知
				if ((NULL != opt) && (NULL != opt->progress)) {
					opt->progress(writeSize);
				}
				recvBodyLen += writeSize;
				if (contentLen <= recvBodyLen) {
					isRecvLoop = false;
				}
			}
		}
		body = NULL;
		headerSize = 0;

		// HTTPレスポンスサイズ加算
		recvSize += n;
	}

	if (NULL != header) {
		free(header);
	}

	if (NULL != resFp) {
		fclose(resFp);
	}

	*resSize = recvSize;

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}

// チャンク形式のデータか
Bool SC_CAL_IsChunked(const Char *res)
{
	Bool	isChunked = false;
	Char	*str = NULL;

	str = strstr(res, (Char*)SC_HTTP_RECV_HEAD_CHUNKED);
	if (NULL != str) {
		isChunked = true;
	}

	return (isChunked);
}

// BASE64エンコード
E_SC_CAL_RESULT SC_CAL_Base64Encode(SMCAL *cal, UChar *srcBuf, INT32 srcLen, UChar **dstBuf)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	const Char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
	UChar	*buf;
	UChar	buf1 = 0;
	UChar	buf2 = 0;
	UChar	buf3 = 0;
	UChar	ind1 = 0;
	UChar	ind2 = 0;
	UChar	ind3 = 0;
	UChar	ind4 = 0;
	INT32	i = 0;
	INT32	j = 0;
	INT32	len = 0;

	len = ((((srcLen * 4) / 3) + 3) & (~0x03 + 1)) + 1;
	// メモリ確保
	buf = (UChar *)malloc(len);
	if (NULL == buf) {
		SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "malloc err, " HERE);
		return (e_SC_CAL_RESULT_MALLOC_ERR);
	}
	memset(buf, 0, len);
	*dstBuf = buf;

    for (i = 0, j = 0; i < srcLen; i += 3) {
        // 文字コード値取得
        buf1 = (i < (srcLen)) ? srcBuf[i] : EOS;
        buf2 = (i < (srcLen - 1)) ? srcBuf[i + 1] : EOS;
        buf3 = (i < (srcLen - 2)) ? srcBuf[i + 2] : EOS;

        // 3文字=>4文字へ分解
        ind1 = (buf1 >> 2);
        ind2 = ((buf1 & 0x03) << 4) | (buf2 >> 4);
        ind3 = ((buf2 & 0x0f) << 2) | (buf3 >> 6);
        ind4 = (buf3 & 0x3f);

        // エンコード
        buf[j] = base64[ind1];
        buf[j + 1] = base64[ind2];
        buf[j + 2] = (i < (srcLen - 1)) ? base64[ind3] : '=';
        buf[j + 3] = (i < (srcLen - 2)) ? base64[ind4] : '=';
        j += 4;
    }

	return (ret);
}


// URLエンコードにエンコードする
void SC_CAL_UrlEncode(const Char *src, UINT32 dstLne, Char *dst)
{
	Char	temp;
	UINT32	len = dstLne;

	// 終端文字分を引く
	len--;
	while ((EOS != *src) && (0 < len)) {
		temp = *src;
		if (((temp >= ASCII_ALPHA1_START) && (temp <= ASCII_ALPHA1_END)) ||
			((temp >= ASCII_ALPHA2_START) && (temp <= ASCII_ALPHA2_END)) ||
			((temp >= ASCII_DIGIT_START)  && (temp <= ASCII_DIGIT_END))  ||
			(temp == ASCII_HYPHEN) ||
			(temp == ASCII_PERIOD) ||
			(temp == ASCII_UNDERSCORE) ||
			(temp == ASCII_TILDA)) {
			*(dst++) = temp;
			len--;
		} else {
			if (3 > len) {
				// 出力バッファが尽きたので終了する
				break;
			}
			*(dst++) = '%';
			*(dst++) = SC_CLA_HexCharFromValue((UChar)temp >> 4);
			*(dst++) = SC_CLA_HexCharFromValue(temp & 0x0F);
			len -= 3;
		}
		src++;
	}
	*dst = EOS;
}

// URLエンコードをデコードする
void SC_CAL_UrlDecode(const Char *src, UINT32 dstLne, Char *dst)
{
	UINT32	len = dstLne;

	len--;
	while ((*src) && (len)) {
		if ('%' == *src) {
			if (EOS == *(src+1)) {
				break;
			}
			*(dst++) = (SC_CAL_ValueFromHexChar(*(src + 1)) << 4) + SC_CAL_ValueFromHexChar(*(src + 2));
			src += 3;
		} else {
			*(dst++) = *(src++);
		}
		len--;
	}
	*dst = EOS;
}

// 16進数文字[A-F|a-f]から数値を得る
INT32 SC_CAL_ValueFromHexChar(Char hex)
{
	INT32	num = 0;

	if (('0' <= hex) && ('9' >= hex)) {
		num = hex - '0';
	} else if (('A' <= hex) && 'F' >= hex) {
		num = hex - 'A' + 10;
	} else if (('a' <= hex) && 'f' >= hex) {
		num = hex - 'a' + 10;
	}

	return (num);
}

// 数値[0～15]から16進数文字を得る
Char SC_CLA_HexCharFromValue(UINT32 value)
{
	if ((0 > value) || (16 <= value)) {
		return ('0');
	}

	return ("0123456789ABCDEF"[value]);
}

/**
 * @brief AWSアクセス用シグネチャ取得
 * @param[in] cal           ファイルディスクリプタ(ソケット)
 * @param[in] awsSecretKey  AWSシークレットキー
 * @param[in] policy        policy
 *                          (例)GET\n\n\n1400293372\n/sms-naviappdev/AppData/JP/dataVersionInfo.tar.gz
 *                                       ^^^^^^^^^^  ^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *                                           ①             ②                         ③
 *                          ①有効期限(UNIX時間)、②バケット名、③key名
 * @param[in] signature     シグネチャ
 * @return 処理結果(E_SC_CAL_RESULT)
 */
E_SC_CAL_RESULT SC_CAL_GetAWSSignature(SMCAL *cal, const Char *awsSecretKey, const Char *policy, UChar *signature)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	UChar	sig[SHA_DIGEST_LENGTH + 1] = {};
	INT32	sigLen = 0;
	UChar	*sigBase64 = NULL;
	//INT32	i = 0;

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	do {
		// パラメータチェック
		if (NULL == awsSecretKey) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "param err[awsSecretKey], " HERE);
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}
		if (NULL == policy) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "param err[policy], " HERE);
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}
		if (NULL == signature) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "param err[signature], " HERE);
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}

		// HMAC SHA-1計算
		if (!HMAC(EVP_sha1(), awsSecretKey, strlen((char*)awsSecretKey), policy, strlen((char*)policy), sig, &sigLen)) {
			// エラー
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "HMAC err, " HERE);
			ret = e_SC_CAL_RESULT_FAIL;
			break;
		}

		// BASE64エンコード
		ret = SC_CAL_Base64Encode(cal, sig, sigLen, &sigBase64);
		if (e_SC_CAL_RESULT_SUCCESS != ret) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "SC_CAL_Base64Encode err, " HERE);
			break;
		}
		strcpy((char *)signature, (char *)sigBase64);
	} while (0);

	// メモリ解放
	if (NULL != sigBase64) {
		free(sigBase64);
	}

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}

// body生成(Multipart)
E_SC_CAL_RESULT SC_CAL_CreateMultipartBody(SMCAL *cal,
										   SMCALPOSTPARAM *param,
										   UINT32 paramNum,
										   UINT32 *paramIdx,
										   UINT32 *remainSize,
										   Char *body,
										   INT32 bodyBufSize,
										   INT32 *bodyLen,
										   const SMCALOPT *opt)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	Char	*data = NULL;
	UINT32	dataLen = 0;
	UINT32	len = 0;
	UINT32	readLen = 0;

//	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	do {
		// 初期化
		data = body;

		for (; *paramIdx < paramNum; (*paramIdx)++) {
			if ((NULL != opt) && (NULL != opt->cancel) && (opt->cancel())) {
				SC_CAL_LOG_WarnPrint(&cal->log, SC_CAL_TAG, "cancel, " HERE);
				ret = e_SC_CAL_RESULT_CANCEL;
				break;
			}

			if (0 == *remainSize) {
				if (0 == *paramIdx) {
					// サイズをチェックし、バッファに空きがあれば格納する
					len = (strlen(SC_HTTP_BOUNDARY) + strlen(SC_HTTP_MULTIPART_FORM_DATA) +
						   strlen(param[*paramIdx].name) + strlen("\r\n\r\n"));
					if ((bodyBufSize - 1) >= (dataLen + len)) {
						sprintf(data, "%s%s%s\r\n\r\n", SC_HTTP_BOUNDARY, SC_HTTP_MULTIPART_FORM_DATA, param[*paramIdx].name);
					} else {
						break;
					}
				} else {
					// サイズをチェックし、バッファに空きがあれば格納する
					len = (strlen("\r\n") + strlen(SC_HTTP_BOUNDARY) + strlen(SC_HTTP_MULTIPART_FORM_DATA) +
						   strlen(param[*paramIdx].name) + strlen("\r\n\r\n"));
					if ((bodyBufSize - 1) >= (dataLen + len)) {
						sprintf(data, "\r\n%s%s%s\r\n\r\n", SC_HTTP_BOUNDARY, SC_HTTP_MULTIPART_FORM_DATA, param[*paramIdx].name);
					} else {
						break;
					}
				}
				data += len;
				dataLen += len;
				*remainSize = param[*paramIdx].len;
			}

			// データサイズをチェックし、空きサイズ分データを格納する
			if ((bodyBufSize - dataLen) >= *remainSize) {
				len = *remainSize;
				*remainSize = 0;
			} else {
				len = (bodyBufSize - dataLen);
				*remainSize -= len;
			}
			if (SMCALPOSTPARAM_DATATYPE_FILE == param[*paramIdx].type) {
				// ファイル
				if (NULL == param[*paramIdx].fp) {
					// ファイルオープン
					param[*paramIdx].fp = fopen((char*)param[*paramIdx].data, (char*)"rb");
					if (NULL == param[*paramIdx].fp) {
						SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "fopen err(0x%08x) %s, " HERE, errno, param[*paramIdx].data);
						ret = e_SC_CAL_RESULT_FILE_ACCESSERR;
						break;
					}
				}

				// ファイルリード
				readLen = fread((char*)data, sizeof(char), len, param[*paramIdx].fp);
				if (len != readLen) {
					SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "fread err(0x%08x) %s, " HERE, errno, param[*paramIdx].data);
					fclose(param[*paramIdx].fp);
					param[*paramIdx].fp = NULL;
					ret = e_SC_CAL_RESULT_FILE_ACCESSERR;
					break;
				}

				// ファイルクローズ
				if (0 == *remainSize) {
					fclose(param[*paramIdx].fp);
					param[*paramIdx].fp = NULL;
				}
				data += len;
				dataLen += len;
			} else {
				// ファイル以外
				if (0 < len) {
					memcpy(data, param[*paramIdx].data, len);
				}
				data += len;
				dataLen += len;
			}

			if (0 != *remainSize) {
				break;
			}
		}

		// bodyの終端
		len = strlen(SC_HTTP_BOUNDARY_END);
		if (*paramIdx == paramNum) {
			if ((bodyBufSize - dataLen) > len) {
				sprintf(data, "%s", SC_HTTP_BOUNDARY_END);
				dataLen += len;
			} else {
				*remainSize = len;
			}
		}

		*bodyLen = dataLen;
	} while (0);

//	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}

// body(Multipart)サイズ取得
UINT32 SC_CAL_GetMultipartBodySize(SMCAL *cal, const SMCALPOSTPARAM *param, UINT32 paramNum)
{
	UINT32	dataLen = 0;
	UINT32	num = 0;

	for (num = 0; num < paramNum; num++) {
		dataLen += strlen(SC_HTTP_BOUNDARY);
		if (0 != num) {
			dataLen += strlen("\r\n");
		}

		dataLen += (strlen(SC_HTTP_MULTIPART_FORM_DATA) + strlen(param[num].name) + strlen("\r\n\r\n"));
		dataLen += param[num].len;
	}

	// bodyの終端
	dataLen += strlen(SC_HTTP_BOUNDARY_END);

	return (dataLen);
}

/*
 * 文字列中の大文字を小文字に変換する
 * @param [in,out] s 変換したい文字列
 */
void SC_CAL_ToLowerString(Char *s)
{
    int i = 0;

    while (EOS !=  s[i]) {
         s[i] = (Char)tolower((unsigned char)s[i]);
         i++;
    }
}

// COOKIE取得
E_SC_CAL_RESULT SC_CAL_GetCookie(SMCAL *cal, const Char *res, UINT32 resSize, Char *cookie)
{
	E_SC_CAL_RESULT	ret = e_SC_CAL_RESULT_SUCCESS;
	INT32	i = 0;
	Char	*str = NULL;
	Char	*header = NULL;
	Char	*sessionId = NULL;

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_START);

	do {
		// パラメータチェック
		if (NULL == cal) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "param err[cal], " HERE);
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}
		if (NULL == res) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "param err[cal], " HERE);
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}
		if (NULL == cookie) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "param err[cal], " HERE);
			ret = e_SC_CAL_RESULT_BADPARAM;
			break;
		}

		// 初期化
		*cookie = EOS;

		// メモリ確保
		header = (Char*)malloc(SC_HEADER_SIZE);
		if (NULL == header) {
			SC_CAL_LOG_ErrorPrint(&cal->log, SC_CAL_TAG, "malloc err, " HERE);
			ret = e_SC_CAL_RESULT_MALLOC_ERR;
			break;
		}

		// BODY部開始位置を探索
		for (i = 0 ; i < resSize; i++) {
			// 改行2連続の直後がBODY部(CRLFが改行)
			if (('\r' == res[i]) && ('\n' == res[i+1]) && ('\r' == res[i+2]) && ('\n' == res[i+3])) {
				memcpy(header, res, i);
				header[i] = EOS;
				break;
			}
			// 改行2連続の直後がBODY部(LFが改行)
			if (('\n' == res[i]) && ('\n' == res[i+1])) {
				memcpy(header, res, i);
				header[i] = EOS;
				break;
			}
		}

		// 大文字を小文字に変換
		SC_CAL_ToLowerString(header);

		// Set-Cookieの解析
		str = strstr(header, (Char*)SC_HTTP_RECV_HEAD_COOKIE);
		if (NULL != str) {
			str = strstr(str, (Char*)SC_HTTP_RECV_HEAD_COOKIE_ID);
			if (NULL != str) {
				str += strlen(SC_HTTP_RECV_HEAD_COOKIE_ID);
				sessionId = strchr(str, ';');
				if (NULL != sessionId) {
					*sessionId = EOS;
					if (SC_HTTP_RECV_HEAD_COOKIE_ID_MAXLEN >= strlen(str)) {
						strcpy(cookie, str);
					}
				}
			}
		}
	} while (0);

	if (NULL != header) {
		free(header);
	}

	SC_CAL_LOG_DebugPrint(&cal->log, SC_CAL_TAG, SC_CAL_LOG_END);

	return (ret);
}
