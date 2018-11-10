#include "work_mksofs.h"

#include "rawdisk.h"
#include "core.h"
#include "bin_mksofs.h"

#include <string.h>
#include <inttypes.h>


namespace sofs18
{
    namespace work
    {
    		/**
             * \brief Format the superblock
             * \details The magic number should be put at 0xFFFF.
             * \param [in] name Volume name
             * \param [in] ntotal Total number of blocks of the disk
             * \param [in] itotal Total number of inodes
             * \param [in] rdsize Initial number of blocks of the root directory
             */

        void fillInSuperBlock(const char *name, uint32_t ntotal, uint32_t itotal, uint32_t rdsize)
        {
            soProbe(602, "%s(%s, %u, %u, %u)\n", __FUNCTION__, name, ntotal, itotal, rdsize);

            // solution by Maria João Lavoura, student 84681 DETI - UA
            
            SOSuperBlock sb;

            /* Header */
            sb.magic = 0xFFFF;
            sb.version = VERSION_NUMBER;
            strncpy(sb.name, name, PARTITION_NAME_SIZE);
            sb.mntstat = 1; 		//mount status (1: properly unmounted; 0: otherwise)
            sb.mntcnt = 0; 			//number of mounts since last file system check
            sb.ntotal = ntotal; 	//total number of blocks in the device

            /* Inodes' metadata */
            sb.filt_start = 1; 							//physical number of the block where the free inode list table starts
            sb.filt_head = 0;							//first filled FILT position
            sb.filt_tail = itotal - 1;					//first empty FILT position

            sb.filt_size = itotal/ReferencesPerBlock;	//number of blocks that the free inode list table comprises
            if(itotal%ReferencesPerBlock != 0){
            	sb.filt_size += 1;
            }

            sb.it_size = itotal/InodesPerBlock;		//number of blocks that the inode table comprises
            sb.it_start = sb.filt_size + 1; 		//physical number of the block where the inode table starts
            sb.itotal = itotal;						//total number of inodes
            sb.ifree = itotal - 1;					//number of free inodes

            /* Data blocks' metadata */
            uint32_t data = ntotal - 1 - sb.filt_size - sb.it_size; //data = fblt + dz = ntotal - sb - filt_size - it_size
            sb.fblt_size = data / (ReferencesPerBlock + 1);			//number of blocks that the free block list table comprises
            sb.dz_total = sb.fblt_size * ReferencesPerBlock;		//total number of blocks in data zone

            uint32_t remBlocks = data%(ReferencesPerBlock + 1);
            if(remBlocks==1){
               	if(rdsize==2){
            		sb.dz_total += 1;
                }
            }
            else if(remBlocks > 1){
            	sb.fblt_size += 1;
            	sb.dz_total += remBlocks - 1;
            }

            sb.fblt_start = sb.it_start + sb.it_size;	//physical number of the block where the free block list table starts
            sb.fblt_head = 0;							//first filled FBLT position
            sb.fblt_tail = sb.dz_total - 1; 			//first empty FBLT position

            sb.dz_start = sb.fblt_start + sb.fblt_size; //physical number of the block where the data zone starts
            sb.dz_free =  sb.dz_total-1; 				//number of free blocks in data zone
            if(rdsize==2){
                sb.dz_free -= 1;
            }


            /* caches of free inodes and free blocks */
            SOInodeReferenceCache irc;
            SOInodeReferenceCache iic;
            for(int i=0; i<INODE_REFERENCE_CACHE_SIZE; i++){
            	irc.ref[i] = NullReference;
            	iic.ref[i] = NullReference;
            }
            irc.idx = INODE_REFERENCE_CACHE_SIZE;
            iic.idx = 0;
            sb.ircache = irc;
            sb.iicache = iic;

            SOBlockReferenceCache brc;
            SOBlockReferenceCache bic;
			for(int i=0; i<BLOCK_REFERENCE_CACHE_SIZE; i++){
				brc.ref[i] = NullReference;
				bic.ref[i] = NullReference;
			}
			brc.idx = BLOCK_REFERENCE_CACHE_SIZE;
			bic.idx = 0;
            sb.brcache = brc;
            sb.bicache = bic;


			soWriteRawBlock(0, &sb);

            /* change the following line by your code */
			//bin::fillInSuperBlock(name, ntotal, itotal, rdsize);
        }

    };

};

