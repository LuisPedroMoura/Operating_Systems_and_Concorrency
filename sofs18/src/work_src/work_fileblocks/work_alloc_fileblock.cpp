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
	    if(fbn > N_DIRECT - 1) {
		//if index beyond i1[0]
		if(fbn > ReferencesPerBlock + N_DIRECT - 1) {
		    //if index beyond i1[1]
		    if(fbn > (2 * ReferencesPerBlock) + N_DIRECT - 1) {
			//if index beyond i2[0]
			if(fbn > (ReferencesPerBlock * ReferencesPerBlock) + (2 * ReferencesPerBlock) + N_DIRECT - 1) {			    
			    soAllocDoubleIndirectFileBlock( ,fbn);
			}
		    }
		    else {
			//add to i1[1]
			soAllocIndirectFileBlock( , fbn);
		    }
		}
		else {
 		    //add to i1[0]
		    soAllocIndirectFileBlock( , fbn);
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
            //throw SOException(ENOSYS, __FUNCTION__); 
            //return 0;

	    //if afbn beyond the total of possible spaces for d[] and i1[0], then it is within i1[1]
    	    if(afbn > ReferencesPerBlock + N_DIRECT - 1) {
 		//add to i1[1]

		//Discover index within i1[1]
		afbn = afbn - ReferencesPerBlock - N_DIRECT;

		

	    }
	    //else, afbn is beyond the spaces for d[] yet not beyond the max of d+i1[0], hence being within i1[0]
	    else {
 		//add to i1[0]

		//Discover index within i1[0]
	   	afbn = afbn - N_DIRECT;
	    }	    

	    return ... ;
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
            //throw SOException(ENOSYS, __FUNCTION__); 
            //return 0;

	    uint32_t afbn_quoc;
	    uint32_t afbn_rem;

	    //if afbn is beyond the total of possible spaces for d[], i1[0], i1[1] and i2[0], then it is within i2[1]
	    if(afbn > (ReferencesPerBlock * ReferencesPerBlock) + (2 * ReferencesPerBlock) + N_DIRECT - 1) {
		//add to i2[1]

		//Discover index within i2[1]
		afbn = afbn - (ReferencesPerBlock * ReferencesPerBlock) - (2 * ReferencesPerBlock) - N_DIRECT;

		//Knowing the index, find out which sub-array of i2[1] is belongs to, and the corresponding index within it
		afbn_quoc = afbn / ReferencesPerBlock;
	        afbn_rem = afbm % ReferencesPerBlock;

		
	    }	    
	    //else, afbn is beyond the spaces for d[], i1[0] and i1[1], but not beyond the max of d+i1[0]+i1[1]+i2[0], hence being within i2[0]
	    else {
		//add to i2[0]

		//Discover index within i2[0]
		afbn = afbn - (2 * ReferencesPerBlock) - N_DIRECT;

		//Knowing the index, find out which sub-array of i2[0] is belongs to, and the corresponding index within it
		afbn_quoc = afbn / ReferencesPerBlock;
	        afbn_rem = afbm % ReferencesPerBlock;
	    }

	    

	    		
	
	    return ... ;
        }
#endif

    };

};

