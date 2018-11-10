#include "work_fileblocks.h"

#include "dal.h"
#include "core.h"
#include "bin_fileblocks.h"

#include <errno.h>

namespace sofs18
{
    namespace work
    {

        /* ********************************************************* */


        static uint32_t soGetIndirectFileBlock(SOInode * ip, uint32_t fbn);
        static uint32_t soGetDoubleIndirectFileBlock(SOInode * ip, uint32_t fbn);

        /* ********************************************************* */

        uint32_t soGetFileBlock(int ih, uint32_t fbn)
        {
            soProbe(301, "%s(%d, %u)\n", __FUNCTION__, ih, fbn);

            /* change the following line by your code */

			SOInode* ip = soITGetInodePointer(ih);
			uint32_t IndirectBegin = N_DIRECT;
			uint32_t DoubleIndirectBegin = (N_INDIRECT * ReferencesPerBlock) + IndirectBegin;
			uint32_t DoubleIndirectEnd = (ReferencesPerBlock * ReferencesPerBlock * N_DOUBLE_INDIRECT) + DoubleIndirectBegin -1;

			if(fbn >= 0 && fbn <= DoubleIndirectEnd){

				if(fbn < IndirectBegin){
					return ip->d[fbn];
				}
				else if(fbn < DoubleIndirectBegin){
					return soGetIndirectFileBlock(ip,fbn-IndirectBegin);
				}
				else {
					return soGetDoubleIndirectFileBlock(ip,fbn-DoubleIndirectBegin);
				}

			}
			else{
				throw SOException(EINVAL, __FUNCTION__);
			}

        }

        /* ********************************************************* */


        static uint32_t soGetIndirectFileBlock(SOInode * ip, uint32_t afbn)
        {
            soProbe(301, "%s(%d, ...)\n", __FUNCTION__, afbn);

            /* change the following line by your code */

            uint32_t db[ReferencesPerBlock];
            uint32_t pos1=afbn / ReferencesPerBlock;
            uint32_t pos2= afbn % ReferencesPerBlock;

            if(ip->i1[pos1] == NullReference){
            	return NullReference;
            }
            else{
                soReadDataBlock(ip->i1[pos1],&db);
                return db[pos2];
            }

        }

        /* ********************************************************* */


        static uint32_t soGetDoubleIndirectFileBlock(SOInode * ip, uint32_t afbn)
        {
            soProbe(301, "%s(%d, ...)\n", __FUNCTION__, afbn);

            /* change the following line by your code */

            uint32_t db[ReferencesPerBlock];
            uint32_t pos1 = afbn / (ReferencesPerBlock*ReferencesPerBlock);
            uint32_t pos2 = afbn / ReferencesPerBlock-(pos1*ReferencesPerBlock);
            uint32_t pos3 = afbn  % ReferencesPerBlock;

            if(ip->i2[pos1] == NullReference){
            	return NullReference;
            }
            else{
                soReadDataBlock(ip->i2[pos1],&db);

                if(db[pos2] == NullReference){
                	return NullReference;
                }
                else{
                	soReadDataBlock(db[pos2],&db);
                	return db[pos3];
                }

            }

        }

    };

};

