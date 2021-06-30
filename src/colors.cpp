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
		},
		// PFL_RANBW
		{
			0x6d00f1af, 0x010ae1af, 0x11dcfdaf, 0x06ce25af,
			0xecff29af, 0xf77a02af, 0xd50014af, 0xd933a3af
		}
	};
	
	const unsigned int COLOR_PFLS_COUNTS[PFL_COUNT] = {
		CLR_WHITE_COUNT, // PFL_WHITE
		CLR_EDITR_COUNT, // PFL_EDITR
		CLR_EDITR_COUNT, // PFL_EDITR_SOL
		CLR_RANBW_COUNT  // PFL_RANBW
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
	
	void apply(enum profile colorProfile,unsigned char colorProfileMember,enum alpha colorAlpha,enum profile arrayProfile){
		uniformColor_u32(get(colorProfile,colorProfileMember,colorAlpha));
		uniformColorArray_u32((uint32_t *)COLOR_PFLS[arrayProfile],COLOR_PFLS_COUNTS[arrayProfile]);
	}
}