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


        static uint32_t soAllocIndirectFileBlock(SOInode * ip, uint32_t afbn);
        static uint32_t soAllocDoubleIndirectFileBlock(SOInode * ip, uint32_t afbn);
	

        /* ********************************************************* */

        uint32_t soAllocFileBlock(int ih, uint32_t fbn)
        {
            soProbe(302, "%s(%d, %u)\n", __FUNCTION__, ih, fbn);

            /* change the following two lines by your code */
            //return bin::soAllocFileBlock(ih, fbn);

	    SOSuperBlock *sb = soSBGetPointer();

	    if(sb -> dz_free == 0)
		throw SOException(ENOSPC,__FUNCTION__);
	
	    uint32_t maxNumber = N_DIRECT + (ReferencesPerBlock * N_INDIRECT) + (ReferencesPerBlock * ReferencesPerBlock) * N_DOUBLE_INDIRECT;

	    if(fbn >= maxNumber)
		throw SOException(EINVAL,__FUNCTION__);

	    SOInode* in = soITGetInodePointer(ih);
	    uint32_t allblock;

	     //if index within d[]
        if(fbn < N_DIRECT) {
        //add to d[]
        allblock = sofs18::soAllocDataBlock();
        in->d[fbn] = allblock ;
	in->blkcnt += 1;
        } 
        //if index within i1[0] or i1[1]
        else if(fbn < (2 * ReferencesPerBlock) + N_DIRECT) {
        //add to i1[0] or i1[1]
        allblock = soAllocIndirectFileBlock(in, fbn);
        }
        //if index within i2[0] or i2[1]
        else {
        //add to i2[0] or i2[1]
        allblock = soAllocDoubleIndirectFileBlock(in, fbn);
        }
	    //return number of allocated block
	    soITSaveInode(ih);
	    return allblock ; 
        }

        /* ********************************************************* */


        /*
         */
        static uint32_t soAllocIndirectFileBlock(SOInode * ip, uint32_t afbn)
        {
            soProbe(302, "%s(%d, ...)\n", __FUNCTION__, afbn);
  
	
            uint32_t allblock_temp;
	    uint32_t allblock = NullReference;
	    uint32_t buff[ReferencesPerBlock];
	    	
	    //if afbn beyond the total of possible spaces for d[] and i1[0], then it is within i1[1]
    	    if(afbn > ReferencesPerBlock + N_DIRECT - 1) {
 		
		//Search if exist already a block alocate in i1[1]		
		if(ip->i1[1] == NullReference) {
			allblock_temp = sofs18::soAllocDataBlock();
			for(uint32_t i=0; i<ReferencesPerBlock; i++) buff[i] = NullReference;
			sofs18::soWriteDataBlock(allblock_temp,buff);
		}
		else{
			allblock_temp = ip->i1[1];
			sofs18::soReadDataBlock(allblock_temp,buff);
		} 
		//Discover index within i1[1]
		afbn = afbn - ReferencesPerBlock - N_DIRECT;
		
		allblock = sofs18::soAllocDataBlock();

		buff[afbn] = allblock;

		sofs18::soWriteDataBlock(allblock_temp,buff);		
		
		//add to i1[1]		
		ip->i1[1] = allblock_temp;
				 
	    }
	    //else, afbn is beyond the spaces for d[] yet not beyond the max of d+i1[0], hence being within i1[0]
	    else {
 	
		//Search if exist already a block alocate in i1[0]
		if(ip->i1[0] == NullReference){
			allblock_temp = sofs18::soAllocDataBlock();
			for(uint32_t i=0; i<ReferencesPerBlock; i++) buff[i] = NullReference;
			sofs18::soWriteDataBlock(allblock_temp,buff);
		}
		else{
			allblock_temp = ip->i1[0];
 			sofs18::soReadDataBlock(allblock_temp,buff);
		}

		//Discover index within i1[0]
		afbn = afbn - N_DIRECT;
		
		allblock = sofs18::soAllocDataBlock();

		buff[afbn] = allblock;

		sofs18::soWriteDataBlock(allblock_temp,buff);		

		//add to i1[0]
		ip->i1[0] = allblock_temp;
	    }	    
	    
	    ip->blkcnt += 2;
	    return allblock ;
        }


        /* ********************************************************* */


        /*
         */
        static uint32_t soAllocDoubleIndirectFileBlock(SOInode * ip, uint32_t afbn)
        {
            soProbe(302, "%s(%d, ...)\n", __FUNCTION__, afbn);

            /* change the following two lines by your code */
     	   	
		
            uint32_t allblock_temp1;
	    uint32_t allblock_temp2;
	    uint32_t allblock = NullReference;
	    uint32_t buff1[ReferencesPerBlock];
	    uint32_t buff2[ReferencesPerBlock];

	    uint32_t afbn_quoc;
	    uint32_t afbn_rem;

	    //if afbn is beyond the total of possible spaces for d[], i1[0], i1[1] and i2[0], then it is within i2[1]
	    if(afbn > (ReferencesPerBlock * ReferencesPerBlock) + (2 * ReferencesPerBlock) + N_DIRECT - 1) {

		//Search if exist already a block alocate in i1[0]
		if(ip->i2[1] == NullReference){
			allblock_temp1 = sofs18::soAllocDataBlock();
			for(uint32_t i=0; i<ReferencesPerBlock; i++) buff1[i] = NullReference;
			sofs18::soWriteDataBlock(allblock_temp1,buff1);
		}
		else{
			allblock_temp1 = ip->i2[1];
 			sofs18::soReadDataBlock(allblock_temp1,buff1);
		}
		//Discover index within i2[1]
		afbn = afbn - (ReferencesPerBlock * ReferencesPerBlock) - (2 * ReferencesPerBlock) - N_DIRECT;

		//Knowing the index, find out which sub-array of i2[1] is belongs to, and the corresponding index within it
		afbn_quoc = afbn / ReferencesPerBlock;
	        afbn_rem = afbn % ReferencesPerBlock;

		if(buff1[afbn_quoc] == NullReference){
			allblock_temp2 = sofs18::soAllocDataBlock();
			for(uint32_t i=0; i<ReferencesPerBlock; i++) buff2[i] = NullReference;
			sofs18::soWriteDataBlock(allblock_temp2,buff2);
		}
		else{
			allblock_temp2 = buff1[afbn_quoc];
			sofs18::soReadDataBlock(allblock_temp2,buff2);
		}
		allblock = sofs18::soAllocDataBlock();
		
		//add to i2[1]
		buff2[afbn_rem] = allblock;
		sofs18::soWriteDataBlock(allblock_temp2,buff2);		

		buff1[afbn_quoc] = allblock_temp2;
		sofs18::soWriteDataBlock(allblock_temp1,buff1);
		
		ip -> i2[1] = allblock_temp1;		

		
	    }	    
	    //else, afbn is beyond the spaces for d[], i1[0] and i1[1], but not beyond the max of d+i1[0]+i1[1]+i2[0], hence being within i2[0]
	    else {

		//Search if exist already a block alocate in i1[0]
		if(ip->i2[0] == NullReference){
			allblock_temp1 = sofs18::soAllocDataBlock();
			for(uint32_t i=0; i<ReferencesPerBlock; i++) buff1[i] = NullReference;
			sofs18::soWriteDataBlock(allblock_temp1,buff1);
		}
		else{
			allblock_temp1 = ip->i2[0];
			sofs18::soReadDataBlock(allblock_temp1,buff1);
		}
		//Discover index within i2[0]
		afbn = afbn - (2 * ReferencesPerBlock) - N_DIRECT;

		//Knowing the index, find out which sub-array of i2[0] is belongs to, and the corresponding index within it
		afbn_quoc = afbn / ReferencesPerBlock;
	        afbn_rem = afbn % ReferencesPerBlock;

		if(buff1[afbn_quoc] == NullReference){
			allblock_temp2 = sofs18::soAllocDataBlock();
			for(uint32_t i=0; i<ReferencesPerBlock; i++) buff2[i] = NullReference;
			sofs18::soWriteDataBlock(allblock_temp2,buff2);
		}
		else{
			allblock_temp2 = buff1[afbn_quoc];
			sofs18::soReadDataBlock(allblock_temp2,buff2);
		}
		allblock = sofs18::soAllocDataBlock();
		
		//add to i2[0]
		buff2[afbn_rem] = allblock;
		sofs18::soWriteDataBlock(allblock_temp2,buff2);		

		buff1[afbn_quoc] = allblock_temp2;
		sofs18::soWriteDataBlock(allblock_temp1,buff1);
		
		ip -> i2[0] = allblock_temp1;		
	    }	

	    ip->blkcnt += 3;
	    return allblock ;
        }


    };

};

