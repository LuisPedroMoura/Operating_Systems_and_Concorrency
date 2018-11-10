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

            if(sb->brcache.idx != BLOCK_REFERENCE_CACHE_SIZE ){
            	return;
            }

            //get references from insertion cache
            if(sb->fblt_head == sb->fblt_tail){
            	
            	uint32_t insertionIDX = sb->bicache.idx;

//            	//insertion cache is empty
//            	if(insertionIDX==0){
//            		// all the disk is ocuppied
//            		throw SOException(ENOSPC,__FUNCTION__);
//            	}

            	uint32_t destStart = BLOCK_REFERENCE_CACHE_SIZE - insertionIDX;
            	memcpy(&(sb->brcache.ref[destStart]), sb->bicache.ref, insertionIDX*sizeof(uint32_t));
            	memset(&(sb->bicache.ref), 0xFF, insertionIDX * sizeof(uint32_t));
            	(sb->brcache).idx = destStart;
            	(sb->bicache).idx = 0;

            }
            else {

				uint32_t headBlock = sb->fblt_head / ReferencesPerBlock;
				uint32_t refHead = sb->fblt_head % ReferencesPerBlock;
				uint32_t tailBlock = sb->fblt_tail / ReferencesPerBlock;
				uint32_t refTail = sb->fblt_tail % ReferencesPerBlock;
				uint32_t lastRef = (headBlock == tailBlock && refTail > refHead) ? refTail : ReferencesPerBlock;
				uint32_t refsAvailable = lastRef - refHead;
				uint32_t *blockPointer = soFBLTOpenBlock(headBlock);

				if (refsAvailable >= BLOCK_REFERENCE_CACHE_SIZE) {
					refsAvailable = BLOCK_REFERENCE_CACHE_SIZE;
				}

				// copy chunk the size of remaining references in block
				uint32_t destStart = BLOCK_REFERENCE_CACHE_SIZE - refsAvailable;
				memcpy(&((sb->brcache).ref[destStart]), &blockPointer[refHead], refsAvailable * sizeof(uint32_t));
				memset(&blockPointer[refHead], 0xFF, refsAvailable * sizeof(uint32_t));

				// update idx
				sb->brcache.idx = destStart;

				// update fblt head
				sb->fblt_head = (sb->fblt_head + refsAvailable) % (sb->fblt_size * ReferencesPerBlock);
				if (sb->fblt_head == sb->fblt_tail) {
					sb->fblt_head = 0;
					sb->fblt_tail = 0;
				}

				soFBLTSaveBlock();
				soFBLTCloseBlock();
			}

			soSBSave();

        }

    };

};

