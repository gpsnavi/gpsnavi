/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

/*
 * SMPAL.cpp
 *
 *  Created on: 2015/11/05
 *      Author: masutani
 */

#include "SMCoreDALInternal.h"

static  char  poi_db_full_name[256];
static	void log_dbg( const char *fmt, ...);

/**
 * @brief ＰＯＩ用フォルダ初期化
 * @param[in] db_path 	ＰＯＩ・ＤＢの格納フォルダ
 * @return				処理結果
 */
E_PAL_RESULT  SC_POI_Initialize(const char* db_path)
{

	if(strlen(db_path) == 0)	{
		log_dbg("SC_POI_Initialize   pppppppppppppppppppppppppppppppppppppppp   \n");
		log_dbg("SC_POI_Initialize   pppppppppppppppppppppppppppppppppppppppp   \n");

		log_dbg("SC_POI_Initialize  パスがNULL pat...%s  \n", poi_db_full_name);

		log_dbg("SC_POI_Initialize   pppppppppppppppppppppppppppppppppppppppp   \n");
		log_dbg("SC_POI_Initialize   pppppppppppppppppppppppppppppppppppppppp   \n");
		return(e_PAL_RESULT_SUCCESS);
	}

	sprintf(poi_db_full_name, "%spoi.db", db_path);

	log_dbg("SC_POI_Initialize   LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL   \n");
	//log_dbg("SC_POI_Initialize   LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL   \n");

	log_dbg("SC_POI_Initialize   pat...%s   %s%d\n", poi_db_full_name, __FILE__, __LINE__);

	log_dbg("SC_POI_Initialize   LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL   \n");
	//log_dbg("SC_POI_Initialize   LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL   \n");

    int rc;
	sqlite3 *db;
	//char *zErrMsg = 0;

	rc = sqlite3_open(poi_db_full_name, &db);
	if(rc != SQLITE_OK)	{
		log_dbg("SC_POI_Initialize   rc != SQLITE_OK  pat...%s   d\n", poi_db_full_name);
		return(e_PAL_RESULT_ACCESS_ERR);
	}

//	rc = sqlite3_exec(db, "DROP TABLE IF EXISTS SM_GC_POI_POINT_TBL", NULL, NULL, &zErrMsg);
//	rc = sqlite3_exec(db, "DROP TABLE IF EXISTS SM_GC_POI_HISTORY_TBL", NULL, NULL, &zErrMsg);
//	rc = sqlite3_exec(db, "DROP TABLE IF EXISTS SM_GC_POI_FAVORITE_TBL", NULL, NULL, &zErrMsg);

	rc = sqlite3_close(db);

	SC_POI_POINT_TBL_Initialize(db_path);
	SC_POI_HISTORY_TBL_Initialize(db_path);
	SC_POI_FAVORITE_TBL_Initialize(db_path);


	return(e_PAL_RESULT_SUCCESS);

}
/* ソート関数 */
int SC_POI_sort_func( const void * a , const void * b ) {
//	第１引数 < 第２引数 の場合は負の整数を返す
//	第１引数 = 第２引数 の場合はゼロを返す
//	第１引数 > 第２引数 の場合は正の整数を返す
	SM_GC_POI_QSORT_WORK_TBL*	a_ptr = (SM_GC_POI_QSORT_WORK_TBL*)a;
	SM_GC_POI_QSORT_WORK_TBL*	b_ptr = (SM_GC_POI_QSORT_WORK_TBL*)b;

  if( a_ptr->len < b_ptr->len ) {
    return (-1);
  }
  else
  if( a_ptr->len == b_ptr->len ) {
    return (0);
  }
  return (1);
}
/**
 * @brief 近距離から並べ替える。
 * @param[in/out] ... ソート対象リスト
 * @return			なし
 */
