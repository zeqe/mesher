#ifndef FILE_IO_INCLUDED
	#include <cstdint>
	#include <cstdio>
	
	namespace fIO{
		namespace u8{
			bool read(uint8_t *val,FILE *in);
			bool write(uint8_t val,FILE *out);
		}
		
		namespace u16{
			bool read(uint16_t *val,FILE *in);
			bool write(uint16_t val,FILE *out);
		}
		
		namespace s16{
			bool read(int16_t *val,FILE *in);
			bool write(int16_t val,FILE *out);
		}
	}
	
	#define FILE_IO_INCLUDED
#endif