/*
 *  \author António Rui Borges - 2012-2015
 *  \authur Artur Pereira - 2016-2018
 */

#include "work_freelists.h"

#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include <iostream>

#include "core.h"
#include "dal.h"
#include "freelists.h"
#include "bin_freelists.h"

namespace sofs18
{
    namespace work
    {

        uint32_t soAllocInode(uint32_t type)
        {
            soProbe(401, "%s(%x)\n", __FUNCTION__, type);

            /* change the following line by your code */
            //return bin::soAllocInode(type);	

			SOSuperBlock *sb = soSBGetPointer();
			
			if(type != S_IFREG && type != S_IFDIR && type != S_IFLNK){
				throw SOException(EINVAL,__FUNCTION__); 
			}

			if(sb->ifree <= 0){
				throw SOException(ENOSPC,__FUNCTION__);
			}

			if(sb->ircache.idx == INODE_REFERENCE_CACHE_SIZE){
				sofs18::soReplenishIRCache(); 
			}
					
			SOInodeReferenceCache RetrivalCache = sb->ircache;
			
			uint32_t inoderef = RetrivalCache.ref[RetrivalCache.idx];
			
			int inode_Handler = soITOpenInode(inoderef);
			SOInode* in = soITGetInodePointer(inode_Handler);

			time_t current_time = time(NULL);
			
			in->mode = type;
			in->atime = current_time;
			in->mtime = current_time;
			in->ctime = current_time;
			in->owner = getuid();
			in->group = getgid();

			soITSaveInode(inode_Handler);
			soITCloseInode(inode_Handler);

			sb->ircache.ref[RetrivalCache.idx] = NullReference;
			sb->ircache.idx += 1;
			sb->ifree -= 1;
		
			if(sb->ifree <= 0){
				sb->ifree = 0;
			}

			soSBSave();
					
			return inoderef;	

        }
    };

};

