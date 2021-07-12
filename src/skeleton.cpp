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
	
	int32_t d_TranslateX,d_TranslateY;
	float d_Scale,d_Rotation;
	
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
			
			poses[i].d_TranslateX = 0;
			poses[i].d_TranslateY = 0;
			poses[i].d_Scale = 1.0;
			poses[i].d_Rotation = 0.0;
			
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
	
	void calculateLocalTransformation(unsigned char currPose){
		float centerX = vw::norm::toD_u((int32_t)boneArray[currPose].x + poses[currPose].translateX + poses[currPose].d_TranslateX);
		float centerY = vw::norm::toD_u((int32_t)boneArray[currPose].y + poses[currPose].translateY + poses[currPose].d_TranslateY);
		
		poses[currPose].transformation =
			sf::Transform().scale(
				poses[currPose].scale * poses[currPose].d_Scale,
				poses[currPose].scale * poses[currPose].d_Scale,
				centerX,
				centerY
			).rotate(
				poses[currPose].rotation + poses[currPose].d_Rotation,
				centerX,
				centerY
			).translate(
				vw::norm::toD_u(poses[currPose].translateX + poses[currPose].d_TranslateX),
				vw::norm::toD_u(poses[currPose].translateY + poses[currPose].d_TranslateY)
			);
	}
	
	void updateModifiers(bool apply,unsigned char currPose){
		if(!opActive()){
			return;
		}
		
		unsigned char currI;
		float localRot;
		sf::Vector2f point;
		
		switch(opType()){
			case TROP_TRANSLATE:
				// Determine local rotation
				currI = boneArray[currPose].parent;
				localRot = 0.0;
				
				while(currI < BONES_MAX_COUNT){
					localRot += poses[currI].rotation;
					currI = boneArray[currI].parent;
				}
				
				// Determine according transformation
				point = sf::Transform().rotate(-localRot).transformPoint(vw::norm::toD_u(opValX()),vw::norm::toD_u(opValY()));
				
				if(apply){
					poses[currPose].translateX += vw::norm::toI_u(point.x);
					poses[currPose].translateY += vw::norm::toI_u(point.y);
					
					poses[currPose].d_TranslateX = 0;
					poses[currPose].d_TranslateY = 0;
				}else{
					poses[currPose].d_TranslateX = vw::norm::toI_u(point.x);
					poses[currPose].d_TranslateY = vw::norm::toI_u(point.y);
				}
				
				break;
			case TROP_ROTATE:
				if(apply){
					poses[currPose].rotation += opValScalar() * 180.0 / PI;
					poses[currPose].d_Rotation = 0.0;
				}else{
					poses[currPose].d_Rotation = opValScalar() * 180.0 / PI;
				}
				
				break;
			case TROP_SCALE:
				if(apply){
					poses[currPose].scale *= opValScalar();
					poses[currPose].d_Scale = 1.0;
				}else{
					poses[currPose].d_Scale = opValScalar();
				}
				
				break;
		}
		
		calculateLocalTransformation(currPose);
	}
	
	void clearUnappliedModifiers(){
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			poses[i].d_TranslateX = 0;
			poses[i].d_TranslateY = 0;
			
			poses[i].d_Scale = 1.0;
			poses[i].d_Rotation = 0.0;
			
			calculateLocalTransformation(i);
		}
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
	
	void calculateGlobalTransformations(){
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			globalTransforms[i] = sf::Transform::Identity;
		}
		
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			if(boneArray[i].parent >= BONES_MAX_COUNT){
				composeParent(i);
			}
		}
	}
	
	sf::Vector2<int32_t> getBonePosition(unsigned char bone){
		sf::Vector2f tPos = globalTransforms[bone & BONE_INDEX_MASK].transformPoint(
			vw::norm::toD(boneArray[bone & BONE_INDEX_MASK].x),
			vw::norm::toD(boneArray[bone & BONE_INDEX_MASK].y)
		);
		
		return sf::Vector2<int32_t>(vw::norm::toI_u(tPos.x),vw::norm::toI_u(tPos.y));
	}
	
	void draw(){
		sf::Vector2<int32_t> src,dst;
		
		// Stems
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			if(boneArray[i].parent < BONES_MAX_COUNT){
				src = getBonePosition(i);
				dst = getBonePosition(boneArray[i].parent);
				
				hud::drawStem(i,src.x,src.y,dst.x,dst.y);
			}
		}
		
		// Marks
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			src = getBonePosition(i);
			hud::drawMark(i,src.x,src.y,false,POINT_RADIUS);
		}
	}
}