#include "work_mksofs.h"

#include "rawdisk.h"
#include "core.h"
#include "bin_mksofs.h"

#include <inttypes.h>
#include <string.h>



namespace sofs18
{
    namespace work
    {

        uint32_t fillInFreeBlockListTable(uint32_t first_block, uint32_t btotal, uint32_t rdsize)
        {
            soProbe(605, "%s(%u, %u, %u)\n", __FUNCTION__, first_block, btotal, rdsize);
            
            /* change the following line by your code */

		
	    uint32_t blocktab [ReferencesPerBlock];
	    unsigned int blocknumb;
	    unsigned int count = 1;
	    unsigned int refnum = btotal-rdsize;

	    blocknumb = refnum/128;
	    if( refnum%128 != 0 ) blocknumb = blocknumb+1;

            for(unsigned int i=0 ; i<blocknumb ; i++){
            	
		for(unsigned int k=0 ; k<128 ; k++){
			
			if( refnum >= count ) blocktab[k] = count++;	
			else blocktab [k] = NullReference;

		}
		
		soWriteRawBlock(first_block,&blocktab);
            	
		first_block++;
            }

	return blocknumb;

        }

    };

};

