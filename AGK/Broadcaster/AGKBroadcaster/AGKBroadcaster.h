#ifndef _H_AGK_BROADCASTER
#define _H_AGK_BROADCASTER

#include "agk.h"
#include <stdio.h>
#include <iostream>

using namespace AGK;

int RunBroadcaster( const char* cmdline );
int ReadStdInput( char* output, int maxlength );
void WriteStdOutput( const char* output );
void WriteStdOutputPrompt();

void WriteError( const char *msg );
void WriteWarning( const char *msg );
void WriteInfo( const char *msg );
void WriteDebug( const char *type, const char *data );
void WriteVariable( const char *varname, const char *value );
void WriteBreak( const char *szInclude, int line );

#endif