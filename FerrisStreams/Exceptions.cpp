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

    $Id: Exceptions.cpp,v 1.2 2010/09/24 05:05:43 ben Exp $

*******************************************************************************
*******************************************************************************
******************************************************************************/

#define BUILDING_FERRISSTREAMS

#include "Exceptions.hh"

namespace Ferris
{
    FerrisExceptionBase::FerrisExceptionBase(
        const FerrisException_CodeState& state,
        fh_ostream log,
        const char* e )
        :
        State(state)
    {
        setExceptionName( "FerrisExceptionBase" );
        setExtraData( e );
        log << whats() << std::endl;
    }

    FerrisExceptionBase::~FerrisExceptionBase() throw ()
    {
    }

    const char* FerrisExceptionBase::what() const throw ()
    {
        return whats().c_str();
    }
    
    
    const std::string&
    FerrisExceptionBase::whats() const
    {
        std::ostringstream ss;
    
        ss << ExceptionName
           << ", " << State.getLineNumber() << ":" << State.getFileName()
           << " " << State.getFunctionName()
            ;

        if( ExtraData.length() )
        {
            ss << " " << ExtraData;
        }
            
        WhatStr = tostr(ss);
        return WhatStr;
    }


    void StreamThrowFromErrno( int eno, const std::string& e, void* )
    {
        std::string es = errnum_to_string( "", eno );

        fh_stringstream ss;
        ss << e << " reason:" << es;
        Throw_FerrisStreamException( tostr(ss), 0 );
    }

    
};
