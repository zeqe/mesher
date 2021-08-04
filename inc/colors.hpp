#ifndef COLORS_INCLUDED
	#include <cstdint>
	
	#define CLR_PFL_COLOR_COUNT 9
	
	#define CLR_WHITE_WHITE 0
	#define CLR_WHITE_COUNT 1
	
	#define CLR_EDITR_OFFWHITE 0
	#define CLR_EDITR_HILIGHT 1
	#define CLR_EDITR_HICONTRAST 2
	#define CLR_EDITR_COUNT 3
	
	#define CLR_RANBW_NULL 8
	#define CLR_RANBW_COUNT 9
	
	namespace clr{
		enum profile{
			PFL_WHITE,
			PFL_EDITR,
			PFL_EDITR_SOL,
			PFL_RANBW,
			PFL_RANBW_SOL,
			
			PFL_COUNT
		};
		
		enum alpha{
			ALF_HALF,
			ALF_ONE,
			ALF_TRI,
			ALF_UTIL,
			ALF_BUTL,
			
			ALF_COUNT
		};
		
		uint32_t get(enum profile colorProfile,unsigned char colorProfileMember,enum alpha colorAlpha);
		uint8_t getAlpha(enum alpha colorAlpha);
		
		uint32_t inverse(uint32_t color);
		
		void apply(enum profile colorProfile,unsigned char colorProfileMember,enum alpha colorAlpha,enum profile arrayProfile);
	}
	
	#define COLORS_INCLUDED
#endif