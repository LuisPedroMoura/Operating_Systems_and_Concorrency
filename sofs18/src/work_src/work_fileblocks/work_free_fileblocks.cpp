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
        static bool soFreeIndirectFileBlocks(SOInode * ip, uint32_t * bl, uint32_t ffbn);

        /* free all blocks between positions ffbn and ReferencesPerBloc**2 - 1
         * existing in the block of indirect references given by i2.
         * Return true if, after the operation, all references become NullReference.
         * It assumes i2 is valid.
         */
        static bool soFreeDoubleIndirectFileBlocks(SOInode * ip, uint32_t * bl, uint32_t ffbn);

        static void free(SOInode * ip, uint32_t ffbn, uint32_t function);

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

            if (ffbn < 4) {

            	// alterar diretamente d
            	for (; ffbn < N_DIRECT; ffbn++){
            		ip->d[ffbn] = NullReference;
            	}

            	free(ip, ffbn, 1);	// chamar indirect para i1
            	free(ip, ffbn, 2);	//chamar double indirect para i2

            }
            else if (ffbn >= 4  && ffbn < 2 * RPB + 4) {

            	free(ip, ffbn, 1);	// chamar indirect para i1
            	free(ip, ffbn, 2);	// chamar double indirect para i2
            }
            else if (ffbn >= 2 * RPB + 3 && ffbn < (RPBSQR + 2 * RPB) + 4) {

            	free(ip, ffbn, 2);	// chamar double indirect para i2
            }
        }

        /* ********************************************************* */
        static void free(SOInode * ip, uint32_t ffbn, uint32_t function) {

        	if (function == 1) {
        		uint32_t i1index = (ffbn - N_DIRECT) % ReferencesPerBlock;
				for (int i = i1index; i < N_INDIRECT; i++) {
					if(soFreeIndirectFileBlocks(ip, &(ip->i1[i]), ffbn)) {
						ip->i1[i] = NullReference;
					}
				}
        	}

        	else if (function == 2) {
        		uint32_t i2AdjustedRef = ffbn - N_DIRECT - N_INDIRECT * ReferencesPerBlock;
        		uint32_t i2index = i2AdjustedRef / ReferencesPerBlock / ReferencesPerBlock;
        		for (int i = i2index; i < N_DOUBLE_INDIRECT; i++) {
					if (soFreeDoubleIndirectFileBlocks(ip, &(ip->i2[i]), ffbn)) {
						ip->i2[i] = NullReference;
					}
        		}
        	}

        }


        /* ********************************************************* */


        static bool soFreeIndirectFileBlocks(SOInode * ip, uint32_t * bl, uint32_t ffbn)
        {
            soProbe(303, "%s(..., %u, %u)\n", __FUNCTION__, bl, ffbn);

            /* change the following line by your code */
            //throw SOException(ENOSYS, __FUNCTION__);

            uint32_t ref = ffbn % ReferencesPerBlock - N_DIRECT; // ref inclusive

            for (uint32_t i = ref; i < ReferencesPerBlock; i++) {
            	bl[i] = NullReference;
            }

            for (uint32_t i = 0; i < ref; i++) {
            	if (bl[i] != NullReference) {
            		return false;
            	}
            }
            return true;
        }


        /* ********************************************************* */


        static bool soFreeDoubleIndirectFileBlocks(SOInode * ip, uint32_t * bl, uint32_t ffbn)
        {
            soProbe(303, "%s(..., %u, %u)\n", __FUNCTION__, bl, ffbn);

            /* change the following line by your code */
            //throw SOException(ENOSYS, __FUNCTION__);

            uint32_t i2AdjustedRef = ffbn - N_DIRECT - N_INDIRECT * ReferencesPerBlock;   // offsets reference value by d and i1 length
            uint32_t i2index = i2AdjustedRef / ReferencesPerBlock / ReferencesPerBlock;   // double indirect index, just to gabate myself
            uint32_t i2block = (i2AdjustedRef / ReferencesPerBlock) % ReferencesPerBlock; // indirect index
            //uint32_t ref = i2AdjustedRef % ReferencesPerBlock;							  // ref index in block, not neede, just to gabate myself

            for (uint32_t i = i2block; i < ReferencesPerBlock; i++) {
            	if (soFreeIndirectFileBlocks(ip, &(bl[i2block]), ffbn)) {
            		ip->i2[i2index] = NullReference;
            	}
            	ffbn = i2AdjustedRef + (ReferencesPerBlock - (i2AdjustedRef % ReferencesPerBlock)); // it could be zero... but just to gabate myself!
            }

            for (uint32_t i = 0; i < i2block; i++) {
				if (bl[i] != NullReference) {
					return false;
				}
			}
            return true;
        }

        /* ********************************************************* */

    };

};

