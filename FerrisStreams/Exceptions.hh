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

    $Id: Exceptions.hh,v 1.3 2010/11/14 21:46:19 ben Exp $

*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef _ALREADY_INCLUDED_LIBFERRISSTREAMS_EXCEPTION_H_
#define _ALREADY_INCLUDED_LIBFERRISSTREAMS_EXCEPTION_H_

#include <exception>
#include <string>

#include <FerrisStreams/Streams.hh>

#ifndef __GNUC
#define __PRETTY_FUNCTION__ ""
#endif

namespace Ferris
{
    class FERRISEXP_API FerrisException_CodeState
    {
        std::string fileName;
        int         lineNumber;
        std::string functionName;

    public:
        FerrisException_CodeState( const std::string& fi, int li, const std::string& fu )
            :
            fileName(fi),
            lineNumber(li),
            functionName(fu)
            {
            }

        inline const std::string& getFileName() const
            {
                return fileName;
            }

        inline const std::string& getFunctionName() const
            {
                return functionName;
            }

        inline int getLineNumber() const
            {
                return lineNumber;
            }
    };

    
    class FERRISEXP_EXCEPTION FerrisExceptionBase  : public std::exception
    {
        FerrisException_CodeState State;
        mutable std::string WhatStr;
        std::string ExceptionName;
        std::string ExtraData;
    
    protected:

        inline void setExceptionName( const std::string& s )
            {
                ExceptionName = s;
            }
        inline void setExtraData( const std::string& s )
            {
                ExtraData = s;
            }
    
    public:

        FerrisExceptionBase( const FerrisException_CodeState& state,
                             fh_ostream log,
                             const char* e = "" );
        ~FerrisExceptionBase() throw ();

        virtual const std::string& whats() const;
        virtual const char* what() const throw ();
    };


    class FERRISEXP_EXCEPTION FerrisStreamException : public FerrisExceptionBase
    {
    public:

        inline FerrisStreamException(
            const FerrisException_CodeState& state,
            fh_ostream log,
            const std::string& e )
            :
            FerrisExceptionBase( state, log, e.c_str() )
            {
                setExceptionName("FerrisStreamException");
            }
    };
#define Throw_FerrisStreamException(e,a) \
throw FerrisStreamException( \
FerrisException_CodeState( __FILE__ ,  __LINE__ , __PRETTY_FUNCTION__ ), \
                           ::Ferris::Factory::fcerr(), (e) )


    FERRISEXP_API void StreamThrowFromErrno( int eno, const std::string& e, void* p = 0 );
    

    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/
    
    class FERRISEXP_EXCEPTION UnspoortedBlockSize : public FerrisStreamException
    {
    public:

        inline UnspoortedBlockSize(
            const FerrisException_CodeState& state,
            fh_ostream log,
            const std::string& e )
            :
            FerrisStreamException( state, log, e.c_str() )
            {
                setExceptionName("UnspoortedBlockSize");
            }
    };
#define Throw_UnspoortedBlockSize(e,a) \
throw UnspoortedBlockSize( \
FerrisException_CodeState( __FILE__ ,  __LINE__ , __PRETTY_FUNCTION__ ), \
                           ::Ferris::Factory::fcerr(), (e) )


    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/
    
    class FERRISEXP_EXCEPTION MalformedURL : public FerrisStreamException
    {
    public:

        inline MalformedURL(
            const FerrisException_CodeState& state,
            fh_ostream log,
            const std::string& e )
            :
            FerrisStreamException( state, log, e.c_str() )
            {
                setExceptionName("MalformedURL");
            }
    };
#define Throw_MalformedURL(e,a) \
throw MalformedURL( \
FerrisException_CodeState( __FILE__ ,  __LINE__ , __PRETTY_FUNCTION__ ), \
                           ::Ferris::Factory::fcerr(), (e) )


    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/
    
    class FERRISEXP_EXCEPTION CreateFIFO : public FerrisStreamException
    {
    public:

        inline CreateFIFO(
            const FerrisException_CodeState& state,
            fh_ostream log,
            const std::string& e )
            :
            FerrisStreamException( state, log, e.c_str() )
            {
                setExceptionName("CreateFIFO");
            }
    };
#define Throw_CreateFIFO(e,a) \
throw CreateFIFO( \
FerrisException_CodeState( __FILE__ ,  __LINE__ , __PRETTY_FUNCTION__ ), \
                           ::Ferris::Factory::fcerr(), (e) )
    


    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/
};

#endif
