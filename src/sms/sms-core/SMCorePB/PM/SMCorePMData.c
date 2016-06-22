/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

//------------------------------------------------------------------------------------------------------------------------
//
//	SMCorePMData.c：プローブデータ作成処理
//
//------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
// インクルード
//---------------------------------------------------------------------------------
#include "sms-core/SMCorePB/SMCorePBInternal.h"

#define PROBE_BIN_SIZE						(256)
#define	MD5_SIZE							16

//---------------------------------------------------------------------------------
// 変数定義
//---------------------------------------------------------------------------------
static FILE* fp_Probe_Dt = NULL;			// プローブデータファイルポインタ
static UINT16 Probe_Data_Cunt = 0;			// プローブデータ数カウンタ (0～)
static UINT32 Probe_Info_Cunt = 1;			// プローブ情報数カウンタ (1～99999)
static Char PrbInf_FileName[SC_PM_PROBE_INFO_FILE_NAME_SZ] = {};	// プローブ情報アップロードデータファイル名
																	// ファイル名「<FirstSeqNo>_<guid>_<dev_id>_<#>.dat」のうち、「<FirstSeqNo>_<guid>_」まで
Bool	isTimerStarted;

static T_SC_PM_PROBE_INFO Probe_Info_t = {};	// プローブ情報テーブル		// 位置情報共有機能対応 2014.03.19

static Char TempDirPath[SC_PM_PROBE_MAX_PATH] = {};		// テンポラリフォルダフルパス
static Char UserInf_File_Path[SC_PM_PROBE_MAX_PATH] = {};	// ユーザ情報ファイルフルパス
static Char ProbeDt_File_Path[SC_PM_PROBE_MAX_PATH] = {};	// プローブデータ蓄積ファイルフルパス
static T_CC_CMN_SMS_API_PRM	SmsApi_prm_t = {};				// SMS API関連パラメタ構造体
static Bool uploadReady;

//---------------------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------------------
static E_SC_RESULT SC_PM_SendUploadReq(Char* p_buff, UINT16 buff_size, Char* p_file_name);																				// 2014.04.18
static E_SC_RESULT SC_PM_PreUploadProbeData(Char** pp_buff, UINT16* p_buff_size, Char** pp_fname_buff);
static E_SC_RESULT SC_PM_WriteProbeInfo(const T_SC_PM_PROBE_INFO* p_probe_inf);
static UINT16 SC_PM_WriteProbeHead(Char* p_Buff_Top);
static void SC_PM_SendStratTimerMsg();
static void GetProbeInf(const pthread_msq_msg_t *msg, T_SC_PM_PROBE_INFO* probe_inf);
static E_SC_RESULT SC_PM_ReadUserInfoFile();				//ユーザ情報ファイル読出し
static void SC_PM_MD5(UChar *buff, UINT16 buffSize, UChar *md5);

//---------------------------------------------------------------------------------
// 外部関数
//---------------------------------------------------------------------------------
//************************************************
// プローブ情報初期化処理
//************************************************
/**
 * @brief プローブ情報初期化
 * @retval 正常終了  :e_SC_RESULT_SUCCESS
 * @note ナビ起動時に１回のみコールされる
 * @note テンポラリフォルダフルパス、各ファイルフルパスの文字列を作成する
 */
E_SC_RESULT SC_PM_ProbeInfo_Initial()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	const char* p_path = NULL;

	SC_LOG_DebugPrint(SC_TAG_PM, SC_LOG_START);

	// ルートフォルダパス(ポインタ)取得
	p_path = SC_MNG_GetApRootDirPath();

	// テンポラリフォルダパス作成
	strcpy((char*)TempDirPath, p_path);									// ルートフォルダパスコピー
	if ('/' == TempDirPath[strlen((const char*)TempDirPath) - 1]){		// 文字列の最後に'/'がある場合
		TempDirPath[strlen((const char*)TempDirPath) - 1] = EOS;		// '/' ⇒ \0に変更 (↓で連結するSC_TEMP_DIRに入っているため)
	}
	strcat(TempDirPath, SC_TEMP_DIR);									// テンポラリフォルダパス

	// ユーザ情報ファイルフルパス文字列作成
	sprintf(UserInf_File_Path, "%s%s", TempDirPath, SC_PM_USERINFO_FILE_NAME);

	// プローブデータ蓄積ファイルフルパス文字列作成
	sprintf(ProbeDt_File_Path, "%s%s", TempDirPath, SC_PM_PROBE_DATA_FILE_NAME);

	uploadReady = true;

	SC_LOG_DebugPrint(SC_TAG_PM, SC_LOG_END);

	return (ret);
}

//************************************************
// プローブ情報作成・送信依頼処理
//************************************************
/**
 * @brief プローブ情報作成・送信依頼
 * @param[in] msg スレッド間通信メッセージテーブルポインタ
 * @retval 正常終了  :e_SC_RESULT_SUCCESS
 * @retval エラー終了:e_SC_RESULT_SUCCESS以外 (コール関数の戻り値)
 * @note ロケータより1秒間隔または、タイマスレッドより1分間隔でコールされる
 */
