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

        void soReplenishIRCache(void)
        {
            soProbe(403, "%s()\n", __FUNCTION__);

            /* change the following line by your code */
            //bin::soReplenishIRCache();

            SOSuperBlock *sb = soSBGetPointer();

            SOInodeReferenceCache cache = sb->ircache;

            if (cache.idx != INODE_REFERENCE_CACHE_SIZE) {
            	return;
            }

            if (sb->filt_tail == sb->filt_head) {

            	SOInodeReferenceCache insertionCache = sb->iicache;
            	uint32_t insertionIDX = insertionCache.idx;

            	if (insertionIDX == 0){
            		// all the disk is ocuppied
            		// TODO verificar com prof o que fazer nesta situação
            		return;
            	}

            	uint32_t destStart = INODE_REFERENCE_CACHE_SIZE - insertionIDX;
            	memcpy(&(cache.ref[destStart]) , insertionCache.ref, insertionIDX);
            	return;
            }

            else {
				uint32_t block = sb->filt_head / ReferencesPerBlock;
				uint32_t ref = sb->filt_head % ReferencesPerBlock;


				uint32_t *blockPointer = soFILTOpenBlock(block);
				uint32_t refsAvailable = ReferencesPerBlock - ref;
				uint32_t *source = &blockPointer[ref];

				if ((refsAvailable) >= INODE_REFERENCE_CACHE_SIZE) {

					// copy cache size chunk from filt to ircache
					memcpy(&cache, source, INODE_REFERENCE_CACHE_SIZE);

					// update idx
					cache.idx = 0;

					// update filt head
					sb->filt_head += INODE_REFERENCE_CACHE_SIZE;
				}

				else {

					// copy chunk the size of remaining references in block
					uint32_t destStart = INODE_REFERENCE_CACHE_SIZE - refsAvailable;
					memcpy(&cache.ref[destStart], source, refsAvailable);

					// update idx
					cache.idx = destStart;

					// update filt head
					sb->filt_head += refsAvailable;
				}
            }

            soFILTSaveBlock();
            soFILTCloseBlock();
            soSBSave();

        }

    };

};

