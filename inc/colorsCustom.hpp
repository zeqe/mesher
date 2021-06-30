#ifndef COLORS_CUSTOM_INCLUDED
	#include <cstdint>
	
	#include "colors.hpp"
	
	namespace clrCstm{
		void init();
		
		void set(unsigned char index,uint32_t newColor);
		uint32_t get(unsigned char index);
		
		void apply(enum clr::profile colorProfile,unsigned char colorProfileMember,enum clr::alpha colorAlpha);
	}
	
	#define COLORS_CUSTOM_INCLUDED
#endif