E_SC_RESULT SC_PM_CreateProbeData(const pthread_msq_msg_t *msg)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	//pthread_msq_msg_t sendMsg = {};
	Char* p_buff = NULL;						// プローブ情報データ格納エリアポインタ
	Char* p_fname_buff = NULL;					// プローブ情報データファイル名格納エリアポインタ
	UINT16 buff_size = 0;						// プローブ情報データサイズ
	//Char* p_posinf_buff = NULL;					// 位置情報共有データ格納エリアポインタ
	//Char* p_posinf_fname_buff = NULL;			// 位置情報共有データファイル名格納エリアポインタ
	//UINT16 posinf_buff_size = 0;				// 位置情報共有データサイズ
	Bool	isUpload = false;
	//UINT32	cnt = 0;

	SC_LOG_DebugPrint(SC_TAG_PM, SC_LOG_START);

	do {

		// パラメタチェック
		if(NULL == msg){
			SC_LOG_ErrorPrint(SC_TAG_PM, "Param error [msg] " HERE);
			ret = e_SC_RESULT_BADPARAM;
			break;
		}

		// ログイン状態取得(ログイン状態ならアップロード可)
		isUpload = SCC_IsLogined();
		// プローブ送信モード設定
		CC_SET_PROBEUPLOAD(isUpload);

		// メッセージ送信元による処理振り分け
		if(SC_CORE_MSQID_PT == pthread_msq_msg_issender(msg)){			// 送信元 ＝ タイマスレッド (この場合、プローブ情報のアップロードを行う)
			if (0 == Probe_Data_Cunt) {
				break;
			}
			if (uploadReady) {
				if (isUpload) {
					// プローブ情報アップロード前処理
					ret = SC_PM_PreUploadProbeData(&p_buff, &buff_size, &p_fname_buff);
					Probe_Data_Cunt = 0;										// プローブデータ数カウンタ初期化

					// プローブ情報アップロードメッセージ送信
					ret = SC_PM_SendUploadReq(p_buff, buff_size, p_fname_buff);
				}
			} else {
				SC_LOG_WarnPrint(SC_TAG_PM, "for uploading other data, it is not possible to upload, " HERE);
				Probe_Data_Cunt = 0;
				if (NULL != fp_Probe_Dt) {
					fclose(fp_Probe_Dt);
					fp_Probe_Dt = NULL;
				}
			}
		} else {														// ロケータのsender定義が未定なので、ひとまずelseでやる	2014.02.28
			if(SC_PM_PROBE_INFO_CUNT_MAX > Probe_Data_Cunt){
				// msgからプローブデータ取得
				GetProbeInf(msg, &Probe_Info_t);

				// プローブデータ蓄積
				ret = SC_PM_WriteProbeInfo(&Probe_Info_t);
				Probe_Data_Cunt++;											// プローブデータ数カウンタ更新
			}
			if(SC_PM_PROBE_INFO_CUNT_MAX == Probe_Data_Cunt){			// プローブデータが６０個溜まったらアップロード
				if (uploadReady) {
					if (isUpload) {
						// タイマ停止
						SC_PT_TimerStop();
						isTimerStarted = false;

						// プローブ情報アップロード前処理
						ret = SC_PM_PreUploadProbeData(&p_buff, &buff_size, &p_fname_buff);
						Probe_Data_Cunt = 0;									// プローブデータ数カウンタ初期化

						// プローブ情報アップロードメッセージ送信
						ret = SC_PM_SendUploadReq(p_buff, buff_size, p_fname_buff);															// 2014.04.18
					} else {
						Probe_Data_Cunt = 0;
						if (NULL != fp_Probe_Dt) {
							fclose(fp_Probe_Dt);
							fp_Probe_Dt = NULL;
						}
					}
				} else {
					SC_LOG_WarnPrint(SC_TAG_PM, "for uploading other data, it is not possible to upload, " HERE);
					Probe_Data_Cunt = 0;
					if (NULL != fp_Probe_Dt) {
						fclose(fp_Probe_Dt);
						fp_Probe_Dt = NULL;
					}
				}
			}

			if (true != isTimerStarted) {
				// タイマ起動
				SC_PM_SendStratTimerMsg();
				isTimerStarted = true;
			}

//		} else {		// 送信元想定外									// ロケータのsender定義が未定なので、ひとまず「送信元想定外」は無しでやる	2014.02.28
			// なにもしない？
		}

	} while (0);

	SC_LOG_DebugPrint(SC_TAG_PM, SC_LOG_END);

	return (ret);
}

/**
 * @brief プローブアップロード完了通知
 */
void SC_PM_NoticeUploadFinish(const pthread_msq_msg_t *msg)
{
	E_SC_RESULT	result;

	SC_LOG_DebugPrint(SC_TAG_PM, SC_LOG_START);

	do {
		// パラメータチェック
		if (NULL == msg) {
			SC_LOG_ErrorPrint(SC_TAG_PM, "param error[msg], " HERE);
			break;
		}
		result = (E_SC_RESULT)msg->data[SC_MSG_RES_UPLOAD_RESULT];
		if (e_SC_RESULT_SUCCESS != result) {
			SC_LOG_ErrorPrint(SC_TAG_PM, "probe upload result error, " HERE);
			break;
		}

		// プローブ情報数カウンタ更新
		Probe_Info_Cunt++;
		if (SC_PM_PROBE_INFO_UPLOAD_CUNT_MAX < Probe_Info_Cunt) {
			// プローブ情報数カウンタ最大値超えの場合
			Probe_Info_Cunt = 1;
		}
	} while (0);

	uploadReady = true;

	SC_LOG_DebugPrint(SC_TAG_PM, SC_LOG_END);
}

