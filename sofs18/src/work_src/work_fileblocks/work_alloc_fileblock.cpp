#include "work_fileblocks.h"

#include "freelists.h"
#include "dal.h"
#include "core.h"
#include "bin_fileblocks.h"

#include <errno.h>

#include <iostream>

namespace sofs18
{
    namespace work
    {

#if false
        static uint32_t soAllocIndirectFileBlock(SOInode * ip, uint32_t afbn);
        static uint32_t soAllocDoubleIndirectFileBlock(SOInode * ip, uint32_t afbn);
#endif

        /* ********************************************************* */

        uint32_t soAllocFileBlock(int ih, uint32_t fbn)
        {
            soProbe(302, "%s(%d, %u)\n", __FUNCTION__, ih, fbn);

            /* change the following two lines by your code */
            //return bin::soAllocFileBlock(ih, fbn);

	    SOInode* in = soITGetInodePointer(ih);

	    //if index beyond d[]
	    if(fbn > N_DIRECT) {
		//if index beyond i1[0]
		if(fbn > ReferencesPerBlock + N_DIRECT) {
		    //if index beyond i1[1]
		    if(fbn > (2 * ReferencesPerBlock) + N_DIRECT) {
			//if index beyond i2[0]
			if(fbn > (ReferencesPerBlock * ReferencesPerBlock) + (2 * ReferencesPerBlock) + N_DIRECT) {			    
			    //add to i2[1]
			    soAllocDoubleIndirectFileBlock( , fbn - ((ReferencesPerBlock * ReferencesPerBlock) + (2 * ReferencesPerBlock) + N_DIRECT) );
			else {
 			    //add to i2[0]
			    soAllocDoubleIndirectFileBlock( , fbn - ((2 * ReferencesPerBlock) + N_DIRECT) );
			}
		    }
		    else {
			//add to i1[1]
			soAllocIndirectFileBlock( , fbn - (ReferencesPerBlock + N_DIRECT) );
		    }
		}
		else {
 		    //add to i1[0]
		    soAllocIndirectFileBlock( , fbn - N_DIRECT );
		}
	    }
	    else {
		//add to d[]
		in->d[fbn] = ;
	    }

	    //return number of allocated block
	    return ... ; 
        }

        /* ********************************************************* */

#if false
        /*
         */
        static uint32_t soAllocIndirectFileBlock(SOInode * ip, uint32_t afbn)
        {
            soProbe(302, "%s(%d, ...)\n", __FUNCTION__, afbn);

            /* change the following two lines by your code */
            throw SOException(ENOSYS, __FUNCTION__); 
            return 0;
        }
#endif

        /* ********************************************************* */

#if false
        /*
         */
        static uint32_t soAllocDoubleIndirectFileBlock(SOInode * ip, uint32_t afbn)
        {
            soProbe(302, "%s(%d, ...)\n", __FUNCTION__, afbn);

            /* change the following two lines by your code */
            throw SOException(ENOSYS, __FUNCTION__); 
            return 0;
        }
#endif

    };

};

