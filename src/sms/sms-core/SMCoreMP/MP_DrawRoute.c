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

static E_SC_RESULT MP_DRAW_RouteIcon(DOUBLE lat, DOUBLE lon, MP_ICON_TYPE_e iconType);
static E_SC_RESULT MP_DRAW_RouteSplitLine(UINT32 aPclId, UINT16 aX, UINT16 aY, UINT32 bPclId, UINT16 bX, UINT16 bY, FLOAT width);

#define _DRAW_LV2SPLITROUTE					(0)		/* レベル２経路情報が入ってきた場合にレベル２のみ断裂表示を行う */
#define _DRAW_COLORCNGROADKIND				(0)		/* 経路の色を道路種別で変更する(デバッグ用) */

#if _DRAW_COLORCNGROADKIND
#define SCMP_SET_ROUTECOLOR(kind)													\
{																					\
	if (3 > kind) {																	\
		MP_GL_ColorRGBA(SET_RGBA(0xEE,0xEE,0x50,0xFF));								\
	} else if (6 < kind) {															\
		MP_GL_ColorRGBA(SET_RGBA(0xEE,0x50,0x50,0xFF));								\
	} else {																		\
		MP_GL_ColorRGBA(ParamRoute_Color());										\
	}																				\
}
#endif

E_SC_RESULT MP_DRAW_Route(void)
{
	E_SC_RESULT ret = e_SC_RESULT_SUCCESS;
	INT32 i = 0;
	INT32 j = 0;
	INT32 k = 0;
	INT32 e = 0;
	SC_RP_LinkInfo* p_link_info = NULL;
	SC_RP_FormInfo* p_form_info = NULL;
	SC_RP_SectInfo* p_sect_info = NULL;
	SC_RP_ParcelInfo* p_parcel_info = NULL;
	SC_RP_RouteMng* p_mng = NULL;
	UINT32 parcel_id = MP_INVALID_VALUE_32;
	UINT32 link_id = 0;
	//DOUBLE lat, lon;
	FLOAT x, y;
	FLOAT width;
	RGBACOLOR color;
	UINT32 route_id;
	UINT32 route_type;
	INT32 draw_point_cnt;
	UINT16 preX = 0;
	UINT16 preY = 0;
	UINT32 prePclId = MP_INVALID_VALUE_32;
	Bool split_f = false;
	//Bool level2_f = false;
	PARCEL_INFO_t parcelInfo;

	// 経路描画用バッファ
	static T_POINT gRouteBuf[MP_LINK_POINT_CNT*2];

	T_VIEW_INFO* p_vi;
	p_vi = MP_GetViewInfo();

	SC_LOG_DebugPrint(SC_TAG_MP, (Char*)"sc_draw_DrawRoute, " HERE);

	// 探索関数から経路取得
	ret = SC_RP_GetCurrentRouteId(&route_id, &route_type);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_LC, (Char*)"SC_RP_GetCurrentRouteId, ret=%d, " HERE, ret);
		return (ret);
	}
	ret = SC_RP_ReadRouteEntry(route_id, route_type, SC_RP_USER_MAP, &p_mng);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_LC, (Char*)"SC_RP_ReadRouteEntry, ret=%d, " HERE, ret);
		return (ret);
	}

	if(p_mng->parcelInfo == NULL) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_RP_GetRoute parcelInfo NULL, " HERE);
		return (e_SC_RESULT_FAIL);
	}
	if(p_mng->linkInfo == NULL) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_RP_GetRoute linkInfo NULL, " HERE);
		return (e_SC_RESULT_FAIL);
	}
	if(p_mng->formInfo == NULL) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"SC_RP_GetRoute formInfo NULL, " HERE);
		return (e_SC_RESULT_FAIL);
	}
	SC_LOG_DebugPrint(SC_TAG_MP, (Char*)"SC_RP_GetRoute OK, " HERE);

	// タイル地図に当て込んで経路描画
	p_sect_info = p_mng->sectInfo;
	p_parcel_info = p_mng->parcelInfo;	// パーセル情報先頭アドレス
	p_link_info = p_mng->linkInfo;	// リンク情報先頭アドレス
	p_form_info = p_mng->formInfo;	// 形状点情報先頭アドレス

	// パーセルID
	parcel_id = p_parcel_info->parcelId;

	width = ParamRoute_Size(p_vi->scale_level) * (1.0f/p_vi->scale);
	color = ParamRoute_Color();
	MP_GL_ColorRGBA(color);

	// 区間数分ループ
	for (e = 0; e < p_mng->sectVol; e++, p_sect_info++) {
		p_parcel_info = p_mng->parcelInfo + p_sect_info->parcelIdx;
		p_link_info = p_mng->linkInfo + p_sect_info->linkIdx;
		p_form_info = p_mng->formInfo + p_sect_info->formIdx;

		// パーセル情報数分ループ
		for (k = 0; k < p_sect_info->parcelVol; k++, p_parcel_info++) {

#if _DRAW_LV2SPLITROUTE
			/* 経路にLV2が含まれている箇所を断裂表示する */
			if (MAP_LEVEL1 != MESHC_GetLevel(p_parcel_info->parcelId)) {
				if (!level2_f) {
					prePclId = p_parcel_info->parcelId;
					SC_RP_LinkInfo* p_link_info_wk = p_link_info + p_parcel_info->linkIdx;
					if (p_link_info_wk->orFlag) {
						preX = (p_form_info + p_link_info_wk->formIdx + p_link_info_wk->formVol - 1)->x;
						preY = (p_form_info + p_link_info_wk->formIdx + p_link_info_wk->formVol - 1)->y;
					} else {
						preX = (p_form_info + p_link_info_wk->formIdx)->x;
						preY = (p_form_info + p_link_info_wk->formIdx)->y;
					}
					level2_f = true;
				}
				continue;
			} else if (level2_f) {
				level2_f = false;
				MP_DRAW_RouteSplitLine(prePclId, preX, preY, p_parcel_info->parcelId,
						(p_form_info + (p_link_info + p_parcel_info->linkIdx)->formIdx)->x,
						(p_form_info + (p_link_info + p_parcel_info->linkIdx)->formIdx)->y,
						ParamRoute_SplitLinkSize(p_vi->scale_level) * (1.0f / p_vi->scale));
			}
#endif
			// パーセルID
			parcel_id = p_parcel_info->parcelId;

			MP_DRAW_GetParcelInfo(parcel_id, &parcelInfo);
			if (MP_INVALID_VALUE_16 == p_sect_info->splitIdx) {
				if (!MP_DRAW_CheckDrawParcel(&parcelInfo)) {
					continue;
				}
			}

			MP_GL_PushMatrix();
			MP_GL_Translatef((FLOAT) (parcelInfo.pixX - p_vi->pixelCoord.x), (FLOAT) (parcelInfo.pixY - p_vi->pixelCoord.y), 0.0);
			draw_point_cnt = 0;

			// リンク数分ループ
			for (i = p_parcel_info->linkIdx; i < (p_parcel_info->linkIdx + p_parcel_info->linkVol); i++) {

				if (p_link_info[i].splitFlag) {
					if (!split_f) {
						split_f = true;
						prePclId = p_parcel_info->parcelId;
						if (p_link_info[i].orFlag) {
							preX = p_form_info[p_link_info[i].formIdx].x;
							preY = p_form_info[p_link_info[i].formIdx].y;
						} else {
							preX = p_form_info[p_link_info[i].formIdx + p_link_info[i].formVol - 1].x;
							preY = p_form_info[p_link_info[i].formIdx + p_link_info[i].formVol - 1].y;
						}
					}
				}
				// リンクID
				link_id = p_link_info[i].linkId;

				// 先頭リンク以外は1つ前のリンクの終点側が現リンクの始点側と一致する為、
				// 1つ前のリンクの終点座標を削除する。
				if (draw_point_cnt != 0) {
					draw_point_cnt--;
				}

				// 形状点数分ループ
				if (p_link_info[i].orFlag == 0) {
					// 順方向：格納順にアクセス
					for (j = 0; j < p_link_info[i].formVol; j++) {
						x = p_form_info[p_link_info[i].formIdx + j].x;
						y = p_form_info[p_link_info[i].formIdx + j].y;

						gRouteBuf[draw_point_cnt].x = x * parcelInfo.keisuuX;
						gRouteBuf[draw_point_cnt].y = y * parcelInfo.keisuuY;
						draw_point_cnt++;
					}
				} else {
					// 逆方向：格納順の逆からアクセス
					for (j = p_link_info[i].formVol-1; j >= 0; j--) {
						x = p_form_info[p_link_info[i].formIdx + j].x;
						y = p_form_info[p_link_info[i].formIdx + j].y;

						gRouteBuf[draw_point_cnt].x = x * parcelInfo.keisuuX;
						gRouteBuf[draw_point_cnt].y = y * parcelInfo.keisuuY;
						draw_point_cnt++;
					}
				}
#if _DRAW_COLORCNGROADKIND
				if (draw_point_cnt > MP_LINK_POINT_CNT) {
					SCMP_SET_ROUTECOLOR(p_link_info[i].roadKind);
					MP_GL_DrawLines(gRouteBuf, draw_point_cnt, width);
					MP_GL_DrawCircleFillEx(&gRouteBuf[0], width / 2);
					MP_GL_DrawCircleFillEx(&gRouteBuf[draw_point_cnt - 1], width / 2);
					draw_point_cnt = 0;
				}
				// 次の道路種別が切り替わるタイミングで表示
				if (i + 1 != (p_parcel_info->linkIdx + p_parcel_info->linkVol)) {
					if (p_link_info[i].roadKind != p_link_info[i + 1].roadKind) {
						if (draw_point_cnt > 1) {
							SCMP_SET_ROUTECOLOR(p_link_info[i].roadKind);
							MP_GL_DrawLines(gRouteBuf, draw_point_cnt, width);
							MP_GL_DrawCircleFillEx(&gRouteBuf[0], width / 2);
							MP_GL_DrawCircleFillEx(&gRouteBuf[draw_point_cnt - 1], width / 2);
							draw_point_cnt = 0;
						}
					}
				}
			}
			if (draw_point_cnt > 1) {
				SCMP_SET_ROUTECOLOR(p_link_info[i - 1].roadKind);
				MP_GL_DrawLines(gRouteBuf, draw_point_cnt, width);
				MP_GL_DrawCircleFillEx(&gRouteBuf[0], width / 2);
				MP_GL_DrawCircleFillEx(&gRouteBuf[draw_point_cnt - 1], width / 2);
				draw_point_cnt = 0;
			}
#else
				if (draw_point_cnt > MP_LINK_POINT_CNT) {
					MP_GL_DrawLines(gRouteBuf, draw_point_cnt, width);
					MP_GL_DrawCircleFillEx(&gRouteBuf[0], width / 2);
					MP_GL_DrawCircleFillEx(&gRouteBuf[draw_point_cnt - 1], width / 2);
					draw_point_cnt = 0;
				}
			}
			if (draw_point_cnt > 1) {
				MP_GL_DrawLines(gRouteBuf, draw_point_cnt, width);
				MP_GL_DrawCircleFillEx(&gRouteBuf[0], width / 2);
				MP_GL_DrawCircleFillEx(&gRouteBuf[draw_point_cnt - 1], width / 2);
				draw_point_cnt = 0;
			}
#endif
			MP_GL_PopMatrix();
		}
