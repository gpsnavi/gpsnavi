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
 * SMCoreDALJniLapper.cpp
 *
 *  Created on: 2015/12/01
 *      Author: masutani
 */

#include "SMCoreDALInternal.h"

#ifdef ANDROID
#include <jni.h>

/**
 * お気に入りテーブル取得用暫定ラッパ関数<br>
 * SMPAL_POI_FAVORIT_TBL.cpp#SC_POI_selectGCfavoriteTBL からの移植<br>
 * 上記関数のJNI実装部分のみ抜き出した処理となります。<br>
 */
int SC_POI_selectGCfavoriteTBL_forJni(
		JNIEnv * env,
		jclass thiz,
		jobject in_obj,
		jobject out_obj,
		jmethodID mid_ListAdd,
		jclass item_cls,
		jmethodID item_method,
		jfieldID out_userid_id,
		jfieldID out_dbtime_id,
		jfieldID out_contentstime_id,
		jfieldID out_ctgry_code_id,
		jfieldID out_id_id,
		jfieldID out_url_id,
		jfieldID out_name_id,
		jfieldID out_pos_name_id,
		jfieldID out_contents_id,
		jfieldID out_binary_data_id,
		jfieldID out_binary_data_len_id,
		jfieldID out_lat_id,
		jfieldID out_log_id,
		SC_POI_SEARCH_COND_TBL* cond_tbl) {

	std::vector<SM_GC_POI_QSORT_WORK_TBL> sort_data_list;
	std::vector<SM_GC_POI_FAVORITE_TBL> favorite_tbl_list;

	int rc = SC_POI_selectGCfavoriteTBL(cond_tbl, &sort_data_list, &favorite_tbl_list);
	if (rc != e_PAL_RESULT_SUCCESS) {
		return (rc);
	}

	jstring wstr = NULL;
	for (int i = 0; i < sort_data_list.size(); i++) {
		SM_GC_POI_QSORT_WORK_TBL worktbl = sort_data_list.at(i);
		SM_GC_POI_FAVORITE_TBL favorite_tbl = favorite_tbl_list.at(worktbl.id);

		jobject obj = env->NewObject(item_cls, item_method);

		wstr = env->NewStringUTF((const char*)favorite_tbl.userid.c_str());
		env->SetObjectField(obj, out_userid_id, wstr);
		env->DeleteLocalRef(wstr);

		wstr = env->NewStringUTF((const char*)favorite_tbl.dbtime.c_str());
		env->SetObjectField(obj, out_dbtime_id, wstr);
		env->DeleteLocalRef(wstr);

		wstr = env->NewStringUTF((const char*)favorite_tbl.contentstime.c_str());
		env->SetObjectField(obj, out_contentstime_id, wstr);
		env->DeleteLocalRef(wstr);

		wstr = env->NewStringUTF((const char*)favorite_tbl.id.c_str());
		env->SetObjectField(obj, out_id_id, wstr);
		env->DeleteLocalRef(wstr);

		wstr = env->NewStringUTF((const char*)favorite_tbl.url.c_str());
		env->SetObjectField(obj, out_url_id, wstr);
		env->DeleteLocalRef(wstr);

		wstr = env->NewStringUTF((const char*)favorite_tbl.name.c_str());
		env->SetObjectField(obj, out_name_id, wstr);
		env->DeleteLocalRef(wstr);

		wstr = env->NewStringUTF((const char*)favorite_tbl.pos_name.c_str());
		env->SetObjectField(obj, out_pos_name_id, wstr);
		env->DeleteLocalRef(wstr);

		wstr = env->NewStringUTF((const char*)favorite_tbl.contents.c_str());
		env->SetObjectField(obj, out_contents_id, wstr);
		env->DeleteLocalRef(wstr);

		if (favorite_tbl.binary_data) {
			jbyteArray jarray = env->NewByteArray(favorite_tbl.binary_data_len);
			env->SetByteArrayRegion(jarray, 0, favorite_tbl.binary_data_len, reinterpret_cast<jbyte*>(favorite_tbl.binary_data));
			env->SetObjectField(obj, out_binary_data_id, jarray);
			delete[] favorite_tbl.binary_data;
		}

		env->SetIntField(obj, out_binary_data_len_id, favorite_tbl.binary_data_len);
		env->SetIntField(obj, out_lat_id, favorite_tbl.lat);
		env->SetIntField(obj, out_log_id, favorite_tbl.log);

		// リストに追加
		env->CallBooleanMethod(out_obj, mid_ListAdd, obj);

		env->DeleteLocalRef(obj);

		rc = e_PAL_RESULT_SUCCESS;
	}

	SC_LOG_DebugPrint(SC_TAG_CORE, SC_LOG_END);
	return (rc);
}

/**
 * 登録地取得用暫定ラッパ関数<br>
 * SMPAL_POI_POINT_TBL.cpp#SC_POI_selectGCpointTBL からの移植<br>
 * 上記関数のJNI実装部分のみ抜き出した処理となります。<br>
 */
