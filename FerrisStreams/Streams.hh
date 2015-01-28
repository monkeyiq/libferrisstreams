/******************************************************************************
*******************************************************************************
*******************************************************************************

    libferris streams
    Copyright (C) 2001-2003 Ben Martin

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    For more details see the COPYING file in the root directory of this
    distribution.

    $Id: Streams.hh,v 1.16 2010/11/14 21:46:19 ben Exp $

*******************************************************************************
*******************************************************************************
******************************************************************************/

/*
 *
 * The strange looking construct shown below is in place to allow streams to
 * "seem" like they are pointers to streams, this is mainly for backward
 * compatibility in the existing codebase and should not be used in new code.
 *
 *    _Self* operator->()
 *         {
 *             return this;
 *         }
 *
 *
 *
 * NOTE THAT THE ORDER OF THESE TWO CALLS MUST BE THIS WAY:
 *  setsbT();
 *  init();
 *
 */
#ifndef _ALREADY_INCLUDED_LIBFERRISSTREAMS_STREAMS_H_
#define _ALREADY_INCLUDED_LIBFERRISSTREAMS_STREAMS_H_

#include "FerrisStreams/HiddenSymbolSupport.hh"

//#include <glib.h>

#include <iosfwd>

#include <sstream>
#include <string>
#include <ios>
#include <fstream>
#include <iostream>
#include <iomanip>
//#include <strstream.h>
#include <map>
#include <AssocVector.h>

//#include <FerrisHandle.hh>
//#include <TypeDecl.hh>
#include <FerrisLoki/Extensions.hh>

#include <SmartPtr.h>
#include <Functor.h>

#include <sigc++/sigc++.h>
#include <sigc++/signal.h>
#include <sigc++/slot.h>
#include <sigc++/object.h>
#include <sigc++/connection.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(STLPORT) || defined(_RWSTD_VER_STR)
#define FERRIS_STD_BASIC_FILEBUF_SUPERCLASS std::basic_filebuf
#else
#include <ext/stdio_filebuf.h>
#define FERRIS_STD_BASIC_FILEBUF_SUPERCLASS __gnu_cxx::stdio_filebuf
#endif

namespace Ferris
{
    template < typename _CharT, typename _Traits >
    inline std::ostreambuf_iterator<_CharT> copy( std::basic_istream<_CharT,_Traits>& iss,
                                                  std::basic_ostream<_CharT,_Traits>& oss )
    {
        std::ostreambuf_iterator<char> ret = std::copy(
            std::istreambuf_iterator<char>(iss),
            std::istreambuf_iterator<char>(),
            std::ostreambuf_iterator<char>(oss));
        oss << std::flush;
        return ret;
    }
    
    
    /**
     * This version handles the case where the app changes its UID
     */
    std::string ferris_g_get_home_dir();
    
        
    /**
     * Convert an errno to a string describing that error.
     *
     * @param  en the error number
     * @param  prefix a string to prefix to the returned error description
     * @return String describing the error
     */
    FERRISEXP_API std::string 
    errnum_to_string( const std::string& prefix, int en );
    
    
    struct ferris_ios
    {
//        typedef guint64 openmode;
        typedef unsigned long long openmode;
        
        /**
         * You can add this flag to openmode to try to get O_DIRECT
         * for this object. Note that this is only a clue and if the
         * module doesn't support it then you dont get it.
         */
        static const openmode  o_direct = 1LL << 63;

        /**
         * Use this flag to get a memory mapped stream if possible
         */
        static const openmode  o_mmap = 1LL << 62;

        /**
         * Use this flags together with o_mmap to flag memory access
         * as MADV_SEQUENTIAL:
         * Expect  page  references  in sequential order.  (Hence, pages in
         * the given range can be aggressively read ahead, and may be freed
         * soon after they are accessed.)
         */
        static const openmode  o_mseq = 1LL << 61;

        /**
         * Get the iostream but do not perform any unciphering of the data
         * that would usually be performed.
         */
        static const openmode  o_nouncrypt = 1LL << 60;

        
        /**
         * Used to mask off ferris specific options when passing to standard
         * C++ IOStreams code.
         */
        static const openmode  all_mask = 0 | o_direct | o_mmap | o_mseq | o_nouncrypt;

        /**
         * Mask off all specific libferris bits from an openmode and return
         * the std::openmode
         */
        FERRISEXP_API inline static std::ios::openmode maskOffFerrisOptions(
            std::ios::openmode m )
            {
                return (std::ios::openmode)(m & (~all_mask));
            }
        FERRISEXP_API inline static std::ios::openmode maskOffFerrisOptions(
            ferris_ios::openmode m )
            {
                return (std::ios::openmode)(m & (~all_mask));
            }
    };


    /**
     * The standard doesn't allow things like ~/whatever or file://where as valid
     * and we can easily clean these up for the user to make them valid too.
     *
     * If you can handle file:// URLs then pass in false as the second argument to
     * leave them in the return string, otherwise prefixes file:// are stripped
     * off to try to make a kernel handlable string.
     */
    FERRISEXP_API std::string CleanupURL( const std::string& s );
    
    /**
     * The standard doesn't allow things like ~/whatever or file://where as valid
     * and we can easily clean these up for the user to make them valid too.
     *
     * @param stripFileSchemePrefix if set to false then the file:// prefix will
     *        not be stripped off as is the case when just using CleanupURL( url );
     * @param leaveURIConstructsIntact if set to true then things like "//" are
     *        not transformed. Setting this to true allows embedded URI strings
     *        to pass through.
     */
    FERRISEXP_API std::string CleanupURL( const std::string& s,
                                          bool stripFileSchemePrefix,
                                          bool leaveURIConstructsIntact );
    
    
    /**
     * 64 bit clean min of streamsize and another integer
     */
    template< class T >
    const std::streamsize  min( T a, std::streamsize b )
    {
        std::streamsize ret = (a < b) ? a : b;
        return ret;
    }


    /**
     * Note that all data members of this class should be considered private.
     * m_sigHolder is only public because of the difficulty in getting the
     * getClosedSignal() friend to stick.
     */
    class CloseSigHolder;
    template<
        typename _CharT,
        typename _Traits = std::char_traits < _CharT >
    >
    class ferris_streambuf
        :
        public FerrisLoki::Handlable,
        public Loki::SmallObject<>
    {
        typedef ferris_streambuf< _CharT, _Traits > _Self;

        
    public:


//         friend closeSignal_t& getClosedSignal(
//             ferris_streambuf< char, std::char_traits<char> >* sh );
//         friend void OnGenericStreamClosed( Handlable* a );
        CloseSigHolder* m_sigHolder;
        
        bool emitter_attached;

        typedef std::char_traits<_CharT> traits_type;
        typedef typename traits_type::int_type    int_type;
        typedef typename traits_type::char_type   char_type;
        typedef typename traits_type::pos_type    pos_type;
        typedef typename traits_type::off_type    off_type;

        explicit ferris_streambuf()
            :
            m_sigHolder( 0 ),
            emitter_attached( false )
            {
            }
    
        virtual ~ferris_streambuf()
            {
            }

        virtual void private_AboutToBeDeleted()
            {
//            cerr << "ferris_streambuf::AboutToBeDeleted() this:" << (void*)this << endl;
                Handlable::private_AboutToBeDeleted();
            }
    };


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

    template<
        typename _CharT,
        typename _Traits = std::char_traits<_CharT>
    >
    class  null_streambuf
        :
        public ferris_streambuf<_CharT, _Traits>,
        public std::basic_streambuf< _CharT, _Traits >
    {
    public:

        typedef std::char_traits<_CharT> traits_type;
        typedef typename traits_type::int_type    int_type;
        typedef typename traits_type::char_type   char_type;
        typedef typename traits_type::pos_type    pos_type;
        typedef typename traits_type::off_type    off_type;
    
        null_streambuf()
            {
                this->setg( 0, 0, 0 );
                this->setp( 0, 0 );
            }

    protected:

        inline std::streamsize xsputn( const char_type* s, std::streamsize n )
            {
                return n;
            }
    
        int_type overflow( int_type c )
            {
                return 1; //traits_type::not_eof(c);
            }
        int sync()
            {
                return 0;
            }
    
    private:

        null_streambuf( const null_streambuf& );
        null_streambuf& operator=( const null_streambuf& );
    };
    

    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/

    class FERRISEXP_API ferris_basic_streambuf_onemeg
    {
    protected:
    
        inline static const std::streamsize getBufSize()
            {
                return 1024 * 1024 + 4;
            }
        inline static const std::streamsize getPBSize()
            {
                return 4;
            }
    };

    
    class FERRISEXP_API ferris_basic_streambuf_sixteenk
    {
    protected:
    
        inline static const std::streamsize getBufSize()
            {
                return 16 * 1024 + 4;
            }
        inline static const std::streamsize getPBSize()
            {
                return 4;
            }
    };


    class FERRISEXP_API ferris_basic_streambuf_fourk
    {
    protected:
    
        inline static const std::streamsize getBufSize()
            {
                return 4 * 1024 + 4;
            }
        inline static const std::streamsize getPBSize()
            {
                return 4;
            }
    };

    class FERRISEXP_API ferris_basic_streambuf_quartk
    {
    protected:
    
        inline static const std::streamsize getBufSize()
            {
                return 256;
            }
        inline static const std::streamsize getPBSize()
            {
                return 4;
            }
    };

    class FERRISEXP_API ferris_basic_streambuf_sixteenbytes
    {
    protected:
    
        inline static const std::streamsize getBufSize()
            {
                return 16;
            }
        inline static const std::streamsize getPBSize()
            {
                return 4;
            }
    };

    class FERRISEXP_API ferris_basic_streambuf_virtual
    {
        std::streamsize sz;
        
    protected:

        ferris_basic_streambuf_virtual()
            :
            sz(0)
            {
            }

        ferris_basic_streambuf_virtual( std::streamsize sz )
            :
            sz( sz )
            {
            }
        
        void setBufSize( const std::streamsize _sz )
            {
                sz = _sz;
            }
        std::streamsize getBufSize()
            {
                return sz;
            }
        std::streamsize getPBSize()
            {
                return 0;
            }
    };
    

