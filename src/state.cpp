#include "state.hpp"

namespace state{
	enum mesherState currState;
	enum drawState currDrawState;
	
	bool isTextual(enum mesherState state){
		return (
			state == STATE_CONSOLE ||
			state == STATE_ATOP_COLOR_SET ||
			state == STATE_ATOP_GRID_SET ||
			state == STATE_ATOP_LAYER_NAME ||
			state == STATE_ATOP_BONE_PARENT_SET
		);
	}
	
	void set(enum mesherState newState){
		currState = newState;
		
		switch(currState){
			case STATE_VERT_XY:
			case STATE_ATOP_TRI_ADD:
			case STATE_ATOP_TRANSFORM:
				currDrawState = D_STATE_XY;
				
				break;
			case STATE_VERT_UV:
				currDrawState = D_STATE_UV;
				
				break;
			case STATE_VERT_COLOR:
			case STATE_ATOP_COLOR_SET:
				currDrawState = D_STATE_COLOR;
				
				break;
			case STATE_VERT_BONE:
			case STATE_BONES:
			case STATE_ATOP_BONE_PARENT_SET:
				currDrawState = D_STATE_BONE;
				
				break;
			case STATE_LAYERS:
			case STATE_CONSOLE:
			case STATE_COUNT:
			case STATE_ATOP_GRID_SET:
			case STATE_ATOP_LAYER_NAME:
			case TOTAL_STATE_COUNT:
				currDrawState = D_STATE_VIEW;
				
				break;
			case STATE_POSE:
			case STATE_ATOP_POSE_TRANSFORM:
				currDrawState = D_STATE_POSE;
				
				break;
			default:
				currDrawState = D_STATE_VIEW;
				
				break;
		}
	}
	
	enum mesherState get(){
		return currState;
	}
	
	enum drawState getDraw(){
		return currDrawState;
	}
}