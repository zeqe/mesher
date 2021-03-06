#ifndef GRAPHICS_INCLUDED
	#include <cstdint>
	#include <vector>
	
	#include <SFML/Graphics.hpp>
	
	extern "C" {
		#include <vecGL/vecTris.h>
	}
	
	#include "state.hpp"
	#include "layer.hpp"
	#include "colors.hpp"
	
	#define POINT_RADIUS 7.5
	#define POINT_RADIUS_AURA 20.0
	
	#define TRI_VERT_MARKER_RADIUS 60.0
	
	namespace graphics{
		bool load(sf::RenderTarget &newTarget,const char *hudFontPath);
		void free();
		
		void calculateTargetDimensions();
	}
	
	namespace render{
		// Texture
		namespace tex{
			bool load(const char *source);
			void unload();
			bool isLoaded();
			
			void toggleSmooth();
			void draw();
		}
		
		// Rendering
		#define VERT_MODE_RAW_XY 1
		#define VERT_MODE_RAW_UV 2
		#define VERT_MODE_POSE_XY 3
		
		#define FRAG_MODE_CLIPPED_CLR 0
		#define FRAG_MODE_FULL_CLR 1
		#define FRAG_MODE_CLIPPED_CLR_SMPL 2
		
		#define CLR_PFL_EDITOR 0
		#define CLR_PFL_RANBW 1
		#define CLR_PFL_CSTM 2
		
		void loadAndDrawTris(struct vecTrisBuf *buf,struct vecTris **tris,unsigned int vert,unsigned int frag,unsigned int pfl,bool wireframe);
	}
	
	namespace hud{
		// Position
		enum corner{
			cTR,
			cBR,
			cTL,
			cBL
		};
		
		sf::Vector2f charPosition(enum corner c,float x,float y);
		unsigned int markColorI(unsigned int i);
		
		// Drawing Static UI
		void drawCenter();
		
		void drawStateState(enum mesherState state);
		void drawHelp(enum mesherState state);
		void drawBottomBar(const char *line,bool snapOn,bool showTris,unsigned char currTri);
		void drawLayerNav(std::vector<class vertLayer *> &layers,unsigned int currLayer,const char *altCurrLayerName,class gridLayer *grid,const char *altGridDisplay);
		void drawCustomColorsRef(unsigned char currColor,const char *altCurrColorHex);
		void drawVBonesRef(unsigned char currBone);
		void drawBonesRef(unsigned char currBone,const char *altCurrBoneParent);
		
		// Drawing Dynamic Model-State-Dependant UI
		void drawCircle(int32_t x,int32_t y,bool isRadNorm,float radius,uint32_t color);
		void drawCircleOutline(int32_t x,int32_t y,bool isRadNorm,float radius,uint32_t color);
		
		void drawWedge(int32_t srcX,int32_t srcY,int32_t aX,int32_t aY,int32_t bX,int32_t bY,float radius,uint32_t color);
		void drawMark(unsigned int i,int32_t x,int32_t y,bool isScaleNorm,float scale);
		void drawStem(unsigned int i,int32_t srcX,int32_t srcY,int32_t destX,int32_t destY);
		
		enum lineType{
			LINE_VERTICAL,
			LINE_HORIZONTAL
		};
		
		void drawLine(enum lineType type,int16_t x,int16_t y,float len,uint32_t color);
		
		// Reference
		namespace ref{
			bool load(const char *source);
			void unload();
			
			void toggleSmooth();
			void draw();
		}
	}
	
	#define GRAPHICS_INCLUDED
#endif