    template<
        class _CharT,
        class _Traits = std::char_traits < _CharT >,
        class _Alloc  = std::allocator   < _CharT >,
        class _BufferSizers = ferris_basic_streambuf_fourk
        >
    class  ferris_basic_streambuf
        :
        public ferris_streambuf< _CharT, _Traits >,
        public std::basic_streambuf< _CharT, _Traits >,
        public _BufferSizers
    {
        typedef ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers > _Self;
        typedef std::basic_streambuf< _CharT, _Traits >                          _Base;
        typedef std::basic_string< _CharT, _Traits > _String;

        // prohibit copy/assign
        ferris_basic_streambuf( const ferris_basic_streambuf& );
        ferris_basic_streambuf operator=( const ferris_basic_streambuf& );

    protected:

        _CharT*  buffer;
    
        enum modes_t {
            mode_reading=1,
            mode_writing=2,
            mode_mute=3
        };

        modes_t CurrentMode;
        
    public:

        typedef std::ios_base::seekdir seekd_t;
        typedef std::char_traits<_CharT>        traits_type;
        typedef typename traits_type::int_type  int_type;
        typedef typename traits_type::char_type char_type;
        typedef typename traits_type::pos_type  pos_type;
        typedef typename traits_type::off_type  off_type;

        typedef Loki::Functor< int,
                               LOKI_TYPELIST_2( char_type*, std::streamsize ) > write_out_given_data_functor_t;
        write_out_given_data_functor_t write_out_given_data_functor;
        void setWriteOutGivenDataFunctor( const write_out_given_data_functor_t& f )
            {
                write_out_given_data_functor = f;
            }
        
        ferris_basic_streambuf()
            :
            buffer(new char_type[ this->getBufSize() ]),
            CurrentMode( mode_mute )
            {
                setWriteOutGivenDataFunctor(
                    write_out_given_data_functor_t( this, &_Self::write_out_given_data ) );
                
                this->setg( 0, 0, 0 );
                this->setp( 0, 0 );
                this->ensureMode( mode_mute );
            }


        /**
         * This constructor is here for the use of datastream only. it was easier to make
         * a special constructor in the main stream buffer just for that subclass' use.
         * This also relies on being inherited from ferris_basic_streambuf_virtual.
         */
        ferris_basic_streambuf( void* data, const int data_size )
            :
            buffer( static_cast<char_type*>(data) ),
            CurrentMode( mode_mute )
            {
                setWriteOutGivenDataFunctor(
                    write_out_given_data_functor_t( this, &_Self::write_out_given_data ) );

                this->setBufSize( data_size );
                this->setg( 0, 0, 0 );
                this->setp( 0, 0 );
                this->ensureMode( mode_mute );
            }
        

        ~ferris_basic_streambuf()
            {
                this->ensureMode( mode_mute );

                if( buffer )
                {
                    delete [] buffer;
                }
            }

    protected:

        /**
         * We need to set there to be no input buffer so that the next attempt to read
         * a byte will cause buffer_in() to be called to obtain data.
         */
        virtual void switch_to_read_mode()
            {
                // beg, next, end
                this->setg( 0, 0, 0 );
            }

        virtual void switch_to_write_mode()
            {
                this->setp( buffer,
                            buffer + this->getBufSize() );
            }
        
        virtual bool ensureMode( modes_t m )
            {
                if( !buffer )          return true;
                if( CurrentMode == m ) return true;

                /*
                 * From mute to a read/write state.
                 */
                if( CurrentMode == mode_mute )
                {
                    CurrentMode = m;
                    if( m == mode_reading )
                    {
                        switch_to_read_mode();
                    }
                    else
                    {
                        switch_to_write_mode();
                    }
                    return true;
                }

                /*
                 * Cleanup for mute mode.
                 */
                if( m == mode_mute )
                {
                    if( CurrentMode == mode_writing )
                    {
                        sync();
                    }
                    CurrentMode = m;
                    this->setg( 0, 0, 0 );
                    this->setp( 0, 0 );
                    return true;
                }
            

                return false;
            }
    

    protected:

        /**
         * This is the only methods that really needs to be here. It gets
         * up to maxsz data into buffer and returns how much data was really
         * read. Return 0 for a failure, you must read atleast one byte.
         */
        virtual int make_new_data_avail( char_type* buffer, std::streamsize maxsz )
            {
                return 0;
            }

        /**
         * Write out the data starting at buffer of length sz to the "external"
         * device. A sublcass should define a with the same specs and pass a
         * functor to it to setWriteOutGivenDataFunctor().
         * 
         * return -1 for error or 0 for success
         */
        virtual int write_out_given_data( const char_type* buffer, std::streamsize sz )
            {
                return -1;
            }
    


        int sync()
            {
                int rc = 0;
            
                if( CurrentMode == mode_writing )
                {
                    rc = buffer_out();
                }
            
                CurrentMode = mode_mute;
                return rc;
            }

        /**
         * This is mainly here so that subclasses can provide a different buffer for
         * writing than reading, thus giving a double buffered stream.
         */
        virtual char_type* getWriteBuffer()
            {
                return buffer;
            }
    
        int buffer_out()
            {
                if( !this->ensureMode( mode_writing ) )
                {
//                cerr << "attempt to buffer_out() while not able to switch to writing mode!" << endl;
                    return traits_type::eof();
                }

                int cnt = this->pptr() - this->pbase();

                if( !cnt )
                {
                    return 0;
                }
                
//                 int retval = write_out_given_data_functor( (char_type*)getWriteBuffer(),
//                                                            (std::streamsize)cnt );
                int retval =0;
                {
                    char_type* b = getWriteBuffer();
                    std::streamsize ss = (std::streamsize)cnt;
                    retval = write_out_given_data_functor( b, ss );
                }
                
                this->pbump(-cnt);
                return retval;
            }

        int_type overflow( int_type c )
            {
                if( !this->ensureMode( mode_writing ) )
                {
                    return traits_type::eof();
                }

                if( buffer_out() < 0 )
                {
                    return traits_type::eof();
                }
                else
                {
                    if( !traits_type::eq_int_type( c, traits_type::eof() ) )
                    {
                        return this->sputc(c);
                    }
                    else
                    {
                        return traits_type::not_eof(c);
                    }
                }
            }


        std::streamsize xsputn( const char_type* s, std::streamsize n )
            {
                if( !this->ensureMode( mode_writing ) )
                {
                    return traits_type::eof();
                }

                if( n < this->epptr() - this->pptr())
                {
                    memcpy( this->pptr(), s, n * sizeof(char_type));
                    this->pbump(n);
                    return n;
                }
                else
                {
                    for( std::streamsize i=0; i<n; i++ )
                    {
                        if( traits_type::eq_int_type(
                                this->sputc(s[i]), traits_type::eof() ))
                        {
                            return i;
                        }
                    }
                    return n;
                }
            }
    
        int buffer_in()
            {
//             cout << "buffer_in()" << endl;
            
                if( !this->ensureMode( mode_reading ) )
                {
                    return traits_type::eof();
                }

//             cout << "gptr():" << (void*)gptr()
//                  << " eback():" << (void*)eback()
//                  << " diff:" << (gptr() - eback())
//                  << endl;
                std::streamsize numPutbacks = min( this->gptr() - this->eback(),
                                                   this->getPBSize() );

                memcpy( buffer + (this->getPBSize()-numPutbacks) * sizeof(char_type),
                        this->gptr() - numPutbacks    * sizeof(char_type),
                        numPutbacks                   * sizeof(char_type));
            
                int retval = make_new_data_avail(
                    buffer + this->getPBSize() * sizeof(char_type),
                    this->getBufSize() - this->getPBSize() );
            
                if( retval <= 0 )
                {
                    this->setg(0,0,0);
                    return -1;
                }
                else
                {
//                 cout << "numPutbacks:" << numPutbacks
//                      << " getPBSize:" << getPBSize()
//                      << " retval:" << retval
//                      << endl;

                    // beg, next, end
                    this->setg( buffer + this->getPBSize() - numPutbacks,
                                buffer + this->getPBSize(),
                                buffer + this->getPBSize() + retval );

//                     cerr << "GDATA:";
//                     copy( buffer + getPBSize(),
//                           buffer + getPBSize() + retval,
//                           std::ostreambuf_iterator<char>(std::cerr) );
//                     cerr << endl;
                    
                    return retval;
                }
            }
    
    
        int_type underflow()
            {
                if( !this->ensureMode( mode_reading ) )
                {
                    return traits_type::eof();
                }

                if( this->gptr() < this->egptr() )
                {
                    return traits_type::to_int_type(*this->gptr());
                }

                if( this->buffer_in() < 0 )
                {
                    return traits_type::eof();
                }
                else
                {
                    return traits_type::to_int_type(*this->gptr());
                }
            }

        int_type pbackfail( int_type c )
            {
                if( !this->ensureMode( mode_reading ) )
                {
                    return traits_type::eof();
                }

                if( this->gptr() != this->eback() )
                {
                    this->gbump(-1);
                    if(!traits_type::eq_int_type(c,traits_type::eof()))
                    {
                        *(this->gptr()) = traits_type::to_char_type(c);
                    }
                    return traits_type::not_eof(c);
                }
                else
                {
                    return traits_type::eof();
                }
            }

        /**
         * Call this after any seek operation to ensure the mute read/write mode.
         */
        virtual pos_type
        have_been_seeked( pos_type r )
            {
                if( !this->ensureMode( mode_mute ) )
                {
                    return traits_type::eof();
                }
                return r;
            }
    
    
    
        virtual pos_type
        seekoff(off_type offset, seekd_t d, std::ios_base::openmode m)
            {
                return have_been_seeked( 
                    _Base::seekoff( offset, d, m ));
            }

        virtual pos_type
        seekpos(pos_type pos, std::ios_base::openmode m)
            {
                return have_been_seeked( 
                    _Base::seekpos( pos, m ) );
            }
    
    
    };

    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/

    /**
     * This class is a mix in, use it by inheriting from both a streambuf
     * like ferris_basic_streambuf_LargeAvailabeData_Mixin and this class.
     * 
     * This class lets your make_new_data_avail() method accept a buffer
     * which is dynamic and (possibly) larger than the buffer of the streambuf
     * object. See gstreamer_readFrom_streambuf in libferris for a use. Basically,
     * your make_new_data_avail should follow the following template:
     * 
     * virtual int make_new_data_avail( char_type* out_buffer, std::streamsize out_maxsz )
     * {
     *   int ret = -1;
     *
     *   if( m_EOS )
     *     return -1;
     *
     *   ret = make_new_data_avail_from_LargeAvailableData( out_buffer, out_maxsz );
     *   if( ret == -1 )
     *   {
     *        // somehow get data into buf and bufsz
     *        char* buf = ...;
     *        copyLargeAvailabeData( buf, bufsz );
     *        delete[] buf;
     *        ret = make_new_data_avail_from_LargeAvailableData( out_buffer, out_maxsz );
     *   }
     *   return ret;
     * }
     */
        template<
            typename _CharT,
            typename _Traits = std::char_traits < _CharT >,
            typename _Alloc  = std::allocator   < _CharT >
            >
        class ferris_basic_streambuf_LargeAvailabeData_Mixin
        {
            typedef _CharT char_type;
            char_type* m_largeAvailableData;
            std::streamsize m_largeAvailableDataSize;
            std::streamsize m_largeAvailableDataCurrentOffset;

        protected:
            ferris_basic_streambuf_LargeAvailabeData_Mixin()
             :
                m_largeAvailableData(0),
                m_largeAvailableDataSize(0),
                m_largeAvailableDataCurrentOffset(0)
            {
            }

            void copyLargeAvailabeData( char_type* d, std::streamsize dlen )
            {
//                cerr << "copyLargeAvailabeData() dlen:" << dlen << endl;
                if(m_largeAvailableData)
                    delete [] m_largeAvailableData;
                m_largeAvailableDataSize = dlen;
                m_largeAvailableData = new char_type[ dlen+1 ];
                m_largeAvailableDataCurrentOffset = 0;
                memcpy( m_largeAvailableData, d, dlen );
            }
            
            int make_new_data_avail_from_LargeAvailableData( char_type* out_buffer, std::streamsize out_maxsz )
            {
                if( !m_largeAvailableDataSize )
                {
//                    cerr << "make_avail, LargeAvailabeData() no buffer..." << endl;
                    return -1;
                }
                
                // cerr << "make_avail, LargeAvailabeData() "
                //      << " bufsz:" << m_largeAvailableDataSize
                //      << " maxsz:" << out_maxsz
                //      << " offset:" << m_largeAvailableDataCurrentOffset
                //      << endl;

                std::streamsize copySize = out_maxsz;
                copySize = std::min( copySize,
                                     m_largeAvailableDataSize - m_largeAvailableDataCurrentOffset );
                if( copySize )
                {
                    memcpy( out_buffer,
                            m_largeAvailableData + m_largeAvailableDataCurrentOffset,
                            copySize );
                    m_largeAvailableDataCurrentOffset += copySize;
                }
                else
                {
                    delete [] m_largeAvailableData;
                    m_largeAvailableDataCurrentOffset = 0;
                    m_largeAvailableDataSize = 0;
                    return -1;
                }
                return copySize;
            }

        };
    
    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/