//---------------------------------------------------------------------------------
// 内部関数
//---------------------------------------------------------------------------------
//************************************************
// プローブ情報アップロード要求メッセージ送信処理
//************************************************
/**
 * @brief プローブ情報アップロード要求メッセージ送信
 * @param[in] p_buff プローブ情報先頭ポインタ
 * @param[in] buff_size プローブ情報サイズ
 * @retval 正常終了  :e_SC_RESULT_SUCCESS
 * @retval エラー終了:e_SC_RESULT_FAIL
 * @note アップロードスレッドに、プローブ情報アップロード要求メッセージを送信する
 */
E_SC_RESULT SC_PM_SendUploadReq(Char* p_buff,
								UINT16 buff_size,
								Char* p_file_name)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	pthread_msq_msg_t sendMsg = {};

	SC_LOG_DebugPrint(SC_TAG_PM, SC_LOG_START);

	// プローブ情報アップロード要求メッセージ生成
	sendMsg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_PU_UPLOAD;
	sendMsg.data[SC_MSG_REQ_PRBDT_MEM_ADDR] = (uintptr_t)p_buff;			// プローブデータメモリアドレス
	sendMsg.data[SC_MSG_REQ_PRBDT_MEM_SIZE] = (int)buff_size;				// プローブデータメモリサイズ
	sendMsg.data[SC_MSG_REQ_PRBDT_FILE_NAME] = (uintptr_t)p_file_name;		// プローブデータファイル名
	sendMsg.data[SC_MSG_REQ_API_PARAM] = (uintptr_t)&SmsApi_prm_t;			// SMS API関連パラメタ構造体

	// 送信メッセージをログ出力
	SC_LOG_DebugPrint(SC_TAG_PM,
			"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
			sendMsg.data[0],  sendMsg.data[1],  sendMsg.data[2],  sendMsg.data[3],  sendMsg.data[4],
			sendMsg.data[5],  sendMsg.data[6],  sendMsg.data[7],  sendMsg.data[8],  sendMsg.data[9]);

	// プローブ情報アップロード要求メッセージ送信
	if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_PU, &sendMsg, SC_CORE_MSQID_PM)) {
		SC_LOG_ErrorPrint(SC_TAG_PM, "pthread_msq_msg_send error " HERE);
		ret = e_SC_RESULT_FAIL;
		uploadReady = true;
	} else {
		uploadReady = false;
	}

	SC_LOG_DebugPrint(SC_TAG_PM, SC_LOG_END);

	return (ret);
}

//************************************************
// プローブ情報アップロード前 (プローブ情報作成) 処理
//************************************************
/**
 * @brief プローブ情報作成
 * @param[out] pp_buff プローブ情報先頭ポインタ格納エリアポインタ
 * @param[out] p_buff_size プローブ情報サイズ格納エリアポインタ
 * @param[out] pp_fname_buff プローブ情報データファイル名先頭ポインタ格納エリアポインタ
 * @retval 正常終了  :e_SC_RESULT_SUCCESS
 * @retval エラー終了:e_SC_RESULT_SUCCESS以外
 * @note プローブヘッダ部・プローブデータ部より、プローブ情報を作成する
 */
E_SC_RESULT SC_PM_PreUploadProbeData(Char** pp_buff,
										UINT16* p_buff_size,
										Char** pp_fname_buff)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	Char* p_buff_pos = NULL;			// メモリ書込みカウンタ用
	UINT32 file_sz = 0;
	UINT32 read_sz = 0;
	UINT16 head_size = 0;
	UINT32* p_uint32 = NULL;
	Char file_name[SC_PM_PROBE_INFO_FILE_NAME_SZ] = {};	// ファイル名仮作成エリア
	UINT16 str_sz = 0;
	UChar md5[MD5_SIZE + 4] = {};
