#pragma once

typedef struct _POLYGON_POINT {
	float	mX;
	float	mY;
} T_POLYGON_POINT;

typedef struct _TRI_POINT {
	float	x;
	float	y;
} T_TRI_POINT;

bool Triangulation(const T_POLYGON_POINT contour[], const int contour_cnt, T_TRI_POINT result[], int* cnt);
