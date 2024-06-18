
#include "AGKBroadcaster.h"
#include "Listener.h"

extern int interactive;

void WriteError( const char *msg )
{
	if ( interactive )
	{
		uString sMsg( msg );
		sMsg.Prepend( "Error: " );
		WriteStdOutput( sMsg.GetStr() );
	}
	else
	{
		uString sMsg( msg );
		sMsg.Prepend( "Error:" );
		WriteStdOutput( sMsg.GetStr() );
	}
}

void WriteWarning( const char *msg )
{
	if ( interactive )
	{
		uString sMsg( msg );
		sMsg.Prepend( "Warning: " );
		WriteStdOutput( sMsg.GetStr() );
	}
	else
	{
		uString sMsg( msg );
		sMsg.Prepend( "Warning:" );
		WriteStdOutput( sMsg.GetStr() );
	}
}

void WriteInfo( const char *msg )
{
	if ( interactive )
	{
		WriteStdOutput( msg );
	}
	else
	{
		uString sMsg( msg );
		sMsg.Prepend( "Log:" );
		WriteStdOutput( sMsg.GetStr() );
	}
}

void WriteDebug( const char *type, const char *data )
{
	//MessageBox( NULL, "Don't know what to do with debug messages yet", "Error", 0 );
	if ( strcmp( type, "Log" ) == 0 )
	{
		WriteInfo( data );
	}
	else if ( strcmp( type, "Variable" ) == 0 )
	{
		const char *colon = strchr( data, ':' );
		if ( colon && colon != data && *(colon+1) ) // colon exists and it is not at the beginning or end of the string
		{
			UINT length = (UINT)(colon-data);
			uString varName;
			varName.SetStrN( data, length );
			WriteVariable( varName, data+length+1 );
		}
	}
	else if ( strcmp( type, "Frame" ) == 0 )
	{
		if ( interactive )
		{
			uString sFrame( "Frame " );
			sFrame.Append( data );
			WriteStdOutput( sFrame.GetStr() );
		}
		else
		{
			uString sFrame( "Frame:" );
			sFrame.Append( data );
			WriteStdOutput( sFrame.GetStr() );
		}
	}
}

void WriteVariable( const char *varname, const char *value )
{
	if ( interactive )
	{
		uString sMsg( varname );
		sMsg.Append( " = " );
		sMsg.Append( value );
		WriteStdOutput( sMsg.GetStr() );
	}
	else
	{
		uString sMsg( "Variable:" );
		sMsg.Append( varname );
		sMsg.Append( ":" );
		sMsg.Append( value );
		WriteStdOutput( sMsg.GetStr() );
	}
}

void WriteBreak( const char *szInclude, int line )
{
	if ( interactive )
	{
		uString sMsg;
		sMsg.Format( "Breakpoint hit in %s on line %d", szInclude, line );
		WriteStdOutput( sMsg.GetStr() );
	}
	else
	{
		uString sMsg;
		sMsg.Format( "Break:%s:%d", szInclude, line );
		WriteStdOutput( sMsg.GetStr() );
	}
}


// ***********
// Main
// ***********

