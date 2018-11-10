#include "work_mksofs.h"

#include "rawdisk.h"
#include "core.h"
#include "bin_mksofs.h"
#include <string.h>
#include <inttypes.h>

namespace sofs18
{
    namespace work
    {

        /*
         filling in the contents of the root directory:
         the first 2 entries are filled in with "." and ".." references
         the other entries are empty.
         If rdsize is 2, a second block exists and should be filled as well.
         */
        uint32_t fillInRootDir(uint32_t first_block, uint32_t rdsize)
        {
            soProbe(606, "%s(%u, %u)\n", __FUNCTION__, first_block, rdsize);
            
            /* change the following line by your code */
            //return bin::fillInRootDir(first_block, rdsize);
	    
			SODirEntry dir[DirentriesPerBlock]; //DirEntry buffer
			memset(dir,0,BlockSize); //Buffer com 0s

			for(uint32_t i=2; i<DirentriesPerBlock; i++) {
				dir[i].in = NullReference;
			}

			memcpy(dir[0].name, ".", 2);
			memcpy(dir[1].name, "..", 2);

			soWriteRawBlock(first_block,dir);

			if(rdsize == 2) {
				SODirEntry dir2[DirentriesPerBlock]; //DirEntry buffer
				memset(dir2,0,BlockSize); //Buffer com 0s
				for(uint32_t i=0; i<DirentriesPerBlock; i++){
					dir2[i].in = NullReference;
				}
				soWriteRawBlock(first_block + 1,dir2);
			}


	    	return rdsize;
        }

    };

};

