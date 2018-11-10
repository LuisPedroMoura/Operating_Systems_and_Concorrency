#include "direntries.h"

#include "core.h"
#include "dal.h"
#include "fileblocks.h"
#include "bin_direntries.h"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>

namespace sofs18 {
	namespace work {

		uint32_t soGetDirEntry(int pih, const char *name) {

			soProbe(201, "%s(%d, %s)\n", __FUNCTION__, pih, name);

			SOInode* ip = sofs18::soITGetInodePointer(pih);
			SODirEntry dir[DirentriesPerBlock];

            if (strcmp(name, "") == 0) {
            	throw SOException(EINVAL, __FUNCTION__);
            }

		    if (S_ISDIR(ip->mode) == 0){
		    	throw SOException(ENOTDIR,__FUNCTION__);
		    }

			for (uint32_t i = 0; i <= ip->size / BlockSize; i++) {
				sofs18::soReadFileBlock(pih, i, dir);

				for (uint32_t j = 0; j < DirentriesPerBlock; j++) {

					if (strcmp(dir[j].name, name) == 0) {
						return dir[j].in;
					}
				}
			}

			return -1;

		}
	};
};

