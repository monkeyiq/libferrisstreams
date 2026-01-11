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

    $Id: Shell.hh,v 1.3 2010/09/24 05:05:44 ben Exp $

*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef _ALREADY_INCLUDED_LIBFERRISSTREAMS_SHELL_H_
#define _ALREADY_INCLUDED_LIBFERRISSTREAMS_SHELL_H_

#include <FerrisStreams/Exceptions.hh>
#include <FerrisStreams/Streams.hh>

namespace Ferris
{
    namespace Factory
    {
        /**
         * unlink the path and create a new fifo there. Open the fifo
         * for read+write in non blocking mode and return that fd.
         *
         * @param  path Where to put the new fifo
         * @return fd open in read+write mode
         */
        FERRISEXP_API int MakeFIFO( const std::string& path );

        /**
         * unlink the path and create a new fifo there. MAYBE Open the fifo
         * in the given mode and return that fd.
         *
         * @param  path Where to put the new fifo
         * @return if(!should_open) then 0 else fd open in openmode
         */
        FERRISEXP_API int MakeFIFO( const std::string& path,
                                    bool should_open,
                                    int openmode = 0 /* O_RDWR | O_NONBLOCK */ );
    };
};
#endif
