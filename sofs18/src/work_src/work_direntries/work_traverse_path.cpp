#include "direntries.h"

#include "core.h"
#include "dal.h"
#include "fileblocks.h"
#include "direntries.h"
#include "bin_direntries.h"

#include <errno.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

namespace sofs18
{
    namespace work
    {

        uint32_t soTraversePath(char *path)
        {
            soProbe(221, "%s(%s)\n", __FUNCTION__, path);

            /* change the following line by your code */
            //return bin::soTraversePath(path);

            // solution by Maria João, student 84681 DETI - UA

            printf("path %s\n",path);

			//stop condition
            if (strcmp(path, "/")==0) {
            	return 0;
            }

            char * baseName = basename(strdupa(path));
            printf("baseName = %s\n", baseName);
            char * dirName = dirname(strdupa(path));
            printf("dirName = %s\n", dirName);


			uint32_t in = soGetDirEntry(soTraversePath(dirName), baseName);

            //verify if is dir or symlink
			SOInode * ip = soITGetInodePointer(in);
			if (!((ip->mode & S_IFDIR) == S_IFDIR) || !((ip->mode & S_IFLNK) == S_IFLNK)){
				throw SOException(ENOENT , __FUNCTION__);
			}

            //verify permissions of execution
			bool permission = sofs18::soCheckInodeAccess(in,X_OK);
			if (!permission) {
				throw SOException(EACCES , __FUNCTION__);
			}
			return in;

			printf("---------------------------------------------------------------");

        }

    };

};

