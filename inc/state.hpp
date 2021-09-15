#ifndef STATE_INCLUDED
	enum mesherState{
		STATE_VERT_XY,
		STATE_VERT_UV,
		STATE_VERT_COLOR,
		STATE_VERT_BONE,
		STATE_LAYERS,
		STATE_BONES,
		STATE_POSE,
		STATE_CONSOLE,
		
		STATE_COUNT,
		
		STATE_ATOP_TRI_ADD,
		STATE_ATOP_TRANSFORM_XY,
		STATE_ATOP_TRANSFORM_UV,
		STATE_ATOP_COLOR_SET,
		STATE_ATOP_GRID_SET,
		STATE_ATOP_LAYER_NAME,
		STATE_ATOP_BONE_PARENT_SET,
		STATE_ATOP_TRANSFORM_POSE,
		
		TOTAL_STATE_COUNT
	};
	
	enum drawState{
		D_STATE_XY,
		D_STATE_UV,
		D_STATE_COLOR,
		D_STATE_BONE,
		D_STATE_VIEW,
		D_STATE_POSE
	};
	
	namespace state{
		bool isTextual(enum mesherState state);
		
		void set(enum mesherState newState);
		enum mesherState get();
		enum drawState getDraw();
	}
	
	#define STATE_INCLUDED
#endif