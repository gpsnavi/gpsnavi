#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "polygon.h"


#define EPSILON		0.0000000001f

static bool Area2(const T_POLYGON_POINT contour[], const int cnt);
static bool Inside(float Ax, float Ay, float Bx, float By, float Cx, float Cy, float Px, float Py);
static bool Divide(const T_POLYGON_POINT contour[], int u, int v, int w, int n, int *V);

bool Area2(const T_POLYGON_POINT contour[], const int cnt)
{
	float area = 0.0f;

	for (int p=cnt-1, q=0; q<cnt; p=q++) {
		area += contour[p].mX * contour[q].mY - contour[q].mX * contour[p].mY;
	}

	if (0.0f < (area * 0.5f)) {
		return (true);
	}

	return (false);
}

bool Inside(float Ax, float Ay, float Bx, float By, float Cx, float Cy, float Px, float Py)

{
	float ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
	float cCROSSap, bCROSScp, aCROSSbp;

	ax = Cx - Bx;  ay = Cy - By;
	bx = Ax - Cx;  by = Ay - Cy;
	cx = Bx - Ax;  cy = By - Ay;
	apx= Px - Ax;  apy= Py - Ay;
	bpx= Px - Bx;  bpy= Py - By;
	cpx= Px - Cx;  cpy= Py - Cy;

	aCROSSbp = ax*bpy - ay*bpx;
	cCROSSap = cx*apy - cy*apx;
	bCROSScp = bx*cpy - by*cpx;

	return ((aCROSSbp >= 0.0f) && (bCROSScp >= 0.0f) && (cCROSSap >= 0.0f));
};

bool Divide(const T_POLYGON_POINT contour[], int u, int v, int w, int n, int *V)
{
	int p;
	float Ax, Ay, Bx, By, Cx, Cy, Px, Py;

	Ax = contour[V[u]].mX;
	Ay = contour[V[u]].mY;

	Bx = contour[V[v]].mX;
	By = contour[V[v]].mY;

	Cx = contour[V[w]].mX;
	Cy = contour[V[w]].mY;

	if ( EPSILON > (((Bx-Ax)*(Cy-Ay)) - ((By-Ay)*(Cx-Ax))) ){
		return (false);
	}

	for (p=0; p<n; p++) {
		if( (p == u) || (p == v) || (p == w) ) {
			continue;
		}

		Px = contour[V[p]].mX;
		Py = contour[V[p]].mY;

		if (Inside(Ax,Ay,Bx,By,Cx,Cy,Px,Py)) {
			return (false);
		}
	}

	return (true);
}

bool Triangulation(const T_POLYGON_POINT contour[], const int contour_cnt, T_TRI_POINT result[], int* cnt)
{
	int n = contour_cnt;
	if ( n < 3 ) {
		return (false);
	}

	//int *V = new int[n];
	static int VVV[10000];
	int *V = &VVV[0];

	if (Area2(contour, contour_cnt)) {
		for (int v=0; v<n; v++) {
			V[v] = v;
		}
	} else {
		for (int v=0; v<n; v++) {
			V[v] = (n-1)-v;
		}
	}

	int nv = n;

	int count = 2*nv;
	*cnt = 0;

	for(int m=0, v=nv-1; nv>2;) {
		if (0 >= (count--)) {
			break;
		}

		// previous
		int u = v;
		if (nv <= u) {
			u = 0;
		}

		// new
		v = u+1;
		if (nv <= v) {
			v = 0;
		}

		// next
		int w = v+1;
		if (nv <= w) {
			w = 0;
		}

		if ( Divide(contour,u,v,w,nv,V) ) {
			int a,b,c,s,t;
			a = V[u];
			b = V[v];
			c = V[w];

			result[*cnt].x = contour[a].mX;
			result[*cnt].y = contour[a].mY;
			*cnt += 1;
			result[*cnt].x = contour[b].mX;
			result[*cnt].y = contour[b].mY;
			*cnt += 1;
			result[*cnt].x = contour[c].mX;
			result[*cnt].y = contour[c].mY;
			*cnt += 1;

			m++;

			for (s=v,t=v+1;t<nv;s++,t++) V[s] = V[t]; nv--;

			count = 2*nv;
		}
	}

	//delete [] V;

	return (true);
}

