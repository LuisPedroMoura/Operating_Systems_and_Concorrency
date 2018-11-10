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

			//stop condition
            if(strcmp(path, "/") == 0){
            	return 0;
            }

            char * baseName = basename(strdupa(path));
            char * dirName = dirname(strdupa(path));

            uint32_t inp = soTraversePath(dirName);

            //verify if is dir or symlink
			uint32_t ih = soITOpenInode(inp);
			SOInode * ip = soITGetInodePointer(ih);

			if((ip->mode & S_IFDIR) != S_IFDIR){
				throw SOException(ENOENT , __FUNCTION__);
			}

            //verify permissions of execution
			if(!sofs18::soCheckInodeAccess(ih, X_OK)) {
				throw SOException(EACCES , __FUNCTION__);
			}

			uint32_t in = soGetDirEntry(ih, baseName);

			if((int)in < 0){
				throw SOException(ENOENT , __FUNCTION__);
			}

            // close open inode
            soITCloseInode(ih);

			return in;
        }

    };

};

