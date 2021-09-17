#include "fileIO.hpp"

namespace fIO{
	namespace u8{
		bool read(uint8_t *val,FILE *in){
			int byte = fgetc(in);
			
			if(byte != EOF){
				*val = (unsigned char)byte;
			}
			
			return byte != EOF;
		}
		
		bool write(uint8_t val,FILE *out){
			int status = fputc(val,out);
			
			return status != EOF;
		}
	}
	
	namespace u16{
		bool read(uint16_t *val,FILE *in){
			uint8_t a,b;
			
			bool aS = u8::read(&a,in);
			bool bS = u8::read(&b,in);
			
			if(aS && bS){
				*val = ((uint16_t)a << 8) | (uint16_t)b;
			}
			
			return aS && bS;
		}
		
		bool write(uint16_t val,FILE *out){
			return u8::write(val >> 8,out) && u8::write(val,out);
		}
	}
	
	namespace s16{
		bool read(int16_t *val,FILE *in){
			uint16_t a;
			bool aS = u16::read(&a,in);
			
			if(aS){
				*val = (int32_t)a + (int32_t)INT16_MIN;
			}
			
			return aS;
		}
		
		bool write(int16_t val,FILE *out){
			return u16::write((int32_t)val - (int32_t)INT16_MIN,out);
		}
	}
}