void  SC_POI_sort(std::vector<SM_GC_POI_QSORT_WORK_TBL>*  sort_data_ptr)
{
	int  cnt = (int)sort_data_ptr->size();
	SM_GC_POI_QSORT_WORK_TBL*	ptr = new  SM_GC_POI_QSORT_WORK_TBL[cnt];


	for(int  i=0;i<cnt;i++)	{
		SM_GC_POI_QSORT_WORK_TBL  tbl = sort_data_ptr->at(i);
		memcpy( ptr + i, &tbl, sizeof(SM_GC_POI_QSORT_WORK_TBL));

	}

	  /* ソート処理 */
	  qsort(( void * )ptr , (size_t)sort_data_ptr->size() , sizeof( SM_GC_POI_QSORT_WORK_TBL) , SC_POI_sort_func );

	  sort_data_ptr->clear();

	  for(int  i=0;i<cnt;i++)	{
		  SM_GC_POI_QSORT_WORK_TBL  tbl;
		  memcpy( &tbl, ptr + i, sizeof(SM_GC_POI_QSORT_WORK_TBL));
		  sort_data_ptr->push_back(tbl);
	  }

	delete[] ptr;
}
/**
 * @brief ログ出力(デバッグ)
 * @param[in] fmt 	書式指定に従った文字列
 * @param[in] ... 	可変個引数リスト
 * @return			なし
 */
static	void log_dbg( const char *fmt, ...)
{
#ifdef ANDROID
	va_list valist;

	va_start(valist, fmt);
	__android_log_vprint(ANDROID_LOG_DEBUG, (char*) SC_TAG_PAL, (char*) fmt, valist);
	va_end(valist);
#endif /* ANDROID */
}
double SC_POI_GetRealLen(
							double		slatitude,	//	時（秒でないよ）
							double		slongitude,
							double		elatitude,
							double		elongitude
						)
{

#define PII 3.141592653589793238462        /* 円周率 */


	DOUBLE	f1,f2;						//	２地点の緯度(°)
	DOUBLE	fr1,fr2;					//	２地点の緯度(rad)
	DOUBLE	g1,g2;						//	２地点の経度（東経基準）(°)
	DOUBLE	gr1,gr2;					//	２地点の経度（東経基準）(rad)
	DOUBLE	h1,h2;						//	標高(m)
	DOUBLE	a=6378136.0;				//	赤道半径(m)
	DOUBLE	e2=0.006694470;				//	地球の離心率の自乗
	DOUBLE	x1,y1,z1,x2,y2,z2;			//	２地点の直交座標値(m)
	DOUBLE	r;							//	２地点間の直距離(m)
	DOUBLE	s;							//	２地点間の地表面距離(m)
	DOUBLE	w;							//	２地点間の半射程角(°) (中心角の１／２)
	DOUBLE	wr;							//	２地点間の半射程角(rad)
	DOUBLE	rad;						//	度→ラジアン変換係数
	DOUBLE	N1,N2;						//	緯度補正した地球の半径(m)

	//***********************************************	経度

	f1 = slatitude;
	g1 = slongitude;

	f2 = elatitude;
	g2 = elongitude;

    rad=PII/180.0;

	h1=h2=0.0;                                       /* ここでは、標高を無視 */

    if(g1<0) g1=360.0+g1;

    fr1=f1*rad; gr1=g1*rad;

    if(g2<0) g2=360.0+g2;

    fr2=f2*rad; gr2=g2*rad;

	N1=a/(sqrt(1.0-e2*sin(fr1)*sin(fr1)));

    x1=(N1+h1)*cos(fr1)*cos(gr1);

    y1=(N1+h1)*cos(fr1)*sin(gr1);

    z1=(N1*(1.0-e2)+h1)*sin(fr1);

	N2=a/(sqrt(1.0-e2*sin(fr2)*sin(fr2)));

    x2=(N2+h2)*cos(fr2)*cos(gr2);

    y2=(N2+h2)*cos(fr2)*sin(gr2);

    z2=(N2*(1.0-e2)+h2)*sin(fr2);

	r=sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2)+(z1-z2)*(z1-z2));		//	直距離

    wr=asin(r/2/a);													//	半射程角(rad)

    w=wr/rad;														//	半射程角(°)

    s=a*2*wr;														//	地表面距離

	return(s);

}
