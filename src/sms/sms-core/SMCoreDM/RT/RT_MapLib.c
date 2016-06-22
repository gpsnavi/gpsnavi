/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

/*-----------------------------------------------------------------------------------------------*/
/* File：RT_MapDataGet.c                                                                         */
/* Info：地図データ取得                                                                          */
/*-----------------------------------------------------------------------------------------------*/

#include "sms-core/SMCoreDM/SMCoreDMInternal.h"

static UINT8* 		RT_MAP_GetSharpRecord(UINT8* );
static E_SC_RESULT 	RT_MAP_GetSharpPoint(UINT8 *, RT_LINKPOINT_t *);

static E_SC_RESULT 	RT_MAP_GetCrossLink(RT_MAPREQ_t *, RT_LINK_t *, RT_CROSSINFO_t *);
static E_SC_RESULT 	RT_MAP_GetCrossNodeAttr(RT_MAPREQ_t *, RT_LINK_t *, RT_CROSSINFO_t *);
static UINT32 		RT_MAP_GetNextParcelID(UINT32 , UINT8 );

/**
 * @brief	地図データ交差点リンク情報取得
 */
E_SC_RESULT RT_MAP_GetCrossInfo(RT_MAPREQ_t *reqtbl_p, RT_LINK_t *in_link_p, RT_CROSSINFO_t *crs_p)
{
	E_SC_RESULT					ret;

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_START);

	if (NULL == reqtbl_p || NULL == crs_p || NULL == in_link_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[MAP] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 出力テーブル初期化
	memset(crs_p, 0x00, sizeof(RT_CROSSINFO_t));

	// 交差点接続リンク取得
	ret = RT_MAP_GetCrossLink(reqtbl_p, in_link_p, crs_p);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[MAP] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 交差点ノード属性情報取得
	ret = RT_MAP_GetCrossNodeAttr(reqtbl_p, in_link_p, crs_p);
	if (e_SC_RESULT_SUCCESS != ret) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[MAP] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	SC_LOG_DebugPrint(SC_TAG_RT, SC_LOG_END);
	return (e_SC_RESULT_SUCCESS);

}

