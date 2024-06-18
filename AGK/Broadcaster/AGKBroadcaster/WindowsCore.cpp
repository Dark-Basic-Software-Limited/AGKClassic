#include "AGKBroadcaster.h"

int interactive = 1;
HANDLE hStdIn = 0;
HANDLE hStdOut = 0;

cLock g_lockOutput;

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, char* cmdline, int showCmd )
{
    if ( strncmp( cmdline, "-nowindow", strlen("-nowindow") ) == 0 ) interactive = 0;
    
	if ( interactive )
	{
		AllocConsole();
		hStdIn = GetStdHandle ( STD_INPUT_HANDLE );
		hStdOut = GetStdHandle ( STD_OUTPUT_HANDLE );
	}
    
    return RunBroadcaster( cmdline );
}

int ReadStdInput( char* output, int maxlength )
{
	if ( !interactive )
	{
		return fgets( output, maxlength, stdin ) ? 1 : 0;
	}
	else
	{
		DWORD written;
		if ( ReadConsole( hStdIn, output, maxlength, &written, 0 ) ) return 1;
		else return 0;
	}
}

void WriteStdOutput( const char* output )
{
	cAutoLock lock(&g_lockOutput);

	if ( !interactive )
	{
		fputs( output, stdout );
		fputs( "\n", stdout );
		fflush( stdout );
	}
	else
	{
		DWORD written;
		WriteConsole( hStdOut, output, strlen(output), &written, 0 );
		WriteConsole( hStdOut, "\n", strlen("\n"), &written, 0 );
	}
}

void WriteStdOutputPrompt()
{
	if ( !interactive ) return;

	cAutoLock lock(&g_lockOutput);

	DWORD written;
	WriteConsole( hStdOut, "(AGK) ", strlen("(AGK) "), &written, 0 );
}