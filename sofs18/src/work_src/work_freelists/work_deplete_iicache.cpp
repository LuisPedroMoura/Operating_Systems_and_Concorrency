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

        void soDepleteIICache(void)
        {
            soProbe(404, "%s()\n", __FUNCTION__);

            /* change the following line by your code */
            
            SOSuperBlock *sb = soSBGetPointer();
	
            uint32_t block = sb->filt_tail / ReferencesPerBlock ;
            uint32_t block_used_refs = sb->filt_tail % ReferencesPerBlock;
            uint32_t *block_pointer = soFILTOpenBlock(block);
            uint32_t block_free_refs;

			if( block == sb->filt_head / ReferencesPerBlock ){

				if( (sb-> filt_head) % ReferencesPerBlock >  (sb->filt_tail) % ReferencesPerBlock){
					block_free_refs = (sb->filt_head) % ReferencesPerBlock - (sb->filt_tail) % ReferencesPerBlock;
				}
				else{
					block_free_refs = ReferencesPerBlock - block_used_refs;
				}
			
			}
			else{

				block_free_refs = ReferencesPerBlock - block_used_refs;
			}


			if( sb->iicache.idx > block_free_refs ){

				memcpy(&(block_pointer[block_used_refs]),&(sb->iicache),block_free_refs * sizeof(uint32_t));

				if(  (sb-> filt_size)-1 > block ){
					sb->filt_tail += block_free_refs;
				}
				else{
					sb->filt_tail = 0;
				}

				memcpy(&(sb->iicache),&(sb->iicache.ref[block_free_refs]),((sb->iicache.idx)-block_free_refs)*sizeof(uint32_t));

				sb->iicache.idx -= block_free_refs ;


				for(uint32_t i= 0 ; i < block_free_refs ; i++ ){

					sb->iicache.ref[(sb->iicache.idx)+i] = NullReference;
				}
			
			}
			else{
		
				memcpy(&(block_pointer[block_used_refs]),&(sb->iicache),(sb->iicache.idx)*sizeof(uint32_t));
	
        		sb->filt_tail += sb->iicache.idx;
			
        		sb->iicache.idx = 0;

        		for( uint32_t i=0 ; i < INODE_REFERENCE_CACHE_SIZE ; i++ ){

        			sb->iicache.ref[i] = NullReference;
        		}
		
			}

			soFILTSaveBlock();
			soFILTCloseBlock();
			soSBSave();

        }

    };

};

