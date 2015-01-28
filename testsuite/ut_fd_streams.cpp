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

    $Id: ut_fd_streams.cpp,v 1.2 2010/09/24 05:05:44 ben Exp $

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

const string PROGRAM_NAME = "ut_fd_streams";

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
    int filedes[2];
    int rc  = pipe( filedes );
    int ifd = filedes[0];
    int ofd = filedes[1];

    if( rc != 0 )
    {
        E() << "pipe() failed" << endl;
        exit(1);
    }
    
    fh_ostream oss = Factory::MakeFdOStream( ofd );
    fh_istream iss = Factory::MakeFdIStream( ifd );
    string data = "He slaps the keys, he's tedious.";
    string readv;
    if( !(oss << data << endl ))
        E() << "writing" << endl;
    if( !getline( iss, readv ) )
        E() << "reading" << endl;

    if( readv != data )
        E() << "read wrong data." << endl
            << " expecting:" << data
            << " got:" << readv
            << endl;
    
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
    return exit_status;
}
