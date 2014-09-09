/*----------------------------------------------------------------------------
  ChucK Concurrent, On-the-fly Audio Programming Language
    Compiler and Virtual Machine

  Copyright (c) 2003 Ge Wang and Perry R. Cook.  All rights reserved.
    http://chuck.stanford.edu/
    http://chuck.cs.princeton.edu/

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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  U.S.A.
-----------------------------------------------------------------------------*/

//-----------------------------------------------------------------------------
// file: chuck_js.cpp
// desc: chuck javascript logic
//
// author: Arve Knudsen (arve.knudsen@gmail.com)
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "chuck_compile.h"
#include "chuck_vm.h"
#include "chuck_bbq.h"
#include "chuck_errmsg.h"
#include "chuck_lang.h"
#include "chuck_otf.h"
#include "chuck_shell.h"
#include "chuck_console.h"
#include "chuck_globals.h"

#include "util_math.h"
#include "util_string.h"
#include "util_network.h"
#include "ulib_machine.h"
#include "hidio_sdl.h"

#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/param.h>   // added 1.3.0.0

// default destination host name
char g_host[256] = "127.0.0.1";

void chuckMain()
{
    Chuck_Compiler * compiler = NULL;
    Chuck_VM * vm = NULL;
    Chuck_VM_Code * code = NULL;
    Chuck_VM_Shred * shred = NULL;

    t_CKBOOL enable_audio = TRUE;
    t_CKBOOL vm_halt = TRUE;
    t_CKUINT srate = SAMPLING_RATE_DEFAULT;
    t_CKBOOL force_srate = FALSE; // added 1.3.1.2
    t_CKUINT buffer_size = BUFFER_SIZE_DEFAULT;
    t_CKUINT num_buffers = NUM_BUFFERS_DEFAULT;
    t_CKUINT dac = 0;
    t_CKUINT adc = 0;
    std::string dac_name = ""; // added 1.3.0.0
    std::string adc_name = ""; // added 1.3.0.0
    t_CKUINT dac_chans = 2;
    t_CKUINT adc_chans = 2;
    t_CKBOOL dump = FALSE;
    t_CKBOOL probe = FALSE;
    t_CKBOOL set_priority = FALSE;
    t_CKBOOL auto_depend = FALSE;
    t_CKBOOL block = FALSE;
    t_CKBOOL enable_shell = FALSE;
    t_CKBOOL no_vm = FALSE;
    t_CKBOOL load_hid = FALSE;
    t_CKBOOL enable_server = TRUE;
    t_CKBOOL do_watchdog = TRUE;
    t_CKINT  adaptive_size = 0;
    t_CKINT  log_level = CK_LOG_CORE;
    t_CKINT  deprecate_level = 1; // 1 == warn
    t_CKINT  chugin_load = 1; // 1 == auto (variable added 1.3.0.0)
    string   filename = "";
    vector<string> args;

    // list of search pathes (added 1.3.0.0)
    std::list<std::string> dl_search_path;
    // initial chug-in path (added 1.3.0.0)
    std::string initial_chugin_path;
    // if set as environment variable (added 1.3.0.0)
    if( getenv( g_chugin_path_envvar ) )
    {
        // get it from the env var
        initial_chugin_path = getenv( g_chugin_path_envvar );
    }
    else
    {
        // default it
        initial_chugin_path = g_default_chugin_path;
    }
    // parse the colon list into STL list (added 1.3.0.0)
    parse_path_list( initial_chugin_path, dl_search_path );
    // list of individually named chug-ins (added 1.3.0.0)
    std::list<std::string> named_dls;

#if defined(__DISABLE_WATCHDOG__)
    do_watchdog = FALSE;
#elif defined(__MACOSX_CORE__)
    do_watchdog = TRUE;
#elif defined(__PLATFORM_WIN32__) && !defined(__WINDOWS_PTHREAD__)
    do_watchdog = TRUE;
#else
    do_watchdog = FALSE;
#endif

    t_CKUINT files = 0;
    t_CKUINT count = 1;
    t_CKINT i;

    // set log level
    EM_setlog( log_level );

    // parse command line args
    // for( i = 1; i < argc; i++ )
    // {
    //     if( argv[i][0] == '-' || argv[i][0] == '+' ||
    //         argv[i][0] == '=' || argv[i][0] == '^' || argv[i][0] == '@' )
    //     {
    //         if( !strcmp(argv[i], "--dump") || !strcmp(argv[i], "+d")
    //             || !strcmp(argv[i], "--nodump") || !strcmp(argv[i], "-d") )
    //             continue;
    //         else if( get_count( argv[i], &count ) )
    //             continue;
    //         else if( !strcmp(argv[i], "--audio") || !strcmp(argv[i], "-a") )
    //             enable_audio = TRUE;
    //         else if( !strcmp(argv[i], "--silent") || !strcmp(argv[i], "-s") )
    //             enable_audio = FALSE;
    //         else if( !strcmp(argv[i], "--halt") || !strcmp(argv[i], "-t") )
    //             vm_halt = TRUE;
    //         else if( !strcmp(argv[i], "--loop") || !strcmp(argv[i], "-l") )
    //         {   vm_halt = FALSE; enable_server = TRUE; }
    //         else if( !strcmp(argv[i], "--server") )
    //             enable_server = TRUE;
    //         else if( !strcmp(argv[i], "--standalone") )
    //             enable_server = FALSE;
    //         else if( !strcmp(argv[i], "--callback") )
    //             block = FALSE;
    //         else if( !strcmp(argv[i], "--blocking") )
    //             block = TRUE;
    //         else if( !strcmp(argv[i], "--hid") )
    //             load_hid = TRUE;
    //         else if( !strcmp(argv[i], "--shell") || !strcmp( argv[i], "-e" ) )
    //         {   enable_shell = TRUE; vm_halt = FALSE; }
    //         else if( !strcmp(argv[i], "--empty") )
    //             no_vm = TRUE;
    //         else if( !strncmp(argv[i], "--srate:", 8) ) // (added 1.3.0.0)
    //         {   srate = atoi( argv[i]+8 ) > 0 ? atoi( argv[i]+8 ) : srate; force_srate = TRUE; }
    //         else if( !strncmp(argv[i], "--srate", 7) )
    //         {   srate = atoi( argv[i]+7 ) > 0 ? atoi( argv[i]+7 ) : srate; force_srate = TRUE; }
    //         else if( !strncmp(argv[i], "-r", 2) )
    //         {   srate = atoi( argv[i]+2 ) > 0 ? atoi( argv[i]+2 ) : srate; force_srate = TRUE; }
    //         else if( !strncmp(argv[i], "--bufsize:", 10) ) // (added 1.3.0.0)
    //             buffer_size = atoi( argv[i]+10 ) > 0 ? atoi( argv[i]+10 ) : buffer_size;
    //         else if( !strncmp(argv[i], "--bufsize", 9) )
    //             buffer_size = atoi( argv[i]+9 ) > 0 ? atoi( argv[i]+9 ) : buffer_size;
    //         else if( !strncmp(argv[i], "-b", 2) )
    //             buffer_size = atoi( argv[i]+2 ) > 0 ? atoi( argv[i]+2 ) : buffer_size;
    //         else if( !strncmp(argv[i], "--bufnum:", 9) ) // (added 1.3.0.0)
    //             num_buffers = atoi( argv[i]+9 ) > 0 ? atoi( argv[i]+9 ) : num_buffers;
    //         else if( !strncmp(argv[i], "--bufnum", 8) )
    //             num_buffers = atoi( argv[i]+8 ) > 0 ? atoi( argv[i]+8 ) : num_buffers;
    //         else if( !strncmp(argv[i], "-n", 2) )
    //             num_buffers = atoi( argv[i]+2 ) > 0 ? atoi( argv[i]+2 ) : num_buffers;
    //         else if( !strncmp(argv[i], "--dac:", 6) ) // (added 1.3.0.0)
    //         {
    //             // advance pointer to beginning of argument
    //             const char * str = argv[i]+6;
    //             char * endptr = NULL;
    //             long dev = strtol(str, &endptr, 10);
    //
    //             // check if arg was a number (added 1.3.0.0)
    //             if( endptr != NULL && *endptr == '\0' )
    //             {
    //                 // successful conversion to # -- clear adc_name
    //                 dac = dev;
    //                 dac_name = "";
    //             }
    //             // check if arg was not a number and not empty (added 1.3.0.0)
    //             else if( *str != '\0' )
    //             {
    //                 // incomplete conversion to #; see if its a possible device name
    //                 dac_name = std::string(str);
    //             }
    //             else
    //             {
    //                 // error
    //                 fprintf( stderr, "[chuck]: invalid arguments for '--dac:'\n" );
    //                 fprintf( stderr, "[chuck]: (see 'chuck --help' for more info)\n" );
    //                 exit( 1 );
    //             }
    //         }
    //         else if( !strncmp(argv[i], "--dac", 5) )
    //             dac = atoi( argv[i]+6 ) > 0 ? atoi( argv[i]+6 ) : 0;
    //         else if( !strncmp(argv[i], "--adc:", 6) ) // (added 1.3.0.0)
    //         {
    //             // advance pointer to beginning of argument
    //             const char * str = argv[i]+6;
    //             char * endptr = NULL;
    //             long dev = strtol(str, &endptr, 10);
    //
    //             // check if arg was a number (added 1.3.0.0)
    //             if( endptr != NULL && *endptr == '\0' )
    //             {
    //                 // successful conversion to # -- clear adc_name
    //                 adc = dev;
    //                 adc_name = "";
    //             }
    //             // check if arg was number a number and not empty (added 1.3.0.0)
    //             else if( *str != '\0' )
    //             {
    //                 // incomplete conversion to #; see if its a possible device name
    //                 adc_name = std::string(str);
    //             }
    //             else
    //             {
    //                 // error
    //                 fprintf( stderr, "[chuck]: invalid arguments for '--adc:'\n" );
    //                 fprintf( stderr, "[chuck]: (see 'chuck --help' for more info)\n" );
    //                 exit( 1 );
    //             }
    //         }
    //         else if( !strncmp(argv[i], "--adc", 5) )
    //             adc = atoi( argv[i]+5 ) > 0 ? atoi( argv[i]+5 ) : 0;
    //         else if( !strncmp(argv[i], "--channels:", 11) ) // (added 1.3.0.0)
    //             dac_chans = adc_chans = atoi( argv[i]+11 ) > 0 ? atoi( argv[i]+11 ) : 2;
    //         else if( !strncmp(argv[i], "--channels", 10) )
    //             dac_chans = adc_chans = atoi( argv[i]+10 ) > 0 ? atoi( argv[i]+10 ) : 2;
    //         else if( !strncmp(argv[i], "-c", 2) )
    //             dac_chans = adc_chans = atoi( argv[i]+2 ) > 0 ? atoi( argv[i]+2 ) : 2;
    //         else if( !strncmp(argv[i], "--out:", 6) ) // (added 1.3.0.0)
    //             dac_chans = atoi( argv[i]+6 ) > 0 ? atoi( argv[i]+6 ) : 2;
    //         else if( !strncmp(argv[i], "--out", 5) )
    //             dac_chans = atoi( argv[i]+5 ) > 0 ? atoi( argv[i]+5 ) : 2;
    //         else if( !strncmp(argv[i], "-o", 2) )
    //             dac_chans = atoi( argv[i]+2 ) > 0 ? atoi( argv[i]+2 ) : 2;
    //         else if( !strncmp(argv[i], "--in:", 5) ) // (added 1.3.0.0)
    //             adc_chans = atoi( argv[i]+5 ) > 0 ? atoi( argv[i]+5 ) : 2;
    //         else if( !strncmp(argv[i], "--in", 4) )
    //             adc_chans = atoi( argv[i]+4 ) > 0 ? atoi( argv[i]+4 ) : 2;
    //         else if( !strncmp(argv[i], "-i", 2) )
    //             adc_chans = atoi( argv[i]+2 ) > 0 ? atoi( argv[i]+2 ) : 2;
    //         else if( !strncmp(argv[i], "--level:", 8) ) // (added 1.3.0.0)
    //         {   g_priority = atoi( argv[i]+8 ); set_priority = TRUE; }
    //         else if( !strncmp(argv[i], "--level", 7) )
    //         {   g_priority = atoi( argv[i]+7 ); set_priority = TRUE; }
    //         else if( !strncmp(argv[i], "--watchdog", 10) )
    //         {   g_watchdog_timeout = atof( argv[i]+10 );
    //             if( g_watchdog_timeout <= 0 ) g_watchdog_timeout = 0.5;
    //             do_watchdog = TRUE; }
    //         else if( !strncmp(argv[i], "--nowatchdog", 12) )
    //             do_watchdog = FALSE;
    //         else if( !strncmp(argv[i], "--remote:", 9) ) // (added 1.3.0.0)
    //             strcpy( g_host, argv[i]+9 );
    //         else if( !strncmp(argv[i], "--remote", 8) )
    //             strcpy( g_host, argv[i]+8 );
    //         else if( !strncmp(argv[i], "@", 1) )
    //             strcpy( g_host, argv[i]+1 );
    //         else if( !strncmp(argv[i], "--port:", 7) ) // (added 1.3.0.0)
    //             g_port = atoi( argv[i]+7 );
    //         else if( !strncmp(argv[i], "--port", 6) )
    //             g_port = atoi( argv[i]+6 );
    //         else if( !strncmp(argv[i], "-p", 2) )
    //             g_port = atoi( argv[i]+2 );
    //         else if( !strncmp(argv[i], "--auto", 6) )
    //             auto_depend = TRUE;
    //         else if( !strncmp(argv[i], "-u", 2) )
    //             auto_depend = TRUE;
    //         else if( !strncmp(argv[i], "--log:", 6) ) // (added 1.3.0.0)
    //             log_level = argv[i][6] ? atoi( argv[i]+6 ) : CK_LOG_INFO;
    //         else if( !strncmp(argv[i], "--log", 5) )
    //             log_level = argv[i][5] ? atoi( argv[i]+5 ) : CK_LOG_INFO;
    //         else if( !strncmp(argv[i], "--verbose:", 10) ) // (added 1.3.0.0)
    //             log_level = argv[i][10] ? atoi( argv[i]+10 ) : CK_LOG_INFO;
    //         else if( !strncmp(argv[i], "--verbose", 9) )
    //             log_level = argv[i][9] ? atoi( argv[i]+9 ) : CK_LOG_INFO;
    //         else if( !strncmp(argv[i], "-v", 2) )
    //             log_level = argv[i][2] ? atoi( argv[i]+2 ) : CK_LOG_INFO;
    //         else if( !strncmp(argv[i], "--adaptive:", 11) ) // (added 1.3.0.0)
    //             adaptive_size = argv[i][11] ? atoi( argv[i]+11 ) : -1;
    //         else if( !strncmp(argv[i], "--adaptive", 10) )
    //             adaptive_size = argv[i][10] ? atoi( argv[i]+10 ) : -1;
    //         else if( !strncmp(argv[i], "--deprecate", 11) )
    //         {
    //             // get the rest
    //             string arg = argv[i]+11;
    //             if( arg == ":stop" ) deprecate_level = 0;
    //             else if( arg == ":warn" ) deprecate_level = 1;
    //             else if( arg == ":ignore" ) deprecate_level = 2;
    //             else
    //             {
    //                 // error
    //                 fprintf( stderr, "[chuck]: invalid arguments for '--deprecate'...\n" );
    //                 fprintf( stderr, "[chuck]: ... (looking for :stop, :warn, or :ignore)\n" );
    //                 exit( 1 );
    //             }
    //         }
    //         // (added 1.3.0.0)
    //         else if( !strncmp(argv[i], "--chugin-load:", sizeof("--chugin-load:")-1) )
    //         {
    //             // get the rest
    //             string arg = argv[i]+sizeof("--chugin-load:")-1;
    //             if( arg == "off" ) chugin_load = 0;
    //             else if( arg == "auto" ) chugin_load = 1;
    //             else
    //             {
    //                 // error
    //                 fprintf( stderr, "[chuck]: invalid arguments for '--chugin-load'...\n" );
    //                 fprintf( stderr, "[chuck]: ... (looking for :auto or :off)\n" );
    //                 exit( 1 );
    //             }
    //         }
    //         // (added 1.3.0.0)
    //         else if( !strncmp(argv[i], "--chugin-path:", sizeof("--chugin-path:")-1) )
    //         {
    //             // get the rest
    //             dl_search_path.push_back( argv[i]+sizeof("--chugin-path:")-1 );
    //         }
    //         // (added 1.3.0.0)
    //         else if( !strncmp(argv[i], "-G", sizeof("-G")-1) )
    //         {
    //             // get the rest
    //             dl_search_path.push_back( argv[i]+sizeof("-G")-1 );
    //         }
    //         // (added 1.3.0.0)
    //         else if( !strncmp(argv[i], "--chugin:", sizeof("--chugin:")-1) )
    //         {
    //             named_dls.push_back(argv[i]+sizeof("--chugin:")-1);
    //         }
    //         // (added 1.3.0.0)
    //         else if( !strncmp(argv[i], "-g", sizeof("-g")-1) )
    //         {
    //             named_dls.push_back(argv[i]+sizeof("-g")-1);
    //         }
    //         else if( !strcmp( argv[i], "--probe" ) )
    //             probe = TRUE;
    //         else if( !strcmp( argv[i], "--poop" ) )
    //             uh();
    //         else if( !strcmp( argv[i], "--caution-to-the-wind" ) )
    //             g_enable_system_cmd = TRUE;
    //         else if( !strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")
    //              || !strcmp(argv[i], "--about") )
    //         {
    //             usage();
    //             exit( 2 );
    //         }
    //         else if( !strcmp( argv[i], "--version" ) )
    //         {
    //             version();
    //             exit( 2 );
    //         }
    //         else
    //         {
    //             // boost log level
    //             g_otf_log = CK_LOG_CORE;
    //             // flag
    //             int is_otf = FALSE;
    //             // log level
    //             EM_setlog( log_level );
    //             // do it
    //             if( otf_send_cmd( argc, argv, i, g_host, g_port, &is_otf ) )
    //                 exit( 0 );
    //
    //             // is otf
    //             if( is_otf ) exit( 1 );
    //
    //             // done
    //             fprintf( stderr, "[chuck]: invalid flag '%s'\n", argv[i] );
    //             usage();
    //             exit( 1 );
    //         }
    //     }
    //     else // doesn't look like an argument
    //     {
    //         // increase number of files
    //         files++;
    //     }
    // }

    // log level
    EM_setlog( log_level );

    // probe
    if( probe )
    {
        Digitalio::probe();

#ifndef __DISABLE_MIDI__
        EM_error2b( 0, "" );
        probeMidiIn();
        EM_error2b( 0, "" );
        probeMidiOut();
        EM_error2b( 0, "" );
#endif  // __DISABLE_MIDI__

        // HidInManager::probeHidIn();

        // exit
        exit( 0 );
    }

    // check buffer size
    buffer_size = ensurepow2( buffer_size );
    // check mode and blocking
    if( !enable_audio && !block ) block = TRUE;
    // // set priority
    // Chuck_VM::our_priority = g_priority_low;

    // make sure vm
    if( no_vm )
    {
        fprintf( stderr, "[chuck]: '--empty' can only be used with shell...\n" );
        exit( 1 );
    }

    // find dac_name if appropriate (added 1.3.0.0)
    if( dac_name.size() > 0 )
    {
        // check with RtAudio
        int dev = Digitalio::device_named( dac_name, TRUE, FALSE );
        if( dev >= 0 )
        {
            dac = dev;
        }
        else
        {
            fprintf( stderr, "[chuck]: unable to find dac '%s'...\n", dac_name.c_str() );
            fprintf( stderr, "[chuck]: | (try --probe to enumerate audio device info)\n" );
            exit( 1 );
        }
    }

    // find adc_name if appropriate (added 1.3.0.0)
    if( adc_name.size() > 0 )
    {
        // check with RtAudio
        int dev = Digitalio::device_named( adc_name, FALSE, TRUE );
        if( dev >= 0 )
        {
            adc = dev;
        }
        else
        {
            fprintf( stderr, "[chuck]: unable to find adc '%s'...\n", adc_name.c_str() );
            fprintf( stderr, "[chuck]: | (try --probe to enumerate audio device info)\n" );
            exit( 1 );
        }
    }

    // allocate the vm - needs the type system
    vm = g_vm = new Chuck_VM;
    if( !vm->initialize( enable_audio, vm_halt, srate, buffer_size,
                         num_buffers, dac, adc, dac_chans, adc_chans,
                         block, adaptive_size, force_srate ) )
    {
        fprintf( stderr, "[chuck]: %s\n", vm->last_error() );
        exit( 1 );
    }

    // if chugin load is off, then clear the lists (added 1.3.0.0 -- TODO: refactor)
    if( chugin_load == 0 )
    {
        // turn off chugin load
        dl_search_path.clear();
        named_dls.clear();
    }

    // allocate the compiler
    compiler = g_compiler = new Chuck_Compiler;
    // initialize the compiler (search_apth and named_dls added 1.3.0.0 -- TODO: refactor)
    if( !compiler->initialize( vm, dl_search_path, named_dls ) )
    {
        fprintf( stderr, "[chuck]: error initializing compiler...\n" );
        exit( 1 );
    }
    // enable dump
    compiler->emitter->dump = dump;
    // set auto depend
    compiler->set_auto_depend( auto_depend );

    // vm synthesis subsystem - needs the type system
    if( !vm->initialize_synthesis( ) )
    {
        fprintf( stderr, "[chuck]: %s\n", vm->last_error() );
        exit( 1 );
    }

#ifndef __ALTER_HID__
    // pre-load hid
    if( load_hid ) HidInManager::init();
#endif // __ALTER_HID__

    // set deprecate
    compiler->env->deprecate_level = deprecate_level;

    // reset count
    count = 1;

    // whether or not chug should be enabled (added 1.3.0.0)
    if( chugin_load )
    {
        // log
        EM_log( CK_LOG_SEVERE, "pre-loading ChucK libs..." );
        EM_pushlog();

        // iterate over list of ck files that the compiler found
        for( std::list<std::string>::iterator j = compiler->m_cklibs_to_preload.begin();
             j != compiler->m_cklibs_to_preload.end(); j++)
        {
            // the filename
            std::string filename = *j;

            // log
            EM_log( CK_LOG_SEVERE, "preloading '%s'...", filename.c_str() );
            // push indent
            EM_pushlog();

            // SPENCERTODO: what to do for full path
            std::string full_path = filename;

            // parse, type-check, and emit
            if( compiler->go( filename, NULL, NULL, full_path ) )
            {
                // TODO: how to compilation handle?
                //return 1;

                // get the code
                code = compiler->output();
                // name it - TODO?
                // code->name += string(argv[i]);

                // spork it
                shred = vm->spork( code, NULL );
            }

            // pop indent
            EM_poplog();
        }

        // clear the list of chuck files to preload
        compiler->m_cklibs_to_preload.clear();

        // pop log
        EM_poplog();
    }

    compiler->env->load_user_namespace();

    // log
    EM_log( CK_LOG_SEVERE, "starting compilation..." );
    // push indent
    EM_pushlog();

    // pop indent
    EM_poplog();

    // reset the parser
    reset_parse();

    // boost priority
    if( Chuck_VM::our_priority != 0x7fffffff )
    {
        // try
        if( !Chuck_VM::set_priority( Chuck_VM::our_priority, vm ) )
        {
            // error
            fprintf( stderr, "[chuck]: %s\n", vm->last_error() );
            exit( 1 );
        }
    }

    // server
    if( enable_server )
    {
#ifndef __DISABLE_OTF_SERVER__
        // log
        EM_log( CK_LOG_SYSTEM, "starting listener on port: %d...", g_port );

        // start tcp server
        g_sock = ck_tcp_create( 1 );
        if( !g_sock || !ck_bind( g_sock, g_port ) || !ck_listen( g_sock, 10 ) )
        {
            fprintf( stderr, "[chuck]: cannot bind to tcp port %li...\n", g_port );
            ck_close( g_sock );
            g_sock = NULL;
        }
        else
        {
#ifndef __EMSCRIPTEN__
    #if !defined(__PLATFORM_WIN32__) || defined(__WINDOWS_PTHREAD__)
            pthread_create( &g_tid_otf, NULL, otf_cb, NULL );
    #else
            g_tid_otf = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)otf_cb, NULL, 0, 0 );
    #endif
#endif
        }
#endif // __DISABLE_OTF_SERVER__
    }
    else
    {
        // log
        EM_log( CK_LOG_SYSTEM, "OTF server/listener: OFF" );
    }

    // start shell on separate thread
    if( enable_shell )
    {
#ifndef __EMSCRIPTEN__
#if !defined(__PLATFORM_WIN32__) || defined(__WINDOWS_PTHREAD__)
        pthread_create( &g_tid_shell, NULL, shell_cb, g_shell );
#else
        g_tid_shell = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)shell_cb, g_shell, 0, 0 );
#endif
#endif
    }

    // run the vm
    vm->run();

    // detach
    all_detach();

    // free vm
    vm = NULL; SAFE_DELETE( g_vm );
    // free the compiler
    compiler = NULL; SAFE_DELETE( g_compiler );
}
