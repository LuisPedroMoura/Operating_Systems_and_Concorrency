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
	   
	    if(pih>sb->itotal)
		throw SOException(EINVAL,__FUNCTION__);

	    if (!S_ISDIR(in->mode))
		throw SOException(ENOTDIR,__FUNCTION__);

	    uint32_t blocks = in->blkcnt; //Number of block use by the file
	    //uint32_t maxBlocks = N_DIRECT + (DirentriesPerBlock*N_INDIRECT) + (DirentriesPerBlock*DirentriesPerBlock)*N_DOUBLE_INDIRECT;
	    uint32_t* arrp; //pointer of the array d[],i1[],i2[]
	    SODirEntry buff[ReferencesPerBlock];
	    
	    arrp = in->d;
	    
	    SODirEntry dir;

	    uint32_t i = 0;
	    uint32_t j = 0;
	    while(i<blocks){
		if(j>=N_DIRECT) arrp=in->i1;
		if(j>=N_DIRECT + (DirentriesPerBlock*N_INDIRECT)) arrp=in->i2;
		if(j>=N_DIRECT + (DirentriesPerBlock*N_INDIRECT) + (DirentriesPerBlock*DirentriesPerBlock)*N_DOUBLE_INDIRECT) i=blocks;

		if(arrp[j]!=NullReference){
			sofs18::soReadFileBlock(pih,arrp[j],buff);
			for(uint32_t t=0;t<ReferencesPerBlock;t++){
				dir = buff[t];
				if(strcmp(dir.name,name)==0){
					memmove(dir.name,newName,SOFS18_MAX_NAME+1);
					i=blocks;
					buff[t]=dir;
					sofs18::soWriteFileBlock(pih,arrp[j],buff);
				}
			}
			i+=1;
		}
		else
			j+=1;
	    }        
	}

    };

};

