#include "work_mksofs.h"

#include "core.h"
#include "bin_mksofs.h"

#include <inttypes.h>
#include <iostream>

namespace sofs18
{
    namespace work
    {

        void computeStructure(uint32_t ntotal, uint32_t & itotal, uint32_t & btotal, uint32_t & rdsize)
        {
            soProbe(601, "%s(%u, %u, ...)\n", __FUNCTION__, ntotal, itotal);
            
            /* change the following line by your code */
            //bin::computeStructure(ntotal, itotal, btotal, rdsize);

            // solution by Luis Moura, student 83808 DETI - UA

            uint32_t block = 1; // size of a block
            uint32_t sp = 1;    // super block size
            uint32_t filt;      // free inode list table size
            uint32_t it;        // inode table size
            uint32_t fblt;      // free data blocks list table size
            rdsize = 1;

            // the number of inodes is given by the user
            // default value is total number of blocks divided by 8
            // if given number of inodes is greater than ntotal/2, reduce to ntotal/2
            if (itotal == 0){
                itotal = ntotal/8;
            }
            else if (itotal > ntotal/2){
                itotal = ntotal/2;
            }

            // defining the number of blocks for the inode table (it)
            // and for the free inode list table (filt)
            it = itotal / InodesPerBlock;
            it = itotal % InodesPerBlock > 0 ? it + 1 : it;
			itotal = it * InodesPerBlock;
            filt = itotal / ReferencesPerBlock;
			filt = itotal % ReferencesPerBlock > 0 ? filt + 1 : filt;
            // number of free reference slots to reference inodes 
            uint32_t emptyFILTrefs = filt * ReferencesPerBlock - it * InodesPerBlock;

            // defining number of blocks for the data zone (btotal) and
            // for the free data blocks list table (fblt)
            // remaining blocks for data
            uint32_t data = ntotal - sp - filt - it;
            fblt = data / (ReferencesPerBlock + block);
            btotal = fblt * ReferencesPerBlock;

            // remaining blocks can be assigned to differente areas depending
            // on its quantity
            uint32_t remBlocks = data - fblt - btotal;

            if (remBlocks == 1) {
                if (emptyFILTrefs > 0){
                    it += 1;
                    itotal = it * InodesPerBlock;
                }
                else {
                    rdsize += 1;
                    btotal += 1;
                }
            }
            else if (remBlocks > 1) {
                fblt += 1;
                btotal += (remBlocks - 1);
            }

        }
    }
}

