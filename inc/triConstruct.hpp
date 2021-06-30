#ifndef TRI_CONSTRUCT_INCLUDED
	#include <cstdint>
	
	#include "layer.hpp"
	
	namespace triCn{
		// Initialization / Destruction
		void init();
		void free();
		
		// Triangle building
		void clear();
		void setType(unsigned char type);
		bool add(class vertLayer *out,int16_t x,int16_t y);
		bool building();
		
		// Preview utilities
		void considerPoint(int16_t x,int16_t y);
		void drawPreview(bool wireframe);
	}
	
	#define TRI_CONSTRUCT_INCLUDED
#endif