    template<
        class _CharT,
        class _Traits = std::char_traits < _CharT >,
        class _Alloc  = std::allocator   < _CharT >,
        class _BufferSizers = ferris_basic_streambuf_fourk
        >
    class  ferris_basic_double_buffered_streambuf
        :
        public ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers >
    {
    public:

//        typedef ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers > _Base;
        
        typedef std::char_traits<_CharT> traits_type;
        typedef typename traits_type::int_type    int_type;
        typedef typename traits_type::char_type   char_type;
        typedef typename traits_type::pos_type    pos_type;
        typedef typename traits_type::off_type    off_type;

        ferris_basic_double_buffered_streambuf()
            :
            WRBuffer(new char_type[ this->getBufSize() ])
            {
                _Base::ensureMode( _Base::mode_mute );
                this->setg( this->buffer + this->getPBSize(),
                            this->buffer + this->getPBSize(),
                            this->buffer + this->getPBSize() );
                this->setp( WRBuffer, WRBuffer + this->getBufSize() );
            }
    

        virtual ~ferris_basic_double_buffered_streambuf()
            {
                this->ensureMode( _Base::mode_mute );
                delete WRBuffer;
            }
    
    private:

        char_type*  WRBuffer;

        typedef ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers > _Base;
        typedef ferris_basic_double_buffered_streambuf< _CharT,
                                                        _Traits,
                                                        _Alloc,
                                                        _BufferSizers > _Self;
        typedef std::basic_string< _CharT, _Traits > _String;
        typedef std::ios_base::seekdir seekd_t;

        // prohibit copy/assign
        ferris_basic_double_buffered_streambuf( const ferris_basic_double_buffered_streambuf& );
        ferris_basic_double_buffered_streambuf operator=(
            const ferris_basic_double_buffered_streambuf& );

        virtual bool ensureMode( typename _Self::modes_t m )
            {
//             if( m == mode_mute )
//             {
//                 if( CurrentMode == mode_writing )
//                 {
//                     sync();
//                 }
//             }
            
                _Base::CurrentMode = m;
                return true;
            }


    protected:

        int sync()
            {
//            cout << "ferris_basic_double_buffered_streambuf()::sync()" << endl;
                int rc = _Base::buffer_out();
                _Base::CurrentMode = _Base::mode_mute;
                return rc;
            }
    
        virtual pos_type 
        have_been_seeked( pos_type r )
            {
//            cout << "ferris_basic_double_buffered_streambuf()::have_been_seeked()" << endl;
                if( _Base::CurrentMode == _Base::mode_writing )
                {
                    sync();
                }

                return _Base::have_been_seeked( r );
            }

        virtual char_type* getWriteBuffer()
            {
                return WRBuffer;
            }
    
    };

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


    template<
        typename _CharT,
        typename _Traits = std::char_traits< _CharT >
    >
    struct  i_ferris_stream_traits
    {
        static const std::ios_base::openmode DefaultOpenMode = std::ios_base::in;
    };

    template<
        typename _CharT,
        typename _Traits = std::char_traits< _CharT >
    >
    struct  o_ferris_stream_traits
    {
        static const std::ios_base::openmode DefaultOpenMode = std::ios_base::out;
    };

    template<
        typename _CharT,
        typename _Traits = std::char_traits< _CharT >
    >
    struct  io_ferris_stream_traits
    {
        static const std::ios_base::openmode
        DefaultOpenMode = (std::ios_base::openmode)(
            ((long)std::ios_base::in) | ((long)std::ios_base::out));
    };

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

    template<
        typename _CharT,
        typename _Traits = std::char_traits< _CharT >
    >
    class  emptystream_methods
    {
    public:

        typedef _CharT                                    char_type;
        typedef _Traits                                   traits_type;
        typedef typename traits_type::int_type            int_type;
        typedef typename traits_type::pos_type            pos_type;
        typedef typename traits_type::off_type            off_type;

        emptystream_methods( void* _delegate = 0 )
            {}

        void set_delegate_methods_object( void* _delegate = 0 )
            {}
    };

    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/


    template<
        typename _CharT,
        typename _Traits = std::char_traits<_CharT>
    >
    class  Ferris_commonstream
        :
        public FerrisLoki::Handlable
    {
        // memory management
        friend class ContextStreamMemoryManager;
        
    protected:

    
        /*
         * Keep a pointer to the stream buffer and a handle so that the ref count
         * is correct.
         */
        typedef std::basic_streambuf<_CharT, _Traits> sb_t;

        typedef ferris_streambuf< _CharT, _Traits > _HandleT;
        
        typedef Loki::SmartPtr< _HandleT, //Handlable,
                                FerrisLoki::FerrisExRefCounted,
                                Loki::DisallowConversion,
                                FerrisLoki::FerrisExSmartPointerChecker,
                                FerrisLoki::FerrisExSmartPtrStorage > sh_t;
    public:
        // FIXME: make these protected and StreamHandlableSigEmitter<T> a friend.
        sb_t* sb;
        sh_t sh;

    public:
        typedef sigc::signal2< void, sb_t*, _HandleT* > StreamBufferChangedSignal_t;
        StreamBufferChangedSignal_t& getStreamBufferChangedSig()
            {
                m_hasUserGottenChangedSignal = true;
                return StreamBufferChangedSignal;
            }

        typedef
        std::basic_streambuf< _CharT, _Traits > sb_changed_callback_std_streambuf;
        typedef
        ferris_streambuf< _CharT, _Traits > sb_changed_callback_ferris_streambuf;
        
        typedef Loki::Functor<
            void,
            LOKI_TYPELIST_2( sb_changed_callback_std_streambuf*,
                        sb_changed_callback_ferris_streambuf* ) > sb_changed_callback_t;
        sb_changed_callback_t sb_changed_callback;
        bool m_sb_changed_callback_used;
        
    protected:
    
        Ferris_commonstream()
            :
            sb(0),
            sh(0),
            m_sb_changed_callback_used( false ),
            m_hasUserGottenChangedSignal( false )
            {}


    
        void setsb( const Ferris_commonstream* x )
            {
                sb = x->sb;
                sh = x->sh;

                if( m_hasUserGottenChangedSignal )
                {
//                    cerr << "m_hasUserGottenChangedSignal(1) is true" << endl;
//                    StreamBufferChangedSignal.emit( sb, GetImpl(sh) );
                }

                if( m_sb_changed_callback_used )
                {
//                    cerr << "m_sb_changed_callback_used(1)" << endl;
                    sb_changed_callback( sb, GetImpl(sh) );
                    m_sb_changed_callback_used = false;
                }
                
            }

        void setsb( sb_t* _sb, sh_t _sh )
            {
//                 sb = dynamic_cast<sb_t*>(_sb);
//                 sh = dynamic_cast<_HandleT*>( GetImpl(_sh) );
                
                sb = static_cast<sb_t*>( _sb );
                sh = static_cast<_HandleT*>( GetImpl(_sh) );

//                 if( m_hasUserGottenChangedSignal )
//                 {
//                     cerr << "m_hasUserGottenChangedSignal(2) is true" << endl;
//                     StreamBufferChangedSignal.emit( sb, GetImpl(sh) );
//                 }

                if( m_sb_changed_callback_used )
                {
//                    cerr << "m_sb_changed_callback_used(2) this" << (void*)this << endl;
                    sb_changed_callback( sb, GetImpl(sh) );
                    m_sb_changed_callback_used = false;
                }
            }

        template <class T>
        void setsbT( T* _sb, bool emit = false )
            {
                bool alreadyHadStreamBufSet = (sb != 0);

                sb = static_cast<sb_t*>( _sb );
                sh = static_cast<_HandleT*>( _sb );
                
//                 if( m_hasUserGottenChangedSignal &&
//                     ( emit || alreadyHadStreamBufSet ))
//                 {
//                     cerr << "m_hasUserGottenChangedSignal(3) is true" << endl;
// //                    StreamBufferChangedSignal.emit( sb, GetImpl(sh) );
//                 }

                if( m_sb_changed_callback_used &&
                    ( emit || alreadyHadStreamBufSet ))
                {
//                    cerr << "m_sb_changed_callback_used(3)" << endl;
                    sb_changed_callback( sb, GetImpl(sh) );
                    m_sb_changed_callback_used = false;
                }
                
            }

    public:

//     void StreamHandlableSigEmitter_Setup( sb_t* _sb, sh_t _sh )
//         {
//             sb = _sb;
//             sh = _sh;
// //            this->init( sb );
//         }

    
        std::basic_streambuf<_CharT, _Traits>* rdbuf() const
            {
                return sb;
            }

    private:
        bool m_hasUserGottenChangedSignal;
        StreamBufferChangedSignal_t StreamBufferChangedSignal;
    };

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


    template<
        typename _CharT,
        typename _Traits = std::char_traits<_CharT>
    >
    class  Ferris_istream
        :
        public std::basic_istream< _CharT, _Traits >,
        public virtual Ferris_commonstream< _CharT, _Traits >
    {
        typedef std::basic_istream< _CharT, _Traits > istream_t;
        typedef std::basic_streambuf<_CharT, _Traits> sb_t;
        typedef Ferris_commonstream<_CharT, _Traits> _CS;
        typedef Ferris_istream<_CharT, _Traits>   _Self;

        typedef std::basic_streambuf<_CharT, _Traits> ss_impl_t;

//     protected:

//         Ferris_istream( sb_t* sb )
//             :
//             std::basic_istream< _CharT, _Traits >( sb )
//             {}
        
    public:

        typedef std::char_traits<_CharT> traits_type;
        typedef typename traits_type::int_type    int_type;
        typedef typename traits_type::char_type   char_type;
        typedef typename traits_type::pos_type    pos_type;
        typedef typename traits_type::off_type    off_type;
        typedef emptystream_methods< _CharT, _Traits > delegating_methods;

        Ferris_istream()
            :
            std::basic_istream< _CharT, _Traits >( 0 )
            {}

        template<
            class _Alloc,
            class _BufferSizers
        >
        Ferris_istream(
            ferris_basic_streambuf< char_type, traits_type, _Alloc, _BufferSizers >* streambuf )
            :
            std::basic_istream< _CharT, _Traits >( 0 )
            {
                this->setsbT( streambuf );
                this->init( _CS::sb );
                this->exceptions( std::ios_base::goodbit );
                this->clear();
            }

        Ferris_istream( const Ferris_istream& rhs )
            :
            std::basic_istream< _CharT, _Traits >( 0 )
            {
                this->setsb( &rhs );

//             LG_IOSTREAM_D << "Ferris_istream( const Ferris_istream& rhs )" << endl;
//             LG_IOSTREAM_D << "Ferris_istream( const Ferris_istream& rhs ) "
//                           << " rhs.sb:" << (void*)rhs.sb
//                           << " rdbuf:" << (void*)Ferris_commonstream< _CharT, _Traits >::rdbuf()
//                           << endl;
            
                this->init( _CS::sb );
                this->exceptions( std::ios_base::goodbit );
                this->clear( rhs.rdstate() );
                this->copyfmt( rhs );
            }

        Ferris_istream& operator=( const Ferris_istream& rhs )
            {
                this->setsb( &rhs );
                this->init( _CS::sb );
                this->exceptions( std::ios_base::goodbit );
                this->clear( rhs.rdstate() );
                this->copyfmt( rhs );

                return *this;
            }

    
        virtual ~Ferris_istream()
            {}

        _Self* operator->()
            {
                return this;
            }

        sb_t* 
        rdbuf() const
            {
                return _CS::sb;
            }

        enum
        {
            stream_readable = true,
            stream_writable = false
        };
    };

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

