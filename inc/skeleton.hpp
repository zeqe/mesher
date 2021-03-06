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
		
		sf::Vector2<int32_t> getBonePosition(unsigned char bone);
		sf::Vector2<int32_t> getPointPosition(unsigned char bone,int16_t x,int16_t y);
		
		void update();
		void upload();
		void draw();
	}
	
	#define SKELETON_INCLUDED
#endif