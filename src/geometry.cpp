#include <cmath>

#include "geometry.hpp"

namespace geom{
	int64_t triArea(int x1,int y1,int x2,int y2,int x3,int y3){
		int64_t l_x1 = x1;
		int64_t l_y1 = y1;
		
		int64_t l_x2 = x2;
		int64_t l_y2 = y2;
		
		int64_t l_x3 = x3;
		int64_t l_y3 = y3;
		
		// Half-Determinant
		return llabs((l_x1 * (l_y2 - l_y3) + l_x2 * (l_y3 - l_y1) + l_x3 * (l_y1 - l_y2)) / 2);
	}
	
	char pointInTri(int16_t x,int16_t y,int x1,int y1,int x2,int y2,int x3,int y3){
		// Comparing areas method---------------------------------
		// Total Triangle
		int64_t A = triArea(
			x1,y1,
			x2,y2,
			x3,y3
		);
		
		// Sub-Triangles
		int64_t A1 = triArea(
			x,y,
			x2,y2,
			x3,y3
		);
		
		int64_t A2 = triArea(
			x1,y1,
			x,y,
			x3,y3
		);
		
		int64_t A3 = triArea(
			x1,y1,
			x2,y2,
			x,y
		);
		
		// Allow odd int division error of 3, 1 per tri
		return (A1 + A2 + A3) < (A + 3);
	}
	
	uint64_t distSquared_I(int x0,int y0,int x1,int y1){
		int64_t dX,dY;
		dX = (int64_t)x0 - (int64_t)x1;
		dY = (int64_t)y0 - (int64_t)y1;
		
		return llabs(dX * dX + dY * dY);
	}
	
	double distSquared_D(double x0,double y0,double x1,double y1){
		double dX,dY;
		dX = x0 - x1;
		dY = y0 - y1;
		
		return fabs(dX * dX + dY * dY);
	}
}