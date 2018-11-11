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

			// solution by Luis Moura, student 83808 DETI - UA

            if (!strcmp(name, "")) {
            	throw SOException(EINVAL, __FUNCTION__);
            }

            if (strlen(name) > SOFS18_MAX_NAME) {
				throw SOException(ENAMETOOLONG, __FUNCTION__);
			}

            SOInode* pi = soITGetInodePointer(pih);
            if(!S_ISDIR(pi->mode)){
				throw SOException(ENOTDIR,__FUNCTION__);
			}

            int emptySlot = -1;
            int emptySlotBlockIndex = -1;
            SODirEntry emptySlotBlock[DirentriesPerBlock];


            SODirEntry d[DirentriesPerBlock];
            uint32_t i = 0;
            for (; i < (pi->size / BlockSize); i++ ) {

            	sofs18::soReadFileBlock(pih, i, d);

				uint32_t j = 0;
				for (; j < DirentriesPerBlock; j++) {
					if (emptySlot < 0 && d[j].name[0] == '\0') {
						sofs18::soReadFileBlock(pih, i, emptySlotBlock);
						emptySlotBlockIndex = i;
						emptySlot = j;
					}

					if (!strcmp(d[j].name, name)) {
						throw SOException(EEXIST,__FUNCTION__);
					}
				}
            }

			if (emptySlot >= 0) {

				memcpy(emptySlotBlock[emptySlot].name, name, SOFS18_MAX_NAME+1);
				memcpy(&emptySlotBlock[emptySlot].in, &cin, sizeof(uint32_t));
				sofs18::soWriteFileBlock(pih, emptySlotBlockIndex, emptySlotBlock);
			}			
			else {

				if (sofs18::soGetFileBlock(pih, i) == NullReference){
					sofs18::soAllocFileBlock(pih, i);
				}

				SODirEntry dir[DirentriesPerBlock];
				memset(dir,0,BlockSize);
				for(uint32_t i = 0; i < DirentriesPerBlock; i++){
					dir[i].in = NullReference;
				}
				memcpy(dir[0].name, name, SOFS18_MAX_NAME+1);
				dir[0].in = cin;
				pi->size += BlockSize;

				sofs18::soWriteFileBlock(pih, i, dir);
			}

        }

    };

};

