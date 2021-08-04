extern "C" {
	#include <vecGL/shader.h>
}

#include "colors.hpp"

namespace clr{
	const uint32_t COLOR_PFLS[PFL_COUNT][CLR_PFL_COLOR_COUNT] = {
		// PFL_WHITE
		{
			0xffffffff, // CLR_WHITE_WHITE
			0x00000000,
			0x00000000,
			0x00000000,
			0x00000000,
			0x00000000,
			0x00000000,
			0x00000000,
			0x00000000
		},
		// PFL_EDITR
		{
			0xf1f1f170, // CLR_EDITR_OFFWHITE
			0xffa73370, // CLR_EDITR_HILIGHT
			0x7f00ff40, // CLR_EDITR_HICONTRAST
			0x00000000,
			0x00000000,
			0x00000000,
			0x00000000,
			0x00000000,
			0x00000000
		},
		// PFL_EDITR_SOL
		{
			0xf1f1f1b0, // CLR_EDITR_OFFWHITE
			0xffa733b0, // CLR_EDITR_HILIGHT
			0x7f00ffb0, // CLR_EDITR_HICONTRAST
			0x00000000,
			0x00000000,
			0x00000000,
			0x00000000,
			0x00000000,
			0x00000000
		},
		// PFL_RANBW
		{
			0x6d00f1af, 0x010ae1af, 0x11dcfdaf, 0x06ce25af,
			0xecff29af, 0xf77a02af, 0xd50014af, 0xd933a3af,
			0xf1f1f1af
		},
		// PFL_RANBW_SOL
		{
			0x6d00f1b0, 0x010ae1b0, 0x11dcfdb0, 0x06ce25b0,
			0xecff29b0, 0xf77a02b0, 0xd50014b0, 0xd933a3b0,
			0xf1f1f1b0
		}
	};
	
	const unsigned int COLOR_PFLS_COUNTS[PFL_COUNT] = {
		CLR_WHITE_COUNT, // PFL_WHITE
		CLR_EDITR_COUNT, // PFL_EDITR
		CLR_EDITR_COUNT, // PFL_EDITR_SOL
		CLR_RANBW_COUNT, // PFL_RANBW
		CLR_RANBW_COUNT  // PFL_RANBW_SOL
	};
	
	const unsigned char COLOR_ALPHAS[ALF_COUNT] = {
		0x80, // ALF_HALF
		0xff, // ALF_ONE
		0x70, // ALF_TRI
		0x40, // ALF_UTIL
		0xb0  // ALF_BUTL
	};
	
	uint32_t get(enum profile colorProfile,unsigned char colorProfileMember,enum alpha colorAlpha){
		return (COLOR_PFLS[colorProfile][colorProfileMember] & 0xffffff00) | COLOR_ALPHAS[colorAlpha];
	}
	
	uint8_t getAlpha(enum alpha colorAlpha){
		return COLOR_ALPHAS[colorAlpha];
	}
	
	uint32_t inverse(uint32_t color){
		uint8_t r = (color >> 24) & 0xff;
		uint8_t g = (color >> 16) & 0xff;
		uint8_t b = (color >> 8) & 0xff;
		uint8_t a = color & 0xff;
		
		return ((0xff - r) << 24) | ((0xff - g) << 16) | ((0xff - b) << 8) | a;
	}
	
	void apply(enum profile colorProfile,unsigned char colorProfileMember,enum alpha colorAlpha,enum profile arrayProfile){
		uniformColor_u32(get(colorProfile,colorProfileMember,colorAlpha));
		uniformColorArray_u32((uint32_t *)COLOR_PFLS[arrayProfile],COLOR_PFLS_COUNTS[arrayProfile]);
	}
}