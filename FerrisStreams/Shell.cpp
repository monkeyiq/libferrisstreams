/******************************************************************************
*******************************************************************************
*******************************************************************************

    Copyright (C) 2001-2003 Ben Martin

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

    $Id: Shell.cpp,v 1.3 2010/09/24 05:05:44 ben Exp $

*******************************************************************************
*******************************************************************************
******************************************************************************/

#define BUILDING_FERRISSTREAMS

#include "FerrisStreams/HiddenSymbolSupport.hh"

#include <Shell.hh>
#include <errno.h>
#include <string.h>

using namespace std;

namespace Ferris
{
    string 
    errnum_to_string( const string& prefix, int en )
    {
        if(!en) en = errno;
    
        char *cptr = strerror(en);
        string ret=prefix;
        ret += " : ";
        ret += cptr;

        return ret;
    }
        
};
