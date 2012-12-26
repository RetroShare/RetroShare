#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <util/rsid.h>

class TestUtils
{
	public:
		// Creates a random file of the given size at the given place. Useful for file transfer tests.
		//
		static bool createRandomFile(const std::string& filename,const uint64_t size)
		{
			FILE *f = fopen(filename.c_str(),"wb") ;

			if(f == NULL)
				return 0 ;

			uint32_t S = 5000 ;
			uint32_t *data = new uint32_t[S] ;

			for(uint64_t i=0;i<size;i+=4*S)
			{
				for(uint32_t j=0;j<S;++j)
					data[j] = lrand48() ;

				uint32_t to_write = std::min((uint64_t)(4*S),size - i) ;

				if(to_write != fwrite((void*)data,1,to_write,f))
					return 0 ;
			}

			fclose(f) ;

			return true ;
		}

		static std::string createRandomSSLId()
		{
			return t_RsGenericIdType<16>::random().toStdString(false);
		}
		static std::string createRandomPGPId()
		{
			return t_RsGenericIdType<8>::random().toStdString(true);
		}
};
