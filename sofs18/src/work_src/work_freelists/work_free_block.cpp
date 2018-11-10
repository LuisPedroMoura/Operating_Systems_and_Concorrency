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

        void soFreeDataBlock(uint32_t bn)
        {
            soProbe(442, "%s(%u)\n", __FUNCTION__, bn);

            /* change the following line by your code */
            // code developed by Fernando Marques 80238
            
            // pt to superblock structure
            SOSuperBlock *sb = soSBGetPointer();

            // verify if cache is full
            if(sb->bicache.idx == BLOCK_REFERENCE_CACHE_SIZE)
            {
            	sofs18::soDepleteBICache();
            }

            // add new value to insertion cache
            sb->bicache.ref[sb->bicache.idx] = bn;
            // idx value refresh
            sb->bicache.idx++;
            // number of free blocks in data zone refresh
            sb->dz_free++;
            //bin::soFreeDataBlock(bn);

            // save the superblock
            soSBSave();
        }

    };

};

