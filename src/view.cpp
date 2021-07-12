#include <cmath>

extern "C" {
	#include <vecGL/shader.h>
}

#include "view.hpp"

namespace vw{
	// State ---------------------------------------------------------------------------
	sf::Vector2f lastPanPos,deltaPanPos;
	sf::Vector2i lastCursorPos;
	bool isPanning = false;
	
	#define VIEW_ZOOM_SCALE_FACTOR (3.0 / 2.0)
	
	#define ZOOM_MAX_POWER 27
	#define ZOOM_MIN_POWER -10
	
	int zoomPower = 0;
	float zoomScale = 1.0;
	
	#define SEEK_RADIUS_DIVS 10
	#define SEEK_RADIUS_MIN 1
	#define SEEK_RADIUS_MAX 10
	
	unsigned int seekRadiusLevel;
	
	// Globals -------------------------------------------------------------------------
	void reset(bool resetSeek){
		lastPanPos = sf::Vector2f(0.0,0.0);
		deltaPanPos = sf::Vector2f(0.0,0.0);
		
		lastCursorPos = sf::Mouse::getPosition();
		
		zoomPower = 0;
		zoomScale = 1.0;
		
		if(resetSeek){
			seekRadiusLevel = (SEEK_RADIUS_MAX - SEEK_RADIUS_MIN) / 2;
		}
	}
	
	sf::Transform transform(){
		return sf::Transform().translate(lastPanPos + deltaPanPos).scale(zoomScale,zoomScale);
	}
	
	// Pan -----------------------------------------------------------------------------
	void panBegin(){
		lastPanPos = lastPanPos + deltaPanPos;
		deltaPanPos = sf::Vector2f(0.0,0.0);
		
		lastCursorPos = sf::Mouse::getPosition();
		isPanning = true;
	}
	
	void panContinue(){
		if(!isPanning){
			return;
		}
		
		deltaPanPos = sf::Vector2f(sf::Mouse::getPosition() - lastCursorPos);
	}
	
	void panEnd(){
		isPanning = false;
	}
	
	// Zoom ----------------------------------------------------------------------------
	void zoomIn(){
		if(isPanning || zoomPower >= ZOOM_MAX_POWER){
			return;
		}
		
		panBegin();
		panEnd();
		
		++zoomPower;
		zoomScale = pow(VIEW_ZOOM_SCALE_FACTOR,zoomPower);
		
		lastPanPos *= (float)VIEW_ZOOM_SCALE_FACTOR;
	}
	
	void zoomOut(){
		if(isPanning || zoomPower <= ZOOM_MIN_POWER){
			return;
		}
		
		panBegin();
		panEnd();
		
		--zoomPower;
		zoomScale = pow(VIEW_ZOOM_SCALE_FACTOR,zoomPower);
		
		lastPanPos /= (float)VIEW_ZOOM_SCALE_FACTOR;
	}
	
	int zoomLevel(){
		return zoomPower;
	}
	
	float zoomFactor(){
		return zoomScale;
	}
	
	// Seek Radius ----------------------------------------------------------------------------
	void seekIncrease(){
		if(seekRadiusLevel >= SEEK_RADIUS_MAX){
			return;
		}
		
		++seekRadiusLevel;
	}
	
	void seekDecrease(){
		if(seekRadiusLevel <= SEEK_RADIUS_MIN){
			return;
		}
		
		--seekRadiusLevel;
	}
	
	unsigned int seekRadius(){
		return ((INT16_MAX / SEEK_RADIUS_DIVS) / zoomFactor()) * seekRadiusLevel;
	}
	
	namespace norm{
		float normScale = 1.0;
		
		// Global ----------------------------------------------------------------------
		sf::Transform transform(){
			return vw::transform().scale(normScale,-normScale);
		}
		
		double toD(int16_t val){
			return (double)val / (double)INT16_MAX;
		}
		
		int16_t toI(double val){
			return (int16_t)(val * (double)INT16_MAX);
		}
		
		double toD_u(int32_t val){
			return (double)val / (double)INT16_MAX;
		}
		
		int32_t toI_u(double val){
			return (int32_t)(val * (double)INT16_MAX);
		}
		
		// Normalized scale ------------------------------------------------------------
		void setScale(float newScale){
			normScale = newScale;
		}
		
		float getScale(){
			return normScale;
		}
		
		float getZoomScale(){
			return normScale * zoomScale;
		}
		
		// Normalized seek radius ------------------------------------------------------
		float seekRadius(){
			return (1.0 / SEEK_RADIUS_DIVS) * seekRadiusLevel;
		}
		
		// Cursor normalized projection ------------------------------------------------
		sf::Vector2<double> cursNorm;
		bool cursIn = false;
		
		void cursorCalc(sf::Window &parent){
			sf::Vector2f cursorPos = sf::Vector2f(sf::Mouse::getPosition(parent)) - sf::Vector2f(parent.getSize() / 2u);
			cursorPos = vw::norm::transform().getInverse().transformPoint(cursorPos);
			
			cursIn = true;
			
			if(cursorPos.x < -1.0){
				cursNorm.x = -1.0;
				cursIn = false;
				
			}else if(cursorPos.x > 1.0){
				cursNorm.x = 1.0;
				cursIn = false;
				
			}else{
				cursNorm.x = (double)cursorPos.x;
			}
			
			if(cursorPos.y < -1.0){
				cursNorm.y = -1.0;
				cursIn = false;
				
			}else if(cursorPos.y > 1.0){
				cursNorm.y = 1.0;
				cursIn = false;
				
			}else{
				cursNorm.y = (double)cursorPos.y;
			}
		}
		
		bool cursorIn(){
			return cursIn;
		}
		
		sf::Vector2<double> cursorPos(){
			return cursNorm;
		}
		
		// vecGL uniform handling
		namespace vecGL{
			void apply(sf::RenderTarget &target,float offsetX,float offsetY,float relScaleX,float relScaleY){
				sf::Vector2f panPos = lastPanPos + deltaPanPos;
				panPos = panPos / (normScale * zoomScale);
				
				uniformPosition(offsetX + panPos.x,offsetY - panPos.y);
				uniformParamsV(relScaleX,relScaleY,0.0,0.0);
				
				uniformSSR(
					zoomScale * normScale * 2.0 / (float)target.getSize().x,
					zoomScale * normScale * 2.0 / (float)target.getSize().y,
					0.0
				);
			}
		}
	}
}