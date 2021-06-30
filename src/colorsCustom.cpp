extern "C" {
	#include <vecGL/shader.h>
}

#include "colorsCustom.hpp"

namespace clrCstm{
	uint32_t colors[COLOR_ARRAY_MAX_COUNT];
	
	void init(){
		for(unsigned int i = 0;i < COLOR_ARRAY_MAX_COUNT;++i){
			if(i < CLR_RANBW_COUNT){
				colors[i] = clr::get(clr::PFL_RANBW,i,clr::ALF_HALF);
			}else{
				colors[i] = 0x00000000;
			}
		}
	}
	
	void set(unsigned char index,uint32_t newColor){
		colors[index & COLOR_ARRAY_INDEX_MASK] = newColor;
	}
	
	uint32_t get(unsigned char index){
		return colors[index & COLOR_ARRAY_INDEX_MASK];
	}
	
	void apply(enum clr::profile colorProfile,unsigned char colorProfileMember,enum clr::alpha colorAlpha){
		uniformColor_u32(clr::get(colorProfile,colorProfileMember,colorAlpha));
		uniformColorArray_u32((uint32_t *)colors,COLOR_ARRAY_MAX_COUNT);
	}
}