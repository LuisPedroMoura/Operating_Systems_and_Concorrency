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

        uint32_t fillInFreeInodeListTable(uint32_t first_block, uint32_t itotal)
        {
            soProbe(603, "%s(%u, %u)\n", __FUNCTION__, first_block, itotal);
            
            /* change the following line by your code */
            //return bin::fillInFreeInodeListTable(first_block, itotal);

			//Number of blocks needed for all the inodes
			uint32_t inodeBlocks = (itotal-1) / ReferencesPerBlock;

			//Array that stores the inodes to write
			uint32_t inodeRL[ReferencesPerBlock];

			uint32_t count = 1;
			for (uint32_t i = 0; i < inodeBlocks; i++) {

				for (uint32_t j = 0; j < ReferencesPerBlock; j++) {

					inodeRL[j] = count++;
				}
				soWriteRawBlock(first_block + i,&inodeRL);
			}

			//Since inodeBlocks is an integer number, we need to account for the mod (division remainder)
			if((itotal-1) % InodesPerBlock != 0){

				for(uint32_t j = 0; j < ReferencesPerBlock; j++) {
					if ((itotal-1) >= count) {
						inodeRL[j] = count++;
					}
					else {
						inodeRL[j] = NullReference;
					}
				}
				soWriteRawBlock(first_block + inodeBlocks,&inodeRL);
				inodeBlocks++;
			}

			return inodeBlocks;
        }
    };
};

