#include <cstdlib>

#include "triConstruct.hpp"
#include "colors.hpp"
#include "graphics.hpp"

namespace triCn{
	// Memory
	#define TRI_VERT_VALUE_COUNT 2
	
	int16_t triVerts[TRI_VERT_COUNT * TRI_VERT_VALUE_COUNT];
	unsigned int vertCount;
	unsigned char triType;
	
	#define CONSTRUCT_X(i) triVerts[(i) * TRI_VERT_VALUE_COUNT + 0]
	#define CONSTRUCT_Y(i) triVerts[(i) * TRI_VERT_VALUE_COUNT + 1]
	
	// Preview
	struct vecTrisBuf constructBuf;
	struct vecTris *constructTri;
	
	int16_t constructXYUV[TRI_XYUV_VALUE_COUNT];
	uint8_t constructTBC[TRI_TBC_VALUE_COUNT];
	
	// Initialization / Destruction
	void init(){
		constructBuf.count = 1;
		constructBuf.xyuv = constructXYUV;
		constructBuf.tbc = constructTBC;
	}
	
	void free(){
		deleteVecTris(constructTri);
	}
	
	// Triangle building
	void clear(){
		vertCount = 0;
	}
	
	void setType(unsigned char type){
		triType = type;
	}
	
	bool add(class vertLayer *out,int16_t x,int16_t y){
		CONSTRUCT_X(vertCount) = x;
		CONSTRUCT_Y(vertCount) = y;
		
		++vertCount;
		
		if(vertCount >= TRI_VERT_COUNT){
			out->tris_Add(CONSTRUCT_X(2),CONSTRUCT_Y(2),CONSTRUCT_X(1),CONSTRUCT_Y(1),CONSTRUCT_X(0),CONSTRUCT_Y(0),triType);
			clear();
			
			return true;
		}
		
		return false;
	}
	
	bool building(){
		return vertCount > 0;
	}
	
	// Preview utilities
	void considerPoint(int16_t x,int16_t y){
		CONSTRUCT_X(vertCount) = x;
		CONSTRUCT_Y(vertCount) = y;
	}
	
	void drawPreview(bool wireframe){
		if(!building()){
			return;
		}
		
		// Draw triangle preview if enough vertices have been staged
		if(vertCount + 1 >= TRI_VERT_COUNT){
			// Copying values into triangle buffer
			VERT_X(&constructBuf,0) = CONSTRUCT_X(2);
			VERT_Y(&constructBuf,0) = CONSTRUCT_Y(2);
			
			VERT_X(&constructBuf,1) = CONSTRUCT_X(1);
			VERT_Y(&constructBuf,1) = CONSTRUCT_Y(1);
			
			VERT_X(&constructBuf,2) = CONSTRUCT_X(0);
			VERT_Y(&constructBuf,2) = CONSTRUCT_Y(0);
			
			for(unsigned int i = 0;i < TRI_VERT_COUNT;++i){
				VERT_TYPE(&constructBuf,i) = triType;
				VERT_COLOR(&constructBuf,i) = CLR_EDITR_OFFWHITE;
			}
			
			// Drawing
			render::loadAndDrawTris(&constructBuf,&constructTri,render::MODE_XY,false,wireframe);
		}
		
		// Draw vertices placed
		for(unsigned int i = 0;i <= vertCount;++i){
			hud::drawCircle(CONSTRUCT_X(i),CONSTRUCT_Y(i),false,POINT_RADIUS,clr::get(clr::PFL_EDITR,CLR_EDITR_OFFWHITE,clr::ALF_HALF));
		}
	}
}