    template<
        typename _CharT,
        typename _Traits = std::char_traits<_CharT>
    >
    class  Ferris_ostream
        :
        public std::basic_ostream< _CharT, _Traits >,
        public virtual Ferris_commonstream< _CharT, _Traits >
    {
        typedef std::basic_ostream< _CharT, _Traits > ostream_t;
        typedef std::basic_streambuf<_CharT, _Traits> sb_t;
        typedef Ferris_commonstream<_CharT, _Traits> _CS;

        typedef Ferris_ostream<_CharT, _Traits>   _Self;
        
//     protected:

//         Ferris_ostream( sb_t* sb )
//             :
//             std::basic_ostream< _CharT, _Traits >( sb )
//             {}
        
    public:

        typedef std::char_traits<_CharT> traits_type;
        typedef typename traits_type::int_type    int_type;
        typedef typename traits_type::char_type   char_type;
        typedef typename traits_type::pos_type    pos_type;
        typedef typename traits_type::off_type    off_type;
        typedef emptystream_methods< _CharT, _Traits > delegating_methods;

        Ferris_ostream()
            :
            std::basic_ostream< _CharT, _Traits >( 0 )
            {
            }

        template<
            class _Alloc,
            class _BufferSizers
        >
        Ferris_ostream(
            ferris_basic_streambuf< char_type, traits_type, _Alloc, _BufferSizers >* streambuf )
            :
            std::basic_ostream< _CharT, _Traits >( 0 )
            {
                this->setsbT( streambuf );
                this->init( _CS::sb );
                this->exceptions( std::ios_base::goodbit );
                this->clear();
            }
        
        Ferris_ostream( const Ferris_ostream& rhs )
            :
            std::basic_ostream< _CharT, _Traits >( 0 )
            {
//            cerr << "Ferris_ostream( const Ferris_ostream& rhs )" << endl; 

                this->setsb( &rhs );
//             LG_IOSTREAM_D << "Ferris_ostream( const Ferris_ostream& rhs )" << endl;
//             LG_IOSTREAM_D << "Ferris_ostream( const Ferris_ostream& rhs ) "
//                           << " rhs.sb:" << (void*)rhs.sb
//                           << " rdbuf:" << (void*)Ferris_commonstream< _CharT, _Traits >::rdbuf()
//                           << endl;
            
                this->init( _CS::sb );
                this->exceptions( std::ios_base::goodbit );
                this->clear( rhs.rdstate() );
                this->copyfmt( rhs );
            }

        Ferris_ostream& operator=( const Ferris_ostream& rhs )
            {
//            cerr << "Ferris_ostream& operator=( const Ferris_ostream& rhs )" << endl;
            
                this->setsb( &rhs );
                this->init( _CS::sb );
                this->exceptions( std::ios_base::goodbit );
                this->clear( rhs.rdstate() );
                this->copyfmt( rhs );

                return *this;
            }

    
        virtual ~Ferris_ostream()
            {
            }


        _Self* operator->()
            {
                return this;
            }

        sb_t* 
        rdbuf() const
            {
                return _CS::sb;
            }
    
        std::basic_ostream< _CharT, _Traits >& operator()()
            {
                return *this;
            }

        enum
        {
            stream_readable = false,
            stream_writable = true
        };


        // FIXME: For StreamHandlableSigEmitter<T> only
//     void StreamHandlableSigEmitter_Setup( sb_t* _sb, sh_t _sh )
//         {
//             this->setsb( _sb, _sh );
//             this->init( _sb );
//             exceptions( std::ios_base::goodbit );
//             this->clear();
//         }
        
    };


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


    FERRISEXP_API void connectStreamBufferClosedSignal(
        std::basic_streambuf< char, std::char_traits<char> >* sb,
        ferris_streambuf< char, std::char_traits<char> >* sh );

//    typedef fh_istream closeSignalStream_t;
    typedef Ferris_istream<char> closeSignalStream_t;
    typedef sigc::signal2< void, closeSignalStream_t&, std::streamsize > closeSignal_t;
    
    FERRISEXP_API closeSignal_t&
    getClosedSignal( ferris_streambuf< char, std::char_traits<char> >* sh );

    
    
//     template <class T>
//     closeSignal_t&
//     getCloseSig( T& stream )
//     {
//         return stream.getCloseSig();
//     }
    
    template <class T>
    class FERRISEXP_API StreamHandlableSigEmitter
//        :
//        public sigc::trackable
    {
    public:

        typedef std::char_traits<char> traits_type;
        typedef typename traits_type::int_type    int_type;
        typedef typename traits_type::char_type   char_type;
        typedef typename traits_type::pos_type    pos_type;
        typedef typename traits_type::off_type    off_type;
        
        typedef StreamHandlableSigEmitter<T> _Self;
    
        typedef T* PointerType;   // type returned by operator->
        typedef T& ReferenceType; // type returned by operator*

//         /**
//          * A signal that is emitted when the object is about to die.
//          */
//         typedef Ferris_istream<char> SigStream_t;
//         typedef sigc::signal2< void, SigStream_t&, std::streamsize > CloseSignal_t;
        
//         class CloseSigHolder : public sigc::trackable
//         {
//             CloseSignal_t CloseSignal;
//             bool m_closeSignalConnected;
//         public:
//             CloseSigHolder()
//                 :
//                 m_closeSignalConnected( false )
//                 {
//                 }
            
//             CloseSignal_t& getCloseSig()
//                 {
//                     m_closeSignalConnected = true;
//                     return CloseSignal;
//                 }
            
//             void EmitClose( Handlable* a );
//         };
    
    public:
    
//    static bool ignoreHookup;

//         typedef std::map< _HandleT*, CloseSigHolder* > m_t;
//         m_t& getMAP();

//        CloseSignal_t& getCloseSig();
//        CloseSigHolder* getCloseSigHolder();
//        void removeCloseSigHolder();

//    void EmitClose( Handlable* a );
    
//         void sb_changed( std::basic_streambuf< char, std::char_traits<char> >* sb,
//                          ferris_streambuf< char, std::char_traits<char> >* sh );
        
    public:
    
        typedef ferris_streambuf< char, std::char_traits<char> > _HandleT;
        closeSignal_t& getCloseSig();
        StreamHandlableSigEmitter( PointerType h );
    };
    

    template<
        typename _CharT,
        typename _Traits = std::char_traits<_CharT>
    >
    class  Ferris_iostream
        :
        public Ferris_ostream< _CharT, _Traits >,
        public Ferris_istream< _CharT, _Traits >,
        public StreamHandlableSigEmitter< Ferris_iostream<_CharT, _Traits> >
    {
        typedef Ferris_iostream<_CharT, _Traits>   _Self;
        typedef Ferris_commonstream<_CharT, _Traits> _CS;

    protected:
    

        typedef std::basic_streambuf<_CharT, _Traits> sb_t;

//         Ferris_iostream( sb_t* sb )
//             :
//             StreamHandlableSigEmitter< Ferris_iostream<_CharT, _Traits> >( this ),
//             Ferris_istream< _CharT, _Traits >( sb ),
//             Ferris_ostream< _CharT, _Traits >( sb )
//             {}
        
    public:

        typedef std::char_traits<_CharT> traits_type;
        typedef typename traits_type::int_type    int_type;
        typedef typename traits_type::char_type   char_type;
        typedef typename traits_type::pos_type    pos_type;
        typedef typename traits_type::off_type    off_type;
        typedef emptystream_methods< _CharT, _Traits > delegating_methods;

        Ferris_iostream()
            :
            StreamHandlableSigEmitter< Ferris_iostream<_CharT, _Traits> >( this )
            {}

        template<
            class _Alloc,
            class _BufferSizers
        >
        Ferris_iostream(
            ferris_basic_streambuf< char_type, traits_type, _Alloc, _BufferSizers >* streambuf )
            :
            StreamHandlableSigEmitter< Ferris_iostream<_CharT, _Traits> >( this )
            {
                this->setsbT( streambuf );
                this->init( _CS::sb );
                this->exceptions( std::ios_base::goodbit );
                this->clear();
            }
        
        Ferris_iostream( sb_t* _sb,
                         typename Ferris::Ferris_iostream<_CharT,_Traits>::sh_t _sh )
            :
            StreamHandlableSigEmitter< Ferris_iostream<_CharT, _Traits> >( this )
            {
//             cerr << "Ferris_iostream( sb_t* _sb, sh_t _sh )" << endl;
//             cerr << "Ferris_iostream( ... ) "
//                  << " sb:" << hex << (void*)_sb
//                  << " sh:" << hex << (void*)_sh
//                  << endl;
            
                this->setsb( _sb, _sh );
                this->init( _sb );
                this->exceptions( std::ios_base::goodbit );
                this->clear();
            }

        Ferris_iostream( const Ferris_iostream& rhs )
            :
            StreamHandlableSigEmitter< Ferris_iostream<_CharT, _Traits> >( this )
            {
                this->setsb( rhs.sb, rhs.sh );
            
//             cerr << "Ferris_iostream( const Ferris_iostream& rhs ) "
//                  << " rhs:" << hex << (void*)&rhs
//                  << " this:" << hex << (void*)this
//                  << " rhs.sb:" << hex << (void*)rhs.sb
//                  << " rhs.sh:" << hex << toVoid(rhs.sh)
//                  << " rdbuf:" << hex << (void*)Ferris_commonstream< _CharT, _Traits >::rdbuf()
//                  << endl;

                this->init( _CS::sb );
                this->exceptions( std::ios_base::goodbit );
                this->clear( rhs.rdstate() );
                this->copyfmt( rhs );
            }
    
        Ferris_iostream& operator=( const Ferris_iostream& rhs )
            {
                this->setsb( &rhs );
                this->init( _CS::sb );
                this->exceptions( std::ios_base::goodbit );
                this->clear( rhs.rdstate() );
                this->copyfmt( rhs );

//             cerr << "Ferris_iostream operator=() "
//                  << " rhs:" << hex << (void*)&rhs 
//                  << " this:" << hex << (void*)this
//                  << " rhs.sb:" << hex << (void*)rhs.sb
//                  << " rhs.sh:" << hex << toVoid(rhs.sh)
//                  << " rdbuf:" << hex << (void*)Ferris_commonstream< _CharT, _Traits >::rdbuf()
//                  << endl;
            
                return *this;
            }

        virtual ~Ferris_iostream()
            {
//            cerr << "~Ferris_iostream() this:" << hex << (void*)this << endl;
            }


        _Self* operator->()
            {
                return this;
            }

        sb_t* 
        rdbuf() const
            {
                return _CS::sb;
            }
    
    
        enum
        {
            stream_readable = true,
            stream_writable = true
        };
    };


    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/

    namespace Priv
    {
        /*
         * This allows the stream close signals to be slot/connected and
         * when they are fired to the stream listeners then the connection
         * can be disconnected()
         */
//        typedef std::map< void*, sigc::connection > conmap_t;
//        typedef std::hash_map< void*, sigc::connection > conmap_t;
        typedef Loki::AssocVector< void*, sigc::connection > conmap_t;
        FERRISEXP_DLLLOCAL conmap_t& getConMAP();

//        typedef std::map< void*, void* > m_t;
//        typedef std::hash_map< void*, void* > m_t;
        typedef Loki::AssocVector< void*, void* > m_t;
        FERRISEXP_DLLLOCAL m_t& getMAP();
        
    };
    
    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/

    template
    <
        typename _CharT,
        typename _Traits = std::char_traits< _CharT >,
        typename _FerrisStreamTraits = i_ferris_stream_traits< _CharT, _Traits >
    >
    class  stringstream_methods
        :
        public _FerrisStreamTraits
    {
        stringstream_methods* delegate;
    
    public:

        typedef _Traits                           traits_type;
        typedef typename traits_type::int_type    int_type;
        typedef typename traits_type::char_type   char_type;
        typedef typename traits_type::pos_type    pos_type;
        typedef typename traits_type::off_type    off_type;

        stringstream_methods( stringstream_methods* _delegate = 0 )
            :
            delegate(_delegate)
            {
            }
    
        void set_delegate_methods_object( stringstream_methods* _delegate = 0 )
            {
                delegate = _delegate;
            }

        virtual std::basic_string<_CharT, _Traits> str() const
            {
                return delegate->str();
            }
    
        virtual void str( const std::basic_string<_CharT, _Traits>& s)
            {
                delegate->str(s);
            }
    
    };


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

    template<
        typename _CharT,
        typename _Traits = std::char_traits < _CharT >,
        typename _Alloc  = std::allocator   < _CharT >
    >
    class  ferris_stringbuf
        :
        public ferris_streambuf<_CharT, _Traits>,
        public std::basic_stringbuf<_CharT, _Traits, _Alloc>
    {
    
        typedef std::basic_stringbuf<_CharT, _Traits, _Alloc> sb;
    
    public:
    
        typedef std::char_traits<_CharT>          traits_type;
        typedef typename traits_type::int_type    int_type;
        typedef typename traits_type::char_type   char_type;
        typedef typename traits_type::pos_type    pos_type;
        typedef typename traits_type::off_type    off_type;
        typedef ferris_streambuf<_CharT, _Traits>           _Base;
        typedef ferris_stringbuf<_CharT, _Traits, _Alloc>   _Self;
        typedef std::basic_string<_CharT, _Traits, _Alloc>  _String;

    
        explicit
        ferris_stringbuf( std::ios_base::openmode m = std::ios_base::in | std::ios_base::out )
            :
            sb(m)
            {
//            cerr << "ferris_stringbuf() this :" << (void*)this << endl;
            }
    
        explicit
        ferris_stringbuf( const _String& s,
                          std::ios_base::openmode m = std::ios_base::in | std::ios_base::out )
            :
            sb(s,m)
            {
//            cerr << "ferris_stringbuf() this :" << (void*)this << endl;
            }
    
        virtual ~ferris_stringbuf()
            {
//            cerr << "~ferris_stringbuf() this :" << (void*)this << endl;
            }
    };

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////



