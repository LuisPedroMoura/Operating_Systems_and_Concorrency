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

            SOSuperBlock *sb = soSBGetPointer();

            SOBlockReferenceCache retrievalCache = sb->brcache;

            if(retrievalCache.idx != BLOCK_REFERENCE_CACHE_SIZE ){
            	return;
            }

            //get references from insertion cache
            if(sb->fblt_head == sb->fblt_tail){
            	SOBlockReferenceCache insertionCache = sb->bicache;
            	uint32_t insertionIDX = insertionCache.idx;

            	//insertion cache is empty
            	if(insertionIDX==0){
            		return ;
            	}

            	uint32_t destStart = BLOCK_REFERENCE_CACHE_SIZE - insertionIDX;
            	memcpy(&(retrievalCache.ref[destStart]), insertionCache.ref, insertionIDX);

            }
            else{
				//get references from fblt
				uint32_t block = sb->fblt_head / ReferencesPerBlock;
				uint32_t ref = sb->fblt_head % ReferencesPerBlock;

				uint32_t *blockPointer = soFBLTOpenBlock(block);
				uint32_t nRefAvailable = ReferencesPerBlock - ref;

				if(nRefAvailable >= BLOCK_REFERENCE_CACHE_SIZE){

					uint32_t *source = blockPointer + ref*sizeof(uint32_t);
					memcpy(&retrievalCache, source, BLOCK_REFERENCE_CACHE_SIZE);

					//update idx
					retrievalCache.idx = 0;

					//update fblt head
					sb->fblt_head += BLOCK_REFERENCE_CACHE_SIZE;

				}
				else{

					uint32_t *source = blockPointer + ref*sizeof(uint32_t);
					uint32_t destStart = BLOCK_REFERENCE_CACHE_SIZE - nRefAvailable;
					memcpy(&retrievalCache.ref[destStart], source, nRefAvailable);

					//update idx
					retrievalCache.idx = destStart;
					//update fblt head
					sb->fblt_head += nRefAvailable;

				}
            }

            //soSBClose();
            //soSBSave();

            /* change the following line by your code */
            //bin::soReplenishBRCache();
        }

    };

};

