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

            // solution by Luis Moura, student 83808 DETI - UA

            SOSuperBlock *sb = soSBGetPointer();

            if (sb->ircache.idx != INODE_REFERENCE_CACHE_SIZE) {
            	return;
            }

            if (sb->filt_tail == sb->filt_head) {

            	SOInodeReferenceCache insertionCache = sb->iicache;
            	uint32_t insertionIDX = insertionCache.idx;

            	uint32_t destStart = INODE_REFERENCE_CACHE_SIZE - insertionIDX;
            	memcpy(&((sb->ircache).ref[destStart]), sb->iicache.ref, insertionIDX * sizeof(uint32_t));
            	memset(sb->iicache.ref, 0xFF, insertionIDX * sizeof(uint32_t));
            	(sb->ircache).idx = destStart;
            	(sb->iicache).idx = 0;
            }
            else {

				uint32_t headBlock = sb->filt_head / ReferencesPerBlock;
				uint32_t refHead = sb->filt_head % ReferencesPerBlock;
				uint32_t tailBlock = sb->filt_tail / ReferencesPerBlock;
				uint32_t refTail = sb->filt_tail % ReferencesPerBlock;
				uint32_t lastRef = (headBlock == tailBlock && refTail > refHead) ? refTail : ReferencesPerBlock;
				uint32_t refsAvailable = lastRef - refHead;
				uint32_t *blockPointer = soFILTOpenBlock(headBlock);

				if (refsAvailable >= INODE_REFERENCE_CACHE_SIZE) {
					refsAvailable = INODE_REFERENCE_CACHE_SIZE;
				}

				// copy chunk the size of remaining references in block
				uint32_t destStart = INODE_REFERENCE_CACHE_SIZE - refsAvailable;
				memcpy(&((sb->ircache).ref[destStart]), &blockPointer[refHead], refsAvailable * sizeof(uint32_t));
				memset(&blockPointer[refHead], 0xFF, refsAvailable * sizeof(uint32_t));

				// update idx
				sb->ircache.idx = destStart;

				// update filt head
				sb->filt_head = (sb->filt_head + refsAvailable) % (sb->filt_size * ReferencesPerBlock);
				if (sb->filt_head == sb->filt_tail) {
					sb->filt_head = 0;
					sb->filt_tail = 0;
				}

				soFILTSaveBlock();
				soFILTCloseBlock();
            }

            soSBSave();

        }

    };

};

