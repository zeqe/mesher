#ifndef SKELETON_INCLUDED
	#include <cstdint>
	
	#include "transformOp.hpp"
	
	namespace bones{
		void init();
		
		void setOrigin(unsigned char i,int16_t x,int16_t y);
		void setParent(unsigned char i,unsigned char parent);
		
		int16_t getX(unsigned char i);
		int16_t getY(unsigned char i);
		unsigned char getParent(unsigned char i);
		
		void draw();
	}
	
	namespace pose{
		void setModifiers(enum transformOp (*type)(),bool (*active)(),float (*xVal)(),float (*yVal));
		void applyModifiers();
		
		void setActive(unsigned char i);
	}
	
	#define SKELETON_INCLUDED
#endif