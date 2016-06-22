/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "SMCoreMPInternal.h"

static UINT32 gColor = 0xFF444444;
static UINT32 gOutLineColor = 0xFFFFFFFF;
static UINT32 gBkgdColor = 0x00000000;
static NC_BITMAPFONTFUNCPTR gpFunc = NULL;

//static jstring mp_FONT_NewStringUTF8(JNIEnv *env, const char *utf8);

void MP_FONT_Initialize(void)
{
	MP_FONT_SetColor(0xFF444444);
	MP_FONT_SetOutLineColor(0xFFFFFFFF);
	MP_FONT_SetBkgdColor(0x00000000);
}

E_SC_RESULT MP_FONT_SetBitmapFontCB(NC_BITMAPFONTFUNCPTR pfunc)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	gpFunc = pfunc;
	return (ret);
}

E_SC_RESULT MP_FONT_CreateBitmapFont(char* str, FLOAT fontSize, INT32 outLineFlg, UChar **ppBitMap, INT32* pWidth, INT32* pHeight, INT32* pStrWidth, INT32* pStrHeight, FLOAT rotation, Bool lineBreak)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	Bool bRet = true;
	NCBITMAPFONTINFO info = {};

	if(NULL == gpFunc) {
		return (e_SC_RESULT_FAIL);
	}

	// IN
	info.str = str;						// 入力文字列
	info.fontSize = fontSize;			// フォントサイズ
	info.outLineFlg = outLineFlg;		// フチ有無
	info.color = gColor;				// 文字色
	info.outLineColor = gOutLineColor;	// フチ色
	info.bkgdColor = gBkgdColor;		// 背景色
	info.rotation = rotation;			// 角度
	info.lineBreak = lineBreak;			// 改行

	// ビットマップフォント生成CB
	bRet = (gpFunc)(&info);
	if (!bRet) {
		ret = e_SC_RESULT_FAIL;
	}

	// OUT
	*ppBitMap = info.pBitMap;
	*pWidth = info.width;
	*pHeight = info.height;
	*pStrWidth = info.strWidth;
	*pStrHeight = info.strHeight;

	return (ret);