    template<
        typename _CharT,
        typename _Traits = std::char_traits < _CharT >,
        typename _Alloc  = std::allocator   < _CharT >
    >
    class  Ferris_istringstream
        :
        public Ferris_istream<_CharT, _Traits>,
        public stringstream_methods<_CharT, _Traits, i_ferris_stream_traits< _CharT, _Traits > >
    {
        typedef ferris_stringbuf<_CharT, _Traits, _Alloc> ss_impl_t;
        typedef Loki::SmartPtr<  ss_impl_t,
                                 FerrisLoki::FerrisExRefCounted,
                                 Loki::DisallowConversion,
                                 FerrisLoki::FerrisExSmartPointerChecker,
                                 FerrisLoki::FerrisExSmartPtrStorage > ss_t;
        ss_t ss;
        typedef i_ferris_stream_traits< _CharT, _Traits > _FerrisStreamTraits;

    public:

        typedef _Traits                           traits_type;
        typedef typename traits_type::int_type    int_type;
        typedef typename traits_type::char_type   char_type;
        typedef typename traits_type::pos_type    pos_type;
        typedef typename traits_type::off_type    off_type;


        typedef stringstream_methods<
            char_type, traits_type,
            i_ferris_stream_traits< char_type, traits_type > > delegating_methods;

        typedef Ferris_istringstream< _CharT, _Traits, _Alloc > _Self;

        explicit Ferris_istringstream(
            std::ios_base::openmode m = _FerrisStreamTraits::DefaultOpenMode )
            :
            ss( new ss_impl_t(m) )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );
            }

        /*
         * Should this be explicit? or is it ok to allow string -> istream?
         * standard == explicit, maybe I should be more tolerant.
         */
        explicit Ferris_istringstream(
            const std::basic_string<_CharT, _Traits>& s,
            std::ios_base::openmode m = _FerrisStreamTraits::DefaultOpenMode )
            :
            ss( new ss_impl_t( s, m ) )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );
            }

        Ferris_istringstream( const Ferris_istringstream& rhs )
            :
            ss( rhs.ss )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );
            }

        virtual ~Ferris_istringstream()
            {}

        _Self* operator->()
            {
                return this;
            }

        /////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////
    
    
        ss_impl_t*
        rdbuf() const
            {
                return GetImpl(ss);
            }

        std::basic_string<_CharT, _Traits> str() const
            {
                return ss->str();
            }

        void str(const std::basic_string<_CharT, _Traits>& s )
            {
                ss->str(s);
            }

        enum
        {
            stream_readable = true,
            stream_writable = true
        };

    };

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

    template<
        typename _CharT,
        typename _Traits = std::char_traits < _CharT >,
        typename _Alloc  = std::allocator   < _CharT >
    >
    class  Ferris_ostringstream
        :
        public Ferris_ostream<_CharT, _Traits>,
//        public StreamHandlableSigEmitter< Ferris_ostringstream<_CharT, _Traits, _Alloc> >,
        public stringstream_methods<_CharT, _Traits, o_ferris_stream_traits< _CharT, _Traits > >
    {
        typedef ferris_stringbuf<_CharT, _Traits, _Alloc> ss_impl_t;
        typedef Loki::SmartPtr<  ss_impl_t,
                                 FerrisLoki::FerrisExRefCounted,
                                 Loki::DisallowConversion,
                                 FerrisLoki::FerrisExSmartPointerChecker,
                                 FerrisLoki::FerrisExSmartPtrStorage > ss_t;
        ss_t ss;
        typedef o_ferris_stream_traits< _CharT, _Traits > _FerrisStreamTraits;

    public:

        typedef _Traits                            traits_type;
        typedef typename traits_type::int_type     int_type;
        typedef typename traits_type::char_type    char_type;
        typedef typename traits_type::pos_type     pos_type;
        typedef typename traits_type::off_type     off_type;

        typedef stringstream_methods<
            char_type, traits_type,
            o_ferris_stream_traits< char_type, traits_type > > delegating_methods;
        typedef Ferris_ostringstream< _CharT, _Traits, _Alloc > _Self;

        explicit Ferris_ostringstream(
            std::ios_base::openmode m = _FerrisStreamTraits::DefaultOpenMode )
            :
//            StreamHandlableSigEmitter< Ferris_ostringstream<_CharT, _Traits, _Alloc> >( this ),
            ss( new ss_impl_t(m) )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );
            }
    
        explicit Ferris_ostringstream(
            const std::basic_string<_CharT, _Traits>& s,
            std::ios_base::openmode m = _FerrisStreamTraits::DefaultOpenMode )
            :
//            StreamHandlableSigEmitter< Ferris_ostringstream<_CharT, _Traits, _Alloc> >( this ),
            ss( new ss_impl_t( s, m ) )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );
            }

        Ferris_ostringstream( const Ferris_ostringstream& rhs )
            :
//            StreamHandlableSigEmitter< Ferris_ostringstream<_CharT, _Traits, _Alloc> >( this ),
            ss( rhs.ss )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );
            }

        
        virtual ~Ferris_ostringstream()
            {}

        _Self* operator->()
            {
                return this;
            }

        /////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////
    
    
        ss_impl_t*
        rdbuf() const
            {
                return GetImpl(ss);
            }

        std::basic_string<_CharT, _Traits> str() const
            {
                return ss->str();
            }

        void str(const std::basic_string<_CharT, _Traits>& s )
            {
                ss->str(s);
            }

        enum
        {
            stream_readable = true,
            stream_writable = true
        };

    };


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


    template<
        typename _CharT,
        typename _Traits = std::char_traits < _CharT >,
        typename _Alloc  = std::allocator   < _CharT >
    >
    class  Ferris_stringstream
        :
        public Ferris_iostream<_CharT, _Traits>,
//        public StreamHandlableSigEmitter< Ferris_stringstream<_CharT, _Traits, _Alloc> >,
        public stringstream_methods<_CharT, _Traits, io_ferris_stream_traits< _CharT, _Traits > >
    {
        typedef ferris_stringbuf<_CharT, _Traits, _Alloc> ss_impl_t;
        typedef Loki::SmartPtr<  ss_impl_t,
                                 FerrisLoki::FerrisExRefCounted,
                                 Loki::DisallowConversion,
                                 FerrisLoki::FerrisExSmartPointerChecker,
                                 FerrisLoki::FerrisExSmartPtrStorage > ss_t;
        ss_t ss;
        typedef Ferris_commonstream<_CharT, _Traits> _CS;
        typedef io_ferris_stream_traits< _CharT, _Traits > _FerrisStreamTraits;

    public:

        typedef _Traits                           traits_type;
        typedef typename traits_type::int_type    int_type;
        typedef typename traits_type::char_type   char_type;
        typedef typename traits_type::pos_type    pos_type;
        typedef typename traits_type::off_type    off_type;

        typedef stringstream_methods<
            char_type, traits_type,
            io_ferris_stream_traits< char_type, traits_type > > delegating_methods;
        typedef Ferris_stringstream< _CharT, _Traits, _Alloc > _Self;

//         /**
//          * Returns the same streambuffer three times in a row then makes a new one
//          * and repeats
//          */
//         static ss_impl_t* getSS( std::ios_base::openmode m )
//             {
//                 static int x = 0;
//                 static ss_impl_t* ret = 0;
//                 if( !x || x==3 )
//                 {
//                     ret = new ss_impl_t(m);
//                     x = 0;
//                 }
//                 ++x;
//                 return ret;
//             }
        
        
//         explicit Ferris_stringstream(
//             std::ios_base::openmode m = _FerrisStreamTraits::DefaultOpenMode )
//             :
//             ss( getSS(m) ),
//             Ferris_iostream<_CharT, _Traits>( getSS(m) )


// //             ss( new ss_impl_t(m) ),
// //             Ferris_iostream<_CharT, _Traits>( new ss_impl_t(m) )
//             {
// //                ss = dynamic_cast<ss_impl_t*>(rdbuf());
// //                 cerr << " Ferris_stringstream() " << endl;

//                 ss_impl_t* impl = getSS(m);
//                 setsbT( impl );
// //                this->init( impl );

// //                 setsbT( GetImpl(ss) );
// //                this->init( rdbuf() );
//             }

        explicit Ferris_stringstream(
            ss_t& _ss )
            :
            ss( _ss )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );
                this->seekg( 0 );
                this->seekp( 0 );
            }


        explicit Ferris_stringstream(
            std::ios_base::openmode m = _FerrisStreamTraits::DefaultOpenMode )
            :
            ss( new ss_impl_t(m) )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );
            }
        
        explicit Ferris_stringstream(
            const std::basic_string<_CharT, _Traits>& s,
            std::ios_base::openmode m = _FerrisStreamTraits::DefaultOpenMode )
            :
//            StreamHandlableSigEmitter< Ferris_stringstream<_CharT, _Traits, _Alloc> >( this ),
            ss( new ss_impl_t( s, m ) )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );
            }

        Ferris_stringstream( const Ferris_stringstream& rhs )
            :
