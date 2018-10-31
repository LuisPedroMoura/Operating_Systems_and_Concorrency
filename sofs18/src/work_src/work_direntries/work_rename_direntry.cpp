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
	    SOSuperBlock *sb = soSBGetPointer();
	   
	    if(pih>(sb->itotal))
		throw SOException(EINVAL,__FUNCTION__);

	    if (!S_ISDIR(in->mode))
		throw SOException(ENOTDIR,__FUNCTION__);

	    uint32_t blocks = in->blkcnt; //Number of block use by the file
	    
	    SODirEntry buff[ReferencesPerBlock];
	   
	    SODirEntry dir;

	    uint32_t index = 0;
	    uint32_t blockn = 0;
	    uint32_t blockcnt = 0;
	    uint32_t j = 0;
	    uint32_t check = 1;
	    while(blockcnt<blocks){
		blockn = sofs18::soGetFileBlock(pih,index);
		if(blockn==NullReference){
			sofs18::soReadFileBlock(pih,index,buff);
			for(j=0;j<ReferencesPerBlock;j++){
				dir = buff[j];
				if(strcmp(dir.name,name)==0){
					memmove(dir.name,newName,SOFS18_MAX_NAME+1);
					blockcnt=blocks-1;
					check = 0;
					buff[j]=dir;
					sofs18::soWriteFileBlock(pih,index,buff);
				}
			}
			blockcnt+=1;
		}
		else
			j+=1;
		index+=1;
	    }
	
	    if(check!=0)
		throw SOException(ENOENT,__FUNCTION__);       
	}

    };

};

