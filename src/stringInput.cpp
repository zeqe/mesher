#include <cctype>

#include "stringInput.hpp"

namespace strIn{
	bool building = false;
	
	char buff[STRIN_MAX_LEN + 2];
	unsigned int buildMaxLen;
	unsigned int currI;
	
	void demarkateEnd(){
		buff[currI] = ']';
		buff[currI + 1] = '\0';
	}
	
	// State ---------------------
	bool active(){
		return building;
	}
	
	void activate(unsigned int len){
		building = true;
		buildMaxLen = (len > STRIN_MAX_LEN) ? STRIN_MAX_LEN : len;
		currI = 0;
		
		demarkateEnd();
	}
	
	void deactivate(){
		building = false;
		currI = 0;
		
		demarkateEnd();
	}
	
	// Use -----------------------
	bool interpret(char newChar){
		if(!building){
			return false;
		}
		
		switch(newChar){
			case '\b':
				if(currI == 0){
					break;
				}
				
				--currI;
				
				demarkateEnd();
				
				break;
			case '\n':
			case '\r':
				buff[currI] = '\0';
				
				return true;
				
				break;
			default:
				if(currI >= buildMaxLen){
					break;
				}
				
				if(isalnum(newChar) || newChar == ' ' || newChar == '_'){
					buff[currI] = newChar;
					++currI;
					
					demarkateEnd();
				}
				
				break;
		}
		
		return false;
	}
	
	const char *buffer(){
		return buff;
	}
}