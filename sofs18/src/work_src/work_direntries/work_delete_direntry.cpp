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

        uint32_t soDeleteDirEntry(int pih, const char *name)
        {
            soProbe(203, "%s(%d, %s)\n", __FUNCTION__, pih, name);

            // code developed by Fernando Marques 80238

			SOInode *inode = soITGetInodePointer(pih);

			SODirEntry ref[DirentriesPerBlock];

		    if (strcmp(name, "") == 0)
		    {
		        throw SOException(EINVAL, __FUNCTION__);
		    }

		    if (strlen(name) > SOFS18_MAX_NAME)
		    {
		        throw SOException(ENAMETOOLONG, __FUNCTION__);
		    }

			uint32_t tmp;

			for(uint32_t i = 0 ; i < inode->size/BlockSize ; i++)
			{
				sofs18::soReadFileBlock(pih,i,ref);
				for(uint32_t j = 0 ; j < DirentriesPerBlock ; j++)
				{
					tmp = ref[j].in;
					if(strcmp(ref[j].name,name) == 0)
					{
						// if name then free state, name '\0' and in NullReference
						memset(ref[j].name, '\0', SOFS18_MAX_NAME+1);
						ref[j].in = NullReference;
						sofs18::soWriteFileBlock(pih,i,ref);
						return tmp;
					}
				}
			}
			// if do not exist corresponding exception is launched
			throw SOException(ENOENT, __FUNCTION__);

			/* change the following line by your code */
			//return bin::soDeleteDirEntry(pih, name);
		}
    }
};

