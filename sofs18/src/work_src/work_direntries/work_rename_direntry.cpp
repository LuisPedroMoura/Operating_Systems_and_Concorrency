#include "direntries.h"

#include "core.h"
#include "dal.h"
#include "fileblocks.h"
#include "bin_direntries.h"

#include <string.h>
#include <errno.h>
#include <sys/stat.h>

namespace sofs18
{
    namespace work
    {

        void soRenameDirEntry(int pih, const char *name, const char *newName)
        {
            soProbe(204, "%s(%d, %s, %s)\n", __FUNCTION__, pih, name, newName);

            /* change the following line by your code */
            //bin::soRenameDirEntry(pih, name, newName);
	    
			SOInode* in = soITGetInodePointer(pih);

			if (!strcmp(newName, "")) {
				throw SOException(EINVAL, __FUNCTION__);
			}

			if (strlen(newName) > SOFS18_MAX_NAME) {
				throw SOException(ENAMETOOLONG, __FUNCTION__);
			}

			if(!S_ISDIR(in->mode)){
				throw SOException(ENOTDIR,__FUNCTION__);
			}

			SOInode* pi = soITGetInodePointer(pih);
			int renameSlot = -1;
			int renameSlotBlockIndex = -1;
			SODirEntry renameSlotBlock[DirentriesPerBlock];

			SODirEntry d[DirentriesPerBlock];
			uint32_t i = 0;
			for (; i < (pi->size / BlockSize); i++) {

				sofs18::soReadFileBlock(pih, i, d);

				uint32_t j = 0;
				for (; j < DirentriesPerBlock; j++) {
					if (renameSlot < 0 && strcmp(d[j].name,name) == 0) {
						sofs18::soReadFileBlock(pih, i, renameSlotBlock);
						renameSlotBlockIndex = i;
						renameSlot = j;
					}

					if (strcmp(d[j].name, newName) == 0) {
						throw SOException(EEXIST,__FUNCTION__);
					}
				}
			}

			if (renameSlot >= 0) {

				memcpy(renameSlotBlock[renameSlot].name, name, SOFS18_MAX_NAME+1);
				sofs18::soWriteFileBlock(pih, renameSlotBlockIndex, renameSlotBlock);
			}

			if(renameSlot < 0){
				throw SOException(ENOENT,__FUNCTION__);
			}
		}

    };

};

