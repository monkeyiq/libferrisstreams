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

    $Id: Streams.cpp,v 1.17 2010/11/14 21:46:19 ben Exp $

*******************************************************************************
*******************************************************************************
******************************************************************************/

#define BUILDING_FERRISSTREAMS

#include <FerrisStreams/Streams.hh>
#include <FerrisStreams/Exceptions.hh>
#include <FerrisStreams/Shell.hh>
#include <FerrisStreams/FerrisPosix.hh>
#include "config.h"

#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>

using namespace std;

#ifndef MREMAP_MAYMOVE
#define MREMAP_MAYMOVE 0
#endif

namespace Ferris
{
    
    std::string ferris_g_get_home_dir()
    {
        string ret = "";

        struct passwd *pw = NULL;
        void* buffer = NULL;
        struct passwd pwd;
        long bufsize = sysconf (_SC_GETPW_R_SIZE_MAX);
	
        if (bufsize < 0)
            bufsize = 64;
        bufsize += 6;

        buffer = (void*) malloc (bufsize + 6);
        errno = 0;
	    
	    int error = getpwuid_r (getuid (), &pwd, (char*)buffer, bufsize, &pw);
        error = error < 0 ? errno : error;

        if( !pw )
        {
            if (error == 0 || error == ENOENT)
            {
                free( buffer );
                buffer = 0;
                stringstream ss;
                ss << "getpwuid_r(): failed due to unknown user id:" << getuid()
                   << " bufsize:" << bufsize
                   << endl;
                {
                    ofstream oss("/tmp/ferr1");
                    oss << ss.str();
                }
            
                StreamThrowFromErrno( error, tostr(ss) );
            }
//            if (bufsize > 32 * 1024)
            {
                free( buffer );
                buffer = 0;
                stringstream ss;
                ss << "getpwuid_r(): failed due to:" << errnum_to_string( "", error ) << endl;
                
                StreamThrowFromErrno( error, tostr(ss) );
                {
                    ofstream oss("/tmp/ferr2");
                    oss << ss.str();
                }
            }
        }
        
        ret = pw->pw_dir;
        
        return ret;
    }

    static char* gnu_getcwd()
    {
        size_t size = 800;
        
        while (1)
        {
            char *buffer = (char *) malloc (size);
            if (getcwd (buffer, size) == buffer)
                return buffer;
            free (buffer);
            if (errno != ERANGE)
                return 0;
            size *= 2;
        }
     }

    
    static std::string getCWDDirPath()
    {
//        char* cdir = g_get_current_dir();
        char* cdir = gnu_getcwd();
        string ret = cdir;
        free(cdir);
        return ret;
    }
    
    std::string CleanupURL( const std::string& s )
    {
        return CleanupURL( s, true, false );
    }
    
    std::string  CleanupURL( const std::string& s,
                             bool stripFileSchemePrefix,
                             bool leaveURIConstructsIntact )
    {
        string ret = s;
        
        if( !s.length() )
        {
            fh_stringstream ss;
            ss << "Zero length URL given.";
            Throw_MalformedURL( tostr(ss), 0 );
        }
        if( s[0] == '~' )
        {
            ret = "file://";
            ret += ferris_g_get_home_dir();
            ret += "/";

            
            if( string::npos != s.find("/") )
            {
                ret += s.substr( s.find("/") );
            }
        }
        else if( s[0] == '.' )
        {
            ret = "file://";
            ret += getCWDDirPath();
            ret += "/";
            ret += s;
        }
        else if( s[0] == '/' )
        {
            ret = "file://";
            ret += s;
        }
        else if( string::npos == s.find(":") )
        {
            ret = "file://";
            ret += getCWDDirPath();
            ret += "/";
            ret += s;
        }

        if( !leaveURIConstructsIntact )
        {
            
            int loc = ret.find("/./");
            while( loc != string::npos )
            {
                fh_stringstream ss;
                ss << ret.substr( 0, loc ) << "/" << ret.substr( loc+3 );
                ret = tostr(ss);
                loc = ret.find("/./");
            }
        
            // Remove unescaped '//' to be '/'
            {
                int loc = ret.find("//");
                while( loc != string::npos )
                {
                    fh_stringstream ss;
                    ss << ret.substr( 0, loc ) << "/" << ret.substr( loc+2 );
                    ret = tostr(ss);
                    loc = ret.find("//");
                }
            }
        }
        
        if( stripFileSchemePrefix && starts_with( ret, "file:" ))
        {
            ret = ret.substr( 5 );
        }

        return ret;
            
    }
    
    
    namespace Private 
    {
        void 
        modifyFileBufferForExtendedFlags(
            FERRIS_STD_BASIC_FILEBUF_SUPERCLASS<char, std::char_traits<char> >* fb,
            ferris_ios::openmode m )
        {
#if !defined (_STLP_USE_SYMBIAN_IO) && defined(O_DIRECT)
//            cerr << "modifyFileBufferForExtendedFlags() m:" << m << endl;
            if( m & ferris_ios::o_direct )
            {
                int fd = fb->fd();

                cerr << "modifyFileBufferForExtendedFlags() "
                     <<" setting O_DIRECT on fd:" << fd << endl;

                int rc = fcntl( fd, F_SETFL, O_DIRECT );
                if( rc )
                {
                    int e = errno;
//                     cerr << "Can not set O_DIRECT for fd:" << fd
//                          << " reason:" << errnum_to_string( "", e ) << endl;
                }
            }
#endif
        }
    };

    namespace Priv
    {
        conmap_t& getConMAP()
        {
            static conmap_t m;
            return m;
        }

        m_t& getMAP()
        {
            static m_t m;
            return m;
        }
    };

    

    /**
     * Class holding per streambuffer signal that clients can register interest
     * on to be informed when the streambuffer is dying.
     */ 
    class FERRISEXP_DLLLOCAL CloseSigHolder : public sigc::trackable
    {
    public:
        closeSignal_t CloseSignal;
        bool m_closeSignalConnected;

        CloseSigHolder()
            :
            m_closeSignalConnected( false )
            {
            }
            
        closeSignal_t& getCloseSig()
            {
                m_closeSignalConnected = true;
                return CloseSignal;
            }
    };
    

    
    /**
     * Return the current, or create, an entry for sh in a lookup table to a
     * SigC::signal
     *
     * FD: sh -> signal
     */
    closeSignal_t&  
    getClosedSignal( ferris_streambuf< char, std::char_traits<char> >* sh )
    {
        if( !sh->m_sigHolder )
        {
            sh->m_sigHolder = new CloseSigHolder();
        }
        return sh->m_sigHolder->getCloseSig();
        
        
//         Priv::m_t& m = Priv::getMAP();

// //         cerr << "getClosedSignal() adding to private map for signal marshalling"
// //              << " sh:" << (void*)sh
// //              << endl;
        
//         Priv::m_t::iterator iter = m.find( sh );
//         if( iter == m.end() )
//         {
//             iter = m.insert( make_pair( sh, new CloseSigHolder() )).first;
//         }

//         CloseSigHolder* csh = static_cast<CloseSigHolder*>( iter->second );
//         return csh->getCloseSig();
    }

    /**
     * If anyone has registered to hear about the streambuffer dying then
     * signal them, and cleanup the map used by getClosedSignal.
     */
    void OnGenericStreamClosed( FerrisLoki::Handlable* a )
    {
        typedef std::basic_streambuf<char> _StreamBuf;
        typedef ferris_streambuf< char, std::char_traits<char> > _HandleT;
        
        if( _StreamBuf* basic_sb = dynamic_cast<_StreamBuf*>(a) )
        {
            if( _HandleT* h = dynamic_cast<_HandleT*>(a) )
            {
                if( CloseSigHolder* csh = h->m_sigHolder )
                {
                    if( csh->m_closeSignalConnected )
                    {
                        fh_iostream ss( basic_sb, (ferris_streambuf<char>*)(a));
                        csh->CloseSignal.emit( ss, ss.tellp() );
                    }
                }
                
//                 if( !Priv::getConMAP().empty() &&
//                     Priv::getConMAP().find(h) != Priv::getConMAP().end() )
//                 {
//                     Priv::getConMAP()[h].disconnect();
//                     Priv::getConMAP().erase(h);
//                 }

//                 Priv::m_t::iterator iter = Priv::getMAP().find(a);
//                 if( iter != Priv::getMAP().end() )
//                 {
//                     CloseSigHolder* csh = static_cast<CloseSigHolder*>( iter->second );
                    
//                     if( csh->m_closeSignalConnected )
//                     {
//                         fh_iostream ss( basic_sb, (ferris_streambuf<char>*)(a));
//                         csh->CloseSignal.emit( ss, ss.tellp() );
//                     }
                    
//                     Priv::getMAP().erase( iter );
//                 }
            }
        }
    }



    /**
     * Connect a marshalling function to the streambuffers generic close
     * signal. Only one marshaller is attached to any streambuffer at any time
     * and all calls to StreamHandlableSigEmitter::getCloseSig() should return
     * the same signal of the streambuffer.
     */
    void  
    connectStreamBufferClosedSignal(
        std::basic_streambuf< char, std::char_traits<char> >* sb,
        ferris_streambuf< char, std::char_traits<char> >* sh )
    {
//         cerr << "connectStreamBufferClosedSignal() sh:" << (void*)sh
//              << " connections:" << Priv::getConMAP().size()
//              << " sh->emitter_attached:" << sh->emitter_attached
//              << endl;
        
        if( !sh->emitter_attached )
        {
            sh->emitter_attached = true;

//             cerr << "connectStreamBufferClosedSignal() sh:" << (void*)sh
//                  << " connections:" << Priv::getConMAP().size()
//                  << endl;

            FerrisLoki::Handlable::GenericCloseSignal_t& sig = sh->getGenericCloseSig();
//            sigc::connection con = sig.connect( sigc::slot(OnGenericStreamClosed) );
            sigc::connection con = sig.connect( sigc::ptr_fun(OnGenericStreamClosed) );

//            Priv::getConMAP()[sh] = con;
        }
    }
    
    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/

    
    string tostr( long x )
    {
        ostringstream oss;
        oss << x;
        return tostr( oss );
    }

    
    string  tostr( fh_char x )
    {
        return string( GetImpl(x) );
    }

