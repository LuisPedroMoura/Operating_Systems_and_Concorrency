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

#include "core.h"
#include "dal.h"
#include "freelists.h"
#include "bin_freelists.h"

namespace sofs18
{
    namespace work
    {

        void soFreeInode(uint32_t in)
        {
            soProbe(402, "%s(%u)\n", __FUNCTION__, in);

            // code developed by Fernando Marques 80238
            SOSuperBlock *sb = soSBGetPointer();

          	// it means that insertion cache is full
          	if (sb->iicache.idx == INODE_REFERENCE_CACHE_SIZE)
          	{
          		sofs18::soDepleteIICache();
          	}

          	// add a new element to the cache
          	// inode handler to the inode to be freed
          	int inode_handler = soITOpenInode(in);
          	// pt to inode to be freed
          	SOInode *inode = soITGetInodePointer(inode_handler);
          	// increment free inodes
          	sb->iicache.ref[sb->iicache.idx] = in;
          	sb->iicache.idx++;
          	sb->ifree++;

          	inode->mode = INODE_FREE;
            inode->lnkcnt = 0;
            inode->owner = 0;
            inode->group = 0;
            inode->size = 0;
            inode->blkcnt = 0;

            inode->atime = 0;
            inode->mtime = 0;
            inode->ctime = 0;

            // save inode changed            
            soITSaveInode(inode_handler);
            // save the superblock
            soSBSave();
            // close open inode
            soITCloseInode(inode_handler);

            /* change the following line by your code */
            //bin::soFreeInode(in);
        }

    };

};

