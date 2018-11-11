#include "work_fileblocks.h"

#include "freelists.h"
#include "dal.h"
#include "core.h"
#include "bin_fileblocks.h"

#include <errno.h>

#include <iostream>

namespace sofs18
{
    namespace work
    {


        static uint32_t soAllocIndirectFileBlocks(SOInode * ip, uint32_t afbn);
        static uint32_t soAllocDoubleIndirectFileBlocks(SOInode * ip, uint32_t afbn);
	

        /* ********************************************************* */

        uint32_t soAllocFileBlock(int ih, uint32_t fbn)
        {
            soProbe(302, "%s(%d, %u)\n", __FUNCTION__, ih, fbn);


            SOInode* ip = soITGetInodePointer(ih);
			uint32_t RPB = ReferencesPerBlock;
			uint32_t RPBSQR = RPB * RPB;
			uint32_t doubleIndirectStart = N_INDIRECT * RPB + N_DIRECT;
			uint32_t doubleIndirectEnd = RPBSQR * N_DOUBLE_INDIRECT + doubleIndirectStart;
			uint32_t allBlock;

			// exit condition - invalid fbn
			if (fbn < 0 || fbn >= doubleIndirectEnd) {
				throw SOException(EINVAL, __FUNCTION__);
			}


			if (fbn < N_DIRECT) {

				allBlock = sofs18::soAllocDataBlock();
				ip->d[fbn] = allBlock ;
				ip->blkcnt += 1;
			}

			if (fbn >= N_DIRECT && fbn < doubleIndirectStart) {

				allBlock = soAllocIndirectFileBlocks(ip, fbn - N_DIRECT);
			}

			if (fbn >= doubleIndirectStart && fbn < doubleIndirectEnd) {

				allBlock = soAllocDoubleIndirectFileBlocks(ip, fbn - doubleIndirectStart);
			}

			soITSaveInode(ih);
			return allBlock;
		}


		/* ********************************************************* */


		static uint32_t soAllocIndirectFileBlocks(SOInode * ip, uint32_t afbn)
		{
			soProbe(302, "%s(%d, ...)\n", __FUNCTION__, afbn);

			/* change the following line by your code */
			//throw SOException(ENOSYS, __FUNCTION__);

			// calculate indirect list index
			uint32_t i1index = (afbn / ReferencesPerBlock) % ReferencesPerBlock;

			// calculate direct list index
			uint32_t ref = afbn % ReferencesPerBlock; // ref inclusive

			// data block frame
			uint32_t db[ReferencesPerBlock];
			uint32_t allBlock;

			// indirect list index is empty
			if (ip->i1[i1index] == NullReference) {

				// create direct block and alloc it to indirect position
				allBlock = sofs18::soAllocDataBlock();
				for (uint32_t i = 0; i < ReferencesPerBlock; i++) {
					db[i] = NullReference;
				}
				sofs18::soWriteDataBlock(allBlock,db);
				// read direct block
				ip->i1[i1index] = allBlock;
				soReadDataBlock(ip->i1[i1index], &db);

				allBlock = sofs18::soAllocDataBlock();
				db[ref] = allBlock;
				ip->blkcnt += 2;
			}
			else {

				soReadDataBlock(ip->i1[i1index], &db);
				allBlock = sofs18::soAllocDataBlock();
				ip->i1[i1index] = allBlock;
				ip->blkcnt += 1;
			}

			return allBlock;
		}


		/* ********************************************************* */


		static uint32_t soAllocDoubleIndirectFileBlocks(SOInode * ip, uint32_t afbn)
		{
			soProbe(302, "%s(%d, ...)\n", __FUNCTION__, afbn);

			/* change the following line by your code */
			//throw SOException(ENOSYS, __FUNCTION__);

			uint32_t i2index = afbn / ReferencesPerBlock / ReferencesPerBlock;
			uint32_t db[ReferencesPerBlock];
			uint32_t allBlock;

			// double indirect list index is empty
			if (ip->i2[i2index] == NullReference) {

				// create indirect block and alloc it to double indirect position
				allBlock = sofs18::soAllocDataBlock();
				for (uint32_t i = 0; i < ReferencesPerBlock; i++) {
					db[i] = NullReference;
				}
				sofs18::soWriteDataBlock(allBlock,db);
				// read indirect block
				ip->i2[i2index] = allBlock;
				soReadDataBlock(ip->i2[i2index], &db);

				allBlock = soAllocIndirectFileBlocks(ip, afbn);
				ip->blkcnt += 1;
			}
			else {

				soReadDataBlock(ip->i2[i2index], &db);
				allBlock = soAllocIndirectFileBlocks(ip, afbn);
			}

			return allBlock;

		}

    };

};