static E_SC_RESULT RT_MAP_GetCrossLink(RT_MAPREQ_t *reqtbl_p, RT_LINK_t *in_link_p, RT_CROSSINFO_t *crs_p)
{

	E_SC_RESULT					ret;
	RT_MAPDATA_t				*c_maptbl_p;
	RT_MAPDATA_t				*n_maptbl_p;
	RT_MAPADDR_t				map_addr[16];
	static RT_LINKPOINT_t		point;
	UINT8						*crd_head_addr;
	UINT8						*crd_addr;
	UINT8						*nrd_head_addr;
	UINT8						*nrd_addr;
	UINT8						*crd_head_cnct_addr;
	UINT8						*nrd_head_cnct_addr;
	UINT8						*crd_ex_addr;
	UINT8						*crd_head_ex_addr;
	UINT8						*nrd_ex_addr;
	UINT8						*nrd_head_ex_addr;
	UINT8						*sp_head_addr;
	UINT8						*sp_addr;

	UINT16						index;
	UINT32						pid;
	UINT32						cid;
	UINT32						offset;
	UINT16						side;
	UINT16						link_vol = 0;
	UINT16						ilp;
	UINT16						idx;
	UINT32						id;

	if (NULL == reqtbl_p || NULL == in_link_p || NULL == crs_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[MAP] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// ワークテーブル初期化
	memset(&(map_addr[0]), 0x00, sizeof(RT_MAPADDR_t) * 16);

	// 自パーセル情報取得
	for (ilp = 0 ; ilp < reqtbl_p->data_vol ; ilp++) {
		c_maptbl_p = &(reqtbl_p->data[ilp]);
		if (c_maptbl_p->parcel_id == in_link_p->parcel_id) break;
	}
	if ((ilp >= reqtbl_p->data_vol) || (NULL == c_maptbl_p->road_p)) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[MAP] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 道路データのリンク情報先頭アドレス取得
	crd_head_addr = SC_MA_A_NWBIN_GET_NWRCD_LINK(c_maptbl_p->road_p);

	// 道路データの接続情報先頭アドレス取得
	crd_head_cnct_addr = SC_MA_A_NWBIN_GET_NWRCD_CNCT(c_maptbl_p->road_p);

	// 道路データのリンク拡張先頭アドレス取得
	crd_head_ex_addr = SC_MA_A_NWBIN_GET_NWLINKEX(c_maptbl_p->road_p);

	// 進入リンクインデックス情報取得
	index = SC_MA_BinSearchNwRecord(c_maptbl_p->road_p, in_link_p->link_id, SC_MA_BINSRC_TYPE_LINK);
	if (ALLF16 == index) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[MAP] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 進入リンクのリンク情報アドレス取得
	crd_addr = SC_MA_A_NWRCD_LINK_GET_RECORD(crd_head_addr, (index - 1));

	// 進入リンクの形状データオフセット取得
	if (SC_MA_D_NWRCD_LINK_GET_LINKEXOFSFLG(crd_addr)) {
		// リンク拡張データ取得
		crd_ex_addr = SC_MA_A_NWRCD_LINKEX_GET_RECORD(crd_head_ex_addr, SC_MA_D_NWRCD_LINK_GET_EXOFS(crd_addr));
		// リンク拡張データ内 形状オフセット取得
		offset = SC_MA_D_NWRCD_EXLINK_GET_FORMOFS(crd_ex_addr);
	} else {
		offset = SC_MA_D_NWRCD_LINK_GET_FORMOFS(crd_addr);
	}

	// 形状データのリンク情報先頭アドレス取得
	sp_head_addr = RT_MAP_GetSharpRecord(c_maptbl_p->sharp_p);
	if (NULL == sp_head_addr) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[MAP] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 進入リンクのリンク情報アドレス取得
	sp_addr = SC_MA_A_SHRCD_GET_RECORD(sp_head_addr, offset);

	// 交差点への接続先始終点を取得
	side = (~in_link_p->link_dir) & 0x0001;

	map_addr[link_vol].parcel_id  = c_maptbl_p->parcel_id;
	map_addr[link_vol].road_addr  = crd_addr;
	map_addr[link_vol].sharp_addr = sp_addr;
	map_addr[link_vol].cnct_side  = side;
	link_vol++;

	while(1){
		// 接続元はリンク始点
		if (0 == side) {
			// 接続リンクと手を結んでいる方向（終点)のＩＤとインデックスを取得
			id  = SC_MA_D_NWRCD_LINK_GET_STID(crd_addr);
			idx = SC_MA_D_NWRCD_LINK_GET_STIDX(crd_addr);
		} else {
			// 接続リンクと手を結んでいる方向（終点)のＩＤとインデックスを取得
			id  = SC_MA_D_NWRCD_LINK_GET_EDID(crd_addr);
			idx = SC_MA_D_NWRCD_LINK_GET_EDIDX(crd_addr);
		}

		// 接続先はリンク始点
		if ((id & SC_MA_NWID_SUB_CNCTSIDE) == 0x02000000) {
			side = 0;
		}else{
			side = 1;
		}

		// 接続先ＩＤはリンクＩＤ
		if (0 == (id & SC_MA_NWID_PNT_TYPE)) {
			// 接続リンクのリンク情報アドレス取得
			crd_addr = SC_MA_A_NWRCD_LINK_GET_RECORD(crd_head_addr, (idx - 1));

			// リンクＩＤが同一
			if (SC_MA_D_NWRCD_LINK_GET_ID(crd_addr) == in_link_p->link_id) {
				break;		// 一周した or 接続なし
			}

			// 進入リンクの形状データオフセット取得
			if (SC_MA_D_NWRCD_LINK_GET_LINKEXOFSFLG(crd_addr)) {
				// リンク拡張データ取得
				crd_ex_addr = SC_MA_A_NWRCD_LINKEX_GET_RECORD(crd_head_ex_addr, SC_MA_D_NWRCD_LINK_GET_EXOFS(crd_addr));
				// リンク拡張データ内 形状オフセット取得
				offset = SC_MA_D_NWRCD_EXLINK_GET_FORMOFS(crd_ex_addr);
			} else {
				offset = SC_MA_D_NWRCD_LINK_GET_FORMOFS(crd_addr);
			}

			// 形状データのリンク情報先頭アドレス取得
			sp_head_addr = RT_MAP_GetSharpRecord(c_maptbl_p->sharp_p);
			if (NULL == sp_head_addr) {
				SC_LOG_ErrorPrint(SC_TAG_RT, "[MAP] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}

			// 進入リンクのリンク情報アドレス取得
			sp_addr = SC_MA_A_SHRCD_GET_RECORD(sp_head_addr, offset);

			map_addr[link_vol].parcel_id  = c_maptbl_p->parcel_id;
			map_addr[link_vol].road_addr  = crd_addr;
			map_addr[link_vol].sharp_addr = sp_addr;
			map_addr[link_vol].cnct_side  = side;
			link_vol++;


		} else {
			// 接続リンクのリンク情報アドレス取得
			crd_addr = SC_MA_A_NWRCD_CNCT_GET_RECORD(crd_head_cnct_addr, (idx - 1));

			// 接続リンクと手を結んでいる方向（終点)のＩＤとインデックスを取得
			id  = SC_MA_D_NWRCD_CNCT_GET_EDID(crd_addr);
			idx = SC_MA_D_NWRCD_CNCT_GET_EDIDX(crd_addr);

			// 図郭方向に対するパーセルＩＤを取得
			pid = RT_MAP_GetNextParcelID(in_link_p->parcel_id, SC_MA_D_NWID_GET_SUB_CNCTDIR(id));
			if (ALLF32 == pid) {
				SC_LOG_ErrorPrint(SC_TAG_RT, "[MAP] ERROR " HERE);
				return (e_SC_RESULT_FAIL);
			}

			// 自パーセル内で接続リンク同士が手を結んでいる
			if (in_link_p->parcel_id == pid) {
				// 接続ＩＤ保管
				cid = SC_MA_D_NWRCD_CNCT_GET_ID(crd_addr);

				// 図郭内パーセル情報取得
				n_maptbl_p = c_maptbl_p;

				// 図郭内接続リンク情報先頭アドレス取得
				nrd_head_cnct_addr = SC_MA_A_NWBIN_GET_NWRCD_CNCT(n_maptbl_p->road_p);

				// 図郭内接続リンク情報アドレス取得
				nrd_addr = SC_MA_A_NWRCD_CNCT_GET_RECORD(nrd_head_cnct_addr, (idx - 1));

				// 道路データのリンク拡張先頭アドレス取得
				nrd_head_ex_addr = SC_MA_A_NWBIN_GET_NWLINKEX(n_maptbl_p->road_p);

				// 接続ＩＤが同一
				if (SC_MA_D_NWRCD_CNCT_GET_ID(nrd_addr) == cid) {
					continue;	// 図郭先収録外
				}

			} else {
				// 図郭外パーセル情報取得
				for (ilp = 0 ; ilp < reqtbl_p->data_vol ; ilp++) {
					n_maptbl_p = &(reqtbl_p->data[ilp]);
					if (n_maptbl_p->parcel_id == pid) break;
				}
				if ((ilp >= reqtbl_p->data_vol) || (NULL == n_maptbl_p->road_p)) {
					continue;	// 図郭先データ無し
				}

				// 図郭外接続リンク情報先頭アドレス取得
				nrd_head_cnct_addr = SC_MA_A_NWBIN_GET_NWRCD_CNCT(n_maptbl_p->road_p);

				// 道路データのリンク拡張先頭アドレス取得
				nrd_head_ex_addr = SC_MA_A_NWBIN_GET_NWLINKEX(n_maptbl_p->road_p);

				// 接続インデックス情報取得
				index = SC_MA_BinSearchNwRecord(n_maptbl_p->road_p, id, SC_MA_BINSRC_TYPE_CNCT);
				if (ALLF16 == index) {
					SC_LOG_ErrorPrint(SC_TAG_RT, "[MAP] ERROR " HERE);
					return (e_SC_RESULT_FAIL);
				}

				// 図郭外接続リンク情報アドレス取得
				nrd_addr = SC_MA_A_NWRCD_CNCT_GET_RECORD(nrd_head_cnct_addr, (index - 1));
			}

			// 接続リンクが手を結んでいる実リンクを検索
			while(1){
				// 実リンクと手を結んでいる方向（始点)のＩＤとインデックスを取得
				id  = SC_MA_D_NWRCD_CNCT_GET_STID(nrd_addr);
				idx = SC_MA_D_NWRCD_CNCT_GET_STIDX(nrd_addr);

				// 接続先ＩＤはリンクＩＤ (発見!)
				if (0 == (id & SC_MA_NWID_PNT_TYPE)) {
					// 図郭外接続リンク情報先頭アドレス取得
					nrd_head_addr = SC_MA_A_NWBIN_GET_NWRCD_LINK(n_maptbl_p->road_p);

					// 接続先リンク情報アドレス取得
					nrd_addr = SC_MA_A_NWRCD_LINK_GET_RECORD(nrd_head_addr, (idx - 1));

					// 進入リンクの形状データオフセット取得
					if (SC_MA_D_NWRCD_LINK_GET_LINKEXOFSFLG(nrd_addr)) {
						// リンク拡張データ取得
						nrd_ex_addr = SC_MA_A_NWRCD_LINKEX_GET_RECORD(nrd_head_ex_addr, SC_MA_D_NWRCD_LINK_GET_EXOFS(nrd_addr));
						// リンク拡張データ内 形状オフセット取得
						offset = SC_MA_D_NWRCD_EXLINK_GET_FORMOFS(nrd_ex_addr);
					} else {
						offset = SC_MA_D_NWRCD_LINK_GET_FORMOFS(nrd_addr);
					}

					// 形状データのリンク情報先頭アドレス取得
					sp_head_addr = RT_MAP_GetSharpRecord(n_maptbl_p->sharp_p);
					if (NULL == sp_head_addr) {
						SC_LOG_ErrorPrint(SC_TAG_RT, "[MAP] ERROR " HERE);
						return (e_SC_RESULT_FAIL);
					}

					// 進入リンクのリンク情報アドレス取得
					sp_addr = SC_MA_A_SHRCD_GET_RECORD(sp_head_addr, offset);

					map_addr[link_vol].parcel_id  = n_maptbl_p->parcel_id;
					map_addr[link_vol].road_addr  = nrd_addr;
					map_addr[link_vol].sharp_addr = sp_addr;

					// 接続先＝始点
					if ((id & SC_MA_NWID_SUB_CNCTSIDE) == 0x02000000) {
						map_addr[link_vol].cnct_side = 0;
					}else{
						map_addr[link_vol].cnct_side = 1;
					}
					link_vol++;
					break;

				} else {
					// 接続のリンク情報アドレス取得
					nrd_addr = SC_MA_A_NWRCD_CNCT_GET_RECORD(nrd_head_cnct_addr, (idx - 1));
				}
			}
		}
	}


	// 交差点接続リンク数分ループ
	for (ilp = 0 ; ilp < link_vol ; ilp++) {
		// パーセルＩＤ設定
		crs_p->link[ilp].id.parcel_id = map_addr[ilp].parcel_id;
		// リンクＩＤ設定
		crs_p->link[ilp].id.link_id   = SC_MA_D_NWRCD_LINK_GET_ID(map_addr[ilp].road_addr);

		// 交差点流入リンク方向設定
		if (0 == map_addr[ilp].cnct_side) {	// 交差点接続＝リンク始点
			crs_p->link[ilp].id.link_dir = 1;
		} else {							// 交差点接続＝リンク終点
			crs_p->link[ilp].id.link_dir = 0;
		}

		// 道路種別設定
		crs_p->link[ilp].road_type  = SC_MA_D_BASE_LINK_GET_ROAD_TYPE(map_addr[ilp].road_addr);
		// リンク種別１設定
		crs_p->link[ilp].link_kind  = SC_MA_D_BASE_LINK_GET_LINK1_TYPE(map_addr[ilp].road_addr);
		// 一方通行コード設定
		crs_p->link[ilp].onway_code = SC_MA_D_BASE_LINK_GET_ONEWAY(map_addr[ilp].road_addr);
		// リンク長設定
		SC_MA_CALC_LINK_DIST(SC_MA_D_BASE_LINK_GET_LINKDIST(map_addr[ilp].road_addr), crs_p->link[ilp].dist );
		// 旅行時間設定
		SC_MA_CALC_LINK_TRAVELTIME(SC_MA_D_NWRCD_LINK_GET_TRAVELTIME(map_addr[ilp].road_addr), crs_p->link[ilp].time );

		// 進入リンクの形状点列情報を取得
		ret = RT_MAP_GetSharpPoint(map_addr[ilp].sharp_addr, &point);
		if (e_SC_RESULT_SUCCESS != ret) {
			SC_LOG_ErrorPrint(SC_TAG_RT, "[MAP] ERROR " HERE);
			return (e_SC_RESULT_FAIL);
		}

		// リンク形状点座標設定
		crs_p->link[ilp].point = point;
	}
	// 交差点接続リンク数設定
	crs_p->link_vol = link_vol;

	return (e_SC_RESULT_SUCCESS);
}


/**
 * @brief	地図データ交差点ノード属性情報取得
 */
static E_SC_RESULT 	RT_MAP_GetCrossNodeAttr(RT_MAPREQ_t *reqtbl_p, RT_LINK_t *link_p, RT_CROSSINFO_t *crs_p)
{
	//E_SC_RESULT					ret;
	RT_MAPDATA_t				*maptbl_p;
	UINT8						*guide_head_addr;
	UINT8						*guide_addr;
	UINT8						*crsname_addr;
	UINT8						*addr;
	//UINT8						*addr2;
	UINT32						offset;
	UINT16						ofs;
	UINT16						ilp;
	UINT16						size;
	UINT16						flg;

	if (NULL == reqtbl_p || NULL == crs_p || NULL == link_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[MAP] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 出力テーブル初期化
	crs_p->tl_f = 0;
	memset(&crs_p->crs_name, 0x00, sizeof(RT_NAME_t));

	// 自パーセル情報取得
	for (ilp = 0 ; ilp < reqtbl_p->data_vol ; ilp++) {
		maptbl_p = &(reqtbl_p->data[ilp]);
		if (maptbl_p->parcel_id == link_p->parcel_id) break;
	}
	if ((ilp >= reqtbl_p->data_vol) || (NULL == maptbl_p->guide_p)) {
		SC_LOG_DebugPrint(SC_TAG_RT, "linkid[%08x] --> No Guide Data " HERE, link_p->link_id);
		return (e_SC_RESULT_SUCCESS);
	}

	// 誘導データのリンク情報先頭アドレス取得
	guide_head_addr = SC_MA_A_GDBIN_GET_GDRCD(maptbl_p->guide_p);

	// 進入リンクインデックス情報取得
	offset = SC_MA_GetGuideLinkOffset(link_p->link_id, maptbl_p->guide_p);
	if (ALLF32 == offset) {
		SC_LOG_DebugPrint(SC_TAG_RT, "linkid[%08x] --> No Guide Data " HERE, link_p->link_id);
		return (e_SC_RESULT_SUCCESS);
	}

	// 進入リンクの誘導リンク先頭アドレス取得
	guide_addr = SC_MA_A_GDRCD_GET_GDLINK(guide_head_addr, offset);

	// 順方向の場合
	if (0 == link_p->link_dir) {
		// 終点情報を取得する
		flg = SC_MA_D_GDLINK_GET_SETFLG(guide_addr) & 0xE000;
		if ((flg & 0x8000) == 0) {
			SC_LOG_DebugPrint(SC_TAG_RT, "linkid[%08x] --> No Guide Data " HERE, link_p->link_id);
			return (e_SC_RESULT_SUCCESS);
		}

		// ノード誘導情報始点アドレス取得
		addr = SC_MA_A_GDLINK_GET_GDLKINFO(guide_addr);

		// リンク情報ありの場合はスキップさせる
		if (flg & 0x2000) {
			addr += SC_MA_D_GDLKINFO_GET_SIZE(addr) * 4;
		}
		// 始点情報ありの場合はスキップさせる
		if (flg & 0x4000) {
			addr += SC_MA_D_GDLKINFO_GET_SIZE(addr) * 4;
		}
	} else {
		// 始点情報を取得する
		flg = SC_MA_D_GDLINK_GET_SETFLG(guide_addr) & 0xE000;
		if ((flg & 0x4000) == 0) {
			SC_LOG_DebugPrint(SC_TAG_RT, "linkid[%08x] --> No Guide Data " HERE, link_p->link_id);
			return (e_SC_RESULT_SUCCESS);
		}

		// ノード誘導情報始点アドレス取得
		addr = SC_MA_A_GDLINK_GET_GDLKINFO(guide_addr);

		// リンク情報ありの場合はスキップさせる
		if (flg & 0x2000) {
			addr += SC_MA_D_GDLKINFO_GET_SIZE(addr) * 4;
		}
	}

	// ノード属性情報あり
	if (SC_MA_D_GDNDINFO_GET_OFSFLG(addr) & SC_MA_M_GDNDINFO_OFSFLG_NDRCD) {

		// ノード属性情報アドレス取得
		ofs   = read2byte(SC_MA_A_GDNDINFO_GET_OFS(addr));
		addr += (ofs * 4);

		// 信号機あり
		if (SC_MA_D_NDRCD_GET_PTKIND(addr) & 0x0800) {
			crs_p->tl_f = 1;
		}
		// 交差点名称情報あり
		if (SC_MA_D_NDRCD_GET_PTKIND(addr) & 0x0400) {
			// 交差点名称レコードへのオフセット取得
			offset = SC_MA_D_NDRCD_GET_CNRCD_OFS(addr);

			// 交差点名称レコード先頭アドレス取得
			crsname_addr = SC_MA_A_GDBIN_GET_CNRCD(maptbl_p->guide_p) + offset * 4;

			// 交差点名称あり
			if (0 < SC_MA_D_CNRCD_GET_VOL(crsname_addr)) {

				// 文字サイズ（バイト数）取得
				size = SC_MA_D_CNRCD_GET_NAMESIZE(crsname_addr);
				// 文字先頭アドレス取得
				addr = SC_MA_A_CNRCD_GET_NAME(crsname_addr);

				// １文字ずつ格納
				for (ilp = 0 ; ilp < size ; ilp++, addr++) {
					crs_p->crs_name.name[ilp] = *addr;
				}
				crs_p->crs_name.name[ilp] = '\0';
				crs_p->crs_name.len       = size;

				SC_LOG_DebugPrint(SC_TAG_RT, "str = %s", crs_p->crs_name.name);
			}
		}
	}

	return (e_SC_RESULT_SUCCESS);

}

/**
 * @brief	形状情報
 */
static UINT8* RT_MAP_GetSharpRecord(UINT8* aBin)
{
	UINT8* bin;
	UINT8 rlp;
	UINT32 ofs = ALLF32;

	if (NULL == aBin) {
		return (NULL);
	}

	bin = SC_MA_A_SHBIN_GET_DIR(aBin);
	for (rlp = 0; rlp < 16; rlp++) {
		ofs = read4byte(bin);
		move4byte(bin);
		if (ALLF32 != ofs) {
			break;
		}
	}
	if (ALLF32 == ofs) {
		return (NULL);
	}
	// バイナリ先頭取得
	return (SC_MA_A_RSHP_GET_BINARY(aBin) + (ofs * 4));
}

/**
 * @brief	地図データリンク情報取得
 */
static E_SC_RESULT RT_MAP_GetSharpPoint(UINT8 *data_p, RT_LINKPOINT_t *point_p)
{

	UINT32		ilp;
	UINT32		vol;
	UINT8		*addr;

	if (NULL == data_p || NULL == point_p) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[MAP] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// ワーク初期化
	memset(point_p, 0x00, sizeof(RT_LINKPOINT_t));

	// 形状点数取得
	vol = SC_MA_D_SHRCD_GET_XYVOL(data_p);
	if (2 > vol) {
		SC_LOG_ErrorPrint(SC_TAG_RT, "[MAP] ERROR " HERE);
		return (e_SC_RESULT_FAIL);
	}

	// 読み飛ばし(XY + VOL + FLAG)
	addr = SC_MA_A_SHRCD_GET_XY(data_p);
	move4byte(addr);

	// ＸＹ座標設定
	for (ilp = 0 ; ilp < vol ; ilp++) {
		// XY情報格納
		point_p->pos[ilp].x = read2byte(addr);
		move2byte(addr);
		point_p->pos[ilp].y = read2byte(addr);
		move2byte(addr);
	}

	// 形状点数設定
	point_p->vol = vol;

	return (e_SC_RESULT_SUCCESS);

}

/**
 * @brief	移動先パーセル取得IFラッパー
 * @param	[I]基準パーセルID
 * @param	[I]移動先
 * @memo	移動先方向	[1][2][3]    [7][0][1]
 * @memo	5中央		[4][5][6] -> [6]   [2]
 * @memo				[7][8][9]    [5][4][3]
 */
static UINT32 RT_MAP_GetNextParcelID(UINT32 aParcelId, UINT8 aDir)
{
	UINT8 dirCnv[10] = { ALLF8, DIR_L_TOP, DIR_TOP, DIR_R_TOP, DIR_L, ALLF8, DIR_R, DIR_L_DOWN, DIR_DOWN, DIR_R_DOWN };

	if (ALLF8 == dirCnv[aDir]) {
		return (aParcelId);
	} else {
		return (SC_MESH_GetNextParcelID(aParcelId, dirCnv[aDir]));
	}
}