UChar	*_ptr = NULL;
Char	prbFileName[128] = {};
FILE	*fp = NULL;

	SC_LOG_DebugPrint(SC_TAG_PM, SC_LOG_START);

	do {

		// プローブデータ蓄積ファイルサイズ取得
		file_sz = ftell(fp_Probe_Dt);
		if(0 == file_sz){
			SC_LOG_ErrorPrint(SC_TAG_PM, "ftell() error " HERE);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}

		// メモリサイズ算出 (ヘッダ部のサイズは仮(実際より少し大きめ))
		*p_buff_size = file_sz + sizeof(T_SC_PM_PROBE_INFO_HEAD);

		// メモリ取得 (ヘッダ部のサイズが仮なので、実際より少し大きめにメモリを確保)
		p_buff_pos = (Char*)SC_MEM_Alloc(*p_buff_size, e_MEM_TYPE_DYNAMIC);		// SC_MEM_Freeはアップロード処理内で行う
		if(NULL == p_buff_pos){
			SC_LOG_ErrorPrint(SC_TAG_PM, "SC_MEM_Alloc() error " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		*pp_buff = p_buff_pos;													// メモリ先頭ポインタ退避

		// ヘッダ部の情報を取得 ＆ p_buff_posに書込む
		head_size = SC_PM_WriteProbeHead(p_buff_pos);
		if(0 == head_size){
			SC_LOG_ErrorPrint(SC_TAG_PM, "SC_PM_WeiteProbeHead() error " HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		} else {
			p_buff_pos += head_size;
		}

		// プローブ情報蓄積ファイルよりプローブ情報コピー
		if(0 != fseek(fp_Probe_Dt, 0, SEEK_SET)){
			SC_LOG_ErrorPrint(SC_TAG_PM, "fseek() error " HERE);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}
		read_sz = fread(p_buff_pos, 1, file_sz, fp_Probe_Dt);
		if(file_sz != read_sz){
			SC_LOG_ErrorPrint(SC_TAG_PM, "fread() error " HERE);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}

		// プローブ情報サイズ格納
		*p_buff_size = (head_size + read_sz);									// プローブ情報のサイズ(MD5チェックサムなし)

		// プローブ情報サイズ設定
		p_uint32 = (UINT32*)((*pp_buff) + (head_size - SC_PM_PROBE_HEAD_SIZE_DATA_SZ));
		*p_uint32 = (UINT32)CONVERT_LITTLE_ENDIAN_INT32(read_sz);

		// MD5ハッシュ値計算
		SC_PM_MD5(*pp_buff, *p_buff_size, md5);
SC_LOG_ErrorPrint(SC_TAG_PM, "bin_size=%d, " HERE, *p_buff_size);
		memcpy(&(*pp_buff)[*p_buff_size], md5, MD5_SIZE);
_ptr = &(*pp_buff)[*p_buff_size];
SC_LOG_ErrorPrint(SC_TAG_PM,
				  "Probe MD5=%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x, " HERE,
				  _ptr[0], _ptr[1], _ptr[2], _ptr[3], _ptr[4], _ptr[5], _ptr[6], _ptr[7],
				  _ptr[8], _ptr[9], _ptr[10], _ptr[11], _ptr[12], _ptr[13], _ptr[14], _ptr[15]);

		*p_buff_size += MD5_SIZE;												// プローブ情報のサイズ(MD5チェックサムあり)

sprintf(prbFileName, "%s/Temp/probe.bin", SC_MNG_GetApRootDirPath());
fp = fopen(prbFileName, "wb");
if (NULL == fp) {
	SC_LOG_ErrorPrint(SC_TAG_PU, (Char*)"file open error[%s], " HERE, prbFileName);
} else {
	if (*p_buff_size != fwrite((const void*)*pp_buff, sizeof(UChar), *p_buff_size, fp)) {
		SC_LOG_ErrorPrint(SC_TAG_PU, "fwrite() error, " HERE);
	}
	fclose(fp);
}

		// プローブ情報データファイル名作成
		str_sz = strlen(PrbInf_FileName);
		strcpy(file_name, PrbInf_FileName);										// アップロードデータファイル名取得
		sprintf(&(file_name[str_sz]), "%05d%s", Probe_Info_Cunt, SC_PM_PROBE_INFO_FILE_EXTENSION);	// ファイル番号、拡張子追加
		str_sz = (strlen(file_name) + 1);										// 文字列サイズ取得 (+1は終端文字分)
		*pp_fname_buff = (Char*)SC_MEM_Alloc(str_sz, e_MEM_TYPE_DYNAMIC);		// SC_MEM_Freeはアップロード処理内で行う
		if(NULL == *pp_fname_buff){
			SC_LOG_ErrorPrint(SC_TAG_PM, "SC_MEM_Alloc() error " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			Probe_Info_Cunt = 1;												// プローブ情報数カウンタ初期化
			break;
		}
		memcpy(*pp_fname_buff, file_name, str_sz);								// アップロードデータファイル名コピー

	} while (0);

	// メモリ取得後にエラー発生の場合は、メモリ解放
	if(e_SC_RESULT_SUCCESS != ret){

		// プローブ情報格納エリア
		if(NULL != *pp_buff){
			// メモリ解放
			SC_MEM_Free(*pp_buff, e_MEM_TYPE_DYNAMIC);
			*pp_buff = NULL;
			*p_buff_size = 0;
		}

		// プローブ情報データファイル名格納エリア
		if(NULL != *pp_fname_buff){
			// メモリ解放
			SC_MEM_Free(*pp_fname_buff, e_MEM_TYPE_DYNAMIC);
			*pp_fname_buff = NULL;
		}
	}

	// プローブデータ蓄積ファイルクローズ
	if(NULL != fp_Probe_Dt){
		fclose(fp_Probe_Dt);
		fp_Probe_Dt = NULL;							// 念のためNULL設定
	}

	SC_LOG_DebugPrint(SC_TAG_PM, SC_LOG_END);

	return (ret);
}

//************************************************
// プローブデータ蓄積処理
//************************************************
/**
 * @brief プローブデータ蓄積
 * @param[in] p_probe_inf プローブデータ構造体先頭ポインタ
 * @retval 正常終了  :e_SC_RESULT_SUCCESS
 * @retval エラー終了:e_SC_RESULT_SUCCESS以外
 * @note プローブデータ１件をプローブデータ蓄積ファイルに書込む
 */
E_SC_RESULT SC_PM_WriteProbeInfo(const T_SC_PM_PROBE_INFO* p_probe_inf)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	UChar		*bin = NULL;
	UChar		*binPtr = NULL;
	UINT32		len = 0;

	SC_LOG_DebugPrint(SC_TAG_PM, SC_LOG_START);

	do {

		// メモリ確保
		bin = (Char*)SC_MEM_Alloc(PROBE_BIN_SIZE, e_MEM_TYPE_DYNAMIC);
		if(NULL == bin){
			SC_LOG_ErrorPrint(SC_TAG_PM, "SC_MEM_Alloc() error " HERE);
			ret = e_SC_RESULT_MALLOC_ERR;
			break;
		}
		memset(bin, 0, PROBE_BIN_SIZE);
		binPtr = bin;

		// プローブデータ蓄積ファイルオープン (センターにデータのアップロード処理するまで、オープンのまま)
		if( NULL == fp_Probe_Dt ){
			fp_Probe_Dt = fopen((const char*)ProbeDt_File_Path, (const char*)"wb+");				// 2014.04.18
			if(NULL == fp_Probe_Dt){
				SC_LOG_ErrorPrint(SC_TAG_PM, "Probe Data File Open error " HERE);
				ret = e_SC_RESULT_FILE_OPENERR;
				break;
			}
			Probe_Data_Cunt = 0;										// プローブデータ数カウンタ初期化
		}

		// 時刻
		len = sizeof(p_probe_inf->data.year) +
			  sizeof(p_probe_inf->data.month) +
			  sizeof(p_probe_inf->data.day) +
			  sizeof(p_probe_inf->data.hour) +
			  sizeof(p_probe_inf->data.minute) +
			  sizeof(p_probe_inf->data.second);
		memcpy(binPtr, &p_probe_inf->data.year, len);
		binPtr += len;
		// GPS位置情報
		len = sizeof(p_probe_inf->data.gps_latitude) + sizeof(p_probe_inf->data.gps_longitude);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.gps_latitude, len);
		binPtr += len;
		// マップマッチング位置情報
		len = sizeof(p_probe_inf->data.map_latitude) + sizeof(p_probe_inf->data.map_longitude);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.map_latitude, len);
		binPtr += len;
		// マップマッチングフラグ
		len = sizeof(p_probe_inf->data.mapMatchingFlg);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.mapMatchingFlg, len);
		binPtr += len;
		// パーセルID
		len = sizeof(p_probe_inf->data.parcelId);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.parcelId, len);
		binPtr += len;
		// リンクID
		len = sizeof(p_probe_inf->data.linkId);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.linkId, len);
		binPtr += len;
		// 方向フラグ
		len = sizeof(p_probe_inf->data.linkDirectionFlg);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.linkDirectionFlg, len);
		binPtr += len;
		// 方向
		len = sizeof(p_probe_inf->data.direction);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.direction, len);
		binPtr += len;
		// 速度
		len = sizeof(p_probe_inf->data.speed);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.speed, len);
		binPtr += len;
		// 保存フラグ
		len = sizeof(p_probe_inf->data.save_flg);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.save_flg, len);
		binPtr += len;
		// 車両タイプ
		len = sizeof(p_probe_inf->data.transfar_type);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.transfar_type, len);
		binPtr += len;
		// 位置情報共有フラグ
		len = sizeof(p_probe_inf->data.echo_flag);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.echo_flag, len);
		binPtr += len;
		// 経路誘導利用ステータス
		len = sizeof(p_probe_inf->data.guideStatus);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.guideStatus, len);
		binPtr += len;
		// センサ加速度有効フラグ
		len = sizeof(p_probe_inf->data.accelerometerFlg);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.accelerometerFlg, len);
		binPtr += len;
		// センサ加速度
		len = sizeof(p_probe_inf->data.accelerometer);
		memcpy(binPtr, (UChar*)p_probe_inf->data.accelerometer, len);
		binPtr += len;
		// ジャイロ有効フラグ
		len = sizeof(p_probe_inf->data.gyroscopeFlg);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.gyroscopeFlg, len);
		binPtr += len;
		// ジャイロ
		len = sizeof(p_probe_inf->data.gyroscope);
		memcpy(binPtr, (UChar*)p_probe_inf->data.gyroscope, len);
		binPtr += len;
		// 磁気有効フラグ
		len = sizeof(p_probe_inf->data.magneticFieldFlg);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.magneticFieldFlg, len);
		binPtr += len;
		// 磁気
		len = sizeof(p_probe_inf->data.magneticField);
		memcpy(binPtr, (UChar*)p_probe_inf->data.magneticField, len);
		binPtr += len;
		// 方位角有効フラグ
		len = sizeof(p_probe_inf->data.orientationFlg);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.orientationFlg, len);
		binPtr += len;
		// 方位角
		len = sizeof(p_probe_inf->data.orientation);
		memcpy(binPtr, (UChar*)p_probe_inf->data.orientation, len);
		binPtr += len;
		// GPS情報有効フラグ
		len = sizeof(p_probe_inf->data.gpsFlg);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.gpsFlg, len);
		binPtr += len;
		// GPS速度
		len = sizeof(p_probe_inf->data.gpsSpeed);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.gpsSpeed, len);
		binPtr += len;
		// GPS方位
		len = sizeof(p_probe_inf->data.gpsAngle);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.gpsAngle, len);
		binPtr += len;
		// 気圧有効フラグ
		len = sizeof(p_probe_inf->data.puressureFlg);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.puressureFlg, len);
		binPtr += len;
		// 気圧
		len = sizeof(p_probe_inf->data.puressure);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.puressure, len);
		binPtr += len;
		// 照度有効フラグ
		len = sizeof(p_probe_inf->data.lightFlg);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.lightFlg, len);
		binPtr += len;
		// 照度
		len = sizeof(p_probe_inf->data.light);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.light, len);
		binPtr += len;
		// GPS高度
		len = sizeof(p_probe_inf->data.gpsAltitude);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.gpsAltitude, len);
		binPtr += len;
		// GPS精度
		len = sizeof(p_probe_inf->data.gpsAccuracy);
		memcpy(binPtr, (UChar*)&p_probe_inf->data.gpsAccuracy, len);
		binPtr += len;

		// 生成したバイナリデータのサイズ
		len = (UINT32)(binPtr - bin);

		// ファイルに書き込む
		if(len != fwrite((const void*)bin, sizeof(UChar), len, fp_Probe_Dt)){
			SC_LOG_ErrorPrint(SC_TAG_PM, "fwrite() error road data, " HERE);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}

	} while (0);

	if (NULL != bin) {
		SC_MEM_Free(bin, e_MEM_TYPE_DYNAMIC);
	}

	SC_LOG_DebugPrint(SC_TAG_PM, SC_LOG_END);

	return (ret);
}

