#ifndef TRANSFORM_OPERATIONS_INCLUDED
	#include <cstdint>
	
	enum transformOp{
		TROP_TRANSLATE,
		TROP_ROTATE,
		TROP_SCALE
	};
	
	enum transformOpState{
		TROP_STATE_PRIME,
		TROP_STATE_UPDATE,
		TROP_STATE_NONE
	};
	
	namespace trOp{
		enum transformOp currentOp();
		enum transformOpState currentState();
		bool dirty();
		
		bool init(enum transformOp op,int32_t x,int32_t y);
		bool prime(int32_t x,int32_t y);
		void update(int32_t x,int32_t y);
		
		int32_t valX();
		int32_t valY();
		float valScalar();
		
		void apply(int16_t *x,int16_t *y);
		void exit();
		
		void drawUI();
	}
	
	#define TRANSFORM_OPERATIONS_INCLUDED
#endif