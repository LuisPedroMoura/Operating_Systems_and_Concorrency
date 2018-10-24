#include "direntries.h"

#include "core.h"
#include "dal.h"
#include "fileblocks.h"
#include "bin_direntries.h"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>

namespace sofs18
{
    namespace work
    {

        void soAddDirEntry(int pih, const char *name, uint32_t cin)
        {
            soProbe(202, "%s(%d, %s, %u)\n", __FUNCTION__, pih, name, cin);

            /* change the following line by your code */
            //bin::soAddDirEntry(pih, name, cin);

            SOInode* pi = soITGetInodePointer(pih);
            uint32_t emptySlot = NullReference;
            uint32_t emptySlotBlock = NullReference;
            bool foundEmptySlot = false;
            bool exists = false;
            SODirEntry d[DirentriesPerBlock];

            uint32_t i = 0;
            for (; i < (pi->size / BlockSize); i++ ) {

            	sofs18::soReadFileBlock(pih, i, d);

				uint32_t j = 0;
				for (; j < DirentriesPerBlock; j++) {

				   if (!foundEmptySlot && d[j] == NullReference) {
					   emptySlotBlock = i;
					   emptySlot = j + i * DirentriesPerBlock;
					   foundEmptySlot = true;
				   }

				   if (strcmp(d[j].name, name)) {
					   exists = true;
					   break;
				   }
				}

				if (exists) {
				   break;
				}
            }


            if (exists) {
            	throw SOException(EEXIST,__FUNCTION__);
            }

			if (foundEmptySlot) {

				sofs18::soReadFileBlock(pih, emptySlotBlock, d);
				d[emptySlot].name = name;
				d[emptySlot].in = cin;
				sofs18::soWriteFileBlock(pih, emptySlotBlock, d);
			}

			else {

				uint32_t bn = sofs18::soAllocFileBlock(pih, i+1);

				SODirEntry block[DirentriesPerBlock];
				memset(block,0,BlockSize);
				for(uint32_t i=1; i < DirentriesPerBlock; i++){
					block[i].in = NullReference;
				}
				block[0].name = name;
				block[0].in = cin;

				sofs18::soWriteFileBlock(pih, i+1, d);

			}

        }

    };

};