#if 0
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	JNIEnv* jnienv = NULL;
	jclass cls = NULL;
	jmethodID cns = NULL;
	jmethodID midCreBF = NULL;
	jmethodID midGetW = NULL;
	jmethodID midGetH = NULL;
	jmethodID midGetStrW = NULL;
	jmethodID midGetStrH = NULL;
	jstring	strString = NULL;
	jobject obj = NULL;
	jintArray ja = NULL;
	jthrowable throwResult = NULL;
	jint* arr1 = NULL;
	INT32 jasize = 0;
	INT32 cnt = 0;
	UChar *pBuf = NULL;
	UChar (*pBufArray)[4];

	jnienv = getJNIEnv();
	if (jnienv == NULL) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*) "getJNIEnv" HERE);
		return (e_SC_RESULT_FAIL);
	}

	do {
		// クラス検索
		cls = jnienv->FindClass("jp/co/hitachi/smsfv/aa/Head/SMBitmapFont");
		if (cls == NULL) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*) "FindClass" HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// クラス情報をインスタンス化するために<init>メソッドのメソッドIDを取得
		cns = jnienv->GetMethodID(cls, "<init>", "()V");
		if (cns == NULL) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*) "GetMethodID" HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}
		// クラスインスタンス化
		obj = jnienv->NewObject(cls, cns);
		if (obj == NULL) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*) "NewObject" HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// メソッドID取得
		midCreBF = jnienv->GetMethodID(cls, "createBitmapFont", "(Ljava/lang/String;FIIIIFZ)[I");
		if (midCreBF == NULL) {
			SC_LOG_DebugPrint(SC_TAG_MP, (Char*) "GetMethodID" HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		midGetW = jnienv->GetMethodID(cls, "getWidth", "()I");
		if (midGetW == NULL) {
			SC_LOG_DebugPrint(SC_TAG_MP, (Char*) "GetMethodID" HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		midGetH = jnienv->GetMethodID(cls, "getHeight", "()I");
		if (midGetH == NULL) {
			SC_LOG_DebugPrint(SC_TAG_MP, (Char*) "GetMethodID" HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		midGetStrW = jnienv->GetMethodID(cls, "getStrWidth", "()I");
		if (midGetStrW == NULL) {
			SC_LOG_DebugPrint(SC_TAG_MP, (Char*) "GetMethodID" HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		midGetStrH = jnienv->GetMethodID(cls, "getStrHeight", "()I");
		if (midGetStrH == NULL) {
			SC_LOG_DebugPrint(SC_TAG_MP, (Char*) "getStrHeight" HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// jstring生成
		//strString = jnienv->NewStringUTF(str);
		// UTF8 4byte特殊コード回避
		strString = mp_FONT_NewStringUTF8(jnienv, str);
		if (strString == NULL) {
			SC_LOG_DebugPrint(SC_TAG_MP, (Char*) "mp_FONT_NewStringUTF8" HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		// BitMapフォント取得
		ja = (jintArray)(jnienv)->CallObjectMethod(obj, midCreBF, strString, fontSize, gColor, gOutLineColor, outLineFlg, gBkgdColor, rotation, lineBreak);
		throwResult = jnienv->ExceptionOccurred();
		if (throwResult != NULL) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*) "CallObjectMethod Exception" HERE);
			jnienv->ExceptionDescribe();
			jnienv->ExceptionClear();
			ret = e_SC_RESULT_FAIL;
			break;
		}
		if (ja == NULL) {
			SC_LOG_DebugPrint(SC_TAG_MP, (Char*) "CallObjectMethod" HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		arr1 = (jnienv)->GetIntArrayElements(ja, 0);
		if (arr1 == NULL) {
			SC_LOG_DebugPrint(SC_TAG_MP, (Char*) "GetIntArrayElements" HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		jasize = jnienv->GetArrayLength(ja);

		pBuf = new UChar[jasize*4];
		if(NULL == pBuf) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*) "new UChar" HERE);
			ret = e_SC_RESULT_FAIL;
			break;
		}

		pBufArray = (UChar (*)[4])pBuf;

		for (cnt = 0; cnt < jasize; cnt++) {
			pBufArray[cnt][0] = (arr1[cnt] & 0x00FF0000) >> 16;	// R
			pBufArray[cnt][1] = (arr1[cnt] & 0x0000FF00) >> 8;	// G
			pBufArray[cnt][2] = (arr1[cnt] & 0x000000FF);		// B
	//		pBufArray[cnt][3] = 0xFF;							// A
			pBufArray[cnt][3] = (arr1[cnt] & 0xFF000000) >> 24;	// A
		}

		*ppBitMap = pBuf;

		// 幅取得
		*pWidth = (INT32)(jnienv)->CallIntMethod(obj, midGetW);

		// 高さ取得
		*pHeight = (INT32)(jnienv)->CallIntMethod(obj, midGetH);

		// X方向スポット取得 文字列開始位置
		*pStrWidth = (INT32)(jnienv)->CallIntMethod(obj, midGetStrW);

		// Y方向スポット取得 文字列開始位置
		*pStrHeight = (INT32)(jnienv)->CallIntMethod(obj, midGetStrH);

	} while (0);

	// リソースを開放
	if (NULL != cls) {
		(jnienv)->DeleteLocalRef(cls);
	}
	if (NULL != strString) {
		(jnienv)->DeleteLocalRef(strString);
	}
	if (NULL != obj) {
		(jnienv)->DeleteLocalRef(obj);
	}
	if (NULL != ja && NULL != arr1) {
		(jnienv)->ReleaseIntArrayElements(ja, arr1, 0);
	}
	if (NULL != ja) {
		(jnienv)->DeleteLocalRef(ja);
	}

	return (ret);
#endif
}

E_SC_RESULT MP_FONT_DeleteBitmapFont(UChar *pBitMap)
{
	if(NULL == pBitMap) {
		return (e_SC_RESULT_FAIL);
	}

	delete [] pBitMap;

	return (e_SC_RESULT_SUCCESS);
}

void MP_FONT_SetColor(UINT32 color)
{
	gColor = color;
}

void MP_FONT_SetColorRGBA(UINT8 r, UINT8 g, UINT8 b, UINT8 a)
{
	gColor = ((a<<24) | (r<<16) | (g<<8) | (b));
}

UINT32 MP_FONT_GetColor(void)
{
	return (gColor);
}

void MP_FONT_SetOutLineColorRGBA(UINT8 r, UINT8 g, UINT8 b, UINT8 a)
{
	gOutLineColor = ((a<<24) | (r<<16) | (g<<8) | (b));
}

void MP_FONT_SetOutLineColor(UINT32 color)
{
	gOutLineColor = color;
}

UINT32 MP_FONT_GetOutLineColor(void)
{
	return (gOutLineColor);
}

void MP_FONT_SetBkgdColor(UINT32 color)
{
	gBkgdColor = color;
}

void MP_FONT_SetBkgdColorRGBA(UINT8 r, UINT8 g, UINT8 b, UINT8 a)
{
	gBkgdColor = ((a<<24) | (r<<16) | (g<<8) | (b));
}

UINT32 MP_FONT_GetBkgdColor(void)
{
	return (gBkgdColor);
}
#if 0
static jstring mp_FONT_NewStringUTF8(JNIEnv *env, const char *utf8)
{
	jbyteArray	arrj = NULL;
	jstring		encj = NULL;
	jclass		clsj = NULL;
	jmethodID	midj = NULL;
	jstring		strj = NULL;
	INT32		len = 0;

	do {
		// 文字列チェック
		if (NULL == utf8) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*) "utf8 NULL" HERE);
			break;
		}

		// 文字列サイズチェック
		len = strlen(utf8);
		if (0 == len) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*) "strlen 0" HERE);
			break;
		}

		// ByteArray生成
		arrj = env->NewByteArray(len);
		if (NULL == arrj) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*) "NewByteArray" HERE);
			break;
		}

		// ByteArrayへ文字列コピー
		env->SetByteArrayRegion(arrj, 0, len, (jbyte*)utf8);

		// エンコードjstring生成
		encj = env->NewStringUTF("UTF8");
		if (NULL == encj) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*) "NewStringUTF (UTF8)" HERE);
			break;
		}

		// Stringクラス検索
		clsj = env->FindClass("java/lang/String");
		if (NULL == clsj) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*) "FindClass" HERE);
			break;
		}

		// StringクラスコンストラクタMethodID取得
		midj = env->GetMethodID(clsj, "<init>", "([BLjava/lang/String;)V");
		if (NULL == midj) {
			SC_LOG_ErrorPrint(SC_TAG_MP, (Char*) "GetMethodID" HERE);
			break;
		}

		// String生成
		strj = (jstring)env->NewObject(clsj, midj, arrj, encj);
	} while (0);

	// リソース解放
	if (NULL != encj) {
		env->DeleteLocalRef(encj);
	}
	if (NULL != clsj) {
		env->DeleteLocalRef(clsj);
	}
	if (NULL != arrj) {
		env->DeleteLocalRef(arrj);
	}

	return strj;
}
#endif