int SC_POI_selectGCpointTBL_forJni(
		JNIEnv * env,
		jclass thiz,
		jobject in_obj,
		jobject out_obj,
		jmethodID mid_ListAdd,
		jclass item_cls,
		jmethodID item_method,
		jfieldID out_data_type_id,
		jfieldID out_userid_id,
		jfieldID out_datetime_id,
		jfieldID out_gemid_id,
		jfieldID out_gemspotid_id,
		jfieldID out_pos_name_id,
		jfieldID out_lat_id,
		jfieldID out_log_id,
		jfieldID out_len_id,
		SC_POI_SEARCH_COND_TBL* cond_tbl){

	std::vector<SM_GC_POI_QSORT_WORK_TBL> sort_data_list;
	std::vector<SM_GC_POI_POINT_TBL> point_tbl_list;

	int rc = SC_POI_selectGCpointTBL(cond_tbl, &sort_data_list, &point_tbl_list);
	if (rc != e_PAL_RESULT_SUCCESS) {
		return (rc);
	}

	jstring wstr = NULL;
	for (int i = 0; i < sort_data_list.size(); i++) {
		SM_GC_POI_QSORT_WORK_TBL worktbl = sort_data_list.at(i);
		SM_GC_POI_POINT_TBL point_tbl = point_tbl_list.at(worktbl.id);

		jobject obj = env->NewObject(item_cls, item_method);

		env->SetIntField(obj, out_data_type_id, point_tbl.data_type);

		wstr = env->NewStringUTF((const char*)point_tbl.userid.c_str());
		env->SetObjectField(obj, out_userid_id, wstr);
		env->DeleteLocalRef(wstr);

		wstr = env->NewStringUTF((const char*)point_tbl.datetime.c_str());
		env->SetObjectField(obj, out_datetime_id, wstr);
		env->DeleteLocalRef(wstr);

		wstr = env->NewStringUTF((const char*)point_tbl.gemid.c_str());
		env->SetObjectField(obj, out_gemid_id, wstr);
		env->DeleteLocalRef(wstr);

		wstr = env->NewStringUTF((const char*)point_tbl.gemspotid.c_str());
		env->SetObjectField(obj, out_gemspotid_id, wstr);
		env->DeleteLocalRef(wstr);

		wstr = env->NewStringUTF((const char*)point_tbl.gemspotid.c_str());
		env->SetObjectField(obj, out_gemspotid_id, wstr);
		env->DeleteLocalRef(wstr);

		wstr = env->NewStringUTF((const char*)point_tbl.pos_name.c_str());
		env->SetObjectField(obj, out_pos_name_id, wstr);
		env->DeleteLocalRef(wstr);

		env->SetIntField(obj, out_lat_id, point_tbl.lat);
		env->SetIntField(obj, out_log_id, point_tbl.log);
		env->SetIntField(obj, out_len_id, point_tbl.len);

		// リストに追加
		env->CallBooleanMethod(out_obj, mid_ListAdd, obj);

		env->DeleteLocalRef(obj);

		rc = e_PAL_RESULT_SUCCESS;
	}

	return (rc);
}

/**
 * 履歴取得用暫定ラッパ関数<br>
 * SMPAL_POI_HISTORY_TBL.cpp#SC_POI_selectGChistoryTBL からの移植<br>
 * 上記関数のJNI実装部分のみ抜き出した処理となります。<br>
 */
int SC_POI_selectGChistoryTBL_forJni(
		JNIEnv * env,
		jclass thiz,
		jobject in_obj,
		jobject out_obj,
		jmethodID mid_ListAdd,
		jclass item_cls,
		jmethodID item_method,
		jfieldID out_userid_id,
		jfieldID out_datetime_id,
		jfieldID out_gemid_id,
		jfieldID out_gemspotid_id,
		jfieldID out_lat_id,
		jfieldID out_log_id,
		SC_POI_SEARCH_COND_2_TBL* cond_tbl){

	std::vector<SM_GC_POI_HISTORY_TBL> history_tbl_list;
	int rc = SC_POI_selectGChistoryTBL(cond_tbl, &history_tbl_list);
	if (rc != e_PAL_RESULT_SUCCESS){
		return (rc);
	}

	jstring wstr = NULL;
	for (int i = 0; i < history_tbl_list.size(); i++) {
		SM_GC_POI_HISTORY_TBL history_tbl = history_tbl_list.at(i);

		jobject obj = env->NewObject(item_cls, item_method);

		wstr = env->NewStringUTF((const char*)history_tbl.userid.c_str());
		env->SetObjectField(obj, out_userid_id, env->NewStringUTF((const char*)history_tbl.userid.c_str()));
		env->DeleteLocalRef(wstr);

		wstr = env->NewStringUTF((const char*)history_tbl.datetime.c_str());
		env->SetObjectField(obj, out_datetime_id, env->NewStringUTF((const char*)history_tbl.datetime.c_str()));
		env->DeleteLocalRef(wstr);

		wstr = env->NewStringUTF((const char*)history_tbl.gemid.c_str());
		env->SetObjectField(obj, out_gemid_id, env->NewStringUTF((const char*)history_tbl.gemid.c_str()));
		env->DeleteLocalRef(wstr);

		wstr = env->NewStringUTF((const char*)history_tbl.gemspotid.c_str());
		env->SetObjectField(obj, out_gemspotid_id, env->NewStringUTF((const char*)history_tbl.gemspotid.c_str()));
		env->DeleteLocalRef(wstr);

		env->SetIntField(obj, out_lat_id, history_tbl.lat);
		env->SetIntField(obj, out_log_id, history_tbl.log);

		// リストに追加
		env->CallBooleanMethod(out_obj, mid_ListAdd, obj);

		env->DeleteLocalRef(obj);

		rc = e_PAL_RESULT_SUCCESS;
	}
	return (rc);
}
#endif /* ANDROID */
