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
#include <string.h>
#include <iostream>
using namespace std;

namespace sofs18
{
    namespace work
    {

        /* only fill the current block to its end */
        void soDepleteBICache(void)
        {
            soProbe(444, "%s()\n", __FUNCTION__);

            /* change the following line by your code */
            //bin::soDepleteBICache();

			SOSuperBlock *sb = soSBGetPointer();

			uint32_t block = sb->fblt_tail / ReferencesPerBlock ;
			uint32_t block_used_refs = sb-> fblt_tail % ReferencesPerBlock;
			uint32_t *block_pointer = soFBLTOpenBlock(block);
			uint32_t block_free_refs;
		
			if(block == sb->fblt_head / ReferencesPerBlock ){
				if(sb->fblt_head % ReferencesPerBlock >  sb->fblt_tail % ReferencesPerBlock){
					block_free_refs = (sb->fblt_head % ReferencesPerBlock) - (sb->fblt_tail % ReferencesPerBlock);
				}
				else{
					block_free_refs = ReferencesPerBlock - block_used_refs ;
				} 
			}
			else{
				block_free_refs = ReferencesPerBlock - block_used_refs;
			}

			if(sb->bicache.idx > block_free_refs){
				memcpy(&(block_pointer[block_used_refs]),&(sb->bicache),block_free_refs * sizeof(uint32_t));

				if(sb->fblt_size - 1 > block){
					sb->fblt_tail += block_free_refs;
				}
				else{
					sb->fblt_tail = 0;
				}

				memcpy(&(sb->bicache),&(sb->bicache.ref[block_free_refs]),((sb->bicache.idx)-block_free_refs)*sizeof(uint32_t));
				sb->bicache.idx -= block_free_refs ;

				for(uint32_t i= 0 ; i < block_free_refs ; i++ ){
					sb->bicache.ref[(sb->bicache.idx)+i] = NullReference;
				}
			}
			else{
				memcpy(&(block_pointer[block_used_refs]),&(sb->bicache),(sb->bicache.idx)*sizeof(uint32_t));
				sb->fblt_tail += sb->bicache.idx;	
				sb->bicache.idx = 0;

				for( uint32_t i=0 ; i < BLOCK_REFERENCE_CACHE_SIZE ; i++ ){
					sb->bicache.ref[i] = NullReference;
				}
			}

			soFBLTSaveBlock();
			soFBLTCloseBlock();
			soSBSave();
        }

    };

};