//            StreamHandlableSigEmitter< Ferris_stringstream<_CharT, _Traits, _Alloc> >( this ),
            ss( rhs.ss )
            {
//                 cerr << "Ferris_stringstream( const Ferris_stringstream& rhs )" << endl;
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );
            }

        
        virtual ~Ferris_stringstream()
            {
//                 cerr << "~Ferris_stringstream() " << endl;
            }


        Ferris_stringstream& operator=( const Ferris_stringstream& rhs )
            {
//                 cerr << "Ferris_stringstream& op = " << endl;

                this->setsb( &rhs );
                this->init( _CS::sb );
                this->exceptions( std::ios_base::goodbit );
                this->clear( rhs.rdstate() );
                this->copyfmt( rhs );
                return *this;
            }

    
        _Self* operator->()
            {
                return this;
            }

    
        /////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////
    
    
        ss_impl_t*
        rdbuf() const
            {
                return GetImpl(ss);
            }

        std::basic_string<_CharT, _Traits> str() const
            {
                return ss->str();
            }

        void str(const std::basic_string<_CharT, _Traits>& s )
            {
                ss->str(s);
            }

        enum
        {
            stream_readable = true,
            stream_writable = true
        };

    };
    

    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/

    template<
        typename _CharT,
        typename _Traits = std::char_traits< _CharT >,
        typename _FerrisStreamTraits = i_ferris_stream_traits< _CharT, _Traits >
    >
    class  filestream_methods
        :
        public _FerrisStreamTraits
    {
        filestream_methods* delegate;
    
    public:

        typedef _Traits                           traits_type;
        typedef typename traits_type::int_type    int_type;
        typedef typename traits_type::char_type   char_type;
        typedef typename traits_type::pos_type    pos_type;
        typedef typename traits_type::off_type    off_type;

        filestream_methods( filestream_methods* _delegate = 0 )
            :
            delegate(_delegate)
            {
            }
    
        void set_delegate_methods_object( filestream_methods* _delegate = 0 )
            {
                delegate = _delegate;
            }

        virtual bool is_open(void)
            {
                return delegate->is_open();
            }

        virtual void open(const char* s, std::ios_base::openmode m = _FerrisStreamTraits::DefaultOpenMode )
            {
                delegate->open( s, m );
            }

        virtual void open( const std::basic_string< _CharT, _Traits >& s,
                           std::ios_base::openmode m = _FerrisStreamTraits::DefaultOpenMode )
            {
                delegate->open( s, m );
            }
    
        virtual void close(void)
            {
                delegate->close();
            }
    };


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


    template<
        typename _CharT,
        typename _Traits = std::char_traits < _CharT >,
        typename _Alloc  = std::allocator   < _CharT >
    >
    class  ferris_filebuf
        :
        public ferris_streambuf<_CharT, _Traits>,
        public FERRIS_STD_BASIC_FILEBUF_SUPERCLASS<_CharT, _Traits>
    {
    
        typedef FERRIS_STD_BASIC_FILEBUF_SUPERCLASS<_CharT, _Traits> sb;
    
    public:
    
        typedef std::char_traits<_CharT>          traits_type;
        typedef typename traits_type::int_type    int_type;
        typedef typename traits_type::char_type   char_type;
        typedef typename traits_type::pos_type    pos_type;
        typedef typename traits_type::off_type    off_type;
        typedef ferris_streambuf<_CharT, _Traits>           _Base;
        typedef ferris_filebuf<_CharT, _Traits, _Alloc>   _Self;
        typedef std::basic_string<_CharT, _Traits, _Alloc>  _String;

    
        explicit
        ferris_filebuf()
            :
            sb()
            {
            }
    
        virtual ~ferris_filebuf()
            {
                this->sync();
            }
    };

    namespace Private 
    {
        FERRISEXP_API void modifyFileBufferForExtendedFlags(
            FERRIS_STD_BASIC_FILEBUF_SUPERCLASS<char, std::char_traits<char> >* fb,
            ferris_ios::openmode m );
    };
    

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


    template<
        typename _CharT,
        typename _Traits = std::char_traits < _CharT >,
        typename _Alloc  = std::allocator   < _CharT >
    >
    class  Ferris_ifstream
        :
        public Ferris_istream<_CharT, _Traits>,
        public filestream_methods<_CharT, _Traits, i_ferris_stream_traits< _CharT, _Traits > >
    {
        typedef ferris_filebuf<_CharT, _Traits, _Alloc> ss_impl_t;
        typedef Loki::SmartPtr<  ss_impl_t,
                                 FerrisLoki::FerrisExRefCounted,
                                 Loki::DisallowConversion,
                                 FerrisLoki::FerrisExSmartPointerChecker,
                                 FerrisLoki::FerrisExSmartPtrStorage > ss_t;
        ss_t ss;

        typedef std::basic_string< _CharT, _Traits > _String;
        typedef i_ferris_stream_traits< _CharT, _Traits > _FerrisStreamTraits;

    public:

        typedef _Traits                           traits_type;
        typedef typename traits_type::int_type    int_type;
        typedef typename traits_type::char_type   char_type;
        typedef typename traits_type::pos_type    pos_type;
        typedef typename traits_type::off_type    off_type;

        typedef filestream_methods<
            _CharT, _Traits,
            i_ferris_stream_traits< _CharT, _Traits > > delegating_methods;

        typedef Ferris_ifstream< _CharT, _Traits, _Alloc > _Self;

        explicit Ferris_ifstream()
            :
            ss( new ss_impl_t() )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );
            }

    
        explicit Ferris_ifstream( const char* s,
                                  std::ios_base::openmode m = _FerrisStreamTraits::DefaultOpenMode )
            :
            ss( new ss_impl_t() )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );

                if ( !ss->open( CleanupURL(s).c_str(),
                                ferris_ios::maskOffFerrisOptions( m ) | std::ios_base::in))
                {
                    this->setstate(std::ios_base::failbit);
                }
                else
                {
                    Private::modifyFileBufferForExtendedFlags( GetImpl(ss), m );
                }
            }
    
        explicit Ferris_ifstream(
            const std::basic_string<_CharT, _Traits>& s,
            std::ios_base::openmode m = _FerrisStreamTraits::DefaultOpenMode )
            :
            ss( new ss_impl_t() )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );

                if ( !ss->open( CleanupURL(s).c_str(),
                                ferris_ios::maskOffFerrisOptions( m ) | std::ios_base::in))
                {
                    this->setstate(std::ios_base::failbit);
                }
                else
                {
                    Private::modifyFileBufferForExtendedFlags( GetImpl(ss), m );
                }
            }

        Ferris_ifstream( const Ferris_ifstream& rhs )
            :
            ss( rhs.ss )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );
            }

        
        virtual ~Ferris_ifstream()
            {}

        _Self* operator->()
            {
                return this;
            }

        /////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////
    
    
        ss_impl_t*
        rdbuf() const
            {
                return GetImpl(ss);
            }


        bool is_open()
            {
                return this->rdbuf()->is_open();
            }

        void open(const char* s, std::ios_base::openmode m = _FerrisStreamTraits::DefaultOpenMode )
            {
                if ( !this->rdbuf()->open( CleanupURL(s).c_str(),
                                           ferris_ios::maskOffFerrisOptions( m ) | std::ios_base::in) )
                {
                    this->setstate(std::ios_base::failbit);
                }
                else
                {
                    Private::modifyFileBufferForExtendedFlags( GetImpl(ss), m );
                }
            }

        void open( const _String& s, std::ios_base::openmode m = _FerrisStreamTraits::DefaultOpenMode )
            {
                if ( !this->rdbuf()->open( CleanupURL(s).c_str(),
                                           ferris_ios::maskOffFerrisOptions( m ) | std::ios_base::in) )
                {
                    this->setstate(std::ios_base::failbit);
                }
                else
                {
                    Private::modifyFileBufferForExtendedFlags( GetImpl(ss), m );
                }
            }

    
        void close()
            {
                if ( !this->rdbuf()->close() )
                {
                    this->setstate(std::ios_base::failbit);
                }
            }    

        enum
        {
            stream_readable = true,
            stream_writable = false
        };
    };

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

    template<
        typename _CharT,
        typename _Traits = std::char_traits < _CharT >,
        typename _Alloc  = std::allocator   < _CharT >
    >
    class  Ferris_ofstream
        :
        public Ferris_ostream<_CharT, _Traits>,
        public filestream_methods<_CharT, _Traits, o_ferris_stream_traits< _CharT, _Traits > >
    {
        typedef ferris_filebuf<_CharT, _Traits, _Alloc> ss_impl_t;
        typedef Loki::SmartPtr<  ss_impl_t,
                                 FerrisLoki::FerrisExRefCounted,
                                 Loki::DisallowConversion,
                                 FerrisLoki::FerrisExSmartPointerChecker,
                                 FerrisLoki::FerrisExSmartPtrStorage > ss_t;
        ss_t ss;

        typedef std::basic_string< _CharT, _Traits > _String;
        typedef o_ferris_stream_traits< _CharT, _Traits > _FerrisStreamTraits;

    public:

        typedef _Traits                           traits_type;
        typedef typename traits_type::int_type    int_type;
        typedef typename traits_type::char_type   char_type;
        typedef typename traits_type::pos_type    pos_type;
        typedef typename traits_type::off_type    off_type;

        typedef filestream_methods<
            _CharT, _Traits,
            o_ferris_stream_traits< _CharT, _Traits > > delegating_methods;

        typedef Ferris_ofstream< _CharT, _Traits, _Alloc > _Self;

        explicit Ferris_ofstream()
            :
            ss( new ss_impl_t() )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );
            }

    
        explicit Ferris_ofstream( const char* s,
                                  std::ios_base::openmode m = _FerrisStreamTraits::DefaultOpenMode )
            :
            ss( new ss_impl_t() )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );

                if ( !ss->open( CleanupURL(s).c_str(),
                                ferris_ios::maskOffFerrisOptions( m ) | std::ios_base::out))
                {
                    this->setstate(std::ios_base::failbit);
                }
                else
                {
                    Private::modifyFileBufferForExtendedFlags( GetImpl(ss), m );
                }
            }
    
        explicit Ferris_ofstream( const _String& s,
                                  std::ios_base::openmode m = _FerrisStreamTraits::DefaultOpenMode )
            :
            ss( new ss_impl_t() )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );

                if ( !ss->open( CleanupURL(s).c_str(),
                                ferris_ios::maskOffFerrisOptions( m ) | std::ios_base::out))
                {
                    this->setstate(std::ios_base::failbit);
                }
                else
                {
                    Private::modifyFileBufferForExtendedFlags( GetImpl(ss), m );
                }
            }

        Ferris_ofstream( const Ferris_ofstream& rhs )
            :
            ss( rhs.ss )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );
            }

    
        virtual ~Ferris_ofstream()
            {}

        _Self* operator->()
            {
                return this;
            }

        /////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////
    
    
        ss_impl_t*
        rdbuf() const
            {
                return GetImpl(ss);
            }


        bool is_open()
            {
                return this->rdbuf()->is_open();
            }

        void open(const char* s, std::ios_base::openmode m = _FerrisStreamTraits::DefaultOpenMode )
            {
                if ( !this->rdbuf()->open( CleanupURL(s).c_str(),
                                           ferris_ios::maskOffFerrisOptions( m ) | std::ios_base::out) )
                {
                    this->setstate(std::ios_base::failbit);
                }
                else
                {
                    Private::modifyFileBufferForExtendedFlags( GetImpl(ss), m );
                }
            }

        void open( const _String& s, std::ios_base::openmode m = _FerrisStreamTraits::DefaultOpenMode )
            {
                if ( !this->rdbuf()->open( CleanupURL(s).c_str(),
                                           ferris_ios::maskOffFerrisOptions( m ) | std::ios_base::out) )
                {
                    this->setstate(std::ios_base::failbit);
                }
                else
                {
                    Private::modifyFileBufferForExtendedFlags( GetImpl(ss), m );
                }
            }

    
        void close()
            {
                if ( !this->rdbuf()->close() )
                {
                    this->setstate(std::ios_base::failbit);
                }
            }    

        enum
        {
            stream_readable = false,
            stream_writable = true
        };
    };

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

    template<
        typename _CharT,
        typename _Traits = std::char_traits < _CharT >,
        typename _Alloc  = std::allocator   < _CharT >
    >
    class  Ferris_fstream
        :
        public Ferris_iostream<_CharT, _Traits>,
        public filestream_methods<_CharT, _Traits, io_ferris_stream_traits< _CharT, _Traits > >
    {
        typedef ferris_filebuf<_CharT, _Traits, _Alloc> ss_impl_t;
        typedef Loki::SmartPtr<  ss_impl_t,
                                 FerrisLoki::FerrisExRefCounted,
                                 Loki::DisallowConversion,
                                 FerrisLoki::FerrisExSmartPointerChecker,
                                 FerrisLoki::FerrisExSmartPtrStorage > ss_t;
        ss_t ss;

        typedef std::basic_string< _CharT, _Traits > _String;
        typedef io_ferris_stream_traits< _CharT, _Traits > _FerrisStreamTraits;

    public:

        typedef _Traits                           traits_type;
        typedef typename traits_type::int_type    int_type;
        typedef typename traits_type::char_type   char_type;
        typedef typename traits_type::pos_type    pos_type;
        typedef typename traits_type::off_type    off_type;

        typedef filestream_methods<
            _CharT, _Traits, io_ferris_stream_traits< _CharT, _Traits > > delegating_methods;

        typedef Ferris_fstream< _CharT, _Traits, _Alloc > _Self;

        explicit Ferris_fstream()
            :
            ss( new ss_impl_t() )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );
            }

    
        explicit Ferris_fstream( const char* s,
                                 ferris_ios::openmode m = _FerrisStreamTraits::DefaultOpenMode )
            :
            ss( new ss_impl_t() )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );

                if ( !ss->open( CleanupURL(s).c_str(),
                                ferris_ios::maskOffFerrisOptions( m ) | std::ios_base::out))
                {
                    this->setstate(std::ios_base::failbit);
                }
                else
                {
                    Private::modifyFileBufferForExtendedFlags( GetImpl(ss), m );
                }
            }
    
        explicit Ferris_fstream( const _String& s,
                                 ferris_ios::openmode m = _FerrisStreamTraits::DefaultOpenMode )
            :
            ss( new ss_impl_t() )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );

                if ( !ss->open( CleanupURL(s).c_str(),
                                ferris_ios::maskOffFerrisOptions( m ) | std::ios_base::out))
                {
                    this->setstate(std::ios_base::failbit);
                }
                else
                {
                    Private::modifyFileBufferForExtendedFlags( GetImpl(ss), m );
                }
            }

        Ferris_fstream( const Ferris_fstream& rhs )
            :
            ss( rhs.ss )
            {
                this->setsbT( GetImpl(ss) );
                this->init( rdbuf() );
            }

    
        virtual ~Ferris_fstream()
            {}

        _Self* operator->()
            {
                return this;
            }

        /////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////
    
    
        ss_impl_t*
        rdbuf() const
            {
                return GetImpl(ss);
            }


        bool is_open()
            {
                return this->rdbuf()->is_open();
            }

        void open(const char* s, ferris_ios::openmode m = _FerrisStreamTraits::DefaultOpenMode )
            {
                if ( !this->rdbuf()->open( CleanupURL(s).c_str(),
                                           ferris_ios::maskOffFerrisOptions( m ) | std::ios_base::out ) )
                {
                    this->setstate(std::ios_base::failbit);
                }
                else
                {
                    Private::modifyFileBufferForExtendedFlags( GetImpl(ss), m );
                }
            }

        void open( const _String& s, ferris_ios::openmode m = _FerrisStreamTraits::DefaultOpenMode )
            {
                if ( !this->rdbuf()->open( CleanupURL(s).c_str(),
                                           ferris_ios::maskOffFerrisOptions( m ) | std::ios_base::out ) )
                {
                    this->setstate(std::ios_base::failbit);
                }
                else
                {
                    Private::modifyFileBufferForExtendedFlags( GetImpl(ss), m );
                }
            }

    
        void close()
            {
                if ( !this->rdbuf()->close() )
                {
                    this->setstate(std::ios_base::failbit);
                }
            }    

        enum
        {
            stream_readable = true,
            stream_writable = true
        };
    };

    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/

    typedef Ferris_istream<char>        f_istream;
    typedef Ferris_ostream<char>        f_ostream;
    typedef Ferris_iostream<char>       f_iostream;
    typedef Ferris_istream<char>        fh_istream;
    typedef Ferris_ostream<char>        fh_ostream;
    typedef Ferris_iostream<char>       fh_iostream;

    typedef Ferris_istringstream<char>  f_istringstream;
    typedef Ferris_ostringstream<char>  f_ostringstream;
    typedef Ferris_stringstream<char>   f_stringstream;
    typedef Ferris_istringstream<char>  fh_istringstream;
    typedef Ferris_ostringstream<char>  fh_ostringstream;
    typedef Ferris_stringstream<char>   fh_stringstream;

    typedef Ferris_ifstream<char>       f_ifstream;
    typedef Ferris_ofstream<char>       f_ofstream;
    typedef Ferris_fstream<char>        f_fstream;
    typedef Ferris_ifstream<char>       fh_ifstream;
    typedef Ferris_ofstream<char>       fh_ofstream;
    typedef Ferris_fstream<char>        fh_fstream;

    typedef Loki::SmartPtr< char,
                            Loki::DestructiveCopy,
                            Loki::DisallowConversion,
                            FerrisLoki::FerrisExSmartPointerChecker,
                            Loki::DefaultSPStorage > fh_char;
    
    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/

    template<
        typename _CharT,
        typename _Traits = std::char_traits< _CharT >
    >
    class  Ferris_ififostream
        :
        public Ferris_ifstream<_CharT,_Traits>
    {
        typedef Ferris_ififostream< _CharT, _Traits > _Self;
        typedef Ferris_ifstream   < _CharT ,_Traits > _Base;
        char FifoFilename[ L_tmpnam ];

//     typedef ferris_filebuf<_CharT, _Traits> ss_impl_t;
//     typedef Loki::SmartPtr<  ss_impl_t,
//                        FerrisRefCounted,
//                        Loki::DisallowConversion,
//                        FerrisSmartPointerChecker,
//                        FerrisSmartPtrStorage > ss_t;
//     ss_t ss;

    
    public:

        Ferris_ififostream()
            {}
    
        Ferris_ififostream(const char *name, int mode=std::ios_base::in )
            :
            Ferris_ifstream<_CharT,_Traits>(name, mode | std::ios_base::in)
            {}

        Ferris_ififostream( fh_istream ss, bool dontCopyLocal = 1 )
            :
            Ferris_ifstream<_CharT,_Traits>()
            {
                const int size = 4096;
                char buf[ size+1 ];

                strcpy( this->FifoFilename, "Ferris_Fifo__XXXXXXXX" );

                tmpnam( this->FifoFilename );
//             int fd = mkstemp( FifoFilename );
            
                {
                    f_ofstream outs( FifoFilename );
                    while( ss.read( buf, size ) )
                    {
                        outs.write( buf, ss->gcount() );
                    }
                }
            
                open( this->FifoFilename );
            }

        _Self* operator->()
            {
                return this;
            }
    

//     Ferris_ififostream( const Ferris_ififostream& rhs )
//         :
//         ss( rhs.ss )
//         {
//             this->setsbT( GetImpl(ss) );
//             this->init( rdbuf() );
//         }

        std::basic_string< _CharT, _Traits > getFileName()
            {
                return FifoFilename;
            }
    
    
        void open(const char *name, std::ios_base::openmode mode=std::ios_base::in )
            {
                Ferris_ifstream< _CharT, _Traits >::open( name,
                                                          (std::ios_base::openmode)(mode | std::ios_base::in));
            }
    };

    typedef Ferris_ififostream<char>            f_ififostream;
    typedef Ferris_ififostream<char>            fh_ififostream;

