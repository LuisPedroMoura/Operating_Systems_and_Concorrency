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
	    
	    char point[SOFS18_MAX_NAME + 1] = ".";
	    char point2[SOFS18_MAX_NAME + 1] = "..";
	    SODirEntry root;
	    root.in = 0;
	    memcpy(root.name, point, SOFS18_MAX_NAME + 1);
	    SODirEntry root2;
	    root2.in = 0;
	    memcpy(root2.name, point2, SOFS18_MAX_NAME + 1);
	    SODirEntry p[BlockSize] = {root,root2};
	    soWriteRawBlock(first_block,p);
	    if(rdsize == 2)
		soWriteRawBlock(first_block + 1,p); 
	   
	    return rdsize;
        }

    };

};