    int  toint( const std::string& s )
    {
        return toType<int>( s );
    }

    double  todouble( const std::string& s )
    {
        return toType<double>( s );
    }




    const string  tostr( istringstream& oss )  { return oss.str(); }
    const string  tostr( ostringstream& oss )  { return oss.str(); }
    const string  tostr(  stringstream& oss )  { return oss.str(); }
    const string  tostr( istringstream*& oss )  { return oss->str(); }
    const string  tostr( ostringstream*& oss )  { return oss->str(); }
    const string  tostr(  stringstream*& oss )  { return oss->str(); }
    const string  tostr( fh_ostringstream& oss ) { return oss->str(); }
    const string  tostr( fh_stringstream& oss ) { return oss->str(); }

    const std::string  StreamToString( fh_istream& iss )
    {
        fh_stringstream ss;
        iss.clear();
//    iss.seekg(0);
        std::copy( std::istreambuf_iterator<char>(iss),
                   std::istreambuf_iterator<char>(),
                   std::ostreambuf_iterator<char>(ss));
        return tostr(ss);
    }

    std::string&  StreamToString( fh_istream& iss, std::string& ret )
    {
        iss.clear();
        ret.clear();
        char ch = 0;
        std::copy( std::istreambuf_iterator<char>(iss),
                   std::istreambuf_iterator<char>(),
                   std::back_inserter( ret ) );
        return ret;
    }
    
    

    const std::string  getFirstLine( fh_istream& ss )
    {
        string s;
        getline( ss, s);
        return s;
    }



// const string tostr( fh_istream& oss )     
// {
//     string s; getline( oss, s); return s;
// }
// const string tostr( fh_iostream& oss )
// {
//     string s; getline( oss, s); return s;
// }


// const string tostr( f_istream* oss )
// {
//     string s; getline( *oss, s); return s;
// }
// const string tostr( f_iostream* oss )
// {
//     string s; getline( *oss, s); return s;
// }




bool  ends_with( const string& s, const string& ending )
{
    if( ending.length() > s.length() )
        return false;
    
    return s.rfind(ending) == (s.length() - ending.length());
}

bool  starts_with( const string& s, const string& starting )
{
    int starting_len = starting.length();
    int s_len = s.length();

    if( s_len < starting_len )
        return false;
    
    return !s.compare( 0, starting_len, starting );
}

    bool  starts_with( const std::string& s, const char* starting )
    {
        int starting_len = strlen( starting );
        int s_len = s.length();
        if( s_len < starting_len )
            return false;
    
        return !s.compare( 0, starting_len, starting );
    }
    
    

/**
 * check if the string 's' contains the target string
 */
bool  contains( const std::string& s, const std::string& target )
{
    int pos = s.find( target );
    return pos != string::npos;
}

int  cmp_nocase( const string& s, const string& s2 )
{
    string::const_iterator p  =  s.begin();
    string::const_iterator p2 = s2.begin();

    while( p != s.end() && p2 != s2.end())
    {
        if( toupper( *p ) != toupper( *p2 ) )
        {
            return toupper( *p ) < toupper( *p2 )
                ? -1
                :  1;
        }
        ++p;
        ++p2;
    }

    return s2.size() - s.size();
}


bool
Nocase::operator()( const string& x, const string& y ) const
{
    string::const_iterator p = x.begin();
    string::const_iterator q = y.begin();

    while( p != x.end() && q != y.end() && toupper(*p) == toupper(*q) )
    {
        ++p;
        ++q;
    }

    if( p == x.end() ) return q != y.end();
    return toupper(*p) < toupper(*q);
}
    

    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/

template<
    class _CharT,
    class _Traits = std::char_traits < _CharT >,
    class _Alloc  = std::allocator   < _CharT >,
    class _BufferSizers = ferris_basic_streambuf_fourk
    >
class FERRISEXP_DLLLOCAL basic_limiting_streambuf
    :
    public ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers >
{
    typedef basic_limiting_streambuf< _CharT, _Traits, _Alloc, _BufferSizers > _Self;
    typedef ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers > _Base;
    
public:
    
    typedef char_traits<_CharT>    traits_type;
    typedef typename traits_type::int_type  int_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::pos_type  pos_type;
    typedef typename traits_type::off_type  off_type;
    typedef typename _Base::seekd_t seekd_t;

    basic_limiting_streambuf( fh_istream ss, streampos be, streampos en )
        :
        RealStream(ss),
        FakeBegin(be),
        FakeEnd(en)
        {
        }

    virtual ~basic_limiting_streambuf()
        {
        }
    
private:

    fh_istream RealStream;
    streampos  FakeBegin;
    streampos  FakeEnd;

    
    // prohibit copy/assign
    basic_limiting_streambuf( const basic_limiting_streambuf& );
    basic_limiting_streambuf& operator = ( const basic_limiting_streambuf& );

    


protected:

    /**
     * This is the only methods that really needs to be here. It gets
     * up to maxsz data into buffer and returns how much data was really
     * read. Return 0 for a failure, you must read atleast one byte.
     */
    virtual int make_new_data_avail( char_type* buffer, streamsize maxsz )
        {
            if( static_cast<int>(FakeEnd) != traits_type::eof() )
            {
//              streamsize p = RealStream->tellg();  //0.3.0
                streampos p = RealStream->tellg();

                
                if( (maxsz+p) > FakeEnd )
                {
                    maxsz = FakeEnd - p;
                }
            }
            
            RealStream->read( buffer, maxsz );
//            return RealStream->gcount();

            streamsize gcount = RealStream->gcount();
//             cerr << "basic_limiting_streambuf() gcount:" << gcount
//                  << " buffer:" << toVoid(buffer)
//                  << endl;
//             cerr << "DATA:"; copy( buffer, buffer+gcount, std::ostreambuf_iterator<char>(cerr) );
//             cerr << endl;
            return gcount;
        }

    virtual pos_type
    seekoff(off_type offset, seekd_t d, std::ios_base::openmode m)
        {
            streampos cloc = -1;
            
            if( m == ios::in ) {cloc = RealStream->tellg();}
//            else               {cloc = RealStream->tellp();}

            if( static_cast<int>(cloc) == -1 )
            {
                return this->have_been_seeked( pos_type(off_type(-1)));
            }
            
            switch( d )
            {
            case ios::cur:
                if( cloc + offset > FakeEnd
                    || cloc + offset < FakeBegin )
                {
                    return this->have_been_seeked( pos_type(off_type(-1)));
                }
                break;
                
            case ios::beg:
                offset += FakeBegin;
                if( offset > FakeEnd )
                {
                    return this->have_been_seeked( pos_type(off_type(-1)));
                }
                break;
                
            case ios::end:
                offset -= FakeEnd;
                if( offset < FakeBegin )
                {
                    return this->have_been_seeked( this->have_been_seeked(pos_type(off_type(-1))));
                }
                break;

            default:
                return this->have_been_seeked( pos_type(off_type(-1)));
            }
            
            return this->have_been_seeked( RealStream->rdbuf()->pubseekoff( offset, d, m ) );
        }

    virtual pos_type
    seekpos(pos_type pos, std::ios_base::openmode m)
        {
            streamsize realpos = FakeBegin + pos;
            
            if( realpos < FakeBegin
                || realpos > FakeEnd
                )
                return this->have_been_seeked( pos_type(off_type(-1)));

            return _Self::have_been_seeked( RealStream->rdbuf()->pubseekpos( realpos, m ));

            return this->have_been_seeked( 
                _Base::seekpos( pos, m ) );
        }

    
    
};


    /**
     * Base class for istreams which limit the start and optionally end of the underlying
     * stream to a given subsize.
     */
    template<
        class _StreamBufClass,
        class _CharT,
        class _Traits = std::char_traits<_CharT>
    >
    class FERRISEXP_DLLLOCAL common_ferris_x_istream
        :
        public Ferris_istream< _CharT, _Traits >,
        public i_ferris_stream_traits< _CharT, _Traits >
    {
        typedef common_ferris_x_istream<_CharT, _Traits>    _Self;
    protected:
        typedef _StreamBufClass _StreamBuf;
    
        typedef _StreamBuf ss_impl_t;
        typedef Loki::SmartPtr<  ss_impl_t,
                                 FerrisLoki::FerrisExRefCounted,
                                 Loki::DisallowConversion,
                                 Loki::AssertCheck,
                                 FerrisLoki::FerrisExSmartPtrStorage > ss_t;
        ss_t ss;

    public:
    
        typedef char_traits<_CharT>    traits_type;
        typedef typename traits_type::int_type  int_type;
        typedef typename traits_type::char_type char_type;
        typedef typename traits_type::pos_type  pos_type;
        typedef typename traits_type::off_type  off_type;

        typedef emptystream_methods< char_type, traits_type > delegating_methods;

        explicit
        common_ferris_x_istream( const ss_t& ss )
            :
            ss( ss )
            {
                this->init( rdbuf() );
                this->setsbT( GetImpl(ss) );
            }
        
        common_ferris_x_istream( const common_ferris_x_istream& rhs )
            :
            ss( rhs.ss )
            {
                this->init( rdbuf() );
                this->setsbT( GetImpl(ss) );
            }

    
        virtual ~common_ferris_x_istream()
            {
            }
    
        _Self* operator->()
            {
                return this;
            }

        ss_impl_t*
        rdbuf() const
            {
                return GetImpl(ss);
            }

        enum
        {
            stream_readable = true,
            stream_writable = false
        };
    };

    
    /**
     * Basically you can get a start/end offset in a bigger istream
     * and have a restricted istream :)
     */
    template<
        class _CharT,
        class _Traits = std::char_traits<_CharT>
    >
    class FERRISEXP_DLLLOCAL ferris_limiting_istream
        :
        public common_ferris_x_istream< basic_limiting_streambuf<_CharT, _Traits>,
                                               _CharT, _Traits >
    {
        typedef common_ferris_x_istream< basic_limiting_streambuf<_CharT, _Traits>,
                                                _CharT, _Traits > _Base;
        typedef ferris_limiting_istream<_CharT, _Traits>    _Self;

    public:
    
        typedef char_traits<_CharT>    traits_type;
        typedef typename traits_type::int_type  int_type;
        typedef typename traits_type::char_type char_type;
        typedef typename traits_type::pos_type  pos_type;
        typedef typename traits_type::off_type  off_type;
        typedef emptystream_methods< char_type, traits_type > delegating_methods;
        typedef typename _Base::ss_impl_t ss_impl_t;
    
        explicit
        ferris_limiting_istream( Ferris_istream< char_type, traits_type > _ss,
                                 streampos be, streampos en )
            :
            _Base( new ss_impl_t( _ss, be, en ) )
            {
            }
        
        ferris_limiting_istream( const ferris_limiting_istream& rhs )
            : _Base( rhs.ss )
            {}
    };
typedef ferris_limiting_istream<char>  f_lim_istream;
typedef ferris_limiting_istream<char> fh_lim_istream;


    
    
