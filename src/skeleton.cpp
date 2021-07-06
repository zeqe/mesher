extern "C" {
	#include <vecGL/shader.h>
	#include <vecGL/bones.h>
}

#include "skeleton.hpp"
#include "graphics.hpp"
#include "colors.hpp"
#include "view.hpp"

#define PI 3.14159265358979323846

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
		unsigned char j = i & BONE_INDEX_MASK;
		
		// Escape case
		if(parent >= BONES_MAX_COUNT){
			boneArray[j].parent = BONES_MAX_COUNT;
			return;
		}
		
		// Check to avoid creation of cycles
		if(j == parent){
			return;
		}
		
		unsigned char currI = parent;
		
		while(boneArray[currI].parent < BONES_MAX_COUNT){
			if(boneArray[currI].parent == j){
				return;
			}
			
			currI = boneArray[currI].parent;
		}
		
		// Assign
		boneArray[j].parent = parent;
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
	int32_t translateX,translateY;
	float scale,rotation;
	
	sf::Transform transformation;
};

struct poseState poses[BONES_MAX_COUNT];
sf::Transform globalTransforms[BONES_MAX_COUNT];

namespace pose{
	enum transformOp (*opType)();
	bool (*opActive)();
	int32_t (*opValX)();
	int32_t (*opValY)();
	float (*opValScalar)();
	
	void reset(){
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			poses[i].translateX = 0;
			poses[i].translateY = 0;
			
			poses[i].scale = 1.0;
			poses[i].rotation = 0.0;
			
			poses[i].transformation = sf::Transform::Identity;
			globalTransforms[i] = sf::Transform::Identity;
		}
	}
	
	void setModifiers(enum transformOp (*type)(),bool (*active)(),int32_t (*valX)(),int32_t (*valY)(),float (*valScalar)()){
		opType = type;
		opActive = active;
		opValX = valX;
		opValY = valY;
		opValScalar = valScalar;
	}
	
	void applyModifiers(unsigned char currPose){
		if(!opActive()){
			return;
		}
		
		switch(opType()){
			case TROP_TRANSLATE:
				poses[currPose].translateX = opValX();
				poses[currPose].translateY = opValY();
				
				break;
			case TROP_ROTATE:
				poses[currPose].rotation = opValScalar();
				
				break;
			case TROP_SCALE:
				poses[currPose].scale = opValScalar();
				
				break;
		}
		
		poses[currPose].transformation =
			sf::Transform().scale(
				poses[currPose].scale,
				poses[currPose].scale,
				vw::norm::toD(boneArray[currPose].x),
				vw::norm::toD(boneArray[currPose].y)
			).rotate(
				poses[currPose].rotation * 180.0 / PI,
				vw::norm::toD(boneArray[currPose].x),
				vw::norm::toD(boneArray[currPose].y)
			).translate(
				vw::norm::toD(poses[currPose].translateX),
				vw::norm::toD(poses[currPose].translateY)
			);
	}
	
	void composeParent(unsigned char parent){
		// Self composition
		globalTransforms[parent] *= poses[parent].transformation;
		
		// Recursive walk down the tree
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			if(boneArray[i].parent == parent){
				globalTransforms[i] *= globalTransforms[parent];
				composeParent(i);
			}
		}
	}
	
	void calculateGlobals(){
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			globalTransforms[i] = sf::Transform::Identity;
		}
		
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			if(boneArray[i].parent >= BONES_MAX_COUNT){
				composeParent(i);
			}
		}
	}
	
	void draw(){
		sf::Vector2f src,dst;
		
		// Stems
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			if(boneArray[i].parent < BONES_MAX_COUNT){
				src = globalTransforms[i].transformPoint(
					vw::norm::toD(boneArray[i].x),
					vw::norm::toD(boneArray[i].y)
				);
				
				dst = globalTransforms[boneArray[i].parent].transformPoint(
					vw::norm::toD(boneArray[boneArray[i].parent].x),
					vw::norm::toD(boneArray[boneArray[i].parent].y)
				);
				
				hud::drawStem(i,vw::norm::toI(src.x),vw::norm::toI(src.y),vw::norm::toI(dst.x),vw::norm::toI(dst.y));
			}
		}
		
		// Marks
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			src = globalTransforms[i].transformPoint(
				vw::norm::toD(boneArray[i].x),
				vw::norm::toD(boneArray[i].y)
			);
			
			hud::drawMark(i,vw::norm::toI(src.x),vw::norm::toI(src.y),false,POINT_RADIUS);
		}
	}
}