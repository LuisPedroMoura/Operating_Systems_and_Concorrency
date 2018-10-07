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
	    uint32_t inodeBlocks = itotal / InodesPerBlock;

	    //Since inodeBlocks is an integer number, we need to account for the mod (division remainder)
	    if(itotal % InodesPerBlock != 0) inodeBlocks++;

	    //Variable to assist in counting the first free block for inode storage
	    uint32_t blockCount = first_block;
	
	    //Array that stores the inodes to write
	    uint32_t inodeRL[InodesPerBlock];

	    for(uint32_t i=0; i<inodeBlocks; i++) {
  	    	for(uint32_t j=0; j<InodesPerBlock; j++) {
		    inodeRL[j] = NullReference;
		}
		soWriteRawBlock(blockCount,&inodeRL);	
		blockCount++;
	    }

	    return inodeBlocks;
        }
    };
};

