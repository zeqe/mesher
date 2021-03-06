#include <cmath>

#include "transformOp.hpp"
#include "graphics.hpp"
#include "colors.hpp"
#include "view.hpp"

namespace trOp{
	// State
	enum transformOp op;
	enum transformOpState opState = TROP_STATE_NONE;
	bool dirtied;
	
	// Transformation values
	int32_t srcX,srcY;
	int32_t handleX,handleY;
	int32_t lastX,lastY;
	
	// Private derivative values
	int32_t distHX(){
		return handleX - srcX;
	}
	
	int32_t distHY(){
		return handleY - srcY;
	}
	
	float distHLen(){
		return sqrt((float)(distHX() * distHX() + distHY() * distHY()));
	}
	
	int32_t distLX(){
		return lastX - srcX;
	}
	
	int32_t distLY(){
		return lastY - srcY;
	}
	
	float distLLen(){
		return sqrt((float)(distLX() * distLX() + distLY() * distLY()));
	}
	
	float angleLH(){
		return atan2(distLY(),distLX()) - atan2(distHY(),distHX());
	}
	
	float scaleLH(){
		return distLLen() / distHLen();
	}
	
	// Status reporters
	enum transformOp currentOp(){
		return op;
	}
	
	enum transformOpState currentState(){
		return opState;
	}
	
	// Operations
	bool dirty(){
		return (opState == TROP_STATE_UPDATE);
	}
	
	bool init(enum transformOp newOp,int32_t x,int32_t y){
		if(opState != TROP_STATE_NONE){
			return false;
		}
		
		op = newOp;
		srcX = x;
		srcY = y;
		
		switch(op){
			case TROP_ROTATE:
			case TROP_SCALE:
				opState = TROP_STATE_PRIME;
				
				break;
			case TROP_TRANSLATE:
				opState = TROP_STATE_UPDATE;
				
				break;
		}
		
		return true;
	}
	
	bool prime(int32_t x,int32_t y){
		if(opState != TROP_STATE_PRIME){
			return false;
		}
		
		if(x == srcX && y == srcY){
			return false;
		}
		
		handleX = x;
		handleY = y;
		
		opState = TROP_STATE_UPDATE;
		
		return true;
	}
	
	void update(int32_t x,int32_t y){
		lastX = x;
		lastY = y;
	}
	
	int32_t valX(){
		if(op == TROP_TRANSLATE){
			return distLX();
		}
		
		return 0;
	}
	
	int32_t valY(){
		if(op == TROP_TRANSLATE){
			return distLY();
		}
		
		return 0;
	}
	
	float valScalar(){
		switch(op){
			case TROP_ROTATE:
				return angleLH();
				
				break;
			case TROP_SCALE:
				return scaleLH();
				
				break;
			default:
				break;
		}
		
		return 0.0;
	}
	
	// Private constrained operation helper functions
	void constrainedSet(int16_t *var,int32_t val){
		if(val > INT16_MAX){
			*var = INT16_MAX;
		}else if(val < INT16_MIN){
			*var = INT16_MIN;
		}else{
			*var = val;
		}
	}
	
	void constrainedAdd(int16_t *var,int32_t val){
		constrainedSet(var,(int32_t)*var + val);
	}
	
	void constrainedRotate(int16_t *x,int16_t *y,float angle){
		int32_t newX = (float)(*x) * cos(angle) - (float)(*y) * sin(angle);
		int32_t newY = (float)(*x) * sin(angle) + (float)(*y) * cos(angle);
		
		constrainedSet(x,newX);
		constrainedSet(y,newY);
	}
	
