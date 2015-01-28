/******************************************************************************
*******************************************************************************
*******************************************************************************

    Copyright (C) 2010 Ben Martin

    This file is part of libferrisstreams.

    libferrisstreams is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libferrisstreams is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libferrisstreams.  If not, see <http://www.gnu.org/licenses/>.

    For more details see the COPYING file in the root directory of this
    distribution.

    $Id: xxx.cpp,v 1.3 2008/05/24 21:31:09 ben Exp $

*******************************************************************************
*******************************************************************************
******************************************************************************/

#include "FerrisPosix.hh"
#include <string.h>


int ferris_madvise(void *addr, size_t length, int advice)
{
#ifdef SOLARIS
  // In Solaris, mmap and madvise take char *
#define MMAP_P(p) static_cast<char *>(p)
#else
#define MMAP_P(p) p
#endif

    int ret = madvise( MMAP_P(addr), length, advice );
    return ret;
}


#define USE_CUTE_MEMCMP_TO_TEST_ZERO_BUFFER

bool isBufferAllZero( const void* buffervp, std::streamsize size )
{
    const char* b = (const char*)buffervp;

    if( !size )
        return true;    

#ifdef USE_CUTE_MEMCMP_TO_TEST_ZERO_BUFFER

    return *b == 0 && 0 == memcmp( b, b+1, size-1 );

#else
        
    while (size--)
    {
        if (*b++)
        {
            return false;
        }
    }
    return true;

#endif
}