#if _DRAW_LV2SPLITROUTE
		if (level2_f || MP_INVALID_VALUE_16 != p_sect_info->splitIdx) {
			MP_DRAW_RouteSplitLine(prePclId, preX, preY, p_sect_info->parcelId, p_sect_info->x, p_sect_info->y,
					ParamRoute_SplitLinkSize(p_vi->scale_level) * (1.0f / p_vi->scale));
			split_f = false;
			level2_f = false;
		}
#else
		if (MP_INVALID_VALUE_16 != p_sect_info->splitIdx) {
			MP_DRAW_RouteSplitLine(prePclId, preX, preY, p_sect_info->parcelId, p_sect_info->x, p_sect_info->y,
					ParamRoute_SplitLinkSize(p_vi->scale_level) * (1.0f / p_vi->scale));
			split_f = false;
		}
#endif
	}

	ret = SC_RP_ReadRouteExit(route_id, route_type, SC_RP_USER_MAP);
	if(e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_LC, (Char*)"SC_RP_ReadRouteExit, ret=%d, " HERE, ret);
	}
	return (ret);
}

E_SC_RESULT MP_DRAW_RoutePoint(void)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	SMRPPOINT	point[SC_CORE_RP_PLACE_MAX] = {};
	INT32		pointNum = 0;
	INT32		i = 0;
	DOUBLE		lat;
	DOUBLE		lon;
	T_VIEW_INFO* p_vi;

	p_vi = MP_GetViewInfo();

	// 目的地緯度経度取得
	ret = SC_MNG_GetAllRPPlace(point, &pointNum);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_CORE, (Char*)"SC_MNG_GetAllRPPlace error(0x%08x), " HERE, ret);
		return (ret);
	}

	// デバッグモード
	for (i=0; i<pointNum; i++) {
		lat = point[i].coord.latitude/1024.0/3600.0;
		lon = point[i].coord.longitude/1024.0/3600.0;
		if (LST_START == point[i].rpPointType) {
			// 出発値
			MP_DRAW_RouteIcon(lat, lon, MP_ICON_TYPE_START);
		} else if (LST_DEST == point[i].rpPointType) {
			// 目的地
			MP_DRAW_RouteIcon(lat, lon, MP_ICON_TYPE_DEST);
		} else if (LST_BYPASS == point[i].rpPointType) {
			// 経由地
			MP_DRAW_RouteIcon(lat, lon, MP_ICON_TYPE_TRANSIT);
		}
	}

	return (ret);
}

