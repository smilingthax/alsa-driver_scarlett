/******************************************************************************

    AudioScience HPI driver
    Copyright (C) 1997-2003  AudioScience Inc. <support@audioscience.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of version 2 of the GNU General Public License as
    published by the Free Software Foundation;

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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
compile_time_assert(sizeof(sym)==size,sizechk##sym)

/* check that size is a multiple of unit */
#define compile_time_unit_check(sym,unit) \
compile_time_assert((sizeof(sym)%unit)==0,unitchk##sym)

/* Each object MSG and RES must be multiple of 4 bytes */
#define compile_time_obj_check(obj) \
compile_time_unit_check(obj##_MSG,4); \
compile_time_unit_check(obj##_RES,4)

/* Perform the checks */
compile_time_size_check(u8, 1);
compile_time_size_check(u16, 2);
compile_time_size_check(u32, 4);

compile_time_size_check(HPI_MESSAGE, 44);
compile_time_size_check(HPI_RESPONSE, 64);

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
