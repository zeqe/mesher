#include "state.hpp"

bool stateIsTextual(enum mesherState state){
	return (state == STATE_CONSOLE || state == STATE_ATOP_COLOR_SET || state == STATE_ATOP_GRID_SET || state == STATE_ATOP_LAYER_NAME);
}