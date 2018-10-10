// done by Fernando Marques 80238

#include "work_mksofs.h"

#include "rawdisk.h"
#include "core.h"
#include "bin_mksofs.h"

#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <inttypes.h>

namespace sofs18
{
    namespace work
    {

        uint32_t fillInInodeTable(uint32_t first_block, uint32_t itotal, uint32_t rdsize)
        {
            soProbe(604, "%s(%u, %u, %u)\n", __FUNCTION__, first_block, itotal, rdsize);
            
            uint32_t nBlocks = itotal/InodesPerBlock;
            SOInode inode[InodesPerBlock];

            for(uint32_t block_num = first_block ; block_num < first_block + nBlocks ; block_num++)
            {
            	for(uint32_t i = 0 ; i < InodesPerBlock ; i++)
            	{
                    inode[i].mode = INODE_FREE;
                    inode[i].lnkcnt = 0;
                    inode[i].owner = 0;
                    inode[i].group = 0;
                    inode[i].size = 0;
                    inode[i].blkcnt = 0;

            		for(uint32_t a = 0 ; a < N_DIRECT ; a++)
            		{
            			inode[i].d[a] = NullReference;
            		}

            		for(uint32_t b = 0 ; b < N_INDIRECT ; b++)
            		{
            			inode[i].i1[b] = NullReference;
            		}

            		for(uint32_t c = 0 ; c < N_DOUBLE_INDIRECT ; c++)
            		{
            			inode[i].i2[c] = NullReference;
            		}

                    if(block_num == first_block && i == 0)
                    {
                        inode[0].mode = S_IFDIR | 0775;
                        inode[0].lnkcnt = 2;
                        inode[0].owner = getuid();
                        inode[0].group = getgid();
                        inode[0].atime = time(NULL);
                        inode[0].mtime = time(NULL);
                        inode[0].ctime = time(NULL);
                        inode[0].d[0] = 0;
                        if(rdsize < 2)
                        {
                            inode[0].size = BlockSize;
                            inode[0].blkcnt = 1;
                        } else {
                            inode[0].d[1] = 1;
                            inode[0].size = BlockSize*2;
                            inode[0].blkcnt = 2;
                        }
                    }
            	}
            	soWriteRawBlock(block_num, &inode);
            }

            return nBlocks;


            /* change the following line by your code */
            //return bin::fillInInodeTable(first_block, itotal, rdsize);
        }

    };

};

