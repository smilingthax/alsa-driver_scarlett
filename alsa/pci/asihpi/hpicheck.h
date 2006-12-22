/******************************************************************************
Copyright (C) 1997-2003 AudioScience, Inc. All rights reserved.

This software is provided 'as-is', without any express or implied warranty.
In no event will AudioScience Inc. be held liable for any damages arising
from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This copyright notice and list of conditions may not be altered or removed 
   from any source distribution.

AudioScience, Inc. <support@audioscience.com>

( This license is GPL compatible see http://www.gnu.org/licenses/license-list.html#GPLCompatibleLicenses )

Compile time checks of hpi message and response sizes.
and union members are multiple of 4 bytes

Include this file in ONLY ONE of the files in a project.

Tested to work with CCS, <add compilers here>
Doesn't work with <add compilers here>

(C) Copyright AudioScience Inc. 2005
******************************************************************************/

	  /* If the assert fails, compiler complains
        something like size of array `msg' is negative
	  */
#define compile_time_assert(cond, msg) \
    typedef char msg[(cond) ? 1 : -1]
 
	  /* check that size is exactly some number */
#define compile_time_size_check(sym,size) \
    compile_time_assert(sizeof(sym)==(size),sizechk##sym)
 
	  /* check that size is a multiple of unit */   
#define compile_time_unit_check(sym,unit) \
    compile_time_assert((sizeof(sym)%(unit))==0,unitchk##sym)

/* Each object MSG and RES must be multiple of 4 bytes */
#define compile_time_obj_check(obj) \
	compile_time_unit_check(obj##_MSG,4); \
	compile_time_unit_check(obj##_RES,4)
	
	  /* Perform the checks */
compile_time_size_check(HW8,1);
compile_time_size_check(HW16,2);
compile_time_size_check(HW32,4);
#ifdef HPI_MESSAGE_FORCE_SIZE
compile_time_size_check(HPI_MESSAGE,HPI_MESSAGE_FORCE_SIZE);
#else
compile_time_size_check(HPI_MESSAGE,44);
#endif

compile_time_size_check(HPI_RESPONSE,64);

compile_time_obj_check(HPI_SUBSYS);
compile_time_obj_check(HPI_ADAPTERX);
compile_time_obj_check(HPI_STREAM);
compile_time_obj_check(HPI_MIXER);
compile_time_obj_check(HPI_CONTROL);
compile_time_obj_check(HPI_CONTROLX);
compile_time_obj_check(HPI_NVMEMORY);
compile_time_obj_check(HPI_GPIO);
compile_time_obj_check(HPI_WATCHDOG);
compile_time_obj_check(HPI_CLOCK);
compile_time_obj_check(HPI_PROFILE);
compile_time_obj_check(HPI_ASYNC);

/* API HPI_FORMAT must have fixed size */
compile_time_size_check(HPI_FORMAT, 5*4);

/* Message FORMAT must fit inside API format */
compile_time_assert((sizeof(HPI_MSG_FORMAT) <= sizeof(HPI_FORMAT)),format_fit);

#ifndef HPI_WITHOUT_HPI_DATA
/* API HPI_FORMAT must have fixed size */
compile_time_size_check(HPI_DATA, 7*4);

/* Message DATA must fit inside API data */
compile_time_assert((sizeof(HPI_MSG_DATA) <= sizeof(HPI_DATA)),data_fit);
#endif