//************************************************
// プローブヘッダ部作成処理
//************************************************
/**
 * @brief プローブヘッダ部作成
 * @param[in] p_Buff_Top プローブ情報作成エリア先頭ポインタ
 * @retval 正常終了  : プローブヘッダ部のサイズ(BYTE単位)
 * @retval エラー終了: 0
 * @note プローブヘッダ部をプローブ情報作成エリアの先頭に書込む
 * @note アプリ起動後の初回コール時のみ、アップロードデータファイル名文字列を作成する
 */
UINT16 SC_PM_WriteProbeHead(Char* p_Buff_Top)
{
	UINT16 head_size = 0;
	UINT16 write_sz = 0;
	Char* p_buff_pos = p_Buff_Top;
	time_t timer = {0};
	struct tm gmt = {};
	Char file_name[SC_PM_PROBE_INFO_FILE_NAME_SZ] = {};	// ファイル名仮作成エリア
	UINT16 str_sz = 0;
	Char buildNo[SC_PM_PROBE_HEAD_SIZE_MAP_BUILD_NO] = {};
	E_SC_RESULT	rslt = e_SC_RESULT_SUCCESS;

	SC_LOG_DebugPrint(SC_TAG_PM, SC_LOG_START);

	do {

		// 現在時刻取得
		if(-1 == time(&timer)){
			SC_LOG_ErrorPrint(SC_TAG_PM, "time() error " HERE);
			break;
		}
		gmtime_r(&timer, &gmt);

		// ユーザ情報ファイル読出し
		rslt = SC_PM_ReadUserInfoFile();
		if(e_SC_RESULT_SUCCESS != rslt){
			SC_LOG_ErrorPrint(SC_TAG_PM, "Usesr Info File Error " HERE);
			break;
		}

		// アップロードデータファイル名未作成 (アプリ起動後の初回コール時) の場合
		if(SC_PM_PROBE_INFO_END_CHAR == PrbInf_FileName[0]){

			// アップロードデータファイル名作成
			// <FirstSeqNo>_部分
			sprintf(file_name, "%02d%02d%02d%02d%02d%02d_", ((1900 + gmt.tm_year) - 2000), (1 + gmt.tm_mon),
															gmt.tm_mday, gmt.tm_hour, gmt.tm_min, gmt.tm_sec);
			str_sz = strlen(file_name);											// <FirstSeqNo>文字列サイズ取得 (終端文字は含まない)

			// <guid>_部分
			strcpy(&(file_name[str_sz]), SmsApi_prm_t.ApiPrmMups.guid);			// 文字列最後の終端文字もコピーされる
			str_sz = strlen(file_name);											// <Userid>文字列サイズ取得 (終端文字は含まない)
			sprintf(&(file_name[str_sz]), SC_PM_PROBE_INFO_FILE_CONNECT);		// 連結文字追加
			str_sz += strlen(SC_PM_PROBE_INFO_FILE_CONNECT);					// 文字列サイズ更新 (連結文字分加算)

			// <dev_id>_部分
			strcpy(&(file_name[str_sz]), SmsApi_prm_t.ApiPrmMups.new_term_id);	// デバイスID文字列追加 (文字列最後の終端文字もコピーされる)
			str_sz += strlen(SmsApi_prm_t.ApiPrmMups.new_term_id);				// <dev_id>文字列サイズ加算 (終端文字は含まない)
			sprintf(&(file_name[str_sz]), SC_PM_PROBE_INFO_FILE_CONNECT);		// 連結文字追加
			str_sz += strlen(SC_PM_PROBE_INFO_FILE_CONNECT);					// 文字列サイズ更新 (連結文字分加算)

			// アップロードデータファイル名セーブ
			strcpy(PrbInf_FileName, file_name);
		}

		// 先頭識別子書込み
		memcpy(p_buff_pos, SC_PM_PROBE_INFO_DATA_ID, sizeof(SC_PM_PROBE_INFO_DATA_ID));	// 直後に終端文字もセットされる
		p_buff_pos += SC_PM_PROBE_INFO_DATA_ID_SZ;

		// データバージョン書込み
		memcpy(p_buff_pos, SC_PM_PROBE_INFO_DATA_VERSION, sizeof(SC_PM_PROBE_INFO_DATA_VERSION));	// 直後に終端文字もセットされる
		p_buff_pos += sizeof(SC_PM_PROBE_INFO_DATA_VERSION);

		// Androidアプリケーションバージョン書込み
		memcpy(p_buff_pos, API_VERSION, strlen(API_VERSION)+1);
		p_buff_pos += (strlen(API_VERSION)+1);

		// 地図ビルド番号書込み
		if (SC_DA_RES_SUCCESS != SC_DA_GetSystemMapBuildNoData(buildNo)) {
			SC_LOG_ErrorPrint(SC_TAG_PM, "SC_DA_GetSystemMapBuildNoData Error " HERE);
			break;
		}
		memcpy(p_buff_pos, buildNo, strlen(buildNo)+1);
		p_buff_pos += (strlen(buildNo)+1);

		// データID書込み
		memset(p_buff_pos, SC_PM_PROBE_HEAD_PROBE_ID, SC_PM_PROBE_HEAD_SIZE_DATA_ID);
		p_buff_pos += SC_PM_PROBE_HEAD_SIZE_DATA_ID;

		// ユーザID書込み
		strcpy(p_buff_pos, SmsApi_prm_t.ApiPrmMups.guid);						// 文字列最後の終端文字もコピーされる
		write_sz = strlen(p_buff_pos);											// 文字列サイズ取得 (終端文字は含まない)
		p_buff_pos = (p_buff_pos + write_sz + 1);								// ポインタ更新

		// デバイスID書込み
		strcpy(p_buff_pos, SmsApi_prm_t.ApiPrmMups.new_term_id);				// 文字列最後の終端文字もコピーされる
		write_sz = strlen(p_buff_pos);											// 文字列サイズ取得 (終端文字は含まない)
		p_buff_pos = (p_buff_pos + write_sz + 1);								// ポインタ更新

		// シーケンス番号書込み
		sprintf(p_buff_pos, "%02d%02d%02d%02d%02d%02d", ((1900 + gmt.tm_year) - 2000), (1 + gmt.tm_mon),
														gmt.tm_mday, gmt.tm_hour, gmt.tm_min, gmt.tm_sec);
		write_sz = strlen(p_buff_pos);							// 文字列サイズ取得 (終端文字は含まない)
		p_buff_pos[write_sz] = SC_PM_PROBE_INFO_END_CHAR;		// シーケンス番号の直後に終端文字セット
		p_buff_pos += (write_sz + 1);

		// データサイズ書込み（初期値：0）
		memset(p_buff_pos, 0, SC_PM_PROBE_HEAD_SIZE_DATA_SZ);
		p_buff_pos += SC_PM_PROBE_HEAD_SIZE_DATA_SZ;

		head_size = p_buff_pos - p_Buff_Top;

	} while (0);

	SC_LOG_DebugPrint(SC_TAG_PM, SC_LOG_END);

	return (head_size);
}

