/*
 *  \author António Rui Borges - 2012-2015
 *  \authur Artur Pereira - 2016-2018
 */

#include "work_freelists.h"

#include "core.h"
#include "dal.h"
#include "freelists.h"
#include "bin_freelists.h"

#include <stdio.h>
#include <errno.h>
#include <iostream>
#include <string.h>

using namespace std;

namespace sofs18
{
    namespace work
    {

        /*
         */
        void soDepleteIICache(void)
        {
            soProbe(404, "%s()\n", __FUNCTION__);

            /* change the following line by your code */
            
	    SOSuperBlock *sb = soSBGetPointer();	
	    SOInodeReferenceCache InsertionCache = sb -> iicache;
	
	    uint32_t block = sb-> filt_tail / ReferencesPerBlock ;
	    uint32_t block_used_refs = sb-> filt_tail % ReferencesPerBlock;
	    uint32_t block_free_refs = ReferencesPerBlock - block_used_refs;
	    uint32_t *block_pointer = soFILTOpenBlock(block);
	    uint32_t *ref_pointer = &block_pointer[block_used_refs];	    
	   


		if( InsertionCache.idx > block_free_refs ){
		
			memcpy(ref_pointer,&InsertionCache,block_free_refs);

                        sb->filt_tail += block_free_refs;

				

                        InsertionCache.idx -= block_free_refs ;

		}

		
		else{
		
			memcpy(ref_pointer,&InsertionCache,InsertionCache.idx);
	
        		sb->filt_tail += InsertionCache.idx;
			
			InsertionCache.idx = 0;	
		
		}

		soFILTSaveBlock();
		soFILTCloseBlock();
		soSBSave();

        }

    };

};

