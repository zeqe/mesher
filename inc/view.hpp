#ifndef VIEW_INCLUDED
	#include <cstdint>
	
	#include <SFML/Graphics.hpp>
	
	namespace vw{
		// Globals
		void reset(bool resetSeek);
		sf::Transform transform();
		
		// Pan
		void panBegin();
		void panContinue();
		void panEnd();
		
		// Zoom
		void zoomIn();
		void zoomOut();
		
		int zoomLevel();
		float zoomFactor();
		
		// Seek Radius
		void seekIncrease();
		void seekDecrease();
		
		unsigned int seekRadius();
		
		namespace norm{
			// Global
			sf::Transform transform();
			
			double toD(int16_t val);
			int16_t toI(double val);
			
			// Normalized scale
			void setScale(float newScale);
			
			float getScale();
			float getZoomScale();
			
			// Normalized seek radius
			float seekRadius();
			
			// Cursor normalized projection
			void cursorCalc(sf::Window &parent);
			
			bool cursorIn();
			sf::Vector2<double> cursorPos();
			
			// vecGL uniform handling
			namespace vecGL{
				void apply(sf::RenderTarget &target,float offsetX,float offsetY,float relScaleX,float relScaleY);
			}
		}
	}
	
	#define VIEW_INCLUDED
#endif