//************************************************
// タイマ開始メッセージ送信
//************************************************
void SC_PM_SendStratTimerMsg()
{
	pthread_msq_msg_t msg = {};

	SC_LOG_DebugPrint(SC_TAG_PM, SC_LOG_START);

	// タイマ要求メッセージ生成
	msg.data[SC_MSG_MSG_ID] = e_SC_MSGID_REQ_PT_START;
	msg.data[SC_MSG_REQ_PT_TIMER] = SC_PM_PROBE_UPLOAD_TIMER;

	// 送信メッセージをログ出力
	SC_LOG_DebugPrint(SC_TAG_PM,
			"sendMsg=0x%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x, " HERE,
			msg.data[0],  msg.data[1],  msg.data[2],  msg.data[3],  msg.data[4],
			msg.data[5],  msg.data[6],  msg.data[7],  msg.data[8],  msg.data[9]);

	// タイマ開始要求メッセージ送信
	if (PTHREAD_MSQ_OK != pthread_msq_msg_send(SC_CORE_MSQID_PT, &msg, SC_CORE_MSQID_PM)) {
		SC_LOG_ErrorPrint(SC_TAG_PM, "pthread_msq_msg_send error, " HERE);
	}

	SC_LOG_DebugPrint(SC_TAG_PM, SC_LOG_END);
}

