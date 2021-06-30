#ifndef STRING_INPUT_INCLUDED
	
	#define STRIN_MAX_LEN 200
	
	namespace strIn{
		// State
		bool active();
		void activate(unsigned int len);
		void deactivate();
		
		// Use
		bool interpret(char newChar);
		const char *buffer();
	}
	
	#define STRING_INPUT_INCLUDED
#endif