int RunBroadcaster( const char* cmdline )
{
    agk::InitConsole();

	AGKListener g_Listener;
	g_Listener.SetPort( 5689, 5691 );
		
	int project_set = 0;
	char data[ 256 ];
	//char output[ 256 ];

	do
	{
		WriteStdOutputPrompt();

		if ( !ReadStdInput( data, 256 ) ) break;
		
		char* newline = strchr( data, '\r' );
		if ( !newline ) newline = strchr( data, '\n' );
		if ( newline ) *newline = 0;

		if ( strcmp(data,"list") == 0 )
		{
			uString devices;
			g_Listener.GetDeviceList( devices );
			WriteStdOutput( devices.GetStr() );
		}
		else if ( strncmp(data,"setproject",strlen("setproject")) == 0 )
		{
			const char* path = data + strlen("setproject");
			if ( *path != ' ' )
			{
				WriteStdOutput( "setproject must be followed by a space and a project path" );
				continue;
			}

			path++;	
			uString sPath( path );
			sPath.Trim( " " );
			if ( sPath.GetLength() == 0 )
			{
				WriteStdOutput( "setproject must be followed by a space and a project path" );
				continue;
			}

			agk::SetRawWritePath( sPath );
			agk::SetFolder( "" );
			if ( agk::GetFileExists( "main.agc" ) == 0 )
			{
				WriteStdOutput( "Not a valid AGK project path" );
				continue;
			}
			
			if ( agk::GetFileExists( "media/bytecode.byc" ) == 0 )
			{
				WriteStdOutput( "Project has not yet been compiled" );
				continue;
			}

			agk::SetFolder( "" );
			//SetCurrentDirectory( sPath.GetStr() );

			if ( sPath.CharAt(sPath.GetLength()-1) == '/' || sPath.CharAt(sPath.GetLength()-1) == '\\' ) sPath.Trunc2(1);
			const char *szSlashF = strrchr( sPath.GetStr(), '/' );
			const char *szSlashB = strrchr( sPath.GetStr(), '\\' );
			if ( szSlashF || szSlashB )
			{
				if ( szSlashF > szSlashB ) g_Listener.SetAppName( szSlashF+1 );
				else g_Listener.SetAppName( szSlashB+1 );
			}
			else g_Listener.SetAppName( "Unknown" );

			project_set = 1;
			g_Listener.ReadProjectFiles();
		}
		else if ( strcmp(data,"connectall") == 0 )
		{
			if ( !project_set )
			{
				WriteStdOutput( "Cannot connect until a project path is set, use \"setproject <path>\" first" );
				continue;
			}

			g_Listener.SetConnectAll( 1 );
			if ( !g_Listener.IsRunning() ) g_Listener.Start();
		}
		else if ( strncmp(data,"connect",strlen("connect")) == 0 )
		{
			const char* szIP = data + strlen("connect");
			if ( *szIP != ' ' )
			{
				WriteStdOutput( "connect must be followed by a space and an IP address" );
				continue;
			}

			szIP++;	
			uString sIP( szIP );
			sIP.Trim( " " );
			if ( sIP.GetLength() == 0 )
			{
				WriteStdOutput( "connect must be followed by a space and an IP address" );
				continue;
			}

			if ( !project_set )
			{
				WriteStdOutput( "Cannot connect until a project path is set, use \"setproject <path>\" first" );
				continue;
			}

			// loopback address doesn't appear to work, discover local IP instead
			// correction: loopback works if the receiver is bound to INADDR_ANY
			/*
			if ( sIP.CompareTo( "127.0.0.1" ) == 0 )
			{
				char* temp = agk::GetDeviceIP();
				sIP.SetStr( temp );
				delete [] temp;
			}
			*/

			if ( strchr(szIP, ':') != 0 ) g_Listener.AddDevice( sIP, 5692 ); // IPv6
			else g_Listener.AddDevice( sIP, 5690 ); // IPv4
			if ( !g_Listener.IsRunning() ) g_Listener.Start();
		}
		else if ( strcmp(data,"disconnectall") == 0 )
		{
			if ( !g_Listener.IsRunning() ) 
			{
				WriteStdOutput( "Broadcaster is not currently connected" );
				continue;
			}

			g_Listener.Stop();
			g_Listener.Join();
			g_Listener.Disconnect();
		}
		else if ( strncmp(data,"breakpoint",strlen("breakpoint")) == 0 )
		{
			const char* szBreakpoint = data + strlen("breakpoint");
			if ( *szBreakpoint != ' ' )
			{
				WriteStdOutput( "breakpoint must be followed by a space then a filename and a line number like so, breakpoint myFile.agc:102" );
				continue;
			}

			szBreakpoint++;
			uString sBreakpoint( szBreakpoint );
			sBreakpoint.Trim( " " );
			if ( sBreakpoint.GetLength() == 0 || sBreakpoint.Find( ':' ) < 0 || sBreakpoint.CountTokens2( ':' ) != 2 )
			{
				WriteStdOutput( "breakpoint must be followed by a space then a filename and a line number like so, breakpoint myFile.agc:102" );
				continue;
			}

			uString *params = 0;
			sBreakpoint.SplitTokens2( ':', params );

			if ( !g_Listener.IsRunning() ) 
			{
				WriteStdOutput( "Broadcaster is not currently connected" );
				continue;
			}

			g_Listener.AddBreakpoint( params[0], params[1].ToInt() );
		}
		else if ( strncmp(data,"delete breakpoint",strlen("delete breakpoint")) == 0 )
		{
			const char* szBreakpoint = data + strlen("delete breakpoint");
			if ( *szBreakpoint != ' ' )
			{
				WriteStdOutput( "delete breakpoint must be followed by a space then a filename and a line number like so, delete breakpoint myFile.agc:102" );
				continue;
			}

			szBreakpoint++;
			uString sBreakpoint( szBreakpoint );
			sBreakpoint.Trim( " " );
			if ( sBreakpoint.GetLength() == 0 || sBreakpoint.Find( ':' ) < 0 || sBreakpoint.CountTokens2( ':' ) != 2 )
			{
				WriteStdOutput( "delete breakpoint must be followed by a space then a filename and a line number like so, delete breakpoint myFile.agc:102" );
				continue;
			}

			uString *params = 0;
			sBreakpoint.SplitTokens2( ':', params );

			if ( !g_Listener.IsRunning() ) 
			{
				WriteStdOutput( "Broadcaster is not currently connected" );
				continue;
			}

			g_Listener.RemoveBreakpoint( params[0], params[1].ToInt() );
		}
		else if ( strcmp(data,"delete all breakpoints") == 0 )
		{
			if ( !g_Listener.IsRunning() ) 
			{
				WriteStdOutput( "Broadcaster is not currently connected" );
				continue;
			}

			g_Listener.RemoveAllBreakpoints();
		}
		else if ( strncmp(data,"watch",strlen("watch")) == 0 )
		{
			const char* szVar = data + strlen("watch");
			if ( *szVar != ' ' )
			{
				WriteStdOutput( "watch must be followed by a space then a variable expression like so, watch myArray[9]" );
				continue;
			}

			szVar++;
			uString sVar( szVar );
			sVar.Trim( " " );
			if ( sVar.GetLength() == 0 )
			{
				WriteStdOutput( "watch must be followed by a space then a variable expression like so, watch myArray[9]" );
				continue;
			}

			if ( !g_Listener.IsRunning() ) 
			{
				WriteStdOutput( "Broadcaster is not currently connected" );
				continue;
			}

			g_Listener.AddVariableWatch( sVar );
		}
		else if ( strncmp(data,"delete watch",strlen("delete watch")) == 0 )
		{
			const char* szVar = data + strlen("delete watch");
			if ( *szVar != ' ' )
			{
				WriteStdOutput( "delete watch must be followed by a space then a variable expression like so, delete watch myArray[9]" );
				continue;
			}

			szVar++;
			uString sVar( szVar );
			sVar.Trim( " " );
			if ( sVar.GetLength() == 0 )
			{
				WriteStdOutput( "delete watch must be followed by a space then a variable expression like so, delete watch myArray[9]" );
				continue;
			}

			if ( !g_Listener.IsRunning() ) 
			{
				WriteStdOutput( "Broadcaster is not currently connected" );
				continue;
			}

			g_Listener.RemoveVariableWatch( sVar );
		}
		else if ( strcmp(data,"delete all watches") == 0 )
		{
			if ( !g_Listener.IsRunning() ) 
			{
				WriteStdOutput( "Broadcaster is not currently connected" );
				continue;
			}

			g_Listener.RemoveAllWatchVariables();
		}
		else if ( strcmp(data,"run") == 0 )
		{
			if ( !g_Listener.IsRunning() ) 
			{
				WriteStdOutput( "Broadcaster is not currently connected" );
				continue;
			}

			g_Listener.RunApp();
		}
		else if ( strcmp(data,"debug") == 0 )
		{
			if ( !g_Listener.IsRunning() ) 
			{
				WriteStdOutput( "Broadcaster is not currently connected" );
				continue;
			}

			g_Listener.DebugApp();
		}
		else if ( strcmp(data,"stop") == 0 )
		{
			if ( !g_Listener.IsRunning() ) 
			{
				WriteStdOutput( "Broadcaster is not currently connected" );
				continue;
			}

			g_Listener.StopApp();
		}
		else if ( strcmp(data,"pause") == 0 )
		{
			if ( !g_Listener.IsRunning() ) 
			{
				WriteStdOutput( "Broadcaster is not currently connected" );
				continue;
			}

			g_Listener.PauseApp();
		}
		else if ( strcmp(data,"continue") == 0 )
		{
			if ( !g_Listener.IsRunning() ) 
			{
				WriteStdOutput( "Broadcaster is not currently connected" );
				continue;
			}

			g_Listener.ResumeApp();
		}
		else if ( strcmp(data,"step") == 0 )
		{
			if ( !g_Listener.IsRunning() ) 
			{
				WriteStdOutput( "Broadcaster is not currently connected" );
				continue;
			}

			g_Listener.Step();
		}
		else if ( strcmp(data,"stepover") == 0 )
		{
			if ( !g_Listener.IsRunning() ) 
			{
				WriteStdOutput( "Broadcaster is not currently connected" );
				continue;
			}

			g_Listener.StepOver();
		}
		else if ( strcmp(data,"stepout") == 0 )
		{
			if ( !g_Listener.IsRunning() ) 
			{
				WriteStdOutput( "Broadcaster is not currently connected" );
				continue;
			}

			g_Listener.StepOut();
		}
		else if ( strncmp(data,"set frame",strlen("set frame")) == 0 )
		{
			const char *szFrame = data + strlen("set frame");
			if ( *szFrame != ' ' )
			{
				WriteStdOutput( "set frame must be followed by a frame number, e.g. set frame 3" );
				continue;
			}

			szFrame++;
			uString sFrame( szFrame );
			sFrame.Trim( " " );
			if ( sFrame.GetLength() == 0 )
			{
				WriteStdOutput( "set frame must be followed by a frame number, e.g. set frame 3" );
				continue;
			}

			int iFrame = sFrame.ToInt();
			if ( iFrame == 0 && sFrame.CharAt(0) != '0' )
			{
				WriteStdOutput( "set frame must be followed by a frame number, e.g. set frame 3" );
				continue;
			}

			if ( !g_Listener.IsRunning() ) 
			{
				WriteStdOutput( "Broadcaster is not currently connected" );
				continue;
			}

			g_Listener.SetFrame( iFrame );
		}
		else if ( strcmp(data,"help") == 0 )
		{
			WriteStdOutput( "Commands:\n\
  help - show this message\n\
  list - show connected devices\n\
  exit - disconnect from all devices and close this program\n\
  setproject <path> - set the path to the AGK project that should be broadcast\n\
  connectall - looks for devices, connects to them, and uploads the project\n\
  connect <ip> - connect directly to specified device and attempt to broadcast to it\n\
  disconnectall - disconnect from all devices\n\
  run - run the uploaded project on all connected devices\n\
  debug - run the uploaded project on all connected devices in debug mode\n\
  pause - pause the devices running the current project\n\
  continue - continue any paused devices from a pause command or a breakpoint\n\
  stop - stop the devices running the current project\n\
  breakpoint <filename>:<linenum> - add a breakpoint at the given filename and line number\n\
  delete breakpoint <filename>:<linenum> - remove a breakpoint from the given filename and line number\n\
  delete all breakpoints - remove all breakpoints\n\
  watch <variable expression> - watch a variable value\n\
  delete watch <variable expression> - stop watching a variable\n\
  delete all watches - stop watching all variables\n\
  set frame <num> - sets the current call stack frame when paused to view other local variables" );
		}
		else if ( strcmp(data,"exit") == 0 )
		{
			break;
		}
		else
		{
			uString sErr( "Unrecognised command: " );
			sErr.Append( data );
			WriteStdOutput( sErr.GetStr() );
		}
		
		//sprintf( output, "You entered: %s\n", data );
		//WriteStdOutput( output );
	} while ( 1 );

	//system("pause");
	return 0;
}

