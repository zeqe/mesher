extern "C" {
	#include <vecGL/shader.h>
	#include <vecGL/bones.h>
}

#include "skeleton.hpp"
#include "graphics.hpp"
#include "colors.hpp"
#include "view.hpp"

#define PI 3.14159265358979323846

bool poseBoneModified[BONES_MAX_COUNT];
bool poseModified;

struct bone{
	int16_t x,y;
	unsigned char parent;
};

struct bone boneArray[BONES_MAX_COUNT];

namespace bones{
	void init(){
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			boneArray[i].parent = BONES_MAX_COUNT;
			
			poseBoneModified[i] = true;
		}
		
		poseModified = true;
	}
	
	void setOrigin(unsigned char i,int16_t x,int16_t y){
		unsigned char j = i & BONE_INDEX_MASK;
		
		boneArray[j].x = x;
		boneArray[j].y = y;
		
		poseBoneModified[j] = true;
		poseModified = true;
	}
	
	void setParent(unsigned char i,unsigned char parent){
		unsigned char j = i & BONE_INDEX_MASK;
		
		// Escape case
		if(parent >= BONES_MAX_COUNT){
			boneArray[j].parent = BONES_MAX_COUNT;
			
			poseBoneModified[j] = true;
			poseModified = true;
			
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
		
		poseBoneModified[j] = true;
		poseModified = true;
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
float globalTransformMat3s[9 * BONES_MAX_COUNT];

bool poseUpload = false;

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
			
			poseBoneModified[i] = true;
		}
		
		poseModified = true;
	}
	
	void setModifiers(enum transformOp (*type)(),bool (*active)(),int32_t (*valX)(),int32_t (*valY)(),float (*valScalar)()){
		opType = type;
		opActive = active;
		opValX = valX;
		opValY = valY;
		opValScalar = valScalar;
	}
	
	void updateModifiers(bool apply,unsigned char currPose){
		if(!opActive()){
			return;
		}
		
		unsigned char j = currPose & BONE_INDEX_MASK;
		
		unsigned char currI;
		float localRot;
		sf::Vector2f point;
		
		switch(opType()){
			case TROP_TRANSLATE:
				// Determine local rotation
				currI = boneArray[j].parent;
				localRot = 0.0;
				
				while(currI < BONES_MAX_COUNT){
					localRot += poses[currI].rotation;
					currI = boneArray[currI].parent;
				}
				
				// Determine according transformation
				point = sf::Transform().rotate(-localRot).transformPoint(vw::norm::toD_u(opValX()),vw::norm::toD_u(opValY()));
				
				if(apply){
					poses[j].translateX += vw::norm::toI_u(point.x);
					poses[j].translateY += vw::norm::toI_u(point.y);
					
					poses[j].d_TranslateX = 0;
					poses[j].d_TranslateY = 0;
				}else{
					poses[j].d_TranslateX = vw::norm::toI_u(point.x);
					poses[j].d_TranslateY = vw::norm::toI_u(point.y);
				}
				
				break;
			case TROP_ROTATE:
				if(apply){
					poses[j].rotation += opValScalar() * 180.0 / PI;
					poses[j].d_Rotation = 0.0;
				}else{
					poses[j].d_Rotation = opValScalar() * 180.0 / PI;
				}
				
				break;
			case TROP_SCALE:
				if(apply){
					poses[j].scale *= opValScalar();
					poses[j].d_Scale = 1.0;
				}else{
					poses[j].d_Scale = opValScalar();
				}
				
				break;
		}
		
		poseBoneModified[j] = true;
		poseModified = true;
	}
	
	void clearUnappliedModifiers(){
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			poses[i].d_TranslateX = 0;
			poses[i].d_TranslateY = 0;
			
			poses[i].d_Scale = 1.0;
			poses[i].d_Rotation = 0.0;
			
			poseBoneModified[i] = true;
		}
		
		poseModified = true;
	}
	
	sf::Vector2<int32_t> getBonePosition(unsigned char bone){
		sf::Vector2f tPos = globalTransforms[bone & BONE_INDEX_MASK].transformPoint(
			vw::norm::toD(boneArray[bone & BONE_INDEX_MASK].x),
			vw::norm::toD(boneArray[bone & BONE_INDEX_MASK].y)
		);
		
		return sf::Vector2<int32_t>(vw::norm::toI_u(tPos.x),vw::norm::toI_u(tPos.y));
	}
	
	sf::Vector2<int32_t> getPointPosition(unsigned char bone,int16_t x,int16_t y){
		sf::Vector2f tPos = globalTransforms[bone & BONE_INDEX_MASK].transformPoint(vw::norm::toD(x),vw::norm::toD(y));
		
		return sf::Vector2<int32_t>(vw::norm::toI_u(tPos.x),vw::norm::toI_u(tPos.y));
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
	
	void update(){
		if(!poseModified){
			return;
		}
		
		// Local transformation update calculations
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			if(!poseBoneModified[i]){
				continue;
			}
			
			float centerX = vw::norm::toD_u((int32_t)boneArray[i].x + poses[i].translateX + poses[i].d_TranslateX);
			float centerY = vw::norm::toD_u((int32_t)boneArray[i].y + poses[i].translateY + poses[i].d_TranslateY);
			
			poses[i].transformation =
				sf::Transform().scale(
					poses[i].scale * poses[i].d_Scale,
					poses[i].scale * poses[i].d_Scale,
					centerX,
					centerY
				).rotate(
					poses[i].rotation + poses[i].d_Rotation,
					centerX,
					centerY
				).translate(
					vw::norm::toD_u(poses[i].translateX + poses[i].d_TranslateX),
					vw::norm::toD_u(poses[i].translateY + poses[i].d_TranslateY)
				);
			
			poseBoneModified[i] = false;
		}
		
		// Global transformation calculations
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			globalTransforms[i] = sf::Transform::Identity;
		}
		
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			if(boneArray[i].parent >= BONES_MAX_COUNT){
				composeParent(i);
			}
		}
		
		// Set Mat3s for uploading
		for(unsigned int i = 0;i < BONES_MAX_COUNT;++i){
			const float *currGlobalMat = globalTransforms[i].getMatrix();
			
			globalTransformMat3s[i * 9 + 0] = currGlobalMat[0];
			globalTransformMat3s[i * 9 + 1] = currGlobalMat[1];
			globalTransformMat3s[i * 9 + 2] = currGlobalMat[3];
			
			globalTransformMat3s[i * 9 + 3] = currGlobalMat[4];
			globalTransformMat3s[i * 9 + 4] = currGlobalMat[5];
			globalTransformMat3s[i * 9 + 5] = currGlobalMat[7];
			
			globalTransformMat3s[i * 9 + 6] = currGlobalMat[12];
			globalTransformMat3s[i * 9 + 7] = currGlobalMat[13];
			globalTransformMat3s[i * 9 + 8] = currGlobalMat[15];
		}
		
		// Set flags
		poseModified = false;
		poseUpload = true;
	}
	
	void upload(){
		if(!poseUpload){
			return;
		}
		
		uniformBones(globalTransformMat3s,BONES_MAX_COUNT);
		poseUpload = false;
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