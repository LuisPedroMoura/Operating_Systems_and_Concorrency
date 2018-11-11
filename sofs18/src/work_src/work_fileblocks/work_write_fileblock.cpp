#include "work_fileblocks.h"

#include "dal.h"
#include "core.h"
#include "fileblocks.h"
#include "bin_fileblocks.h"

#include <string.h>
#include <inttypes.h>

namespace sofs18
{
    namespace work
    {

        void soWriteFileBlock(int ih, uint32_t fbn, void *buf)
        {
            soProbe(332, "%s(%d, %u, %p)\n", __FUNCTION__, ih, fbn, buf);

            /* change the following line by your code */
            //bin::soWriteFileBlock(ih, fbn, buf);

            // code developed by Fernando Marques 80238

            // get the block number of the file
            uint32_t nBlock = sofs18::soGetFileBlock(ih, fbn);
            
            // if the block is not allocated, alloc data
            if(nBlock == NullReference)
            {
            	nBlock = sofs18::soAllocFileBlock(ih, fbn);
            }
			soWriteDataBlock(nBlock, buf);
        }

    };

};

