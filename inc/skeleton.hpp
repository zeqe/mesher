#ifndef SKELETON_INCLUDED
	#include <cstdint>
	
	#include <SFML/Graphics.hpp>
	
	#include "transformOp.hpp"
	
	namespace bones{
		void init();
		
		void setOrigin(unsigned char i,int16_t x,int16_t y);
		void setParent(unsigned char i,unsigned char parent);
		
		int16_t getX(unsigned char i);
		int16_t getY(unsigned char i);
		unsigned char getParent(unsigned char i);
		
		void draw();
	}
	
	namespace pose{
		void reset();
		
		void setModifiers(enum transformOp (*type)(),bool (*active)(),int32_t (*valX)(),int32_t (*valY)(),float (*valScalar)());
		void updateModifiers(bool apply,unsigned char currPose);
		void clearUnappliedModifiers();
		
		void calculateGlobals();
		sf::Vector2<int16_t> getBonePosition(unsigned char bone);
		
		void draw();
	}
	
	#define SKELETON_INCLUDED
#endif