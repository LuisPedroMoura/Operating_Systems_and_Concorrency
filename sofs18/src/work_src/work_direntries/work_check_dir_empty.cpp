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

        bool soCheckDirEmpty(int ih)
        {
            soProbe(205, "%s(%d)\n", __FUNCTION__, ih);

            /* change the following line by your code */
            //"return bin::soCheckDirEmpty(ih);
	
			//Reference to the node being handled
			SOInode* node = soITGetInodePointer(ih);
			
			uint32_t blk = (node->size) / BlockSize;
			SODirEntry buff[DirentriesPerBlock];

			uint32_t blkCount = 0;
			uint32_t blkNum = 0;
			uint32_t tmpBlk;

			while(blkCount < blk) {
				tmpBlk = sofs18::soGetFileBlock(ih,blkNum);

				if(tmpBlk != NullReference) {
					sofs18::soReadFileBlock(ih,blkNum,buff);
				
					for(uint32_t i = 2; i < DirentriesPerBlock; i++) {
						if(strcmp(buff[i].name,"\0") != 0){
							return false;
						}
					}

					blkCount++;
				}

				blkNum++;
			}

			return true; 
        }

    };

};

