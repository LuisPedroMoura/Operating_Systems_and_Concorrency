#include "work_fileblocks.h"

#include "freelists.h"
#include "dal.h"
#include "core.h"
#include "bin_fileblocks.h"

#include <inttypes.h>
#include <errno.h>
#include <assert.h>

namespace sofs18
{
    namespace work
    {

        /* free all blocks between positions ffbn and ReferencesPerBlock - 1
         * existing in the block of references given by i1.
         * Return true if, after the operation, all references become NullReference.
         * It assumes i1 is valid.
         */
        static uint32_t soFreeIndirectFileBlocks(uint32_t * bl, uint32_t ffbn, uint32_t size);

        /* free all blocks between positions ffbn and ReferencesPerBloc**2 - 1
         * existing in the block of indirect references given by i2.
         * Return true if, after the operation, all references become NullReference.
         * It assumes i2 is valid.
         */
        static uint32_t soFreeDoubleIndirectFileBlocks(uint32_t * bl, uint32_t ffbn);

        /* ********************************************************* */

        void soFreeFileBlocks(int ih, uint32_t ffbn)
        {
            soProbe(303, "%s(%d, %u)\n", __FUNCTION__, ih, ffbn);

            /* change the following line by your code */
            //bin::soFreeFileBlocks(ih, ffbn);

            // solution by Luis Moura, student 83808 DETI - UA and
            //			   Maria João, student 84681 DETI - UA

            SOInode* ip = soITGetInodePointer(ih);
            uint32_t RPB = ReferencesPerBlock;
            uint32_t RPBSQR = RPB * RPB;
            uint32_t doubleIndirectStart = N_INDIRECT * RPB + N_DIRECT;
            uint32_t doubleIndirectEnd = RPBSQR * N_DOUBLE_INDIRECT + doubleIndirectStart;

            uint32_t count = 0;

            // exit condition - invalid ffbn
            if (ffbn < 0 || ffbn >= doubleIndirectEnd) {
				throw SOException(EINVAL, __FUNCTION__);

			}

            if (ffbn < N_DIRECT) {

            	// alterar diretamente d
            	for (; ffbn < N_DIRECT; ffbn++){
            		if (ip->d[ffbn] != NullReference) {
						count++;
					}
            		ip->d[ffbn] = NullReference;
            	}
            }

            if (ffbn >= N_DIRECT && ffbn < doubleIndirectStart) {

            	// free indirect list
            	count += soFreeIndirectFileBlocks(ip->i1, ffbn - N_DIRECT, N_INDIRECT);
            	ffbn = doubleIndirectStart;
            }

            if (ffbn >= doubleIndirectStart && ffbn < doubleIndirectEnd) {

            	// free double indirect list
            	count += soFreeDoubleIndirectFileBlocks(ip->i2, ffbn - N_DIRECT - N_INDIRECT * ReferencesPerBlock);
            }

            ip->blkcnt -= count;
            soITSaveInode(ih);
        }


        /* ********************************************************* */


        static uint32_t soFreeIndirectFileBlocks(uint32_t * bl, uint32_t ffabn, uint32_t size)
        {
            soProbe(303, "%s(..., %u, %u)\n", __FUNCTION__, bl, ffabn);

            /* change the following line by your code */
            //throw SOException(ENOSYS, __FUNCTION__);

            uint32_t count = 0;

            // calculate indirect list index
            uint32_t i1index = (ffabn / ReferencesPerBlock) % ReferencesPerBlock;

            // calculate direct list index
            uint32_t ref = ffabn % ReferencesPerBlock; // ref inclusive

            // free file blocks from ref to end of indirect list
            uint32_t db[ReferencesPerBlock];
            for (uint32_t i = i1index; i < size; i++) {

            	if (bl[i] == NullReference){
            		continue;
            	}

            	soReadDataBlock(bl[i], &db);

            	// free direct list
            	for (uint32_t j = ref; j < ReferencesPerBlock; j++) {
            		if (db[j] != NullReference) {
            			count++;
            		}
					db[j] = NullReference;
				}

            	// verify direct list is completely empty
            	bool del = true;
            	for (uint32_t j = 0; j < ref; j++) {

					if (db[j] != NullReference) {
						del = false;
					}
				}

            	soWriteDataBlock(bl[i], &db);

            	// if empty, free indirect list entry
            	if (del) {
            		bl[i] = NullReference;
            		count++;
            	}

            	// adjust ref (next iteration free complete direct lists)
            	ref = 0;
            }

            return count;
        }


        /* ********************************************************* */


        static uint32_t soFreeDoubleIndirectFileBlocks(uint32_t * bl, uint32_t ffabn)
        {
            soProbe(303, "%s(..., %u, %u)\n", __FUNCTION__, bl, ffabn);

            /* change the following line by your code */
            //throw SOException(ENOSYS, __FUNCTION__);

            uint32_t count = 0;

            uint32_t i2index = ffabn / ReferencesPerBlock / ReferencesPerBlock;   // double indirect index, just to gabate myself
            uint32_t i2block = (ffabn / ReferencesPerBlock) % ReferencesPerBlock; // indirect index

            uint32_t db[ReferencesPerBlock];
			for (uint32_t i = i2index; i < N_DOUBLE_INDIRECT; i++) {

				if (bl[i] == NullReference){
					continue;
				}

				soReadDataBlock(bl[i], &db);

				// free indirect list
				count += soFreeIndirectFileBlocks(db, ffabn, ReferencesPerBlock);

				// verify indirect list is completely empty
				bool del = true;
				for (uint32_t j = 0; j < i2block; j++) {
					if (db[j] != NullReference) {
						del = false;
					}
				}

				soWriteDataBlock(bl[i], &db);

				// if empty, free indirect list entry
				if (del) {
					bl[i] = NullReference;
					count++;
				}

				// adjust i2block and ffabn (next iteration free complete direct lists)
				i2block = 0;
				ffabn = 0;
			}

			return count;
        }

        /* ********************************************************* */

    };

};

