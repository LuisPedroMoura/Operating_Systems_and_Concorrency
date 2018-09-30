/*
 *  \author António Rui Borges - 2012-2015
 *  \authur Artur Pereira - 2016-2018
 */

#include "work_freelists.h"

#include "core.h"
#include "dal.h"
#include "freelists.h"
#include "bin_freelists.h"

#include <stdio.h>
#include <errno.h>
#include <iostream>
using namespace std;

namespace sofs18
{
    namespace work
    {

        /*
         */
        void soDepleteIICache(void)
        {
            soProbe(404, "%s()\n", __FUNCTION__);

            /* change the following line by your code */
            bin::soDepleteIICache();
        }

    };

};