    namespace Factory
    {

        fh_istream  
        MakeLimitingIStream( fh_istream ss, streampos be, streampos en )
        {
//        cout << "MakeLimitingIStream be: " << be << " en:" << en << endl;
            ss->seekg( be );
            return fh_lim_istream( ss, be, en );
        }

    
    };


/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/

template<
    class _CharT,
    class _Traits = std::char_traits < _CharT >,
    class _Alloc  = std::allocator   < _CharT >,
    class _BufferSizers = ferris_basic_streambuf_fourk
    >
class FERRISEXP_DLLLOCAL basic_holey_streambuf
    :
    public ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers >
{
    typedef ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers > _Base;
    
public:
    
    typedef char_traits<_CharT>    traits_type;
    typedef typename traits_type::int_type  int_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::pos_type  pos_type;
    typedef typename traits_type::off_type  off_type;
    typedef typename _Base::seekd_t seekd_t;


    basic_holey_streambuf( fh_ostream ss )
        :
        RealStream(ss)
        {
        }

    virtual ~basic_holey_streambuf()
        {
        }
    
private:

    fh_ostream RealStream;

    // prohibit copy/assign
    basic_holey_streambuf( const basic_holey_streambuf& );
    basic_holey_streambuf& operator = ( const basic_holey_streambuf& );

protected:

    /**
     * Write out the data starting at buffer of length sz to the "external"
     * device.
     * 
     * return -1 for error or 0 for success
     */
    virtual int write_out_given_data( const char_type* buffer, std::streamsize sz )
        {

            if( sz != _BufferSizers::getBufSize() - _BufferSizers::getPBSize() )
            {
                fh_stringstream ss;
                ss << "Can not create a holey file for block size:" << sz;
                Throw_UnspoortedBlockSize( tostr(ss), 0 );
            }

            if( isBufferAllZero( buffer, sz ))
            {
                /* Create a hole */
                RealStream->seekp( sz, ios::cur );
                return RealStream->good() ? 0 : -1;
            }
            
            if( RealStream->write( buffer, sz ) )
            {
                return 0;
            }
            return -1;
        }
    
    virtual pos_type
    seekoff(off_type offset, seekd_t d, std::ios_base::openmode m)
        {
            return this->have_been_seeked( RealStream->rdbuf()->pubseekoff( offset, d, m ) );
        }

    virtual pos_type
    seekpos(pos_type pos, std::ios_base::openmode m)
        {
            return this->have_been_seeked( RealStream->rdbuf()->pubseekpos( pos, m ) );
        }
};



/*
 * If any blocks are written that are full of 0 then a hole is created instead.
 *
 * Note that this stream proxy needs to sit atop something that supports creation
 * of holes using lseek()
 */
template<
    template <class,class,class,class> class _StreamBufRawClass,
    class _CharT,
    class _Traits = std::char_traits<_CharT>,
    class _Alloc  = std::allocator   < _CharT >,
    class _BufferSizers = ferris_basic_streambuf_fourk
    >
class FERRISEXP_DLLLOCAL common_ferris_x_ostream
    :
    public Ferris_ostream< _CharT, _Traits >,
    public o_ferris_stream_traits< _CharT, _Traits >
{
protected:
    typedef _StreamBufRawClass<_CharT, _Traits, _Alloc, _BufferSizers> _StreamBuf;
    typedef common_ferris_x_ostream<_StreamBufRawClass,_CharT, _Traits,_Alloc, _BufferSizers>    _Self;

    typedef _StreamBuf ss_impl_t;
    typedef Loki::SmartPtr<  ss_impl_t,
                             FerrisLoki::FerrisExRefCounted,
                             Loki::DisallowConversion,
                             Loki::AssertCheck,
                             FerrisLoki::FerrisExSmartPtrStorage > ss_t;
    ss_t ss;

public:
    
    typedef char_traits<_CharT>    traits_type;
    typedef typename traits_type::int_type  int_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::pos_type  pos_type;
    typedef typename traits_type::off_type  off_type;
    typedef emptystream_methods< char_type, traits_type > delegating_methods;
    

    explicit
    common_ferris_x_ostream( const ss_t& _ss )
        :
        ss( _ss )
        {
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }

    common_ferris_x_ostream( const common_ferris_x_ostream& rhs )
        :
        ss( rhs.ss )
        {
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }


    virtual ~common_ferris_x_ostream()
        {
        }
    
    _Self* operator->()
         {
             return this;
         }

    ss_impl_t*
    rdbuf() const
        {
            return GetImpl(ss);
        }

    enum
    {
        stream_readable = false,
        stream_writable = true
    };
};



/*
 * If any blocks are written that are full of 0 then a hole is created instead.
 *
 * Note that this stream proxy needs to sit atop something that supports creation
 * of holes using lseek()
 */
template<
    class _CharT,
    class _Traits = std::char_traits<_CharT>,
    class _Alloc  = std::allocator   < _CharT >,
    class _BufferSizers = ferris_basic_streambuf_fourk
    >
class FERRISEXP_DLLLOCAL ferris_holey_ostream
    :
    public common_ferris_x_ostream<
    basic_holey_streambuf, _CharT, _Traits, _Alloc, _BufferSizers >
{
    typedef common_ferris_x_ostream<
        basic_holey_streambuf, _CharT, _Traits, _Alloc, _BufferSizers
        > _Base;
    typedef ferris_holey_ostream<_CharT, _Traits>    _Self;

public:
    
    typedef char_traits<_CharT>    traits_type;
    typedef typename traits_type::int_type  int_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::pos_type  pos_type;
    typedef typename traits_type::off_type  off_type;
    typedef typename _Base::ss_impl_t ss_impl_t;
    typedef emptystream_methods< char_type, traits_type > delegating_methods;
    
    explicit
    ferris_holey_ostream( Ferris_ostream< char_type, traits_type > _ss )
        :
        _Base( new ss_impl_t( _ss ) )
        {}

    ferris_holey_ostream( const ferris_holey_ostream& rhs )
        : _Base( rhs.ss )
        {}

    virtual ~ferris_holey_ostream()
        {}
};
typedef ferris_holey_ostream<char>  f_holey_ostream;
typedef ferris_holey_ostream<char> fh_holey_ostream;

namespace Factory
{

