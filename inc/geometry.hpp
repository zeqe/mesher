#ifndef GEOM_INCLUDED
	#include <cstdint>
	
	namespace geom{
		int64_t triArea(int x1,int y1,int x2,int y2,int x3,int y3);
		char pointInTri(int16_t x,int16_t y,int x1,int y1,int x2,int y2,int x3,int y3);
		
		uint64_t distSquared_I(int x0,int y0,int x1,int y1);
		double distSquared_D(double x0,double y0,double x1,double y1);
	}
	
	#define GEOM_INCLUDED
#endif