//************************************************
// プローブデータ取得処理
//************************************************
/**
 * @brief スレッド間通信メッセージからプローブデータ取得
 * @param[in] msg スレッド間通信メッセージ構造体先頭ポインタ
 * @param[in] probe_inf プローブ情報構造体先頭ポインタ
 */
void GetProbeInf(const pthread_msq_msg_t *msg, T_SC_PM_PROBE_INFO *probe_inf)
{
	T_SC_PM_PROBE_INFO* msg_probe_info = NULL;
	INT32	num = 0;

	msg_probe_info = (T_SC_PM_PROBE_INFO*)msg->data[1];

	probe_inf->data.mapMatchingFlg   = msg_probe_info->data.mapMatchingFlg;
	probe_inf->data.linkDirectionFlg = msg_probe_info->data.linkDirectionFlg;
	probe_inf->data.accelerometerFlg = msg_probe_info->data.accelerometerFlg;
	probe_inf->data.gyroscopeFlg     = msg_probe_info->data.gyroscopeFlg;
	probe_inf->data.magneticFieldFlg = msg_probe_info->data.magneticFieldFlg;
	probe_inf->data.orientationFlg   = msg_probe_info->data.orientationFlg;
	probe_inf->data.puressureFlg     = msg_probe_info->data.puressureFlg;
	probe_inf->data.lightFlg         = msg_probe_info->data.lightFlg;
	probe_inf->data.gpsFlg           = msg_probe_info->data.gpsFlg;

	probe_inf->data.year             = msg_probe_info->data.year;
	probe_inf->data.month            = msg_probe_info->data.month;
	probe_inf->data.day              = msg_probe_info->data.day;
	probe_inf->data.hour             = msg_probe_info->data.hour;
	probe_inf->data.minute           = msg_probe_info->data.minute;
	probe_inf->data.second           = msg_probe_info->data.second;
	probe_inf->data.gps_latitude     = CONVERT_LITTLE_ENDIAN_INT32(msg_probe_info->data.gps_latitude);
	probe_inf->data.gps_longitude    = CONVERT_LITTLE_ENDIAN_INT32(msg_probe_info->data.gps_longitude);
	probe_inf->data.map_latitude     = CONVERT_LITTLE_ENDIAN_INT32(msg_probe_info->data.map_latitude);
	probe_inf->data.map_longitude    = CONVERT_LITTLE_ENDIAN_INT32(msg_probe_info->data.map_longitude);
	probe_inf->data.direction        = CONVERT_LITTLE_ENDIAN_INT16(msg_probe_info->data.direction);
	probe_inf->data.speed            = CONVERT_LITTLE_ENDIAN_INT32(msg_probe_info->data.speed);
	probe_inf->data.save_flg         = msg_probe_info->data.save_flg;
	probe_inf->data.transfar_type    = msg_probe_info->data.transfar_type;
	probe_inf->data.echo_flag        = msg_probe_info->data.echo_flag;
	probe_inf->data.guideStatus      = msg_probe_info->data.guideStatus;

	for (num = 0; num < 3; num++) {
		probe_inf->data.accelerometer[num] = CONVERT_LITTLE_ENDIAN_INT32(msg_probe_info->data.accelerometer[num]);
		probe_inf->data.gyroscope[num]     = CONVERT_LITTLE_ENDIAN_INT32(msg_probe_info->data.gyroscope[num]);
		probe_inf->data.magneticField[num] = CONVERT_LITTLE_ENDIAN_INT32(msg_probe_info->data.magneticField[num]);
		probe_inf->data.orientation[num]   = CONVERT_LITTLE_ENDIAN_INT32(msg_probe_info->data.orientation[num]);
	}

	probe_inf->data.puressure        = CONVERT_LITTLE_ENDIAN_INT32(msg_probe_info->data.puressure);
	probe_inf->data.light            = CONVERT_LITTLE_ENDIAN_INT32(msg_probe_info->data.light);
	probe_inf->data.gpsAltitude      = CONVERT_LITTLE_ENDIAN_INT32(msg_probe_info->data.gpsAltitude);
	probe_inf->data.gpsAngle         = CONVERT_LITTLE_ENDIAN_INT16(msg_probe_info->data.gpsAngle);
	probe_inf->data.gpsSpeed         = CONVERT_LITTLE_ENDIAN_INT16(msg_probe_info->data.gpsSpeed);
	probe_inf->data.gpsAccuracy      = CONVERT_LITTLE_ENDIAN_INT32(msg_probe_info->data.gpsAccuracy);
	probe_inf->data.parcelId         = CONVERT_LITTLE_ENDIAN_INT32(msg_probe_info->data.parcelId);
	probe_inf->data.linkId           = CONVERT_LITTLE_ENDIAN_INT32(msg_probe_info->data.linkId);

	free((void*)msg_probe_info);
}