    fh_ostream  
    MakeHoleyOStream( fh_ostream ss, const int blocksize )
    {
        if( blocksize == 4096 )
        {
            fh_ostream ret = ferris_holey_ostream<
                char, std::char_traits <char>,
                std::allocator<char>, ferris_basic_streambuf_fourk >( ss );

            return ret;
        }
        
        {
            fh_stringstream ss;
            ss << "Can not create a holey file for block size:" << blocksize;
            Throw_UnspoortedBlockSize( tostr(ss), 0 );
        }
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
class FERRISEXP_DLLLOCAL ferris_proxyibuf
    :
    public ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers >
{

    typedef ferris_proxyibuf<_CharT, _Traits, _Alloc, _BufferSizers> _Self;
    typedef typename std::ios_base::seekdir seekd_t;
    
    typedef std::basic_streambuf<_CharT, _Traits> ss_impl_t;
    typedef Loki::SmartPtr<  ss_impl_t,
                       Loki::RefCounted,
//                       FerrisLoki::FerrisExRefCounted,
                       Loki::DisallowConversion,
                       Loki::AssertCheck,
                       Loki::DefaultSPStorage > ss_t;
    ss_t ss;
    
public:

    typedef std::char_traits<_CharT>          traits_type;
    typedef typename traits_type::int_type             int_type;
    typedef typename traits_type::char_type            char_type;
    typedef typename traits_type::pos_type             pos_type;
    typedef typename traits_type::off_type             off_type;
    typedef ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers > _Base;
    typedef std::basic_istream< _CharT, _Traits >                            _ProxyStreamBuf;
    typedef std::basic_string<_CharT, _Traits, _Alloc>                       _String;

    explicit
    ferris_proxyibuf( _ProxyStreamBuf* _RealStream, bool _TakeOwnerShip )
        :
        RealStream( _RealStream ),
        ss( _RealStream->rdbuf() ),
        TakeOwnerShip( _TakeOwnerShip )
        {
            // the ss member will try to cleanup RealStream for us
            // if we are not ment to do this we must stop that autocleaning
            if( !TakeOwnerShip )
            {
                ss.Clone( &(*ss) );
            }
        }

    virtual ~ferris_proxyibuf()
        {
//             if( TakeOwnerShip )
//                 delete RealStream;
        }

    
private:
    
    // prohibit copy/assign
    ferris_proxyibuf( const ferris_proxyibuf& );
    ferris_proxyibuf operator = ( const ferris_proxyibuf& );

    std::basic_istream< _CharT, _Traits >* RealStream;
    bool        TakeOwnerShip;
    
protected:
    
    /*
     * This is the only methods that really needs to be here. It gets
     * up to maxsz data into buffer and returns how much data was really
     * read.
     */
    int make_new_data_avail( char_type* buffer, streamsize maxsz )
        {
            RealStream->read( buffer, maxsz );
            return RealStream->gcount();
        }

    
    virtual pos_type
    seekoff(off_type offset, seekd_t d, std::ios_base::openmode m)
        {
            return this->have_been_seeked( ss->pubseekoff( offset, d, m ) );
        }

    virtual pos_type
    seekpos(pos_type pos, std::ios_base::openmode m)
        {
            return this->have_been_seeked( ss->pubseekpos( pos, m ) );
        }
};



/*
 * For a ferris proxy to a non ferris stream.
 */
template<
    typename _CharT,
    typename _Traits = std::char_traits < _CharT >,
    typename _Alloc  = std::allocator   < _CharT >
    >
class FERRISEXP_DLLLOCAL FerrisProxyIStream
    :
    public Ferris_istream< _CharT, _Traits >
{
    typedef FerrisProxyIStream<_CharT, _Traits,_Alloc>   _Self;

public:
    
    typedef char_traits< _CharT >  traits_type;
    typedef typename traits_type::int_type  int_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::pos_type  pos_type;
    typedef typename traits_type::off_type  off_type;

    istream*    RealStream;


    typedef ferris_proxyibuf<_CharT, _Traits> ss_impl_t;
    typedef Loki::SmartPtr<  ss_impl_t,
                             FerrisLoki::FerrisExRefCounted,
                             Loki::DisallowConversion,
                             Loki::AssertCheck,
                             FerrisLoki::FerrisExSmartPtrStorage > ss_t;
    ss_t ss;

    
public:

    FerrisProxyIStream( istream* _ss, bool _TakeOwnerShip )
        :
        RealStream( _ss ),
        ss( new ss_impl_t( _ss, _TakeOwnerShip ))
        {
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }

    FerrisProxyIStream( const FerrisProxyIStream& rhs )
        :
        ss( rhs.ss )
        {
            RealStream = rhs.RealStream;
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }

    

    virtual ~FerrisProxyIStream()
        {
        }

    _Self* operator->()
        {
            return this;
        }

    ss_impl_t*
    rdbuf() const
        {
            return GetImpl(ss);
        }

    enum
    {
        stream_readable = true,
        stream_writable = false
    };
};

/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/

template<
    class _CharT,
    class _Traits = std::char_traits < _CharT >,
    class _Alloc  = std::allocator   < _CharT >,
    class _BufferSizers = ferris_basic_streambuf_fourk
    >
class FERRISEXP_DLLLOCAL ferris_proxyobuf
    :
    public ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers >
{

    typedef ferris_proxyobuf<_CharT, _Traits, _Alloc, _BufferSizers> _Self;
    typedef typename std::ios_base::seekdir seekd_t;
    
    typedef std::basic_streambuf<_CharT, _Traits> ss_impl_t;
    typedef Loki::SmartPtr<  ss_impl_t,
                       Loki::RefCounted,
                       Loki::DisallowConversion,
                       Loki::AssertCheck,
                       Loki::DefaultSPStorage > ss_t;
    ss_t ss;
    
public:

    typedef std::char_traits<_CharT>          traits_type;
    typedef typename traits_type::int_type             int_type;
    typedef typename traits_type::char_type            char_type;
    typedef typename traits_type::pos_type             pos_type;
    typedef typename traits_type::off_type             off_type;
    typedef ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers > _Base;
    typedef std::basic_ostream< _CharT, _Traits >                            _ProxyStreamBuf;
    typedef std::basic_string<_CharT, _Traits, _Alloc>                       _String;

    explicit
    ferris_proxyobuf( _ProxyStreamBuf* _RealStream, bool _TakeOwnerShip )
        :
        RealStream( _RealStream ),
        ss( _RealStream->rdbuf() ),
        TakeOwnerShip( _TakeOwnerShip )
        {
        }

    virtual ~ferris_proxyobuf()
        {
            if( TakeOwnerShip )
                delete RealStream;
        }

    
private:
    
    // prohibit copy/assign
    ferris_proxyobuf( const ferris_proxyobuf& );
    ferris_proxyobuf operator = ( const ferris_proxyobuf& );

    std::basic_ostream< _CharT, _Traits >* RealStream;
    bool        TakeOwnerShip;
    
protected:
    
    /**
     * Write out the data starting at buffer of length sz to the "external"
     * device.
     * 
     * return -1 for error or 0 for success
     */
    virtual int write_out_given_data( const char_type* buffer, std::streamsize sz )
        {
            if( RealStream->write( buffer, sz ) )
            {
                return 0;
            }
            return -1;
        }
    
    virtual pos_type
    seekoff(off_type offset, seekd_t d, std::ios_base::openmode m)
        {
            return this->have_been_seeked( ss->pubseekoff( offset, d, m ) );
        }

    virtual pos_type
    seekpos(pos_type pos, std::ios_base::openmode m)
        {
            return this->have_been_seeked( ss->pubseekpos( pos, m ) );
        }
};



/*
 * For a ferris proxy to a non ferris stream.
 */
template<
    typename _CharT,
    typename _Traits = std::char_traits < _CharT >,
    typename _Alloc  = std::allocator   < _CharT >
    >
class FERRISEXP_DLLLOCAL FerrisProxyOStream
    :
    public Ferris_ostream< _CharT, _Traits >
{
    typedef FerrisProxyOStream<_CharT, _Traits,_Alloc>   _Self;

public:
    
    typedef char_traits< _CharT >  traits_type;
    typedef typename traits_type::int_type  int_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::pos_type  pos_type;
    typedef typename traits_type::off_type  off_type;

    ostream*    RealStream;


    typedef ferris_proxyobuf<_CharT, _Traits> ss_impl_t;
    typedef Loki::SmartPtr<  ss_impl_t,
                       FerrisLoki::FerrisExRefCounted,
                       Loki::DisallowConversion,
                       Loki::AssertCheck,
                       FerrisLoki::FerrisExSmartPtrStorage > ss_t;
    ss_t ss;

    
public:

    FerrisProxyOStream( ostream* _ss, bool _TakeOwnerShip )
        :
        RealStream( _ss ),
        ss( new ss_impl_t( _ss, _TakeOwnerShip ))
        {
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }

    FerrisProxyOStream( const FerrisProxyOStream& rhs )
        :
        ss( rhs.ss )
        {
            RealStream = rhs.RealStream;
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }

    
    virtual ~FerrisProxyOStream()
        {
        }

    _Self* operator->()
        {
            return this;
        }

    ss_impl_t*
    rdbuf() const
        {
            return GetImpl(ss);
        }

    enum
    {
        stream_readable = false,
        stream_writable = true
    };
};



/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/


template<
    typename _CharT,
    typename _Traits = std::char_traits < _CharT >,
    typename _Alloc  = std::allocator   < _CharT >
    >
class FERRISEXP_DLLLOCAL UnbufferedStreamBuf
    :
    public ferris_streambuf<_CharT, _Traits>,
    public std::basic_streambuf< _CharT, _Traits >
{

public:

    typedef typename std::char_traits<_CharT>          traits_type;
    typedef typename traits_type::int_type             int_type;
    typedef typename traits_type::char_type            char_type;
    typedef typename traits_type::pos_type             pos_type;
    typedef typename traits_type::off_type             off_type;
    typedef ferris_streambuf<_CharT, _Traits>            _Base;
    typedef UnbufferedStreamBuf<_CharT, _Traits, _Alloc> _Self;
    typedef std::basic_string<_CharT, _Traits, _Alloc>   _String;
    
protected:

    explicit
    UnbufferedStreamBuf()
        :
        takeFromBuf(false)
        {
        }


    // -1 == err, 0 == ok
    virtual int char_to_device( char c )
        {
            return -1;
        }

    // -1 == err, 0 == ok
    virtual int char_from_device( char* c )
        {
            return -1;
        }

    int_type overflow( int_type c )
        {
            if(!traits_type::eq_int_type(c, traits_type::eof()))
            {
                if( char_to_device( traits_type::to_char_type(c)) < 0 )
                {
                    return traits_type::eof();
                }
                else {
                    return c;
                }
            }
            return traits_type::not_eof(c);
        }

    int_type uflow()
        {
            if( takeFromBuf )
            {
                takeFromBuf = false;
                return traits_type::to_int_type(charBuf);
            }
            else
            {
                char_type c;

                if( char_from_device( &c ) < 0 )
                {
                    return traits_type::eof();
                }
                else
                {
                    charBuf = c;
                    return traits_type::to_int_type(c);
                }
            }
        }

    int_type underflow()
        {
            if( takeFromBuf )
            {
                return traits_type::to_int_type(charBuf);
            }
            else
            {
                char_type c;

                if( char_from_device( &c ) < 0 )
                {
                    return traits_type::eof();
                }
                else
                {
                    takeFromBuf = true;
                    charBuf = c;
                    return traits_type::to_int_type(c);
                }
            }
        }

    int_type pbackfail( int_type c )
        {
            if( !takeFromBuf )
            {
                if( !traits_type::eq_int_type(c,traits_type::eof()))
                    charBuf = traits_type::to_char_type(c);

                takeFromBuf = true;

                return traits_type::to_int_type(charBuf);
            }
            else
            {
                return traits_type::eof();
            }
        }

private:

    char_type charBuf;
    bool takeFromBuf;

    UnbufferedStreamBuf( const UnbufferedStreamBuf& );
    UnbufferedStreamBuf& operator=(const UnbufferedStreamBuf& );
};


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


template<
    typename _CharT,
    typename _Traits = std::char_traits < _CharT >,
    typename _Alloc  = std::allocator   < _CharT >
    >
class FERRISEXP_DLLLOCAL ProxyIOStreamBuf
    :
    public UnbufferedStreamBuf< _CharT, _Traits, _Alloc >
{
    bool TakeOwnerShip;
    iostream* RealStream;

    typedef std::basic_streambuf<_CharT, _Traits>* ss_t;
    ss_t ss;

    
public:
    
    typedef std::char_traits<_CharT>          traits_type;
    typedef typename traits_type::int_type             int_type;
    typedef typename traits_type::char_type            char_type;
    typedef typename traits_type::pos_type             pos_type;
    typedef typename traits_type::off_type             off_type;
    typedef ferris_streambuf<_CharT, _Traits>           _Base;
    typedef ferris_stringbuf<_CharT, _Traits, _Alloc>   _Self;
    typedef std::basic_string<_CharT, _Traits, _Alloc>  _String;


protected:

    // -1 == err, 0 == ok
    virtual int char_to_device( char c )
        {
//            cerr << "ProxyIOStreamBuf::char_to_device() c:" << c << endl;
            return ss->sputc(c);
        }

    // -1 == err, 0 == ok
    virtual int char_from_device( char* c )
        {
            int_type ch = ss->sbumpc();

//            cerr << "ProxyIOStreamBuf::char_from_device() ch:" << ch << endl;
            if( ch != traits_type::eof() )
            {
                *c = traits_type::to_char_type(ch);
                return 0;
            }
            return -1;
        }

    virtual int sync()
        {
            return ss->pubsync();
        }


public:

    ProxyIOStreamBuf( iostream* _ss, bool _TakeOwnerShip )
        :
        RealStream( _ss),
        TakeOwnerShip(_TakeOwnerShip),
        ss( _ss->rdbuf() )
        {
        }

    virtual ~ProxyIOStreamBuf()
        {
//            cerr << "~ProxyIOStreamBuf() RealStream:" << (void*)RealStream << endl;
            if( TakeOwnerShip )
                delete RealStream;
        }
    
    
private:

    ProxyIOStreamBuf( const ProxyIOStreamBuf& );
    ProxyIOStreamBuf operator=( const ProxyIOStreamBuf& );
    
};


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

/*
 * For a ferris proxy to a non ferris io stream.
 */
template<
    typename _CharT,
    typename _Traits = std::char_traits < _CharT >,
    typename _Alloc  = std::allocator   < _CharT >
    >
class FERRISEXP_DLLLOCAL FerrisProxyIOStream
    :
    public Ferris_iostream< _CharT, _Traits >
{
    typedef FerrisProxyIOStream<_CharT, _Traits,_Alloc>   _Self;

    typedef ProxyIOStreamBuf<_CharT, _Traits> ss_impl_t;
    typedef Loki::SmartPtr<  ss_impl_t,
                       FerrisLoki::FerrisExRefCounted,
                       Loki::DisallowConversion,
                       Loki::AssertCheck,
                       FerrisLoki::FerrisExSmartPtrStorage > ss_t;
    ss_t ss;
    
public:

    typedef typename std::char_traits<_CharT> traits_type;
    typedef typename traits_type::int_type    int_type;
    typedef typename traits_type::char_type   char_type;
    typedef typename traits_type::pos_type    pos_type;
    typedef typename traits_type::off_type    off_type;
    typedef emptystream_methods< _CharT, _Traits > delegating_methods;
    
    FerrisProxyIOStream( iostream* _ss, bool _TakeOwnerShip )
        :
        ss( new ss_impl_t( _ss, _TakeOwnerShip ) )
        {
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }

    FerrisProxyIOStream( const FerrisProxyIOStream& rhs )
        :
        ss( rhs.ss )
        {
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }

    
    
    virtual ~FerrisProxyIOStream()
        {
        }
    
    _Self* operator->()
        {
            return this;
        }

    ss_impl_t*
    rdbuf() const
        {
            return GetImpl(ss);
        }


    enum
    {
        stream_readable = true,
        stream_writable = true
    };
};



namespace Factory
{

    fh_istream  MakeProxyStream( istream* base, bool takeOwnerShip )
    {
        return FerrisProxyIStream<char>( base, takeOwnerShip );
    }

    fh_ostream  MakeProxyStream( ostream* base, bool takeOwnerShip )
    {
        return FerrisProxyOStream<char>( base, takeOwnerShip );
    }
    
    fh_iostream  MakeProxyStream( iostream* base, bool takeOwnerShip )
    {
        return FerrisProxyIOStream<char>( base, takeOwnerShip );
    }
    
    
    

};

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/


template<
    class _CharT,
    class _Traits = std::char_traits < _CharT >,
    class _Alloc  = std::allocator   < _CharT >,
    class _BufferSizers = ferris_basic_streambuf_fourk
    >
class FERRISEXP_DLLLOCAL basic_tee_streambuf
    :
    public ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers >
{
    typedef ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers > _Base;
    
public:
    
    typedef char_traits<_CharT>    traits_type;
    typedef typename traits_type::int_type  int_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::pos_type  pos_type;
    typedef typename traits_type::off_type  off_type;
    typedef typename _Base::seekd_t seekd_t;


    basic_tee_streambuf( fh_ostream ss1, fh_ostream ss2 )
        :
        m_ss1( ss1 ),
        m_ss2( ss2 )
        {
        }

    virtual ~basic_tee_streambuf()
        {
        }
    
private:

    fh_ostream m_ss1;
    fh_ostream m_ss2;

    // prohibit copy/assign
    basic_tee_streambuf( const basic_tee_streambuf& );
    basic_tee_streambuf& operator = ( const basic_tee_streambuf& );

protected:

    /**
     * Write out the data starting at buffer of length sz to the "external"
     * device.
     * 
     * return -1 for error or 0 for success
     */
    virtual int write_out_given_data( const char_type* buffer, std::streamsize sz )
        {
            streamsize wsz;
            
            wsz = m_ss1.rdbuf()->sputn( buffer, sz );
            if( wsz < sz )
                return -1;
            
            wsz = m_ss2.rdbuf()->sputn( buffer, sz );
            if( wsz < sz )
                return -1;

            return sz;
        }
    
    virtual pos_type
    seekoff(off_type offset, seekd_t d, std::ios_base::openmode m)
        {
            m_ss1->rdbuf()->pubseekoff( offset, d, m );
            return this->have_been_seeked( m_ss2->rdbuf()->pubseekoff( offset, d, m ) );
        }

    virtual pos_type
    seekpos(pos_type pos, std::ios_base::openmode m)
        {
            m_ss1->rdbuf()->pubseekpos( pos, m );
            return this->have_been_seeked( m_ss2->rdbuf()->pubseekpos( pos, m ) );
        }
};

    
/**
 * Tee output to two different streams at once.
 */
template<
    class _CharT,
    class _Traits = std::char_traits<_CharT>,
    class _Alloc  = std::allocator   < _CharT >,
    class _BufferSizers = ferris_basic_streambuf_fourk
    >
class FERRISEXP_DLLLOCAL ferris_tee_ostream
    :
    public common_ferris_x_ostream<
    basic_tee_streambuf, _CharT, _Traits, _Alloc, _BufferSizers >
{
    typedef common_ferris_x_ostream<
    basic_tee_streambuf, _CharT, _Traits, _Alloc, _BufferSizers
        > _Base;
    typedef ferris_tee_ostream<_CharT, _Traits>    _Self;

public:
    
    typedef char_traits<_CharT>    traits_type;
    typedef typename traits_type::int_type  int_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::pos_type  pos_type;
    typedef typename traits_type::off_type  off_type;
    typedef typename _Base::ss_impl_t ss_impl_t;
    typedef emptystream_methods< char_type, traits_type > delegating_methods;
    
    explicit
    ferris_tee_ostream( Ferris_ostream< char_type, traits_type > _ss1,
                        Ferris_ostream< char_type, traits_type > _ss2 )
        :
        _Base( new ss_impl_t( _ss1, _ss2 ) )
        {}

    ferris_tee_ostream( const ferris_tee_ostream& rhs )
        : _Base( rhs.ss )
        {}
    virtual ~ferris_tee_ostream() {}
};
typedef ferris_tee_ostream<char>  f_tee_ostream;
typedef ferris_tee_ostream<char> fh_tee_ostream;

namespace Factory
{
    fh_ostream MakeTeeStream( fh_ostream oss1, fh_ostream oss2 )
    {
        return fh_tee_ostream( oss1, oss2 );
    }
};


/**
 * Tee output to two different streams at once.
 */
template<
    class _CharT,
    class _Traits = std::char_traits<_CharT>,
    class _Alloc  = std::allocator   < _CharT >,
    class _BufferSizers = ferris_basic_streambuf_fourk
    >
class FERRISEXP_DLLLOCAL ferris_tee_iostream
    :
    public Ferris_iostream< _CharT, _Traits >,
    public io_ferris_stream_traits< _CharT, _Traits >
{
    typedef Ferris_iostream< _CharT, _Traits >     _Base;
    typedef ferris_tee_iostream<_CharT, _Traits>    _Self;

public:
    
    typedef char_traits<_CharT>    traits_type;
    typedef typename traits_type::int_type  int_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::pos_type  pos_type;
    typedef typename traits_type::off_type  off_type;
    typedef basic_tee_streambuf<_CharT, _Traits, _Alloc, _BufferSizers> ss_impl_t;
    typedef emptystream_methods< char_type, traits_type > delegating_methods;
    
    explicit
    ferris_tee_iostream( Ferris_ostream< char_type, traits_type > _ss1,
                        Ferris_ostream< char_type, traits_type > _ss2 )
        :
        _Base( new ss_impl_t( _ss1, _ss2 ) )
        {}

    ferris_tee_iostream( const ferris_tee_iostream& rhs )
        : _Base( rhs.sb, rhs.sh )
        {}
    virtual ~ferris_tee_iostream() {}
};
typedef ferris_tee_iostream<char>  f_tee_iostream;
typedef ferris_tee_iostream<char> fh_tee_iostream;

namespace Factory
{
    fh_iostream MakeTeeIOStream( fh_ostream oss1, fh_ostream oss2 )
    {
        return fh_tee_iostream( oss1, oss2 );
    }
};


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/



template<
    class _CharT,
    class _Traits = std::char_traits < _CharT >,
    class _Alloc  = std::allocator   < _CharT >,
    class _BufferSizers = ferris_basic_streambuf_fourk
    >
class FERRISEXP_DLLLOCAL basic_fd_streambuf
    :
    public ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers >
{
public:
    
    typedef char_traits<_CharT>    traits_type;
    typedef typename traits_type::int_type  int_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::pos_type  pos_type;
    typedef typename traits_type::off_type  off_type;


    basic_fd_streambuf( int _fd, bool closeFD )
        :
        fd(_fd),
        closeFD( closeFD )
        {
        }

    virtual ~basic_fd_streambuf()
        {
            if( closeFD )
            {
                close( fd );
            }
        }
    
private:

    int fd;
    bool closeFD;
    
    // prohibit copy/assign
    basic_fd_streambuf( const basic_fd_streambuf& );
    basic_fd_streambuf& operator = ( const basic_fd_streambuf& );

protected:

    /**
     * This is the only methods that really needs to be here. It gets
     * up to maxsz data into buffer and returns how much data was really
     * read. Return 0 for a failure, you must read atleast one byte.
     */
    virtual int make_new_data_avail( char_type* buffer, streamsize maxsz )
        {
            streamsize br = 0;
            bool virgin = true;
            
            while( !br && (virgin || errno == EINTR ))
            {
                virgin = false;
                br = read(fd, buffer, min(SSIZE_MAX,maxsz));
            }

//            cerr << "make_new_data_avail br:" << br << " maxsz:" << maxsz << endl;

            /* an error */
            if( br < 0 )
            {
                br = 0;
            }

            return br;
        }

    /**
     * Write out the data starting at buffer of length sz to the "external"
     * device.
     * 
     * return -1 for error or 0 for success
     */
    virtual int write_out_given_data( const char_type* buffer, std::streamsize sz )
        {
            // hacking for DIO testing
//            if( sz > 4096 ) sz = 4096;
            
            int bw = 0;
            bool virgin = true;

            while( !bw && (virgin || errno == EINTR ))
            {
                virgin = false;
                bw = write( fd, buffer, sz );
            }

            /* an error */
            if( bw < 0 )
            {
                bw = -1;
            }

            return bw;
        }
};


template<
    typename _CharT,
    typename _Traits = std::char_traits < _CharT >,
    typename _Alloc  = std::allocator   < _CharT >
    >
class FERRISEXP_DLLLOCAL FerrisFdIStream
    :
    public Ferris_istream< _CharT, _Traits >
{
    typedef FerrisFdIStream<_CharT, _Traits,_Alloc>   _Self;

    typedef basic_fd_streambuf<_CharT, _Traits> ss_impl_t;
    typedef Loki::SmartPtr<  ss_impl_t,
                       FerrisLoki::FerrisExRefCounted,
                       Loki::DisallowConversion,
                       Loki::AssertCheck,
                       FerrisLoki::FerrisExSmartPtrStorage > ss_t;
    ss_t ss;
    
public:

    typedef std::char_traits<_CharT> traits_type;
    typedef typename traits_type::int_type    int_type;
    typedef typename traits_type::char_type   char_type;
    typedef typename traits_type::pos_type    pos_type;
    typedef typename traits_type::off_type    off_type;
    typedef emptystream_methods< _CharT, _Traits > delegating_methods;
    
    FerrisFdIStream( int fd, bool closeFD )
        :
        ss( new ss_impl_t( fd, closeFD ) )
        {
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }

    FerrisFdIStream( const FerrisFdIStream& rhs )
        :
        ss( rhs.ss )
        {
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }

    
    virtual ~FerrisFdIStream()
        {
        }
    
    _Self* operator->()
        {
            return this;
        }

    ss_impl_t*
    rdbuf() const
        {
            return GetImpl(ss);
        }


    enum
    {
        stream_readable = true,
        stream_writable = false
    };
};


template<
    typename _CharT,
    typename _Traits = std::char_traits < _CharT >,
    typename _Alloc  = std::allocator   < _CharT >
    >
class FERRISEXP_DLLLOCAL FerrisFdOStream
    :
    public Ferris_ostream< _CharT, _Traits >
{
    typedef FerrisFdOStream<_CharT, _Traits,_Alloc>   _Self;

    typedef basic_fd_streambuf<_CharT, _Traits> ss_impl_t;
    typedef Loki::SmartPtr<  ss_impl_t,
                       FerrisLoki::FerrisExRefCounted,
                       Loki::DisallowConversion,
                       Loki::AssertCheck,
                       FerrisLoki::FerrisExSmartPtrStorage > ss_t;
    ss_t ss;
    
public:

    typedef typename std::char_traits<_CharT> traits_type;
    typedef typename traits_type::int_type    int_type;
    typedef typename traits_type::char_type   char_type;
    typedef typename traits_type::pos_type    pos_type;
    typedef typename traits_type::off_type    off_type;
    typedef emptystream_methods< _CharT, _Traits > delegating_methods;
    
    FerrisFdOStream( int fd, bool closeFD )
        :
        ss( new ss_impl_t( fd, closeFD ) )
        {
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }

    FerrisFdOStream( const FerrisFdOStream& rhs )
        :
        ss( rhs.ss )
        {
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }

    
    
    virtual ~FerrisFdOStream()
        {
        }
    
    _Self* operator->()
        {
            return this;
        }

    ss_impl_t*
    rdbuf() const
        {
            return GetImpl(ss);
        }


    enum
    {
        stream_readable = false,
        stream_writable = true
    };
};


template<
    typename _CharT,
    typename _Traits = std::char_traits < _CharT >,
    typename _Alloc  = std::allocator   < _CharT >
    >
class FERRISEXP_DLLLOCAL FerrisFdIOStream
    :
    public Ferris_iostream< _CharT, _Traits >
{
    typedef FerrisFdIOStream<_CharT, _Traits,_Alloc>   _Self;

    typedef basic_fd_streambuf<_CharT, _Traits> ss_impl_t;
    typedef Loki::SmartPtr<  ss_impl_t,
                       FerrisLoki::FerrisExRefCounted,
                       Loki::DisallowConversion,
                       Loki::AssertCheck,
                       FerrisLoki::FerrisExSmartPtrStorage > ss_t;
    ss_t ss;
    
public:

    typedef typename std::char_traits<_CharT> traits_type;
    typedef typename traits_type::int_type    int_type;
    typedef typename traits_type::char_type   char_type;
    typedef typename traits_type::pos_type    pos_type;
    typedef typename traits_type::off_type    off_type;
    typedef emptystream_methods< _CharT, _Traits > delegating_methods;
    
    FerrisFdIOStream( int fd, bool closeFD )
        :
        ss( new ss_impl_t( fd, closeFD ) )
        {
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }

    FerrisFdIOStream( const FerrisFdIOStream& rhs )
        :
        ss( rhs.ss )
        {
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }

    
    
    virtual ~FerrisFdIOStream()
        {
        }
    
    _Self* operator->()
        {
            return this;
        }

    ss_impl_t*
    rdbuf() const
        {
            return GetImpl(ss);
        }


    enum
    {
        stream_readable = true,
        stream_writable = true
    };
};



    namespace Factory
    {
        fh_istream   MakeFdIStream( int fd )
        {
            return MakeFdIStream( fd, false );
        }
        
        fh_ostream   MakeFdOStream( int fd )
        {
            return MakeFdOStream( fd, false );
        }

        fh_iostream  MakeFdIOStream( int fd )
        {
            return MakeFdIOStream( fd, false );
        }

        fh_istream   MakeFdIStream( int fd, bool closeFD )
        {
            FerrisFdIStream<char> ret( fd, closeFD );
            return ret;
        }
            
        fh_ostream   MakeFdOStream( int fd, bool closeFD )
        {
            return FerrisFdOStream<char>( fd, closeFD );
        }

        fh_iostream  MakeFdIOStream( int fd, bool closeFD )
        {
            return FerrisFdIOStream<char>( fd, closeFD );
        }

        

        fh_istream  fcin()
        {
            return MakeFdIStream( STDIN_FILENO );
        }
        
        fh_ostream  fcout()
        {
            return MakeFdOStream( STDOUT_FILENO );
        }
        
        fh_ostream  fcerr()
        {
            return MakeFdOStream( STDERR_FILENO );
        }
        fh_ostream  fcnull()
        {
            static int fd = open( "/dev/null", O_RDWR );
            return MakeFdOStream( fd );
            
//             static f_ofstream ret( "/dev/null" );
//             ret.rdbuf().AddRef();
//             static f_ofstream tmp = ret;
//             return ret;
        }
        


        int  MakeFIFO( const std::string& path_const )
        {
            return MakeFIFO( path_const, true, O_RDWR | O_NONBLOCK );
        }

        FERRISEXP_API int MakeFIFO( const std::string& path_const,
                                    bool should_open,
                                    int openmode )
        {
            string path = CleanupURL( path_const );

            if( starts_with( path, "file:" ) )
                path = path.substr( 5 );
            
            unlink( path.c_str() );
            int rc = mkfifo( path.c_str(), S_IRUSR | S_IWUSR | O_NONBLOCK );
            if( rc == -1 )
            {
                string es = errnum_to_string( "", errno );
                fh_stringstream ss;
                ss << "Can not create fifo for incomming requests at:" << path << endl
                   << " reason:" << es << endl;
//                cerr << tostr(ss) << endl;
                Throw_CreateFIFO( tostr( ss ), 0 );
            }

            if( !should_open )
                return 0;
            if( !openmode )
                openmode = O_RDWR | O_NONBLOCK;
            
            
            int fd = open( path.c_str(), openmode );
            if( fd == -1 )
            {
                string es = errnum_to_string( "", errno );
                fh_stringstream ss;
                ss << "Can not open fifo for incomming requests at:" << path << endl
                   << " reason:" << es << endl;
                Throw_CreateFIFO( tostr( ss ), 0 );
            }        
            return fd;
        }
        
        
        
    };



    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/

template<
    class _CharT,
    class _Traits = std::char_traits < _CharT >,
    class _Alloc  = std::allocator   < _CharT >,
    class _BufferSizers = ferris_basic_streambuf_virtual
    >
class FERRISEXP_DLLLOCAL basic_memory_streambuf
    :
    public ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers >
{
    typedef basic_memory_streambuf< _CharT, _Traits, _Alloc, _BufferSizers > _Self;
    typedef ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers > _Base;
    
public:
    
    typedef char_traits<_CharT>    traits_type;
    typedef typename traits_type::int_type  int_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::pos_type  pos_type;
    typedef typename traits_type::off_type  off_type;

    basic_memory_streambuf( void* data, const int data_size )
        :
        _Base( data, data_size )
        {
        }

    virtual ~basic_memory_streambuf()
        {
            this->ensureMode( this->mode_mute );
            this->buffer = 0;
        }

protected:

    virtual void switch_to_read_mode()
        {
            this->setg( this->buffer + this->getPBSize(),
                  this->buffer + this->getPBSize(),
                  this->buffer + this->getBufSize() );
        }

    
private:

    // prohibit copy/assign
    basic_memory_streambuf( const basic_memory_streambuf& );
    basic_memory_streambuf& operator = ( const basic_memory_streambuf& );
};


template<
    typename _CharT,
    typename _Traits = std::char_traits < _CharT >,
    typename _Alloc  = std::allocator   < _CharT >
    >
class FERRISEXP_DLLLOCAL FerrisMemoryIStream
    :
    public Ferris_istream< _CharT, _Traits >
{
    typedef FerrisMemoryIStream<_CharT, _Traits,_Alloc>   _Self;

    typedef basic_memory_streambuf<_CharT, _Traits> ss_impl_t;
    typedef Loki::SmartPtr<  ss_impl_t,
                       FerrisLoki::FerrisExRefCounted,
                       Loki::DisallowConversion,
                       Loki::AssertCheck,
                       FerrisLoki::FerrisExSmartPtrStorage > ss_t;
    ss_t ss;
    
public:

    typedef typename std::char_traits<_CharT> traits_type;
    typedef typename traits_type::int_type    int_type;
    typedef typename traits_type::char_type   char_type;
    typedef typename traits_type::pos_type    pos_type;
    typedef typename traits_type::off_type    off_type;
    typedef emptystream_methods< _CharT, _Traits > delegating_methods;
    
    FerrisMemoryIStream( void* data, const int data_size )
        :
        ss( new ss_impl_t( data, data_size ) )
        {
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }

    FerrisMemoryIStream( const FerrisMemoryIStream& rhs )
        :
        ss( rhs.ss )
        {
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }

    
    virtual ~FerrisMemoryIStream()
        {}
    
    _Self* operator->()
        {
            return this;
        }

    ss_impl_t*
    rdbuf() const
        {
            return GetImpl(ss);
        }

    enum
    {
        stream_readable = true,
        stream_writable = false
    };
};


template<
    typename _CharT,
    typename _Traits = std::char_traits < _CharT >,
    typename _Alloc  = std::allocator   < _CharT >
    >
class FERRISEXP_DLLLOCAL FerrisMemoryIOStream
    :
    public Ferris_iostream< _CharT, _Traits >
{
    typedef FerrisMemoryIOStream<_CharT, _Traits,_Alloc>   _Self;

    typedef basic_memory_streambuf<_CharT, _Traits> ss_impl_t;
    typedef Loki::SmartPtr<  ss_impl_t,
                       FerrisLoki::FerrisExRefCounted,
                       Loki::DisallowConversion,
                       Loki::AssertCheck,
                       FerrisLoki::FerrisExSmartPtrStorage > ss_t;
    ss_t ss;
    
public:

    typedef typename std::char_traits<_CharT> traits_type;
    typedef typename traits_type::int_type    int_type;
    typedef typename traits_type::char_type   char_type;
    typedef typename traits_type::pos_type    pos_type;
    typedef typename traits_type::off_type    off_type;
    typedef emptystream_methods< _CharT, _Traits > delegating_methods;
    
    FerrisMemoryIOStream( void* data, const int data_size )
        :
        ss( new ss_impl_t( data, data_size ) )
        {
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }

    FerrisMemoryIOStream( const FerrisMemoryIOStream& rhs )
        :
        ss( rhs.ss )
        {
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }

    
    
    virtual ~FerrisMemoryIOStream()
        {}
    
    _Self* operator->()
        {
            return this;
        }

    ss_impl_t*
    rdbuf() const
        {
            return GetImpl(ss);
        }


    enum
    {
        stream_readable = true,
        stream_writable = true
    };
};



    namespace Factory
    {
        fh_istream   MakeMemoryIStream( void* data, const int data_size )
        {
            FerrisMemoryIStream<char> ret( data, data_size );
            return ret;
        }
        
        fh_iostream   MakeMemoryIOStream( void* data, const int data_size )
        {
            return FerrisMemoryIOStream<char>( data, data_size );
        }
    };

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

template<
    class _CharT,
    class _Traits = std::char_traits < _CharT >,
    class _Alloc  = std::allocator   < _CharT >,
    class _BufferSizers = ferris_basic_streambuf_virtual
    >
class FERRISEXP_DLLLOCAL basic_mmap_streambuf
    :
    public ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers >
{
    typedef basic_mmap_streambuf< _CharT, _Traits, _Alloc, _BufferSizers > _Self;
    typedef ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers > _Base;
    
public:

    /**
     * How much to extend the file by each time a remap is needed
     */
    enum {
        extendSize = 256 * 1024
    };
    /**
     * Store the byte offset from the start of file of where the user data writes ended
     * so that we can truncate off extra extensions made to the mmap at destruction time.
     */
    std::streamsize ShouldBeEndOfFile;

    /**
     * This becomes true when the user writes past eof
     */
    bool            isExtendedFile;

    /**
     * FD for the map
     */
    int fd;
    
    typedef char_traits<_CharT>    traits_type;
    typedef typename traits_type::int_type  int_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::pos_type  pos_type;
    typedef typename traits_type::off_type  off_type;

    basic_mmap_streambuf( int fd, void*p, std::streamsize length, std::streamsize ShouldBeEndOfFile = 0 )
        :
        _Base( p, length ),
        ShouldBeEndOfFile( ShouldBeEndOfFile ),
        fd( fd ),
        isExtendedFile( false )
        {
//             cerr << "basic_mmap_streambuf() p:" << toVoid(p)
//                  << " len:" << length
//                  << endl;
        }

    virtual ~basic_mmap_streambuf()
        {
//            cerr << "~basic_mmap_streambuf(1)" << endl;
            maybeUpdateEndOfFileMarker();
            this->ensureMode( this->mode_mute );
            munmap( this->buffer, this->getBufSize() );
            this->buffer = 0;

            /**
             * If they have extended the file then we have to trunc to the last byte written
             */
            if( ShouldBeEndOfFile )
            {
                int rc = ftruncate( fd, ShouldBeEndOfFile );
                if( rc != 0 )
                {
                    int en = errno;
                    fh_stringstream ss;
                    ss << "Failed to trim the end of file for fd:" << fd
                       << " to size:" << ShouldBeEndOfFile
                       << " during un memory mapping file."
                       << "File will have additional zero trailing data" << endl;
                    StreamThrowFromErrno( en, tostr(ss) );
                }
            }
        }

protected:

    void maybeUpdateEndOfFileMarker()
        {
            if( _Base::CurrentMode == _Base::mode_writing && ShouldBeEndOfFile )
            {
//                streamsize partBlock = pptr() - pbase();
                size_t x = lseek( fd, 0, SEEK_CUR );
            
                /*
                 * If they have written to part of the remaped file then we need to store the exact byte
                 * where they last wrote to so we can truncate the file to that byte at dtor time.
                 */
                if( x > ShouldBeEndOfFile )
                {
                    ShouldBeEndOfFile = x;
                }
            }
        }
    
    virtual void switch_to_read_mode()
        {
            maybeUpdateEndOfFileMarker();
            this->setg( this->buffer + this->getPBSize(),
                  this->buffer + this->getPBSize(),
                  this->buffer + this->getBufSize() );
        }


    /**
     * Write out the data starting at buffer of length sz to the "external"
     * device. At startup we memory map the entire file and set that to the buffer
     * so for this method to be called the user has written past the end of the old file.
     *
     * What we do here is to keep track of the real should-be-end-of-file and extend
     * the file by a largish chunk, like 256K, and then mremap() the file again.
     *
     * This does require us to ftruncate() the file in our destructor if we have
     * extended it here.
     * 
     * return -1 for error or 0 for success
     */
    int buffer_out()
        {
#if !defined (_STLP_USE_SYMBIAN_IO)
            
            if( !this->ensureMode( _Base::mode_writing ) )
            {
                return traits_type::eof();
            }
            isExtendedFile = true;
            std::streamsize newSize = this->getBufSize() + extendSize;
            
            int rc = ftruncate( fd, newSize );
            if( rc != 0 )
            {
                int en = errno;
                fh_stringstream ss;
                ss << "Failed to extend memory mapped underlying file for fd:" << fd
                   << " to size:" << newSize
                   << " during buffer_out() call." << endl;
                StreamThrowFromErrno( en, tostr(ss) );
            }

            void* p = mremap( this->buffer,
                              this->getBufSize(),
                              newSize, MREMAP_MAYMOVE );
            
            streamsize x = lseek( fd, 0, SEEK_CUR );
            if( x > ShouldBeEndOfFile )
            {
                ShouldBeEndOfFile = x;
            }
            this->buffer = static_cast<char_type*>(p);
            this->setBufSize( newSize );
            this->setg( 0, 0, 0 );
            this->setp( 0, 0 );
            this->switch_to_write_mode();

            int cnt = this->pptr() - this->pbase();
            if( !cnt )
            {
                return 0;
            }
            int retval = write_out_given_data( this->getWriteBuffer(), cnt );
            return 0;
#else
            return -1;
#endif
        }
    
    
private:

    // prohibit copy/assign
    basic_mmap_streambuf( const basic_mmap_streambuf& );
    basic_mmap_streambuf& operator = ( const basic_mmap_streambuf& );
};

    
template<
    typename _CharT,
    typename _Traits = std::char_traits < _CharT >,
    typename _Alloc  = std::allocator   < _CharT >
>
class FERRISEXP_DLLLOCAL FerrisMMapIOStream
    :
    public Ferris_iostream< _CharT, _Traits >
{
    typedef FerrisMMapIOStream<_CharT, _Traits,_Alloc>   _Self;

    typedef basic_mmap_streambuf<_CharT, _Traits> ss_impl_t;
    typedef Loki::SmartPtr<  ss_impl_t,
                             FerrisLoki::FerrisExRefCounted,
                             Loki::DisallowConversion,
                             Loki::AssertCheck,
                             FerrisLoki::FerrisExSmartPtrStorage > ss_t;
    ss_t ss;
    
public:

    typedef typename std::char_traits<_CharT> traits_type;
    typedef typename traits_type::int_type    int_type;
    typedef typename traits_type::char_type   char_type;
    typedef typename traits_type::pos_type    pos_type;
    typedef typename traits_type::off_type    off_type;
    typedef emptystream_methods< _CharT, _Traits > delegating_methods;
    
    FerrisMMapIOStream( int fd, void* p, std::streamsize length, std::streamsize ShouldBeEndOfFile )
        :
        ss( new ss_impl_t( fd, p, length, ShouldBeEndOfFile ) )
        {
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }

    FerrisMMapIOStream( const FerrisMMapIOStream& rhs )
        :
        ss( rhs.ss )
        {
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }

    
    
    virtual ~FerrisMMapIOStream()
        {}
    
    _Self* operator->()
        {
            return this;
        }

    ss_impl_t*
    rdbuf() const
        {
            return GetImpl(ss);
        }


    enum
    {
        stream_readable = true,
        stream_writable = true
    };
};

namespace Factory
{
    static fh_iostream MakeMMapStream( int fd,
                                       ferris_ios::openmode m,
                                       int mmap_flags = PROT_READ | PROT_WRITE,
                                       const std::string pathdesc = "" )
    {
        std::streamsize ShouldBeEndOfFile = 0;
        std::streamsize length = 0;
        struct stat statb;
        if( fstat( fd, &statb ) )
        {
                int ec = errno;
                fh_stringstream ss;
                ss << "Can not stat to get size for memory mapped IO" << pathdesc << endl;
                StreamThrowFromErrno( ec, tostr(ss), 0 );
        }
        length = statb.st_size;

//         cerr << "-----> MakeMMapIStream() fd:" << fd << " len:" << length << endl;

        if( m & ios::trunc )
        {
            /**
             * We create a 4K area that should be truncated off the file when it is done
             * This is mainly to allow mmap() an initial area.
             */
            length = 4096;
            ftruncate( fd, length );
            ShouldBeEndOfFile = 0;
        }

        if( m & ios::ate || m & ios::app )
        {
            lseek( fd, 0, SEEK_END );
        }
        
        void* p = mmap( 0, length, mmap_flags , MAP_SHARED, fd, 0 );
        if( MAP_FAILED == p )
        {
            int ec = errno;
            fh_stringstream ss;
            ss << "Can not mmap file" << pathdesc << endl;
            StreamThrowFromErrno( ec, tostr(ss), 0 );
        }
        
#if !defined (_STLP_USE_SYMBIAN_IO)

        if( m & ferris_ios::o_mseq )
        {
            if( ferris_madvise( p, length, MADV_SEQUENTIAL ) )
            {
                int ec = errno;
                fh_stringstream ss;
                ss << "Can not advise kernel of sequential access desire" << pathdesc << endl;
                StreamThrowFromErrno( ec, tostr(ss), 0 );
            }
        }

#endif
        
        return FerrisMMapIOStream<char>( fd, p, length, ShouldBeEndOfFile );
    }


    fh_istream   MakeMMapIStream( int fd, ferris_ios::openmode m, const std::string pathdesc )
    {
        return MakeMMapStream( fd, m, PROT_READ, pathdesc );
    }

    fh_iostream  MakeMMapIOStream( int fd, ferris_ios::openmode m, const std::string pathdesc )
    {
        return MakeMMapStream( fd, m, PROT_READ | PROT_WRITE, pathdesc );
    }

    
};

    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/


 /*
 * For type safety on getIStream() type methods the getIOStream() method can
 * call here if it knows that the open mode was read only but you still want
 * an fh_iostream as a result.
 */
template<
    class _CharT,
    class _Traits = std::char_traits < _CharT >,
    class _Alloc  = std::allocator   < _CharT >,
    class _BufferSizers = ferris_basic_streambuf_fourk
    >
class FERRISEXP_DLLLOCAL basic_readonly_iostream_streambuf
    :
    public ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers >
{
    typedef ferris_basic_streambuf< _CharT, _Traits, _Alloc, _BufferSizers > _Base;
    
public:
    
    typedef char_traits<_CharT>    traits_type;
    typedef typename traits_type::int_type  int_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::pos_type  pos_type;
    typedef typename traits_type::off_type  off_type;
    typedef typename _Base::seekd_t seekd_t;


    basic_readonly_iostream_streambuf( fh_istream ss )
        :
        RealStream(ss)
        {
        }

    virtual ~basic_readonly_iostream_streambuf()
        {
        }
    
private:

    fh_istream RealStream;

    // prohibit copy/assign
    basic_readonly_iostream_streambuf( const basic_readonly_iostream_streambuf& );
    basic_readonly_iostream_streambuf& operator = ( const basic_readonly_iostream_streambuf& );

protected:

    /**
     * This is the only methods that really needs to be here. It gets
     * up to maxsz data into buffer and returns how much data was really
     * read. Return 0 for a failure, you must read atleast one byte.
     */
    virtual int make_new_data_avail( char_type* buffer, streamsize maxsz )
        {
            RealStream->read( buffer, maxsz );
            int gcount = RealStream->gcount();
            return gcount;
        }
    
    /**
     * Write out the data starting at buffer of length sz to the "external"
     * device.
     * 
     * return -1 for error or 0 for success
     */
    virtual int write_out_given_data( const char_type* buffer, std::streamsize sz )
        {
            return -1;
        }
    
    virtual pos_type
    seekoff(off_type offset, seekd_t d, std::ios_base::openmode m)
        {
            return this->have_been_seeked( RealStream->rdbuf()->pubseekoff( offset, d, m ) );
        }

    virtual pos_type
    seekpos(pos_type pos, std::ios_base::openmode m)
        {
            return this->have_been_seeked( RealStream->rdbuf()->pubseekpos( pos, m ) );
        }
};




/*
 * If any blocks are written that are full of 0 then a hole is created instead.
 *
 * Note that this stream proxy needs to sit atop something that supports creation
 * of holes using lseek()
 */
template<
    class _CharT,
    class _Traits = std::char_traits<_CharT>,
    class _Alloc  = std::allocator   < _CharT >,
    class _BufferSizers = ferris_basic_streambuf_fourk
    >
class FERRISEXP_DLLLOCAL ferris_readonly_iostream
    :
    public Ferris_iostream< _CharT, _Traits >,
    public io_ferris_stream_traits< _CharT, _Traits >
{
    typedef ferris_readonly_iostream<_CharT, _Traits>    _Self;
    typedef basic_readonly_iostream_streambuf<_CharT, _Traits, _Alloc, _BufferSizers> _StreamBuf;

    typedef basic_readonly_iostream_streambuf<_CharT, _Traits, _Alloc, _BufferSizers> ss_impl_t;
    typedef Loki::SmartPtr<  ss_impl_t,
                             FerrisLoki::FerrisExRefCounted,
                             Loki::DisallowConversion,
                             Loki::AssertCheck,
                             FerrisLoki::FerrisExSmartPtrStorage > ss_t;
    ss_t ss;

public:
    
    typedef char_traits<_CharT>    traits_type;
    typedef typename traits_type::int_type  int_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::pos_type  pos_type;
    typedef typename traits_type::off_type  off_type;

    typedef emptystream_methods< char_type, traits_type > delegating_methods;
    

    explicit
    ferris_readonly_iostream( Ferris_istream< char_type, traits_type > _ss )
        :
        ss( new ss_impl_t( _ss ) )
        {
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }

    ferris_readonly_iostream( const ferris_readonly_iostream& rhs )
        :
        ss( rhs.ss )
        {
            this->init( rdbuf() );
            this->setsbT( GetImpl(ss) );
        }


    virtual ~ferris_readonly_iostream()
        {
        }
    
    _Self* operator->()
         {
             return this;
         }

    ss_impl_t*
    rdbuf() const
        {
            return GetImpl(ss);
        }

    enum
    {
        stream_readable = false,
        stream_writable = true
    };
};

typedef ferris_readonly_iostream<char> fh_readonly_iostream;

namespace Factory
{

    fh_iostream  
    MakeReadOnlyIOStream( fh_istream ss )
    {
        return fh_readonly_iostream( ss );
    }
    
    
};

    
    
};
