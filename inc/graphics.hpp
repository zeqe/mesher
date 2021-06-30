#ifndef HUD_INCLUDED
	#include <cstdint>
	
	#include <string>
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
	
	namespace graphics{
		bool load(sf::RenderTarget &newTarget,const char *hudFontPath);
		void free();
		
		void calculateTargetDimensions();
		sf::Vector2f calculateStringDimensions(std::string in);
	}
	
	namespace render{
		void setColors(enum clr::profile mAr,enum clr::profile wAr);
		void loadAndDrawTris(struct vecTrisBuf *buf,struct vecTris **tris,bool UV,bool customClr,bool wireframe);
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
		
		// Drawing Static UI
		void drawCenter();
		
		void drawStateState(enum mesherState state);
		void drawHelp(enum mesherState state);
		void drawBottomBar(const std::string &line,bool snapOn,bool showTris,unsigned char currTri);
		void drawLayerNav(std::vector<class vertLayer *> &layers,unsigned int currLayer,const char *altCurrLayerName,class gridLayer *grid,const char *altGridDisplay);
		void drawCustomColorsReff(unsigned char currColor,const char *altCurrColorHex);
		void drawBonesReff(unsigned char currBone);
		
		// Drawing Dynamic Model-State-Dependant UI
		enum markType{
			MARK_CIRCLE,
			MARK_RHOMBUS,
			MARK_SQUARE,
			MARK_TRIANGLE,
			
			MARK_COUNT
		};
		
		void drawMark(enum markType type,int16_t x,int16_t y,bool isScaleNorm,float scale,uint32_t color);
		void drawCircleOutline(int16_t x,int16_t y,bool isRadNorm,float radius,uint32_t color);
		
		enum lineType{
			LINE_VERTICAL,
			LINE_HORIZONTAL
		};
		
		void drawLine(enum lineType type,int16_t x,int16_t y,float len,uint32_t color);
	}
	
	#define HUD_INCLUDED
#endif