	// Application operations
	void apply(int16_t *x,int16_t *y){
		if(opState != TROP_STATE_UPDATE){
			return;
		}
		
		if(distLX() == 0 && distLY() == 0){
			return;
		}
		
		// Operation
		int32_t pX,pY;
		
		switch(op){
			case TROP_TRANSLATE:
				constrainedAdd(x,distLX());
				constrainedAdd(y,distLY());
				
				break;
			case TROP_ROTATE:
				*x -= srcX;
				*y -= srcY;
				
				constrainedRotate(x,y,angleLH());
				
				constrainedAdd(x,srcX);
				constrainedAdd(y,srcY);
				
				break;
			case TROP_SCALE:
				pX = (float)(*x - srcX) * scaleLH();
				pY = (float)(*y - srcY) * scaleLH();
				
				*x = srcX;
				*y = srcY;
				
				constrainedAdd(x,pX);
				constrainedAdd(y,pY);
				
				break;
		}
	}
	
	void exit(){
		opState = TROP_STATE_NONE;
	}
	
	void drawUI(){
		if(opState == TROP_STATE_NONE){
			return;
		}
		
		switch(op){
			case TROP_TRANSLATE:
				// Handle
				hud::drawCircle(lastX,lastY,false,POINT_RADIUS_AURA,clr::get(clr::PFL_EDITR,CLR_EDITR_HICONTRAST,clr::ALF_UTIL));
				
				break;
			case TROP_ROTATE:
				switch(opState){
					case TROP_STATE_UPDATE:
						// Background circle
						hud::drawCircle(
							srcX,
							srcY,
							true,
							(distHLen() / (float)INT16_MAX) + (POINT_RADIUS / vw::norm::getScale()),
							clr::get(clr::PFL_EDITR,CLR_EDITR_HICONTRAST,clr::ALF_UTIL)
						);
						
						// Source circle
						hud::drawCircle(srcX,srcY,false,POINT_RADIUS,clr::get(clr::PFL_EDITR,CLR_EDITR_OFFWHITE,clr::ALF_HALF));
						
						// Handle circle
						hud::drawCircle(handleX,handleY,false,POINT_RADIUS,clr::get(clr::PFL_EDITR,CLR_EDITR_OFFWHITE,clr::ALF_HALF));
						
						// Current angle circle
						hud::drawCircle(
							(float)srcX + distLX() * (distHLen() / distLLen()),
							(float)srcY + distLY() * (distHLen() / distLLen()),
							false,
							POINT_RADIUS,
							clr::get(clr::PFL_EDITR,CLR_EDITR_OFFWHITE,clr::ALF_HALF)
						);
						
						break;
					case TROP_STATE_PRIME:
						// WIP handle circle
						hud::drawCircle(
							srcX,
							srcY,
							true,
							(distLLen() / (float)INT16_MAX) + (POINT_RADIUS / vw::norm::getScale()),
							clr::get(clr::PFL_EDITR,CLR_EDITR_HICONTRAST,clr::ALF_UTIL)
						);
						
						// Source circle
						hud::drawCircle(srcX,srcY,false,POINT_RADIUS,clr::get(clr::PFL_EDITR,CLR_EDITR_OFFWHITE,clr::ALF_HALF));
						
						break;
					case TROP_STATE_NONE:
						break;
				}
				
				break;
			case TROP_SCALE:
				switch(opState){
					case TROP_STATE_UPDATE:
						// Handle circle
						hud::drawCircle(srcX,srcY,true,distHLen() / (float)INT16_MAX,clr::get(clr::PFL_EDITR,CLR_EDITR_HICONTRAST,clr::ALF_UTIL));
						
					case TROP_STATE_PRIME:
						// WIP circle - handle / current scale factor
						hud::drawCircle(srcX,srcY,true,distLLen() / (float)INT16_MAX,clr::get(clr::PFL_EDITR,CLR_EDITR_HICONTRAST,clr::ALF_UTIL));
						
						// Source circle
						hud::drawCircle(srcX,srcY,false,POINT_RADIUS,clr::get(clr::PFL_EDITR,CLR_EDITR_OFFWHITE,clr::ALF_HALF));
						
						break;
					case TROP_STATE_NONE:
						break;
				}
				
				break;
		}
	}
}