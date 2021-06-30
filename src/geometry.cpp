#include <cmath>

#include "geometry.hpp"

namespace geom{
	int triArea(int x1,int y1,int x2,int y2,int x3,int y3){
		// Half-Determinant
		return abs((x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2)) / 2);
	}
	
	char pointInTri(int16_t x,int16_t y,int x1,int y1,int x2,int y2,int x3,int y3){
		// Comparing areas method---------------------------------
		// Total Triangle
		int A = triArea(
			x1,y1,
			x2,y2,
			x3,y3
		);
		
		// Sub-Triangles
		int A1 = triArea(
			x,y,
			x2,y2,
			x3,y3
		);
		
		int A2 = triArea(
			x1,y1,
			x,y,
			x3,y3
		);
		
		int A3 = triArea(
			x1,y1,
			x2,y2,
			x,y
		);
		
		// Allow odd int division error of 3, 1 per tri
		return (A1 + A2 + A3) < (A + 3);
	}
	
	uint64_t distSquared_I(int x0,int y0,int x1,int y1){
		int64_t dX,dY;
		dX = x0 - x1;
		dY = y0 - y1;
		
		return llabs(dX * dX + dY * dY);
	}
	
	double distSquared_D(double x0,double y0,double x1,double y1){
		double dX,dY;
		dX = x0 - x1;
		dY = y0 - y1;
		
		return fabs(dX * dX + dY * dY);
	}
}