/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

#include "sms-core/SMCoreDM/SMCoreDMInternal.h"

static void 	RT_LIB_Vector2_Diff(RT_VECTOR2_t *, const RT_VECTOR2_t *, const RT_VECTOR2_t *);
static DOUBLE 	RT_LIB_Vector2_DotProduct(const RT_VECTOR2_t *, const RT_VECTOR2_t *);
static DOUBLE 	RT_LIB_Vector2_CrossProduct(const RT_VECTOR2_t *, const RT_VECTOR2_t *);

// 中心点を共有する
// ベクトル C→P と C→Q のなす角θおよび回転方向を求める．
INT32 RT_LIB_GetCrossAngle(DOUBLE i_x, DOUBLE i_y, DOUBLE c_x, DOUBLE c_y, DOUBLE o_x, DOUBLE o_y)
{
	RT_VECTOR2_t			c, p, q;   		// 入力データ
	RT_VECTOR2_t			cp;       		// ベクトル C→P
	RT_VECTOR2_t			cq;        		// ベクトル C→Q
	//DOUBLE					lati;
	//DOUBLE					longi;
	DOUBLE					s; 				// 外積：(C→P) × (C→Q)
	DOUBLE					t; 				// 内積：(C→P) ・ (C→Q)
	DOUBLE					theta;			// θ (ラジアン)
	INT32					angle;

	c.x = c_x;
	c.y = c_y;
	p.x = i_x;
	p.y = i_y;
	q.x = o_x;
	q.y = o_y;

	// 回転方向および角度θを計算する．
	RT_LIB_Vector2_Diff(&cp, &p, &c);          		// cp ← p - c
	RT_LIB_Vector2_Diff(&cq, &q, &c);          		// cq ← q - c
	s = RT_LIB_Vector2_CrossProduct(&cp, &cq); 		// s ← cp × cq
	t = RT_LIB_Vector2_DotProduct(&cp, &cq);   		// t ← cp ・ cq
	theta = atan2(s, t);							// ラジアン

	// 0-360度へ
	if (theta < 0.0) {
		theta = theta + RT_M_DEGTORAD(360.0);
	}

	angle = (INT32)RT_M_RADTODEG(theta);			// 角度

	return (angle);
}

// 中心点を共有しない
// ベクトル C→P と C→Q のなす角θおよび回転方向を求める．
INT32 RT_LIB_GetCrossAngle2(DOUBLE ii_x, DOUBLE ii_y, DOUBLE io_x, DOUBLE io_y, DOUBLE oi_x, DOUBLE oi_y, DOUBLE oo_x, DOUBLE oo_y)
{
	RT_VECTOR2_t			c, p, q;   		// 入力データ
	RT_VECTOR2_t			cp;       		// ベクトル C→P
	RT_VECTOR2_t			cq;        		// ベクトル C→Q
	//DOUBLE					lati;
	DOUBLE					longi;
	DOUBLE					s; 				// 外積：(C→P) × (C→Q)
	DOUBLE					t; 				// 内積：(C→P) ・ (C→Q)
	DOUBLE					theta;			// θ (ラジアン)
	INT32					angle;

	// 回転方向および角度θを計算する．
	p.x = ii_x;
	p.y = ii_y;
	c.x = io_x;
	c.y = io_y;
	RT_LIB_Vector2_Diff(&cp, &p, &c);          		// cp ← p - c

	c.x = oi_x;
	c.y = oi_y;
	q.x = oo_x;
	q.y = oo_y;
	RT_LIB_Vector2_Diff(&cq, &q, &c);          		// cq ← q - c
	s = RT_LIB_Vector2_CrossProduct(&cp, &cq); 		// s ← cp × cq
	t = RT_LIB_Vector2_DotProduct(&cp, &cq);   		// t ← cp ・ cq
	theta = atan2(s, t);							// ラジアン

	// 0-360度へ
	if (theta < 0.0) {
		theta = theta + RT_M_DEGTORAD(360.0);
	}

	angle = (INT32)RT_M_RADTODEG(theta);			// 角度

	return (angle);
}