////typedef FerrisIOStreamPtr< f_ififostream  > fh_ififostream;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


    FERRISEXP_API std::string tostr( long x );
    FERRISEXP_API std::string tostr( fh_char x );
    FERRISEXP_API int         toint( const std::string& s );
    FERRISEXP_API double      todouble( const std::string& s );
    
    FERRISEXP_API const std::string  tostr( std::istringstream& oss );
    FERRISEXP_API const std::string  tostr( std::ostringstream& oss );
    FERRISEXP_API const std::string  tostr( std::stringstream& oss );
    FERRISEXP_API const std::string  tostr( std::istringstream*& oss );
    FERRISEXP_API const std::string  tostr( std::ostringstream*& oss );
    FERRISEXP_API const std::string  tostr( std::stringstream*& oss );

    FERRISEXP_API const std::string  tostr( fh_ostringstream& oss );
    FERRISEXP_API const std::string  tostr( fh_stringstream& oss );

    /* Maybe deprecate these, or rename them to know that only line 1 is returned */
//     FERRISEXP_API const std::string  tostr( fh_istream& oss );
//     FERRISEXP_API const std::string  tostr( fh_iostream& oss );
//     FERRISEXP_API const std::string  tostr( f_istream * oss );
//     FERRISEXP_API const std::string  tostr( f_iostream* oss );

    /**
     * Convert the whole stream into a string.
     */
    FERRISEXP_API const std::string  StreamToString( fh_istream& iss );
    /**
     * Takes a string to place the data into as the last arg.
     * the 'ret' arg is also returned
     */
    FERRISEXP_API std::string&       StreamToString( fh_istream& iss, std::string& ret );
    FERRISEXP_API const std::string  getFirstLine( fh_istream& iss );

    FERRISEXP_API bool  ends_with( const std::string& s, const std::string& ending );
    FERRISEXP_API bool  starts_with( const std::string& s, const std::string& starting );
    FERRISEXP_API bool  starts_with( const std::string& s, const char* starting );
    FERRISEXP_API bool  contains( const std::string& s, const std::string& target );
    FERRISEXP_API int   cmp_nocase( const std::string& s, const std::string& s2 );

    class FERRISEXP_API Nocase 
    {
    public:
        bool operator()( const std::string&, const std::string& ) const;
    };


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


    template <class charT, class _Traits, class _Alloc >
    f_stringstream*  
    operator>> (f_stringstream* is, std::basic_string <charT, _Traits, _Alloc>& s)
    {
        *(is) >> s;
        return is;
    }

    template <class charT, class traits>
    f_istringstream*  
    operator>> (f_istringstream* is, std::basic_string <charT, traits>& s)
    {
        *(is) >> s;
        return is;
    }


#if defined (_STLP_USE_SYMBIAN_IO)
    
    template <class charT>
    Ferris_istream<charT>
    operator>> (Ferris_istream<charT> is, std::basic_string <charT>& s)
    {
        ((std::istream&)is) >> s;
        return is;
    }
    
#endif // defined (_STLP_USE_SYMBIAN_IO)


// template <class _CharT, class _Traits, class _Alloc >
// Ferris_istream<_CharT,_Traits>&
// operator>> (Ferris_istream<_CharT,_Traits>& is, std::basic_string < _CharT, _Traits, _Alloc >& s)
// {
//     std::basic_istream< _CharT, _Traits >& ss = is;
//     ss >> s;
//     return is;
// }



    template <class charT, class traits>
    f_ostream*  
    operator<< (f_ostream* os, const std::basic_string <charT, traits>& s)
    {
        *(os) << s;
        return os;
    }

    template <class charT, class traits>
    f_ostringstream*  
    operator<< (f_ostringstream* os, const std::basic_string <charT, traits>& s)
    {
        *os << s;
        return os;
    }

    template <class charT, class traits>
    f_stringstream*  
    operator<< (f_stringstream* os, const std::basic_string <charT, traits>& s)
    {
        *os << s;
        return os;
    }


#if defined (_STLP_USE_SYMBIAN_IO)

    template <class charT>
    Ferris_ostream<charT>
    operator<< (Ferris_ostream<charT> os, const std::basic_string <charT>& s)
    {
        ((std::ostream&)os) << s;
        return os;
    }

    template <class charT>
    Ferris_ostream<charT>
    operator<< (Ferris_ostream<charT> os, char* s)
    {
        ((std::ostream&)os) << s;
        return os;
    }
    template <class charT>
    Ferris_ostream<charT>
    operator<< (Ferris_ostream<charT> os, const char* s)
    {
        ((std::ostream&)os) << s;
        return os;
    }
        
#endif // defined (_STLP_USE_SYMBIAN_IO)
    

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////



    template <class _CharT, class _Traits>
    inline std::basic_ostream<_CharT, _Traits>&   
    nl(std::basic_ostream<_CharT, _Traits>& __os)
    {
        __os.put(__os.widen('\n'));
        return __os;
    }


    template <class _CharT, class _Traits>
    inline std::basic_ostream<_CharT, _Traits>&   
    nlf(std::basic_ostream<_CharT, _Traits>& __os)
    {
        __os.put(__os.widen('\n'));
        __os.flush();
        return __os;
    }

    template <class _CharT, class _Traits>
    inline std::basic_ostream<_CharT, _Traits>&  
    crnl(std::basic_ostream<_CharT, _Traits>& __os)
    {
        __os.put(__os.widen('\r'));
        __os.put(__os.widen('\n'));
        return __os;
    }

    template <class _CharT, class _Traits>
    inline std::basic_ostream<_CharT, _Traits>&  
    crnlf(std::basic_ostream<_CharT, _Traits>& __os)
    {
        __os.put(__os.widen('\r'));
        __os.put(__os.widen('\n'));
        __os.flush();
        return __os;
    }



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////







    template <class SuperStream = f_ostream>
    class  NullStream : public SuperStream
    {
        typedef SuperStream SuperStream_t;

//    typedef std::strstreambuf streambuf_t;
        typedef null_streambuf<char> streambuf_t;
        mutable streambuf_t sb;

    protected:

    
//     locale pubimbue(const locale& loc)
//         {
//             return loc;
//         }

    public:
    
        NullStream()
            {
                this->init(&sb);
            }

        virtual ~NullStream()
            {
            }

        inline null_streambuf<char>*
        rdbuf() const
            {
                return &sb;
            }
    
    };



    template <class T, class Y>
    inline NullStream<T>&  
    operator<<( NullStream<T>& ss, Y& )
    {
//    cerr << "nullstream op() << " << endl;
        return ss;
    }

