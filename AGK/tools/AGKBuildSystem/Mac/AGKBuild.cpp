#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "UserLocal.h" // just defines your username, e.g. USER_PAUL
#include "Common.h"

int main( int argc, char* argv[] )
{
	// set some path variables
#if defined( USER_PAUL )
	const char* szSharedFolder = "/Volumes/Receive";
    const char* szMacBuildFiles = "/Volumes/FilesAlphaMac";
    const char* szBuildFolder = "/Users/paultgc/Projects";
	#define USERNAME "paultgc"
	#define BUNDLE_SCRIPT_NAME "bundleNewPaul.sh"
#elif define( USER_MIKE )
    const char* szSharedFolder = "/Volumes/Shared/MacReceive";
    const char* szMacBuildFiles = "/Volumes/Shared/FilesAlphaMac";
    //const char* szBuildFolder = "/Users/michaeljohnson/AGKClassicBuild";
    const char* szBuildFolder = "/Users/michaeljohnson/Projects";
	#define USERNAME "michaeljohnson"
	#define BUNDLE_SCRIPT_NAME "bundleNewMike.sh"
#else
	#error "Unknown user, you need to set your variables"
#endif

	SetCurrentDirectoryWithCheck( "../../.." ); // AGKTrunk

	int index = -1;
	bool bSingleCommand = false;
	bool bListCommands = true;

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
        /*
         // no longer works after OS upgrade
        Message( "Updating SVN" );
		int status = RunCmd( indexCheck, "svn", "update" );
		if ( status != 0 ) Error( "  Failed" );
		else Message( "  Success" );
         */
        
        if ( !bSingleCommand )
        {
            // check shared folder paths
            Message( "Checking shared folder paths" );
            char currPath[ 1024 ];
            getcwd( currPath, 1024 );
            SetCurrentDirectoryWithCheck( szSharedFolder );
            SetCurrentDirectoryWithCheck( szMacBuildFiles );
            SetCurrentDirectoryWithCheck( currPath );
        }
	}
	
	// Build Mac
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Build Mac lib", indexCheck );
		else
		{
			Message( "Building Mac lib" );
			int status = RunCmd( indexCheck, "xcodebuild", "-project AGKMac.xcodeproj -target AGKMac -configuration Release -jobs 4" );
			if ( status != 0 ) Error( "  Failed" );
			else Message( "  Success" );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}
    
    // Build iOS Release Device
    if ( index <= ++indexCheck )
    {
        if ( bListCommands ) Message1( "%d: Build iOS Release Device lib", indexCheck );
        else
        {
            Message( "Building iOS Release Device lib" );
            int status = RunCmd( indexCheck, "xcodebuild", "-project AGKiOS.xcodeproj -target AGKiOS -configuration Release -jobs 4" );
            if ( status != 0 ) Error( "  Failed" );
            else Message( "  Success" );
            
            if ( bSingleCommand ) goto endPoint;
        }
    }
    /*
    // Build iOS Release Sim
    if ( index <= ++indexCheck )
    {
        if ( bListCommands ) Message1( "%d: Build iOS Release Sim lib", indexCheck );
        else
        {
            Message( "Building iOS Release Sim lib" );
            int status = RunCmd( indexCheck, "xcodebuild", "-project AGKiOS.xcodeproj -target AGKiOS -configuration Release -sdk iphonesimulator14.1 -arch i386 -arch x86_64 ONLY_ACTIVE_ARCH=NO \"VALID_ARCHS=i386 x86_64\" -jobs 4" );
            if ( status != 0 ) Error( "  Failed" );
            else Message( "  Success" );
            
            if ( bSingleCommand ) goto endPoint;
        }
    }
    */
    // Build iOS ReleaseLite Device
    if ( index <= ++indexCheck )
    {
        if ( bListCommands ) Message1( "%d: Build iOS ReleaseLite Device lib", indexCheck );
        else
        {
            Message( "Building iOS ReleaseLite Device lib" );
            int status = RunCmd( indexCheck, "xcodebuild", "-project AGKiOS.xcodeproj -target AGKiOS -configuration ReleaseLite -jobs 4" );
            if ( status != 0 ) Error( "  Failed" );
            else Message( "  Success" );
            
            if ( bSingleCommand ) goto endPoint;
        }
    }
    /*
    // Build iOS ReleaseLite Sim
    if ( index <= ++indexCheck )
    {
        if ( bListCommands ) Message1( "%d: Build iOS ReleaseLite Sim lib", indexCheck );
        else
        {
            Message( "Building iOS ReleaseLite Sim lib" );
            int status = RunCmd( indexCheck, "xcodebuild", "-project AGKiOS.xcodeproj -target AGKiOS -configuration ReleaseLite -sdk iphonesimulator14.1 -arch i386 -arch x86_64 ONLY_ACTIVE_ARCH=NO \"VALID_ARCHS=i386 x86_64\" -jobs 4" );
            if ( status != 0 ) Error( "  Failed" );
            else Message( "  Success" );
            
            if ( bSingleCommand ) goto endPoint;
        }
    }
    */
    // Build/Archive iOS interpreter_ios project
    if ( index <= ++indexCheck )
    {
        if ( bListCommands ) Message1( "%d: Build/Archive iOS interpreter", indexCheck );
        else
        {
            Message( "Building and Archiving iOS interpreter" );
            SetCurrentDirectoryWithCheck( "apps/interpreter_ios" );
            int status = RunCmd( indexCheck, "xcodebuild", "-project agkinterpreter.xcodeproj -scheme agk_interpreter -destination \"generic/platform=iOS\" -configuration Release archive -archivePath \"Build/Release/AppGameKit Player.xcarchive\" -jobs 4" );
            if ( status != 0 ) Error( "  Failed" );
            else Message( "  Success" );
            
            // strip debug symbols
            Message( "Copying binary to IDE" );
            status = RunCmd( indexCheck, "cp", "\"Build/Release/AppGameKit Player.xcarchive/Products/Applications/AppGameKit Player.app/AppGameKit Player\" \"../../IDE/Geany-1.24.1/data/ios/source/AppGameKit Player.app/AppGameKit Player\"" );
            if ( status != 0 ) Error( "  Failed" );
            else Message( "  Success" );
            
            SetCurrentDirectoryWithCheck( "../.." ); // AGKTrunk
            
            if ( bSingleCommand ) goto endPoint;
        }
    }
    
    // Export iOS interpreter IPA
    if ( index <= ++indexCheck )
    {
        if ( bListCommands ) Message1( "%d: Export iOS interpreter IPA", indexCheck );
        else
        {
            Message( "Exporting iOS interpreter IPA" );
            SetCurrentDirectoryWithCheck( "apps/interpreter_ios" );
            int status = RunCmd( indexCheck, "xcodebuild", "-exportArchive -archivePath \"Build/Release/AppGameKit Player.xcarchive\" -exportOptionsPlist \"exportOptions.txt\" -exportPath /Users/" USERNAME "/Desktop/AGKPlayeriOS" );
            if ( status != 0 ) Error( "  Failed" );
            else Message( "  Success" );
            
            SetCurrentDirectoryWithCheck( "../.." ); // AGKTrunk
            
            if ( bSingleCommand ) goto endPoint;
        }
    }
    
    // Build/Archive iOS interpreter_ios_noads project
    if ( index <= ++indexCheck )
    {
        if ( bListCommands ) Message1( "%d: Build/Archive iOS No Ads interpreter", indexCheck );
        else
        {
            Message( "Building and Archiving iOS No Ads interpreter" );
            SetCurrentDirectoryWithCheck( "apps/interpreter_ios_noads" );
            int status = RunCmd( indexCheck, "xcodebuild", "-project agkinterpreter.xcodeproj -scheme agk_interpreter -destination \"generic/platform=iOS\" -configuration Release archive -archivePath \"Build/Release/AppGameKit Player.xcarchive\" -jobs 4" );
            if ( status != 0 ) Error( "  Failed" );
            else Message( "  Success" );
            
            // strip debug symbols
            Message( "Copying binary to IDE" );
            status = RunCmd( indexCheck, "cp", "\"Build/Release/AppGameKit Player.xcarchive/Products/Applications/AppGameKit Player.app/AppGameKit Player\" \"../../IDE/Geany-1.24.1/data/ios/source/AppGameKit Player No Ads\"" );
            if ( status != 0 ) Error( "  Failed" );
            else Message( "  Success" );
            
            SetCurrentDirectoryWithCheck( "../.." ); // AGKTrunk
            
            if ( bSingleCommand ) goto endPoint;
        }
    }
    
    // Build/Archive Mac interpreter_mac project
    if ( index <= ++indexCheck )
    {
        if ( bListCommands ) Message1( "%d: Build/Archive Mac interpreter", indexCheck );
        else
        {
            Message( "Building and Archiving Mac interpreter" );
            SetCurrentDirectoryWithCheck( "apps/interpreter_mac" );
            int status = RunCmd( indexCheck, "xcodebuild", "-project agkinterpreter.xcodeproj -scheme agk_interpreter -configuration Release archive -archivePath \"Build/Release/AppGameKit Player.xcarchive\" -jobs 4" );
            if ( status != 0 ) Error( "  Failed" );
            else Message( "  Success" );
            
            SetCurrentDirectoryWithCheck( "../.." ); // AGKTrunk
            
            if ( bSingleCommand ) goto endPoint;
        }
    }
            
    // Export/Copy Mac interpreter_mac project
    if ( index <= ++indexCheck )
    {
        if ( bListCommands ) Message1( "%d: Export/Copy Mac interpreter to shared folder", indexCheck );
        else
        {
            // copy Mac interpreter to desktop
            SetCurrentDirectoryWithCheck( "apps/interpreter_mac" );
            Message( "Copying Mac Player to Desktop" );
            char srcFolder[ 1024 ];
            char dstFolder[ 1024 ];
            getcwd( srcFolder, 1024 ); strcat( srcFolder, "/Build/Release/AppGameKit Player.xcarchive/Products/Users/" USERNAME "/Applications/AppGameKit Player.app" );
            strcpy( dstFolder, "/Users/" USERNAME "/Desktop/MacPlayer/AppGameKit Player.app" );
            DeleteFolder( dstFolder );
            CopyFolder( srcFolder, dstFolder );
            
            Message( "Making binary executable" );
            int status = system("chmod 0755 \"/Users/" USERNAME "/Desktop/MacPlayer/AppGameKit Player.app/Contents/MacOS/AppGameKit Player\"");
            if ( WEXITSTATUS(status) != 0 ) Error( "Failed" );
            else Message( "Success" );
            
            Message( "Zipping up Mac Player" );
            getcwd( srcFolder, 1024 );
            SetCurrentDirectoryWithCheck( "/Users/" USERNAME "/Desktop/MacPlayer" );
            status = system("rm -f \"AppGameKit Player.app.zip\"");
            if ( WEXITSTATUS(status) != 0 ) Error( "Failed" );
            else Message( "Success" );

            status = system("zip -r \"AppGameKit Player.app.zip\" \"AppGameKit Player.app\"");
            if ( WEXITSTATUS(status) != 0 ) Error( "Failed" );
            else Message( "Success" );
            SetCurrentDirectoryWithCheck( srcFolder );
            
            Message( "Copying Mac Player to shared folder" );
            strcpy( dstFolder, szSharedFolder ); strcat( dstFolder, "/Classic/AppGameKit Player.app.zip" );
            CopyFile2( "/Users/" USERNAME "/Desktop/MacPlayer/AppGameKit Player.app.zip", dstFolder );
            
            SetCurrentDirectoryWithCheck( "../.." ); // AGKTrunk
            
            if ( bSingleCommand ) goto endPoint;
            
            Message( "\n** Windows build can be completed now **\n" );
            usleep( 2000 );
        }
    }
    
    // Build Mac Compiler
    if ( index <= ++indexCheck )
    {
        if ( bListCommands ) Message1( "%d: Build Mac Compiler", indexCheck );
        else
        {
            Message( "Building Mac Compiler" );
            SetCurrentDirectoryWithCheck( "CompilerNew" );
            int status = RunCmd( indexCheck, "xcodebuild", "-project AGKCompiler.xcodeproj -scheme AGKCompiler -configuration Release -jobs 4" );
            if ( status != 0 ) Error( "  Failed" );
            else Message( "  Success" );
            
            SetCurrentDirectoryWithCheck( ".." ); // AGKTrunk
            
            if ( bSingleCommand ) goto endPoint;
        }
    }
    
    // Build Mac Broadcaster
    if ( index <= ++indexCheck )
    {
        if ( bListCommands ) Message1( "%d: Build Mac Broadcaster", indexCheck );
        else
        {
            Message( "Building Mac Broadcaster" );
            SetCurrentDirectoryWithCheck( "Broadcaster/AGKBroadcaster" );
            int status = RunCmd( indexCheck, "xcodebuild", "-project AGKBroadcaster.xcodeproj -scheme AGKBroadcaster -configuration Release -jobs 4" );
            if ( status != 0 ) Error( "  Failed" );
            else Message( "  Success" );
            
            SetCurrentDirectoryWithCheck( "../.." ); // AGKTrunk
            
            if ( bSingleCommand ) goto endPoint;
        }
    }
    
    // Copy files to IDE
    if ( index <= ++indexCheck )
    {
        if ( bListCommands ) Message1( "%d: Copy files to IDE folder", indexCheck );
        else
        {
            Message( "Copying files to IDE folder" );
            char srcFolder[1024];
            char dstFolder[1024];
            
            // copy HTML5 files
            Message( "Copying HTML5 files" );
            strcpy( srcFolder, szSharedFolder ); strcat( srcFolder, "/Classic/HTML5" );
            getcwd( dstFolder, 1024 ); strcat( dstFolder, "/IDE/Geany-1.24.1/data/html5" );
            DeleteFolder( dstFolder );
            CopyFolder( srcFolder, dstFolder );
            
            if ( bSingleCommand ) goto endPoint;
        }
    }
    
    // Build IDE
    if ( index <= ++indexCheck )
    {
        if ( bListCommands ) Message1( "%d: Build Mac IDE", indexCheck );
        else
        {
            Message( "Building Mac IDE" );
            SetCurrentDirectoryWithCheck( "IDE/Geany-1.24.1" );
			int status = RunCmd( indexCheck, "make", "clean" );
            status = RunCmd( indexCheck, "make", "-j4" );
            if ( status != 0 ) Error( "  Failed" );
            else Message( "  Success" );
            
            SetCurrentDirectoryWithCheck( "../.." ); // AGKTrunk
            
            if ( bSingleCommand ) goto endPoint;
        }
    }
    
    // remove old build
    if ( index <= ++indexCheck )
    {
        if ( bListCommands ) Message1( "%d: Delete old build", indexCheck );
        else
        {
            Message( "Deleting old AppGameKit.app" );
            char srcFolder[ 1024 ]; getcwd( srcFolder, 1024 );
            char dstFolder[ 1024 ];
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/AppGameKit.app" );
            if ( chdir( dstFolder) == 0 )
            {
                DeleteFolder( dstFolder );
                rmdir( dstFolder );
            }
            SetCurrentDirectoryWithCheck( srcFolder );
            
            if ( bSingleCommand ) goto endPoint;
        }
    }
    
    // Copy external files
    if ( index <= ++indexCheck )
    {
        if ( bListCommands ) Message1( "%d: Copy external files", indexCheck );
        else
        {
            Message( "Copying external files" );
            char srcFolder[ 1024 ];
            char dstFolder[ 1024 ];
            
            // copy players
            Message( "Copying players" );
            strcpy( srcFolder, "/Users/" USERNAME "/Desktop/MacPlayer/AppGameKit Player.app.zip" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/Players/Mac/AppGameKit Player.app.zip" );
            CopyFile2( srcFolder, dstFolder );
            
            strcpy( srcFolder, szMacBuildFiles ); strcat( srcFolder, "/AGK/Players/Windows/Windows.exe" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/Players/Windows/Windows.exe" );
            CopyFile2( srcFolder, dstFolder );
            
            strcpy( srcFolder, szMacBuildFiles ); strcat( srcFolder, "/AGK/Players/Android/AppGameKit Player.apk" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/Players/Android/AppGameKit Player.apk" );
            CopyFile2( srcFolder, dstFolder );
            
            //strcpy( srcFolder, szSharedFolder ); strcat( srcFolder, "/Classic/LinuxPlayer32" );
            //strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/Players/Linux/LinuxPlayer32" );
            //CopyFile2( srcFolder, dstFolder );
            
            //strcpy( srcFolder, szSharedFolder ); strcat( srcFolder, "/Classic/LinuxPlayer64" );
           // strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/Players/Linux/LinuxPlayer64" );
            //CopyFile2( srcFolder, dstFolder );
            
            Message( "Copying change log" );
            strcpy( srcFolder, szMacBuildFiles ); strcat( srcFolder, "/AGK/ChangeLog.txt" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/ChangeLog.txt" );
            CopyFile2( srcFolder, dstFolder );

			// copy android libs
            Message( "Copying Android libs" );
            strcpy( srcFolder, szSharedFolder ); strcat( srcFolder, "/Classic/Android/lib" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AndroidExport/lib" );
            DeleteFolder( dstFolder );
            CopyFolder( srcFolder, dstFolder );
            
            // copy android google files
            Message( "Copying Android Google files" );
            strcpy( srcFolder, szSharedFolder ); strcat( srcFolder, "/Classic/Android/sourceGoogle" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AndroidExport/sourceGoogle" );
			DeleteFolder( dstFolder );
            CopyFolder( srcFolder, dstFolder );
            
            // copy android amazon files
            Message( "Copying Android Amazon files" );
			strcpy( srcFolder, szSharedFolder ); strcat( srcFolder, "/Classic/Android/sourceAmazon" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AndroidExport/sourceAmazon" );
			DeleteFolder( dstFolder );
            CopyFolder( srcFolder, dstFolder );

            // copy android ouya files
            Message( "Copying Android Ouya files" );
			strcpy( srcFolder, szSharedFolder ); strcat( srcFolder, "/Classic/Android/sourceOuya" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AndroidExport/sourceOuya" );
			DeleteFolder( dstFolder );
            CopyFolder( srcFolder, dstFolder );
			            
            if ( bSingleCommand ) goto endPoint;
        }
    }
    
    // Create app bundle
    if ( index <= ++indexCheck )
    { 
        if ( bListCommands ) Message1( "%d: Create app bundle", indexCheck );
        else
        {
            char srcFolder[ 1024 ];
            char dstFolder[ 1024 ];
            
            Message( "Deleting any existing Geany-Compiled folder" );
            getcwd( srcFolder, 1024 );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/Geany-Compiled" );
            if ( chdir( dstFolder) == 0 )
            {
                DeleteFolder( dstFolder );
                rmdir( dstFolder );
            }
            SetCurrentDirectoryWithCheck( srcFolder );
            
            Message( "Creating Geany-Compiled folder" );
            SetCurrentDirectoryWithCheck( "IDE/Geany-1.24.1" );
            int status = RunCmd( indexCheck, "make", "install" );
            if ( status != 0 ) Error( "  Failed" );
            else Message( "  Success" );
            
            SetCurrentDirectoryWithCheck( "../.." ); // AGKTrunk
            
            // copy compiler
            Message( "Copying compiler" );
            strcpy( srcFolder, "CompilerNew/build/Release/AGKCompiler" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/Geany-Compiled/share/applications/AGKCompiler" );
            CopyFile2( srcFolder, dstFolder );
            
			// no longer needed
            //strcpy( srcFolder, "CompilerNew/CommandList.dat" );
            //strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/Geany-Compiled/share/applications/CommandList.dat" );
            //CopyFile2( srcFolder, dstFolder );

			getcwd( srcFolder, 1024 ); strcat( srcFolder, "/plugins/Plugins" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/Plugins" );
            mkdir( dstFolder, 0755 );
            CopyFolder( srcFolder, dstFolder );
            
            // copy broadcaster
            Message( "Copying broadcaster" );
            strcpy( srcFolder, "Broadcaster/AGKBroadcaster/build/Release/AGKBroadcaster" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/Geany-Compiled/share/applications/AGKBroadcaster" );
            CopyFile2( srcFolder, dstFolder );
            
            // copy Mac interpreter
            Message( "Copying Mac interpreter" );
            getcwd( srcFolder, 1024 ); strcat( srcFolder, "/apps/interpreter_mac/Build/Release/AppGameKit Player.xcarchive/Products/Users/" USERNAME "/Applications/AppGameKit Player.app" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/Geany-Compiled/share/applications/interpreters" );
            mkdir( dstFolder, 0755 );
            strcat( dstFolder, "/Mac.app" );
            mkdir( dstFolder, 0755 );
            CopyFolder( srcFolder, dstFolder );
            
            // copy help
            Message( "Copying help files" );
            strcpy( srcFolder, szMacBuildFiles ); strcat( srcFolder, "/AGK/Tier 1/Help" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/Geany-Compiled/share/Help" );
            mkdir( dstFolder, 0755 );
            CopyFolder( srcFolder, dstFolder );

			// copy SnapChat SDK to IDE (must copy from the exported cut down versions that don't have x86_64 architecture)
            Message( "Copying SnapChat Core SDK to IDE" );
            getcwd( srcFolder, 1024 ); strcat( srcFolder, "/apps/interpreter_ios/build/Release/AppGameKit Player.xcarchive/Products/Applications/AppGameKit Player.app/Frameworks/SCSDKCoreKit.framework" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/Geany-Compiled/share/geany/ios/source/AppGameKit Player.app/Frameworks/SCSDKCoreKit.framework" );
            DeleteFolder( dstFolder );
            mkdir( dstFolder, 0755 );
            CopyFolder( srcFolder, dstFolder );

			Message( "Copying SnapChat Creative SDK to IDE" );
            getcwd( srcFolder, 1024 ); strcat( srcFolder, "/apps/interpreter_ios/build/Release/AppGameKit Player.xcarchive/Products/Applications/AppGameKit Player.app/Frameworks/SCSDKCreativeKit.framework" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/Geany-Compiled/share/geany/ios/source/AppGameKit Player.app/Frameworks/SCSDKCreativeKit.framework" );
            DeleteFolder( dstFolder );
            mkdir( dstFolder, 0755 );
            CopyFolder( srcFolder, dstFolder );
            
            // bundle files
            Message( "Creating bundle" );
            status = system( "IDE/Geany-1.24.1/" BUNDLE_SCRIPT_NAME );
            if ( WEXITSTATUS(status) != 0 ) Error( "  Failed" );
            else Message( "  Success" );
            
            // cleaning up
            Message( "Cleaning up" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/Geany-Compiled" );
            // mike - not for now
            //DeleteFolder( dstFolder );
            rmdir( dstFolder );
            
            // mike - new step
            //Message( "Copying AppGameKit.app" );
            //CopyFolder( "/Users/" USERNAME "/Projects/AGKMac/", "/Users/" USERNAME "/AGKClassicBuild/AGKMac/" );
            
			Message( "Copying geany launcher" );
            strcpy( srcFolder, szBuildFolder ); strcat( srcFolder, "/GeanyBundle/Launcher/geany/build/geany/Build/Products/Release/geany.app/Contents/MacOS/geany" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/AppGameKit.app/Contents/MacOS/geany" );
            Message( srcFolder );
            Message( dstFolder );
            CopyFile2( srcFolder, dstFolder );

			Message( "Renaming geany-bin" );
            strcpy( srcFolder, szBuildFolder ); strcat( srcFolder, "/AGKMac/AppGameKit.app/Contents/MacOS/geany-bin" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/AppGameKit.app/Contents/MacOS/geanybin" );
            CopyFile2( srcFolder, dstFolder );
			unlink( srcFolder );
            
            Message( "Copying provisioning profile" );
            strcpy( srcFolder, szBuildFolder ); strcat( srcFolder, "/GeanyBundle/Launcher/geany/build/geany/Build/Products/Release/geany.app/Contents/embedded.provisionprofile" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/AppGameKit.app/Contents/embedded.provisionprofile" );
            CopyFile2( srcFolder, dstFolder );
            
            if ( bSingleCommand ) goto endPoint;
        }
    }
    
    // copy internal files
    if ( index <= ++indexCheck )
    {
        if ( bListCommands ) Message1( "%d: Copy internal files (those inside AppGameKit.app)", indexCheck );
        else
        {
            Message( "Copying internal files" );
            char srcFolder[ 1024 ];
            char dstFolder[ 1024 ];

			// delete android files, not currently supported by Apple
			Message( "Deleting Android files" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/AppGameKit.app/Contents/Resources/share/geany/android" ); 
            DeleteFolder( dstFolder );
            
            // copy projects
            Message( "Copying projects" );
            getcwd( srcFolder, 1024 ); strcat( srcFolder, "/Examples" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/AppGameKit.app/Contents/Resources/Projects" );
            mkdir( dstFolder, 0755 );
            CopyFolder( srcFolder, dstFolder );
            
            // copy Tier 2
            Message( "Copying Tier 2 files" );
            strcpy( srcFolder, szMacBuildFiles ); strcat( srcFolder, "/AGK/Tier 2" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/Tier 2" );
            mkdir( dstFolder, 0755 );
            CopyFolder( srcFolder, dstFolder );

			// copy Example Plugin
			Message( "Copying Example Plugin files" );
			strcpy( srcFolder, szMacBuildFiles ); strcat( srcFolder, "/AGK/Example Plugin" );
			strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/Example Plugin" );
			mkdir( dstFolder, 0755 );
			CopyFolder( srcFolder, dstFolder );
            
            // copy Mac lib
            Message( "Copying Mac lib" );
            getcwd( srcFolder, 1024 ); strcat( srcFolder, "/platform/mac/Lib/Release/libAGKMac.a" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/Tier 2/platform/mac/Lib/Release/libAGKMac.a" );
            CopyFile2( srcFolder, dstFolder );
            
            // copy iOS libs
            Message( "Copying iOS libs" );
            getcwd( srcFolder, 1024 ); strcat( srcFolder, "/platform/apple/Lib/Release/libAGKiOS.a" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/Tier 2/platform/apple/Lib/Release/libAGKiOS.a" );
            CopyFile2( srcFolder, dstFolder );
            
            getcwd( srcFolder, 1024 ); strcat( srcFolder, "/platform/apple/Lib/ReleaseLite/libAGKiOS.a" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/Tier 2/platform/apple/Lib/ReleaseLite/libAGKiOS.a" );
            CopyFile2( srcFolder, dstFolder );
            
            // copy Chartboost SDK
            Message( "Copying Chartboost SDK" );
            getcwd( srcFolder, 1024 ); strcat( srcFolder, "/platform/apple/Source/Social Plugins/Chartboost.framework" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/Tier 2/platform/apple/Source/Social Plugins/Chartboost.framework" );
            DeleteFolder( dstFolder );
            mkdir( dstFolder, 0755 );
            CopyFolder( srcFolder, dstFolder );
            
            // copy AdMob SDK
            Message( "Copying AdMob SDK" );
            getcwd( srcFolder, 1024 ); strcat( srcFolder, "/platform/apple/Source/Social Plugins/GoogleMobileAds.framework" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/Tier 2/platform/apple/Source/Social Plugins/GoogleMobileAds.framework" );
            DeleteFolder( dstFolder );
            mkdir( dstFolder, 0755 );
            CopyFolder( srcFolder, dstFolder );

			Message( "Copying AdMob SDK2" );
            getcwd( srcFolder, 1024 ); strcat( srcFolder, "/platform/apple/Source/Social Plugins/GoogleAppMeasurement.framework" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/Tier 2/platform/apple/Source/Social Plugins/GoogleAppMeasurement.framework" );
            DeleteFolder( dstFolder );
            mkdir( dstFolder, 0755 );
            CopyFolder( srcFolder, dstFolder );

			Message( "Copying AdMob SDK3" );
            getcwd( srcFolder, 1024 ); strcat( srcFolder, "/platform/apple/Source/Social Plugins/GoogleUtilities.framework" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/Tier 2/platform/apple/Source/Social Plugins/GoogleUtilities.framework" );
            DeleteFolder( dstFolder );
            mkdir( dstFolder, 0755 );
            CopyFolder( srcFolder, dstFolder );

			Message( "Copying AdMob SDK4" );
            getcwd( srcFolder, 1024 ); strcat( srcFolder, "/platform/apple/Source/Social Plugins/nanopb.framework" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/Tier 2/platform/apple/Source/Social Plugins/nanopb.framework" );
            DeleteFolder( dstFolder );
            mkdir( dstFolder, 0755 );
            CopyFolder( srcFolder, dstFolder );

			Message( "Copying AdMob Consent SDK" );
            getcwd( srcFolder, 1024 ); strcat( srcFolder, "/platform/apple/Source/Social Plugins/UserMessagingPlatform.framework" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/Tier 2/platform/apple/Source/Social Plugins/UserMessagingPlatform.framework" );
            DeleteFolder( dstFolder );
            mkdir( dstFolder, 0755 );
            CopyFolder( srcFolder, dstFolder );
			
			// copy SnapChat SDK
            Message( "Copying SnapChat Core SDK" );
            getcwd( srcFolder, 1024 ); strcat( srcFolder, "/platform/apple/Source/Social Plugins/SCSDKCoreKit.framework" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/Tier 2/platform/apple/Source/Social Plugins/SCSDKCoreKit.framework" );
            DeleteFolder( dstFolder );
            mkdir( dstFolder, 0755 );
            CopyFolder( srcFolder, dstFolder );

			Message( "Copying SnapChat Creative SDK" );
            getcwd( srcFolder, 1024 ); strcat( srcFolder, "/platform/apple/Source/Social Plugins/SCSDKCreativeKit.framework" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/Tier 2/platform/apple/Source/Social Plugins/SCSDKCreativeKit.framework" );
            DeleteFolder( dstFolder );
            mkdir( dstFolder, 0755 );
            CopyFolder( srcFolder, dstFolder );

			// copy Firebase SDK
            Message( "Copying Firebase SDK" );
            getcwd( srcFolder, 1024 ); strcat( srcFolder, "/platform/apple/Source/Social Plugins/Firebase" );
            strcpy( dstFolder, szBuildFolder ); strcat( dstFolder, "/AGKMac/Tier 2/platform/apple/Source/Social Plugins/Firebase" );
            DeleteFolder( dstFolder );
            mkdir( dstFolder, 0755 );
            CopyFolder( srcFolder, dstFolder );
            
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