//************************************************
// ユーザ情報ファイル読出し
//************************************************
/**
 * @brief ユーザ情報ファイル読出し
 * @retval OK : e_SC_RESULT_SUCCESS
 * @retval NG : e_SC_RESULT_FILE_OPENERR
 * @note ユーザ情報ファイルから、各種API送信時に使用する情報を読出す
 */
E_SC_RESULT SC_PM_ReadUserInfoFile()
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	FILE* fp_user_inf = NULL;			// ファイルポインタ
	INT32 result = 0;

	SC_LOG_DebugPrint(SC_TAG_PM, SC_LOG_START);

	do {
		// ファイルオープン
		fp_user_inf = fopen(UserInf_File_Path, "r");
		if(NULL == fp_user_inf){
			SC_LOG_ErrorPrint(SC_TAG_PM, "File Open Error[SMCC_UserInfo.txt] " HERE);
			ret = e_SC_RESULT_FILE_OPENERR;
			break;
		}

		// ユーザ情報ファイルから、SMS API関連パラメタ構造体読出し
		result = fread(&SmsApi_prm_t, 1, sizeof(T_CC_CMN_SMS_API_PRM), fp_user_inf);
		if(0 >= result){
			SC_LOG_ErrorPrint(SC_TAG_PM, "File Read Error[SMCC_UserInfo.txt] " HERE);
			ret = e_SC_RESULT_FILE_ACCESSERR;
			break;
		}
	} while (0);

	// ファイルクローズ
	if(NULL != fp_user_inf){
		fclose(fp_user_inf);
	}

	SC_LOG_DebugPrint(SC_TAG_PM, SC_LOG_END);

	return (ret);
}

/**
 * @brief MD5ハッシュ値を計算する
 * @param[in] buff MD5ハッシュ値の計算対象データ
 * @param[in] buffSize MD5ハッシュ値の計算対象データサイズ
 * @param[in] md5 MD5ハッシュ値
 */
void SC_PM_MD5(UChar *buff, UINT16 buffSize, UChar *md5)
{
	T_CC_CMN_MD5_CTX	ctx = {};

	// MD5初期化
	CC_MD5_Init(&ctx);

	// MD5更新
	CC_MD5_Update(&ctx, (UINT8*)buff, buffSize);

	// MD5終了化
	CC_MD5_Final(md5, &ctx);

	SC_LOG_ErrorPrint(SC_TAG_PM,
					  "Probe MD5=%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x, " HERE,
					  md5[0], md5[1], md5[2], md5[3], md5[4], md5[5], md5[6], md5[7],
					  md5[8], md5[9], md5[10], md5[11], md5[12], md5[13], md5[14], md5[15]);
}
