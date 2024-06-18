#include "AGKBroadcaster.h"

int interactive = 1;
cLock g_lockOutput;

int main( int argc, char** argv )
{
	if ( argc > 1 && strcmp( argv[1], "-nowindow" ) == 0 ) interactive = 0;

    uString sCmdLine;
    for( int i = 1; i < argc; i++ )
    {
        if ( i != 1 ) sCmdLine.AppendAscii( ' ' );
        sCmdLine.Append( argv[i] );
    }
    return RunBroadcaster( sCmdLine.GetStr() );
}

int ReadStdInput( char* output, int maxlength )
{
	return fgets( output, maxlength, stdin ) ? 1 : 0;
}

void WriteStdOutput( const char* output )
{
	cAutoLock lock(&g_lockOutput);

	fputs( output, stdout );
	fputs( "\n", stdout );
	fflush( stdout );
}

void WriteStdOutputPrompt()
{
	if ( !interactive ) return;

	cAutoLock lock(&g_lockOutput);

	fputs( "(AGK) ", stdout );
	fflush( stdout );
}
