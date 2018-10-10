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

        /*
         filling in the contents of the root directory:
         the first 2 entries are filled in with "." and ".." references
         the other entries are empty.
         If rdsize is 2, a second block exists and should be filled as well.
         */
        uint32_t fillInRootDir(uint32_t first_block, uint32_t rdsize)
        {
            soProbe(606, "%s(%u, %u)\n", __FUNCTION__, first_block, rdsize);
            
            /* change the following line by your code */
            //return bin::fillInRootDir(first_block, rdsize);
	    
	    SODirEntry dir[DirentriesPerBlock]; //DirEntry buffer
	    
	    memset(dir,0x0,DirentriesPerBlock); //Buffer com 0s
	    
	//Criação de um DirEntry sem referencia para inode
	    SODirEntry NullDirEntry;
	    NullDirEntry.in = NullReference;
	    memcpy(NullDirEntry.name, "", SOFS18_MAX_NAME + 1);
	    
	//Preenchimento do buffer com DirEntry sem referencia para inode
	    for(uint32_t i=2; i<DirentriesPerBlock; i++)
		dir[i]=NullDirEntry;	

	//Criaçao do DirEntry "root" ou "." e do ".."
	    char point[SOFS18_MAX_NAME + 1] = ".";
	    char point2[SOFS18_MAX_NAME + 1] = "..";

	    SODirEntry root;
	    root.in = 0;
	    memcpy(root.name, point, SOFS18_MAX_NAME + 1);

	    SODirEntry root2;
	    root2.in = 0;
	    memcpy(root2.name, point2, SOFS18_MAX_NAME + 1);
 
	//Adição do "." e do ".." no inicio do buffer
	    dir[0] = root; dir[1] = root2;

	//Escrita do buffer no respectivo block
	    soWriteRawBlock(first_block,dir);
	    if(rdsize == 2)
		soWriteRawBlock(first_block + 1,dir); 
	   
	    return rdsize;
        }

    };

};

