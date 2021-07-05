extern "C" {
	#include <vecGL/shader.h>
	#include <vecGL/bones.h>
}

#include "skeleton.hpp"
#include "graphics.hpp"
#include "colors.hpp"

struct bone{
	int16_t x,y;
	unsigned char parent;
};

struct bone boneArray[BONES_MAX_COUNT];

namespace bones{
	void init(){
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			boneArray[i].parent = BONES_MAX_COUNT;
		}
	}
	
	void setOrigin(unsigned char i,int16_t x,int16_t y){
		boneArray[i & BONE_INDEX_MASK].x = x;
		boneArray[i & BONE_INDEX_MASK].y = y;
	}
	
	void setParent(unsigned char i,unsigned char parent){
		boneArray[i & BONE_INDEX_MASK].parent = parent;
	}
	
	int16_t getX(unsigned char i){
		return boneArray[i & BONE_INDEX_MASK].x;
	}
	
	int16_t getY(unsigned char i){
		return boneArray[i & BONE_INDEX_MASK].y;
	}
	
	unsigned char getParent(unsigned char i){
		return boneArray[i & BONE_INDEX_MASK].parent;
	}
	
	void draw(){
		// Stems
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			if(boneArray[i].parent < BONES_MAX_COUNT){
				hud::drawStem(i,boneArray[i].x,boneArray[i].y,boneArray[boneArray[i].parent].x,boneArray[boneArray[i].parent].y);
			}
		}
		
		// Marks
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			hud::drawMark(i,boneArray[i].x,boneArray[i].y,false,POINT_RADIUS);
		}
	}
}

struct poseState{
	
};

namespace pose{
	void setModifiers(enum transformOp (*type)(),bool (*active)(),float (*xVal)(),float (*yVal)){
		
	}
	
	void applyModifiers(){
		
	}
	
	void setActive(unsigned char i){
		
	}
}