// diff ← ベクトル v1 - v2
static void RT_LIB_Vector2_Diff(RT_VECTOR2_t *diff, const RT_VECTOR2_t *v1, const RT_VECTOR2_t *v2)
{
  diff->x = v1->x - v2->x;
  diff->y = v1->y - v2->y;
}

// ベクトル v1 と v2 の内積 (v1・v2) を返す．
static DOUBLE RT_LIB_Vector2_DotProduct(const RT_VECTOR2_t *v1, const RT_VECTOR2_t *v2)
{
  return (v1->x * v2->x + v1->y * v2->y);
}

// ベクトル v1 と v2 の外積 (v1×v2) を返す．
static DOUBLE RT_LIB_Vector2_CrossProduct(const RT_VECTOR2_t *v1, const RT_VECTOR2_t *v2)
{
  return (v1->x * v2->y - v1->y * v2->x);
}

/**
 * @brief	垂線の足の座標を求める 入力はメートル座標で。
 * @param	cx						垂線の足Ｘ
 * @param	cy						垂線の足Ｙ
 * @param	sx						始点のX座標
 * @param	sy						始点のY座標
 * @param	ex						終点のX座標
 * @param	ey						終点のY座標
 * @param	ret						戻り値
 * @param	dx						垂線の足の座標Ｘ
 * @param	dy						垂線の足の座標Ｙ
 * @param	l_len					リンク長
 * @param	v_len					垂線距離
 * @param	ratio					内分率
 * @param	cxx						垂線の足Ｘ
 * @param	cyy						垂線の足Ｙ
 * @param	angle					角度
 * @param	len_v_dir				垂線の長さ
 * @param	len_l_dir				推定位置からのリンク方向の距離
 * @retval	RESULT_SUCCESS			成功
 * @retval	RESULT_SUCCESS以外		エラー
 */
