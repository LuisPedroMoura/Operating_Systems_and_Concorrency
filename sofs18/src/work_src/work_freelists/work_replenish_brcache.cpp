/*
 *  \author António Rui Borges - 2012-2015
 *  \authur Artur Pereira - 2016-2018
 */

#include "work_freelists.h"

#include <string.h>
#include <errno.h>
#include <iostream>

#include "core.h"
#include "dal.h"
#include "freelists.h"
#include "bin_freelists.h"

namespace sofs18
{
    namespace work
    {

        using namespace std;

        void soReplenishBRCache(void)
        {
            soProbe(443, "%s()\n", __FUNCTION__);

            // solution by Maria João Lavoura, student 84681 DETI - UA

            SOSuperBlock *sb = soSBGetPointer();

            //SOBlockReferenceCache retrievalCache = sb->brcache;

            if(sb->brcache.idx != BLOCK_REFERENCE_CACHE_SIZE ){
            	return;
            }

            //get references from insertion cache
            if(sb->fblt_head == sb->fblt_tail){
            	//SOBlockReferenceCache insertionCache = sb->bicache;
            	uint32_t insertionIDX = sb->bicache.idx;

            	//insertion cache is empty
            	if(insertionIDX==0){
            		// all the disk is ocuppied
            		throw SOException(ENOSPC,__FUNCTION__);
            	}

            	uint32_t destStart = BLOCK_REFERENCE_CACHE_SIZE - insertionIDX;
            	memcpy(&(sb->brcache.ref[destStart]), sb->bicache.ref, insertionIDX*sizeof(uint32_t));
            	memset(&(sb->bicache.ref), 0xFF, insertionIDX * sizeof(uint32_t));


            }
            //get references from fblt
            else{

				uint32_t block = sb->fblt_head / ReferencesPerBlock;
				uint32_t ref = sb->fblt_head % ReferencesPerBlock;

				uint32_t *blockPointer = soFBLTOpenBlock(block);

				uint32_t nRefAvailable = ReferencesPerBlock - ref;
				if((sb->fblt_tail - sb->fblt_head) < nRefAvailable)
					nRefAvailable = sb->fblt_tail - sb->fblt_head;


				if(nRefAvailable >= BLOCK_REFERENCE_CACHE_SIZE){

					memcpy(&(sb->brcache), &blockPointer[ref], BLOCK_REFERENCE_CACHE_SIZE*sizeof(uint32_t));
					memset(&(blockPointer[ref]), 0xFF, BLOCK_REFERENCE_CACHE_SIZE * sizeof(uint32_t));

					//update idx
					sb->brcache.idx = 0;

					//update fblt head
					sb->fblt_head += BLOCK_REFERENCE_CACHE_SIZE;

				}
				else{

					uint32_t destStart = BLOCK_REFERENCE_CACHE_SIZE - nRefAvailable;
					memcpy(&(sb->brcache).ref[destStart], &blockPointer[ref], nRefAvailable*sizeof(uint32_t));
					memset(&(blockPointer[ref]), 0xFF, nRefAvailable * sizeof(uint32_t));

					//update idx
					sb->brcache.idx = destStart;
					//update fblt head
					sb->fblt_head += nRefAvailable;

				}
            }

            soFBLTSaveBlock();
            soFBLTCloseBlock();
            soSBSave();

            /* change the following line by your code */
            //bin::soReplenishBRCache();
        }

    };

};

