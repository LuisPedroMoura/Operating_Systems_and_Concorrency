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

            uint32_t blocktab [ReferencesPerBlock];
            uint32_t blocknumb = btotal / ReferencesPerBlock;

            if( btotal % ReferencesPerBlock != 0 ){
                blocknumb = blocknumb+1;
            }

            for(uint32_t i=0 ; i<blocknumb ; i++){

                for(uint32_t k=0 ; k<ReferencesPerBlock ; k++){

					if( btotal > rdsize ){
						blocktab[k] = rdsize++;
					}
					else{
						blocktab [k] = NullReference;
					}
                }

                soWriteRawBlock(first_block,&blocktab);
                first_block++;
            }

            return blocknumb;
        }


    };

};