//     template <class T, class Y>
//     inline NullStream<T>&
//     operator<<( NullStream<T>& ss, Y )
//     {
//         return ss;
//     }


    template<
        typename _CharT,
        typename _Traits = std::char_traits<_CharT>
    >
    class  Ferris_Logging_ostream
        :
        public std::basic_ostream< _CharT, _Traits >,
        public Ferris_commonstream< _CharT, _Traits >
    {
        typedef std::basic_ostream< _CharT, _Traits > ostream_t;
        typedef std::basic_streambuf<_CharT, _Traits> sb_t;

        typedef Ferris_Logging_ostream<_CharT, _Traits>   _Self;
        typedef Ferris_commonstream<_CharT, _Traits> _CS;

    public:

        typedef std::char_traits<_CharT>          traits_type;
        typedef typename traits_type::int_type    int_type;
        typedef typename traits_type::char_type   char_type;
        typedef typename traits_type::pos_type    pos_type;
        typedef typename traits_type::off_type    off_type;
        typedef emptystream_methods< _CharT, _Traits > delegating_methods;

        explicit Ferris_Logging_ostream()
            :
            std::basic_ostream< _CharT, _Traits >( 0 )
            {
            }

//null_streambuf<char>
        explicit Ferris_Logging_ostream( sb_t* sb )
            :
            std::basic_ostream< _CharT, _Traits >( 0 )
            {
//            this->setsb( _sb, _sh );
                this->init( sb );
                this->exceptions( std::ios_base::goodbit );
                this->clear();
            }
    
    
    
//     Ferris_ostream( const Ferris_ostream& rhs )
//         :
//         std::basic_ostream< _CharT, _Traits >( 0 )
//         {
// //            cerr << "Ferris_ostream( const Ferris_ostream& rhs )" << endl; 

//             this->setsb( &rhs );
// //             LG_IOSTREAM_D << "Ferris_ostream( const Ferris_ostream& rhs )" << endl;
// //             LG_IOSTREAM_D << "Ferris_ostream( const Ferris_ostream& rhs ) "
// //                           << " rhs.sb:" << (void*)rhs.sb
// //                           << " rdbuf:" << (void*)Ferris_commonstream< _CharT, _Traits >::rdbuf()
// //                           << endl;
            
//             this->init( sb );
//             this->exceptions( std::ios_base::goodbit );
//             clear( rhs.rdstate() );
//             copyfmt( rhs );
//         }


    
        virtual ~Ferris_Logging_ostream()
            {
            }


        _Self* operator->()
            {
                return this;
            }

        std::basic_streambuf<_CharT, _Traits>* rdbuf() const
            {
                return _CS::sb;
            }

        void rdbuf(std::basic_streambuf<_CharT, _Traits>* x)
            {
                _CS::sb = x;
                ostream_t::rdbuf(x);
            }
    

        enum
        {
            stream_readable = false,
            stream_writable = true
        };
    };


    typedef Ferris_Logging_ostream<char>        fhl_ostream;
    

    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/

    namespace Factory
    {
        FERRISEXP_API fh_istream MakeLimitingIStream( fh_istream ss, std::streampos be, std::streampos en );


    
        FERRISEXP_API fh_istream  MakeProxyStream( std::istream* base,  bool takeOwnerShip = false );
        FERRISEXP_API fh_iostream MakeProxyStream( std::iostream* base, bool takeOwnerShip = false );

        FERRISEXP_API fh_ostream MakeTeeStream( fh_ostream oss1, fh_ostream oss2 );
        // Note that the return value is only an ostream, this is
        // here for libferris to use for getIOStream()
        FERRISEXP_API fh_iostream MakeTeeIOStream( fh_ostream oss1, fh_ostream oss2 );
    };
    
    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/

    template <class T>
    closeSignal_t& 
    StreamHandlableSigEmitter<T>::getCloseSig()
    {
        T* baseType = (T*)this;
        
        _HandleT* h = GetImpl(baseType->sh);
        if( h )
        {
            /* Streambuffer is already setup, just connect */
            connectStreamBufferClosedSignal( baseType->sb, h );

//             cerr << "StreamHandlableSigEmitter<T>::getCloseSig(1)"
//                  << " this:" << (void*)this
//                  << " h:" << (void*)h
//                  << endl;
        }
        else
        {
            /* Delay connection to streambuffer until it is set */
            baseType->m_sb_changed_callback_used = true;
            baseType->sb_changed_callback =
                Ferris_commonstream<char>::sb_changed_callback_t( connectStreamBufferClosedSignal );

//             cerr << "StreamHandlableSigEmitter<T>::getCloseSig(2)"
//                  << " this:" << (void*)this
//                  << " h:" << (void*)h
//                  << endl;
        }

        return getClosedSignal( h );
    }
    

    template <class T> 
    StreamHandlableSigEmitter<T>::StreamHandlableSigEmitter( PointerType h )
    {
//         cerr << "StreamHandlableSigEmitter<T>::StreamHandlableSigEmitter(v2) h:"
//              << hex << (void*)h 
//              << " this:" << (void*)this
//              << endl;
        
        
// //    if( !ignoreHookup )
//         {
//             h->getStreamBufferChangedSig().connect( slot(this, &_Self::sb_changed) );
//         }
    
    }


    namespace Factory
    {
        FERRISEXP_API fh_istream  fcin();
        FERRISEXP_API fh_ostream  fcout();
        FERRISEXP_API fh_ostream  fcerr();
        FERRISEXP_API fh_ostream  fcnull();

        /**
         * By default the fd is not closed.
         */
        FERRISEXP_API fh_istream   MakeFdIStream( int fd );
        FERRISEXP_API fh_ostream   MakeFdOStream( int fd );
        FERRISEXP_API fh_iostream  MakeFdIOStream( int fd );
        FERRISEXP_API fh_istream   MakeFdIStream( int fd, bool closeFD );
        FERRISEXP_API fh_ostream   MakeFdOStream( int fd, bool closeFD );
        FERRISEXP_API fh_iostream  MakeFdIOStream( int fd, bool closeFD );
        
        FERRISEXP_API fh_ostream   MakeHoleyOStream( fh_ostream ss, const int blocksize = 4096 );

        FERRISEXP_API fh_istream   MakeMemoryIStream ( void* data, const int data_size );
        FERRISEXP_API fh_iostream  MakeMemoryIOStream( void* data, const int data_size );

        /**
         * This method buys you no write access to the stream. It is only here
         * for cases where you are sometimes wanting to write to the stream
         * and sometimes not. If you are not looking to write but still want
         * to use a fh_iostream to hold the reference to the stream then you
         * can use this method to get a wrapper which is still a read only
         * stream but will happily sit in a fh_iostream type.
         */
        FERRISEXP_API fh_iostream MakeReadOnlyIOStream( fh_istream ss );
        

        /**
         * Make a istream for a memory mapped version of fd
         *
         * fd is the file descriptor to memory map.
         * @param m describes extra information relating to the openmode of fd and
         * how to memory map fd. If ferris_ios::mseq is set then madvise is done to
         * tell the kernel about access patterns.
         * @param pathdesc if provided is used to embed a URL into exceptions that are thrown.
         */
        FERRISEXP_API fh_istream
        MakeMMapIStream( int fd,
                         ferris_ios::openmode m = std::ios::in,
                         const std::string pathdesc = "" );
        
        
        /**
         * Make a iostream for a memory mapped version of fd
         *
         * Note that the memory map automatically extends itself if you write past
         * end of file. The file is extended in largish chunks and remaped for you.
         *
         * fd is the file descriptor to memory map.
         * @param m describes extra information relating to the openmode of fd and
         * how to memory map fd. If ferris_ios::mseq is set then madvise is done to
         * tell the kernel about access patterns.
         * @param pathdesc if provided is used to embed a URL into exceptions that are thrown.
         */
        FERRISEXP_API fh_iostream
        MakeMMapIOStream( int fd,
                          ferris_ios::openmode m = std::ios::in | std::ios::out,
                          const std::string pathdesc = "" );
        
    };

//     template <class R>
//     R toType( const std::string& s )
//     {
//         R ret;
//         fh_stringstream ss(s);
//         ss >> ret;
//         return ret;
//     }
    template < class R >
    struct  toTypeSwitch
    {
        static R toType( const std::string& s )
            {
//                 R ret;
//                 ::Ferris::fh_stringstream ss(s);
//                 ss >> ret;
//                 return ret;

                R ret;
                std::stringstream ss(s);
                ss >> ret;
                return ret;
            }
    };
    template <>
    struct  toTypeSwitch< std::string >
    {
        static std::string toType( const std::string& s )
            {
                return s;
            }
    };
    
    template < class R >
    inline R  toType( const std::string& s )
    {
        return toTypeSwitch<R>::toType( s );
    }

//     template < class R >
//     R toType( fh_istream ss )
//     {
//         R ret;
//         ss >> ret;
//         return ret;
//     }


    template < class R >
    struct  toTypeSwitchStream
    {
        static R toType( fh_istream ss )
            {
                R ret;
                ss >> ret;
                return ret;
            }
    };
    template <>
    struct  toTypeSwitchStream< std::string >
    {
        static std::string toType( fh_istream ss )
            {
                std::string ret;
                getline( ss, ret );
                return ret;
            }
    };
    
    template < class R >
    inline R  toType( fh_istream ss )
    {
        return toTypeSwitchStream<R>::toType( ss );
    }

    
    template <class T>
    std::string  toString( T x )
    {
        fh_stringstream ss;
        ss << x;
        return tostr(ss);
    }


    /**
     * The following template is based on the one_arg_manip<> code
     * from the "Standard C++ IOStreams and Locales" book.
     */
    template < class Argument >
    class  one_arg_manip
    {
    public:
        typedef void (*manipFct)(std::ios_base&, Argument);

        one_arg_manip( manipFct pf, const Argument& arg )
            : pf_(pf), arg_(arg) 
            {}

    private:

        manipFct pf_;
        const Argument arg_;

        template < class CharT, class Traits >
        friend std::basic_istream< CharT, Traits >& operator>>
        (std::basic_istream< CharT, Traits >& is, const one_arg_manip& oam )
            {
                if( !is.good()) return is;
                (*(oam.pf_))( is, oam.arg_ );
                return is;
            }

        template < class CharT, class Traits >
        friend std::basic_ostream< CharT, Traits >& operator<<
        (std::basic_ostream< CharT, Traits >& os, const one_arg_manip& oam )
            {
                if( !os.good()) return os;
                (*(oam.pf_))( os, oam.arg_ );
                return os;
            }
    };

    /**
     * dump the range first to last-1 to the stream os in the given radix
     *
     * @param os Stream to dump output to
     * @param first like STL begin
     * @param last  like STL end
     * @param radix one of 2,8,10,16
     * @param consoleWidth width of line before a new line is created
     * @param seperate a string of commaGaps bytes with a comma and space
     */
    template <class Stream, class InputIterator>
    Stream  
    radixdump( Stream os, InputIterator first, InputIterator last,
               long radix=16,
               int consoleWidth = 80,
               int commaGaps = 0 )
    {
        int i=-1;
        int bytesWrittenToLine = 0;
        int symbolWidth = 2;
        bool startOfSymbol = true;
        
        for( ; first != last; ++i )
        {
            if( commaGaps && !startOfSymbol && i % commaGaps == commaGaps - 1 )
            {
                os << ", " << std::flush;
                bytesWrittenToLine += 2;
                startOfSymbol = true;
            }

            if( bytesWrittenToLine >= consoleWidth ||
                (startOfSymbol && commaGaps < consoleWidth
                 && bytesWrittenToLine + symbolWidth * commaGaps > consoleWidth))
            {
                bytesWrittenToLine=0;
                i=-1;
                os << std::endl;
                startOfSymbol = true;
            }

            bytesWrittenToLine += symbolWidth;
            os << std::setw(2)
               << std::setbase(radix)
               << std::setfill('0')
               << (int)*first
               << std::flush;
            
            startOfSymbol = false;
            ++first;
        }
        os << std::flush;
//        os << std::endl;
        return os;
    }


    
    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/
    
};
#endif