INT32 RT_LIB_VerticalPoint(
	DOUBLE cx,
	DOUBLE cy,
	DOUBLE sx,
	DOUBLE sy,
	DOUBLE inEx,
	DOUBLE inEy,
	INT8* ret,
	DOUBLE* dx,
	DOUBLE* dy,
	DOUBLE* l_len,
	DOUBLE* v_len,
	DOUBLE* ratio,
	DOUBLE* cxx,
	DOUBLE* cyy,
	DOUBLE* angle,
	DOUBLE* len_v_dir,
	DOUBLE* len_l_dir )
{
	DOUBLE ex = inEx;
	DOUBLE ey = inEy;

	DOUBLE Xe = 0.0;
	DOUBLE X0 = 0.0;
	DOUBLE Y0 = 0.0;

	DOUBLE s = 0.0;
	DOUBLE c = 0.0;

	int area = 0;

	// リンクが等しい場合
	if ((CompareDouble(sx, ex)) && (CompareDouble(sy, ey))) {
		ex = ex + 1;
		*ret = -1;
		*dx = cx;							// 垂線の足Ｘ
		*dy = cy;							// 垂線の足Ｙ
		*v_len = 0.0;						// ユークリッド距離
		*l_len = 0.0;						// リンク長
		return(0);
	} else {
		*angle = atan2(ey - sy, ex - sx);

		if (*angle < 0.0) {
			*angle = *angle + RT_M_DEGTORAD(360.0);
		}

		s = sin(*angle);
		c = cos(*angle);

		Xe = c * (ex - sx) + s * (ey - sy);
		// 始点からのリンクに沿った長さ
		X0 = c * (cx - sx) + s * (cy - sy);
		// 垂線の足までの距離
		Y0 = -s * (cx - sx) + c * (cy - sy);

		// Y0は正の値へ
		if (Y0 < 0.0) {
			Y0 = -Y0;
		}

		if (Xe >= 0.0) {
			if (X0 < 0.0) {
				area = -1;
			} else if (X0 > Xe) {
				area = 1;
			} else {
				area = 0;
			}
		} else {
			if (X0 > 0.0) {
				area = -1;
			} else if (X0 < Xe) {
				area = 1;
			} else {
				area = 0;
			}
		}
	}

	// 垂線の足の座標
	*dx = c * X0 + sx;
	*dy = s * X0 + sy;
	// 垂線距離
	*v_len = Y0;
	// リンク長
	if (Xe < 0.0) {
		Xe = -Xe;
	}
	*l_len = Xe;
	// 内分率
	if (!CompareDouble(Xe, (DOUBLE)0.0)) {
		*ratio = X0 / Xe;
	} else {
		*ratio = 0.0;
	}

	// 次に調整する　例えば外分点の場合は、始点・終点へ
	switch (area) {
	case 0:
		//  0：内分点の場合
		*ret = 0;
		break;
	case -1:
		// -1：リンク始点側の外分点
		*dx = sx;
		*dy = sy;
		*ratio = 0.0;
		*v_len = sqrt((*dx - cx) * (*dx - cx) + (*dy - cy) * (*dy - cy));
		*ret = 0;
		break;
	case 1:
		//  1：リンク終点側の外分点
		*dx = ex;
		*dy = ey;
		*ratio = 1.0;
		*v_len = sqrt((*dx - cx) * (*dx - cx) + (*dy - cy) * (*dy - cy));
		*ret = 0.0;
		break;
	default:
		// その他のケースはありえない為、何もしない
		break;
	}

	*cxx = cx;
	*cyy = cy;

	// 角度をラジアンから度へ
	*angle = RT_M_RADTODEG(*angle);

	// 垂線の長さ
	*len_v_dir = Y0;

	// 推定位置からのリンク方向の距離
	switch (area) {
	case 0:
		//  0：内分点の場合
		*len_l_dir = 0;
		break;
	case -1:
		// -1：リンク始点側の外分点
		*len_l_dir = -X0;
		break;
	case 1:
		//  1：リンク終点側の外分点
		*len_l_dir = X0 - Xe;
		break;
	default:
		// その他のケースはありえない為、何もしない
		break;
	}
	return (1);
}

/**
 * @brief	音声リストマージ
 */
E_SC_RESULT RG_CTL_JointVoiceList(RT_VOICE_t* dst, RT_VOICE_t* src) {

	UINT16 i;

	if (NULL == dst || NULL == src) {
		return (e_SC_RESULT_BADPARAM);
	}

	for (i = 0; i < src->current; i++) {
		RT_SET_VOICE_LIST(dst, src->voice_list[i]);
		// 可変数値
		if (VOICE_TEXT_301 == src->voice_list[i]) {
			dst->valiavbleNum = src->valiavbleNum;
		}
		if (VOICE_TEXT_302 == src->voice_list[i]) {
			sprintf(dst->valiavbleName, "%s", src->valiavbleName);
		}
	}

	return (e_SC_RESULT_SUCCESS);
}

/**
 * @brief 指定位置へデータ追加
 */
E_SC_RESULT RG_CTL_InsertVoiceList(RT_VOICE_t* dst, UINT16 val, UINT8 index) {

	UINT16 i = 0;
	UINT16 set = 0;
	RT_VOICE_t tmp = {};

	if (NULL == dst) {
		return (e_SC_RESULT_BADPARAM);
	}

	// 末尾に追加
	if (index == dst->current) {
		RT_SET_VOICE_LIST(dst, val);
	}
	// 末尾以外に追加
	else {

		// バックアップ
		memcpy(&tmp, dst, sizeof(RT_VOICE_t));

		// List初期化
		memset(dst->voice_list, 0, sizeof(dst->voice_list));

		// 値設定
		for (i = 0; i < dst->current + 1; i++) {
			if (i == index) {
				dst->voice_list[i] = val;
				set = 1;
			} else {
				dst->voice_list[i] = tmp.voice_list[i - set];
			}
		}
		// 使用位置更新
		dst->current++;
	}

	return (e_SC_RESULT_SUCCESS);
}

