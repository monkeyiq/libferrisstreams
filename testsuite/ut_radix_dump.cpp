/******************************************************************************
*******************************************************************************
*******************************************************************************

    Copyright (C) 2003 Ben Martin

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

    $Id: ut_radix_dump.cpp,v 1.3 2010/09/24 05:05:45 ben Exp $

*******************************************************************************
*******************************************************************************
******************************************************************************/

/*
 * return 0 for success
 * return 1 for generic error
*/

#include <FerrisStreams/Streams.hh>

#include <popt.h>
#include <unistd.h>

#include <string>
#include <vector>
#include <map>
#include <list>
#include <iterator>
#include <iostream>

using namespace std;
using namespace Ferris;

const string PROGRAM_NAME = "ut_radix_dump";

void usage(poptContext optCon, int exitcode, char *error, char *addl)
{
    poptPrintUsage(optCon, stderr, 0);
    if (error) fprintf(stderr, "%s: %s0", error, addl);
    exit(exitcode);
}

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

int errors = 0;

fh_ostream& E()
{
    ++errors;
    static fh_ostream ret = Factory::fcerr();
    return ret;
}



void runtest()
{
    string buf = "Hello there, I will be radix dumped for you. I have been made long for the purpose of testing the new line and comma seperation code.";
    fh_stringstream ss;
    radixdump( ss, buf.data(), buf.data() + buf.length(), 16, 80, 4 );

    string expected =
        "48656c6c, 6f207468, 6572652c, 20492077, 696c6c20, 62652072, 61646978, 2064756d, \n"
        "70656420, 666f7220, 796f752e, 20492068, 61766520, 6265656e, 206d6164, 65206c6f, \n"
        "6e672066, 6f722074, 68652070, 7572706f, 7365206f, 66207465, 7374696e, 67207468, \n"
        "65206e65, 77206c69, 6e652061, 6e642063, 6f6d6d61, 20736570, 65726174, 696f6e20, \n"
        "636f6465, 2e";

    string readv = ss.str();
    int cutlen = 81 * 4;
    
    readv    = readv.substr( 0, cutlen );
    expected = expected.substr( 0, cutlen );
    
    if( expected != readv )
    {
        E() << "Didn't get back expected radix dump output" << endl;
        cerr << readv.length() << " " << expected.length() << endl;
        
        copy( readv.begin(), readv.end(), ostreambuf_iterator<char>(cerr));
        cerr << "------------------------------" << endl;
        copy( expected.begin(), expected.end(), ostreambuf_iterator<char>(cerr));
    }
}


int main( int argc, char** argv )
{
    int exit_status = 0;
    
    try
    {
        unsigned long Verbose              = 0;

        struct poptOption optionsTable[] =
            {
                { "verbose", 'v', POPT_ARG_NONE, &Verbose, 0,
                  "show what is happening", "" },

                POPT_AUTOHELP
                POPT_TABLEEND
            };
        poptContext optCon;

        optCon = poptGetContext(PROGRAM_NAME.c_str(), argc, (const char**)argv, optionsTable, 0);
        poptSetOtherOptionHelp(optCon, "[OPTIONS]*  ...");

        /* Now do options processing */
        char c=-1;
        while ((c = poptGetNextOpt(optCon)) >= 0)
        {}

        runtest();
    }
    catch( exception& e )
    {
        cerr << "cought error:" << e.what() << endl;
        exit(1);
    }
    if( !errors )
        cerr << "Success" << endl;
    else
        cerr << "error: count:" << errors << endl;
    
    return exit_status;
}