static E_SC_RESULT MP_DRAW_RouteIcon(DOUBLE lat, DOUBLE lon, MP_ICON_TYPE_e iconType)
{
	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	FLOAT		point_x;
	FLOAT		point_y;
	T_VIEW_INFO* p_vi;

	MP_GL_BeginBlend();

	p_vi = MP_GetViewInfo();

	MP_DRAW_GetPixelPos(lat, lon, &point_x, &point_y);

	// アイコン表示
	ret = MP_ICON_Draw(point_x, point_y, p_vi->disp_angle, (1.0f/p_vi->scale), ParamIcon_IconID(iconType));
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_MP, (Char*)"MP_ICON_Draw, ret=%d, " HERE, ret);
	}

	MP_GL_EndBlend();

	return (ret);
}

static E_SC_RESULT MP_DRAW_RouteSplitLine(UINT32 aPclId, UINT16 aX, UINT16 aY, UINT32 bPclId, UINT16 bX, UINT16 bY,
		FLOAT width) {

	E_SC_RESULT	ret = e_SC_RESULT_SUCCESS;
	RGBACOLOR color;
	DOUBLE aLat, aLon;
	DOUBLE bLat, bLon;
	FLOAT x, y;
	static T_POINT gRouteBuf[2];

	color = ParamRoute_SplitLinkColor();
	MP_GL_ColorRGBA(color);

	MESHC_ChgParcelIDToLatLon(MESHC_GetLevel(aPclId), aPclId, aX, aY, &aLat, &aLon);
	MP_DRAW_GetPixelPos(aLat / 3600.0, aLon / 3600.0, &x, &y);

	gRouteBuf[0].x = x;
	gRouteBuf[0].y = y;

	MESHC_ChgParcelIDToLatLon(MESHC_GetLevel(bPclId), bPclId, bX, bY, &bLat, &bLon);
	MP_DRAW_GetPixelPos(bLat / 3600.0, bLon / 3600.0, &x, &y);

	gRouteBuf[1].x = x;
	gRouteBuf[1].y = y;

	MP_GL_DrawLines(gRouteBuf, 2, width);
	MP_GL_DrawCircleFillEx(&gRouteBuf[0], width/2);
	MP_GL_DrawCircleFillEx(&gRouteBuf[1], width/2);

	color = ParamRoute_Color();
	MP_GL_ColorRGBA(color);

	return (ret);
}
