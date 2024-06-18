#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "Common.h"

int main( int argc, char* argv[] )
{
	// set some path variables
	//const char* szSharedFolder = "/home/paul/Shares/"; // for Mac and Linux access
	const char* szSharedFolder = "/home/michael/Shares/"; // for Mac and Linux access
	
	SetCurrentDirectoryWithCheck( "../../.." ); // AGKTrunk

	int index = -1;
	bool bSingleCommand = false;
	bool bListCommands = true;
	int iAndroidSigningIndex = 0;
	char szPassword[ 256 ];	

startPoint:

	if ( !bListCommands )
	{
		printf( "Enter start point (use 's' to do a single step): " );

		char input[256];
		if ( !fgets(input, 256, stdin) ) Error( "Failed to read input" );
		if ( *input == 's' )
		{
			bSingleCommand = true;
			index = atoi( input+1 );
		}
		else
		{
			index = atoi( input );
		}
	}

	int indexCheck = 0;

	// Update SVN
	if ( !bListCommands )
	{
		int status = 0;
		status = RunCmd( indexCheck, "svn", "update" );
		if ( status != 0 ) Error( "  Failed" );
		else Message( "  Success" );
	}
	
	bool bHTML5Init = false;
	
	// Clean HTML5
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Clean HTML5", indexCheck );
		else
		{
			Message( "Cleaning HTML5" );
			int status = RunCmd( indexCheck, "make", "clean -f Makefile_html5" );
			if ( WEXITSTATUS(status) != 0 ) Error( "  Failed" );
			else Message( "  Success" );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}
	
	// Build HTML5
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Build HTML5", indexCheck );
		else
		{
			Message( "Building HTML5" );
			SetCurrentDirectoryWithCheck( "tools/AGKBuildSystem/Linux" );
			int status = system("./HTML5Build.sh");
			if ( WEXITSTATUS(status) != 0 ) Error( "Failed" );
			else Message( "Success" );
			
			SetCurrentDirectoryWithCheck( "../../.." ); // AGKTrunk
			
			const char *szPaths[] = { "3D", "3Ddynamic", "2D", "2Ddynamic" };
			
			for ( int i = 0; i < 4; i++ )
			{
				char str[ 256 ]; sprintf( str, "Editing AGKPlayer.js for %s", szPaths[i] );
				Message( str );
				
				char path[ 1024 ];
				strcpy( path, "IDE/Geany-1.24.1/data/html5/" );
				strcat( path, szPaths[i] );
				strcat( path, "/AGKPlayer.js" );
				
				char *data = 0;
				int size = GetFileContents( path, &data );
				if ( !size || !data ) Error( "Failed to get AGKPlayer.js data" );
				
				if ( strstr( data, "%%LOADPACKAGE%%" ) != 0 ) Error( "Failed - File already has %%LOADPACKAGE%%" );
				if ( strstr( data, "%%ADDITIONALFOLDERS%%" ) != 0 ) Error( "Failed - File already has %%ADDITIONALFOLDERS%%" );
				
				char *module = strstr( data, "Module[\"FS_createPath" );
				if ( !module ) Error( "Failed to find Module[FS_createPath]" );
				char *endModule = strstr( module, ");" );
				if ( !endModule ) Error( "Failed to find end of Module[FS_createPath]" );
			
				char *loadPackage = strstr( endModule, "loadPackage({" );
				if ( !loadPackage ) Error( "Failed to find loadPackage" );
				char *endLoadPackage = strstr( loadPackage, "})" );
				if ( !endLoadPackage ) Error( "Failed to find end of loadPackage" );
				
				*module = 0;
				endModule += 2;
				
				*loadPackage = 0;
				endLoadPackage += 2;
				
				char *newData = new char[ size + 100 ];
				strcpy( newData, data );
				strcat( newData, "%%ADDITIONALFOLDERS%%" );
				strcat( newData, endModule );
				strcat( newData, "%%LOADPACKAGE%%" );
				strcat( newData, endLoadPackage );
				
				FILE *fp = fopen( path, "wb" );
				if ( !fp ) Error( "Failed to open AGKPlayer.js for writing" );
				fwrite( newData, 1, strlen(newData), fp );
				fclose( fp );
				
				delete [] newData;
				delete [] data;
			}
			
			if ( bSingleCommand ) goto endPoint;
		}
	}

	// Clean Linux
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Clean Linux lib", indexCheck );
		else
		{
			Message( "Cleaning Linux" );
			int status = RunCmd( indexCheck, "make", "clean" );
			if ( WEXITSTATUS(status) != 0 ) Error( "  Failed" );
			else Message( "  Success" );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}
	
	// Build Linux
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Build Linux lib", indexCheck );
		else
		{
			Message( "Building Linux" );
			int status = RunCmd( indexCheck, "make", "-j2" );
			if ( WEXITSTATUS(status) != 0 ) Error( "  Failed" );
			else Message( "  Success" );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}
	
	// Build Linux interpreter
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Build Linux interpreter", indexCheck );
		else
		{
			Message( "Building Linux interpreter" );
			SetCurrentDirectoryWithCheck( "apps/interpreter_linux" );
			
			int status = RunCmd( indexCheck, "make", "clean" );
			if ( WEXITSTATUS(status) != 0 ) Error( "  Failed" );
			else Message( "  Success" );
			
			status = RunCmd( indexCheck, "make", "-j2" );
			if ( WEXITSTATUS(status) != 0 ) Error( "  Failed" );
			else Message( "  Success" );
			
			SetCurrentDirectoryWithCheck( "../.." );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}
	
	// Build Linux template (just to test, doesn't influence final build)
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Build Linux template", indexCheck );
		else
		{
			Message( "Building Linux template" );
			SetCurrentDirectoryWithCheck( "apps/template_linux" );
			
			int status = RunCmd( indexCheck, "make", "clean" );
			if ( WEXITSTATUS(status) != 0 ) Error( "  Failed" );
			else Message( "  Success" );
			
			status = RunCmd( indexCheck, "make", "-j2" );
			if ( WEXITSTATUS(status) != 0 ) Error( "  Failed" );
			else Message( "  Success" );
			
			SetCurrentDirectoryWithCheck( "../.." );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}
	
	// Build Linux compiler
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Build Linux compiler", indexCheck );
		else
		{
			Message( "Building Linux compiler" );
			SetCurrentDirectoryWithCheck( "CompilerNew" );
			
			int status = RunCmd( indexCheck, "make", "clean" );
			if ( WEXITSTATUS(status) != 0 ) Error( "  Failed" );
			else Message( "  Success" );
			
			status = RunCmd( indexCheck, "make", "-j2" );
			if ( WEXITSTATUS(status) != 0 ) Error( "  Failed" );
			else Message( "  Success" );
			
			SetCurrentDirectoryWithCheck( ".." );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}
	
	// Build Linux broadcaster
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Build Linux broadcaster", indexCheck );
		else
		{
			Message( "Building Linux broadcaster" );
			SetCurrentDirectoryWithCheck( "Broadcaster/AGKBroadcaster" );
			
			int status = RunCmd( indexCheck, "make", "clean" );
			if ( WEXITSTATUS(status) != 0 ) Error( "  Failed" );
			else Message( "  Success" );
			
			status = RunCmd( indexCheck, "make", "-j2" );
			if ( WEXITSTATUS(status) != 0 ) Error( "  Failed" );
			else Message( "  Success" );
			
			SetCurrentDirectoryWithCheck( "../.." );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}
	
	// Build Linux IDE
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Build Linux IDE", indexCheck );
		else
		{
			Message( "Building Linux IDE" );
			SetCurrentDirectoryWithCheck( "IDE/Geany-1.24.1" );
			
			int status = RunCmd( indexCheck, "make", "-j2" );
			if ( WEXITSTATUS(status) != 0 ) Error( "  Failed" );
			else Message( "  Success" );
			
			status = RunCmd( indexCheck, "make", "install" );
			if ( WEXITSTATUS(status) != 0 ) Error( "  Failed" );
			else Message( "  Success" );
			
			SetCurrentDirectoryWithCheck( "../.." );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}
	
	// Copy Linux build files
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Copy Linux build files", indexCheck );
		else
		{
			Message( "Copying Linux build files" );
			int status = system("tools/AGKBuildSystem/Linux/CopyLinux64.sh");
			if ( WEXITSTATUS(status) != 0 ) Error( "  Failed" );
			else Message( "  Success" );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}

	if ( bListCommands ) 
	{
		bListCommands = false;
		goto startPoint;
	}
	
endPoint:
	return 0;
}
