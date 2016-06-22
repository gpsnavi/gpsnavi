/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#ifndef SMCALAPI_H_
#define SMCALAPI_H_

// OpenSSL初期化
void SC_CAL_Initialize_OpenSSL();
// 初期化
E_SC_CAL_RESULT SC_CAL_Initialize(SMCAL *cal, const Char *logFile);
// 終了化
E_SC_CAL_RESULT SC_CAL_Finalize(SMCAL *cal);
// ホスト名取得
E_SC_CAL_RESULT SC_CAL_GetHostName(SMCAL *cal, const Char *url, Char *hostName, INT32 *port);
// 接続
E_SC_CAL_RESULT SC_CAL_Connect(SMCAL *cal, const Char *hostName, INT32 portNo, const SMCALOPT *opt);
// 切断
E_SC_CAL_RESULT SC_CAL_DisConnect(SMCAL *cal);
// GETリクエスト
E_SC_CAL_RESULT SC_CAL_GetRequest(SMCAL *cal, const Char *url, Char *res, UINT32 resBufSize, UINT32 *resSize, const SMCALOPT *opt);
// POSTリクエスト
E_SC_CAL_RESULT SC_CAL_PostRequest(SMCAL *cal, const Char *url, const Char *data, UINT32 dataLen, Char *res, UINT32 resBufSize, UINT32 *resSize, const SMCALOPT *opt);
E_SC_CAL_RESULT SC_CAL_PostRequest_Multipart(SMCAL *cal, const Char *url, const SMCALPOSTPARAM *param, UINT32 paramNum, Char *res, UINT32 resBufSize, UINT32 *resSize, const SMCALOPT *opt);
// POSTリクエスト(ファイルアップロード)
E_SC_CAL_RESULT SC_CAL_UploadFile(SMCAL *cal, const Char *url, const Char *data, UINT32 dataLen, const Char *dir, const Char *fileName, Char *res, UINT32 resBufSize, UINT32 *resSize, const SMCALOPT *opt);
// レスポンス解析
E_SC_CAL_RESULT SC_CAL_AnalyzeResponse(SMCAL *cal, const Char *res, UINT32 resSize, const Char **body, UINT32 *bodyLen, E_CONTEXT_TYPE *contextType);
E_SC_CAL_RESULT SC_CAL_AnalyzeResponseStatus(SMCAL *cal, const Char *res, UINT32 resSize, const Char **body, UINT32 *bodyLen, E_CONTEXT_TYPE *contextType, INT32 *httpStatus);
// URLエンコード
void SC_CAL_UrlEncode(const Char *src, UINT32 dstLen, Char *dst);
// URLデコード
void SC_CAL_UrlDecode(const Char *src, UINT32 dstLen, Char *dst);
// BASE64エンコード
E_SC_CAL_RESULT SC_CAL_Base64Encode(SMCAL *cal, UChar *srcBuf, INT32 srcLen, UChar **dstBuf);
// AWSシグネチャ取得
E_SC_CAL_RESULT SC_CAL_GetAWSSignature(SMCAL *cal, const Char *awsSecretKey, const Char *policy, UChar *signature);
// COOKIE取得
E_SC_CAL_RESULT SC_CAL_GetCookie(SMCAL *cal, const Char *res, UINT32 resSize, Char *cookie);

#endif /* SMCALAPI_H_ */
