#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <cassert>
#include <sys/types.h>
#include <sys/stat.h>
#include <wininet.h>

#include "miniz.h"
#include "Common.h"

class ResourceFile
{
	public:
		char* m_pString = 0;

		ResourceFile() {}
		~ResourceFile() { if ( m_pString ) delete [] m_pString; }

		void SetStr( const char* str )
		{
			if ( m_pString ) delete [] m_pString;
			int length = strlen( str );
			m_pString = new char[ length+1 ];
			strcpy( m_pString, str );
		}
};

int CompareResourceItems( const void* a, const void* b )
{
	ResourceFile* pA = *((ResourceFile**) a);
	ResourceFile* pB = *((ResourceFile**) b);

	if ( pA->m_pString == 0 ) return -1;
	if ( pB->m_pString == 0 ) return 1;

	return strcmp( pA->m_pString, pB->m_pString );
}

template<class T> class DynArrayObject
{
	public:
		T **m_pData;
		int m_iSize;
		int m_iCount;
		int m_iSorted;
		int (*CompareItems)(const void*, const void*);

		DynArrayObject( int size=0 )
		{
			CompareItems = 0;
			m_pData = 0;
			m_iSize = 0;
			m_iCount = 0;
			m_iSorted = 0;
			if ( size > 0 )
			{
				int newSize = 1;
				while ( newSize < size ) newSize <<= 1;
				m_pData = new T*[ newSize ];
				m_iSize = newSize;
			}
		}

		~DynArrayObject()
		{
			if ( m_pData ) 
			{
				for( int i = 0; i < m_iCount; i++ )
				{
					delete m_pData[ i ];
				}
					
				delete [] m_pData;
			}
		}

		void Clear()
		{
			if ( m_pData ) 
			{
				for( int i = 0; i < m_iCount; i++ )
				{
					delete m_pData[ i ];
				}
					
				m_pData = 0;
			}
			m_iSize = 0;
			m_iCount = 0;
		}

		void SetSorted( bool flag, int (*Compare)(const void*, const void*) )
		{
			CompareItems = Compare;
			if ( Compare == 0 ) flag = false;
			m_iSorted = flag ? 1 : 0;

			if ( m_iSorted && m_iCount > 0 )
			{
				// sort existing array
				qsort( m_pData, m_iCount, sizeof(T*), Compare );
			}
		}

		int Add( T *item, bool duplicates=true )
		{
			if ( m_iCount+1 > m_iSize )
			{
				m_iSize *= 2;
				if ( m_iSize == 0 ) m_iSize = 4;
				T** newArray = new T*[m_iSize];
				for ( int i = 0; i < m_iCount; i++ )
				{
					newArray[ i ] = m_pData[ i ];
				}
				if ( m_pData ) delete [] m_pData;
				m_pData = newArray;
			}

			T *storeItem = new T();
			*storeItem = *item;

			if ( m_iSorted == 0 )
			{
				m_pData[ m_iCount++ ] = storeItem;
				return m_iCount-1;
			}
			else
			{
				int high = m_iCount-1;
				int low = 0;
					
				// binary search
				int result = 0;
				int mid = 0;
				while( high >= low )
				{
					mid = (high+low)/2;
					result = CompareItems( &(m_pData[ mid ]), &storeItem );
					if( result > 0 ) high = mid-1;
					else if ( result < 0 ) low = mid+1;
					else 
					{
						if ( duplicates ) break;
						else return -1; // not allowed dulplicates
					}
				}

				if ( result < 0 ) mid++;
				if ( mid >= m_iCount )
				{
					m_pData[ m_iCount++ ] = storeItem;
					return m_iCount-1;
				}
				else
				{
					// insert new element at mid index
					for( int i = m_iCount-1; i >= mid; i-- )
					{
						m_pData[ i+1 ] = m_pData[ i ];
					}
					m_pData[ mid ] = storeItem;
					m_iCount++;
					return mid;
				}
			}
		}

		int Find( T *item )
		{
			if ( !m_iSorted ) return -1;

			int high = m_iCount-1;
			int low = 0;
				
			// binary search
			int result = 0;
			while( high >= low )
			{
				int mid = (high+low)/2;
				result = CompareItems( &(m_pData[ mid ]), &item );
				if( result > 0 ) high = mid-1;
				else if ( result < 0 ) low = mid+1;
				else return mid;
			}

			return -1;
		}
};

DynArrayObject<ResourceFile> g_allResources;

int main( int argc, char* argv[] )
{
	// set some path variables
#if defined(MIKE_BUILD)
	/*
	const char* szVS2015 = "C:\\Programs\\Visual Studio 14.0\\Common7\\IDE\\devenv.exe";
	const char* szVS2017 = "E:\\Programs\\Visual Studio 2017\\Common7\\IDE\\devenv.exe";
	const char* szDstFolderWin = "C:\\TGC\\BUILD2011-AGK\\FilesAlphaWindows";
	const char* szDstFolderMac = "C:\\TGC\\BUILD2011-AGK\\FilesAlphaMac";
	const char* szDstFolderLinux = "C:\\TGC\\BUILD2011-AGK\\FilesAlphaLinux";
	const char* szDstFolderWinTrial = "C:\\TGC\\BUILD2011-AGK\\FilesAlphaWindowsTrial";
	const char* szSharedFolder = "E:\\Receive"; // for Mac and Linux access
	const char* szTortoiseSVN = "C:\\Programs\\TortoiseSVN\\bin\\TortoiseProc.exe";
	const char* szZipAlign = "E:\\Data\\AndroidSDK\\build-tools\\29.0.3\\zipalign.exe";
	const char* szJarSigner = "C:\\Programs\\jdk1.8.0_221_x64\\bin\\jarsigner.exe";
	const char* szKeyStore = "C:\\Paul\\TGC\\keystore\\keystore.keystore";
	const char* szGradleRes = "C:\\Users\\PSJoh\\.gradle";
	const char* szTemp = "E:\\Temp";
	*/

	const char* szVS2015 = "D:\\Program Files (x86)\\Microsoft Visual Studio 14.0\\Common7\\IDE\\devenv.exe";
	const char* szVS2017 = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\Common7\\IDE\\devenv.exe";
	const char* szDstFolderWin = "D:\\AGK\\Builds\\Classic\\FilesAlphaWindows";
	const char* szDstFolderMac = "D:\\AGK\\Builds\\Classic\\FilesAlphaMac";
	const char* szDstFolderLinux = "D:\\AGK\\Builds\\Classic\\FilesAlphaLinux";
	const char* szDstFolderWinTrial = "D:\\AGK\\Builds\\Classic\\FilesAlphaWindowsTrial";
	const char* szSharedFolder = "D:\\AGK\\Shared"; // for Mac and Linux access
	const char* szTortoiseSVN = "D:\\Program Files (x86)\\TortoiseSVN\\bin\\TortoiseProc.exe";
	const char* szZipAlign = "D:\\Projects\\Android\\SDK\\build-tools\\28.0.3\\zipalign.exe";
	const char* szJarSigner = "D:\\Projects\\Android\\Java\\bin\\jarsigner.exe";
	const char* szKeyStore = "D:\\AGK\\Signing\\keystore.keystore";
	const char* szTemp = "D:\\AGK\\Temp";
	//const char* szGradleRes = "D:\\Documents\\.gradle";
	const char* szGradleRes = "C:\\Users\\Mike Johnson\\.gradle";

	SetCurrentDirectoryWithCheck ( "D:\\AGK\\Classic" ); // AGKTrunk

#elif defined(PAUL_BUILD)
	const char* szVS2015 = "C:\\Programs\\Visual Studio 2015\\Common7\\IDE\\devenv.exe";
	const char* szVS2017 = "C:\\Programs\\Visual Studio 2017\\Common7\\IDE\\devenv.exe";
	const char* szDstFolderWin = "C:\\TGC\\BUILD2011-AGK\\FilesAlphaWindows";
	const char* szDstFolderMac = "C:\\TGC\\BUILD2011-AGK\\FilesAlphaMac";
	const char* szDstFolderLinux = "C:\\TGC\\BUILD2011-AGK\\FilesAlphaLinux";
	const char* szDstFolderWinTrial = "C:\\TGC\\BUILD2011-AGK\\FilesAlphaWindowsTrial";
	const char* szSharedFolder = "E:\\Receive"; // for Mac and Linux access
	const char* szTortoiseSVN = "C:\\Programs\\TortoiseSVN\\bin\\TortoiseProc.exe";
	const char* szZipAlign = "E:\\Data\\AndroidSDK\\build-tools\\29.0.3\\zipalign.exe";
	const char* szJarSigner = "C:\\Programs\\jdk1.8.0_291\\bin\\jarsigner.exe";
	const char* szKeyStore = "C:\\Paul\\TGC\\keystore\\keystore.keystore";
	const char* szGradleRes = "C:\\Users\\PSJoh\\.gradle";
	const char* szTemp = "E:\\Temp";

	SetCurrentDirectoryWithCheck ( "..\\..\\.." ); // AGKTrunk
#else
	#error No build user defined
#endif

	

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

		bool keystorePasswordRequired = false;
		if ( !bSingleCommand )
		{
			if ( index <= iAndroidSigningIndex ) keystorePasswordRequired = true;
		}
		else
		{
			if ( index == iAndroidSigningIndex )
				keystorePasswordRequired = true;
		}

		
		if ( keystorePasswordRequired ) 
		{
			printf( "Enter keystore password: " );
			DWORD mode = 0;
			HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
			GetConsoleMode(hStdin, &mode);
			SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
			char* result = fgets(szPassword, 256, stdin);
			SetConsoleMode(hStdin, mode | ENABLE_ECHO_INPUT);
			if ( !result ) Error( "Failed to read input" );
			char* szEnd = szPassword + strlen(szPassword) - 1;
			while( *szEnd == '\n' || *szEnd == '\r' ) 
			{
				*szEnd = 0;
				szEnd--;
			}
		}
	}

	int indexCheck = 0;

	// Update SVN
	if ( !bListCommands )
	{
		int status = 0;
		status = RunCmd( indexCheck, szTortoiseSVN, "/command:update /path:\".\" /closeonend:3" );
		if ( status != 0 ) Error( "Failed" );
		else Message( "  Success" );
	}

	// What's new file
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Update What's New File", indexCheck );
		else
		{
			Message( "Updating What's New file" );
			if ( !UpdateWhatsNewFile() ) Error( "Failed" );
			else Message( "  Success" );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}
	
	// Command list
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Command Parser", indexCheck );
		else
		{
			SetCurrentDirectoryWithCheck( "CommandParserNew" );
			if ( !SetCurrentDirectory("Final") ) 
			{
				Error( "CommandParser must be compiled before the build can be run" );
			}

			int status = 0;
			status = RunCmd( indexCheck, "CommandParser.exe", "nopause" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );
			SetCurrentDirectoryWithCheck( "..\\.." ); // AGKTrunk
			
			if ( bSingleCommand ) goto endPoint;
		}
	}

	// VS2015
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Compile VS2015 Release", indexCheck );
		else
		{
			int status = 0;
			status = RunCmd( indexCheck, szVS2015, "AGKWindows2015.sln /rebuild Release" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}

	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Compile VS2015 ReleaseFree", indexCheck );
		else
		{
			int status = 0;
			status = RunCmd( indexCheck, szVS2015, "AGKWindows2015.sln /rebuild ReleaseFree" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}

	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Compile VS2015 DebugCompatible", indexCheck );
		else
		{
			int status = 0;
			status = RunCmd( indexCheck, szVS2015, "AGKWindows2015.sln /rebuild DebugCompatible" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}

	// VS2015 64-bit
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Compile VS2015 64-bit Release", indexCheck );
		else
		{
			int status = 0;
			status = RunCmd( indexCheck, szVS2015, "AGKWindows2015-64.sln /rebuild Release" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );

			if ( bSingleCommand ) goto endPoint;
		}
	}

	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Compile VS2015 64-bit ReleaseFree", indexCheck );
		else
		{
			int status = 0;
			status = RunCmd( indexCheck, szVS2015, "AGKWindows2015-64.sln /rebuild ReleaseFree" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );

			if ( bSingleCommand ) goto endPoint;
		}
	}

	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Compile VS2015 64-bit DebugCompatible", indexCheck );
		else
		{
			int status = 0;
			status = RunCmd( indexCheck, szVS2015, "AGKWindows2015-64.sln /rebuild DebugCompatible" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );

			if ( bSingleCommand ) goto endPoint;
		}
	}

	// VS2017
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Compile VS2017 Release", indexCheck );
		else
		{
			int status = 0;
			status = RunCmd( indexCheck, szVS2017, "AGKWindows2017.sln /rebuild Release" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}

	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Compile VS2017 DebugCompatible", indexCheck );
		else
		{
			int status = 0;
			status = RunCmd( indexCheck, szVS2017, "AGKWindows2017.sln /rebuild DebugCompatible" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}

	// VS2017 64-bit
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Compile VS2017 64-bit Release", indexCheck );
		else
		{
			int status = 0;
			status = RunCmd( indexCheck, szVS2017, "AGKWindows2017-64.sln /rebuild Release" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );

			if ( bSingleCommand ) goto endPoint;
		}
	}

	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Compile VS2017 64-bit DebugCompatible", indexCheck );
		else
		{
			int status = 0;
			status = RunCmd( indexCheck, szVS2017, "AGKWindows2017-64.sln /rebuild DebugCompatible" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );

			if ( bSingleCommand ) goto endPoint;
		}
	}

	// Compiler
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Compile AGK Compiler", indexCheck );
		else
		{
			SetCurrentDirectoryWithCheck( "CompilerNew" );
			int status = 0;
			status = RunCmd( indexCheck, szVS2015, "AGKCompiler2.sln /rebuild Release" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );
			SetCurrentDirectoryWithCheck( ".." );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}

	// Broadcaster
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Compile AGK Broadcaster", indexCheck );
		else
		{
			SetCurrentDirectoryWithCheck( "Broadcaster\\AGKBroadcaster" );
			int status = 0;
			status = RunCmd( indexCheck, szVS2015, "AGKBroadcaster.sln /rebuild Release" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );
			SetCurrentDirectoryWithCheck( "..\\.." );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}

	// interpreter
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Compile Windows interpreter ReleaseSteam", indexCheck );
		else
		{
			SetCurrentDirectoryWithCheck( "apps\\interpreter" );
			int status = 0;
			status = RunCmd( indexCheck, szVS2015, "interpreter.sln /rebuild \"ReleaseSteam|Win32\"" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );
			SetCurrentDirectoryWithCheck( "..\\.." );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}

	// interpreter 64-bit
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Compile Windows interpreter 64-bit ReleaseSteam", indexCheck );
		else
		{
			SetCurrentDirectoryWithCheck( "apps\\interpreter" );
			int status = 0;
			status = RunCmd( indexCheck, szVS2015, "interpreter.sln /rebuild \"ReleaseSteam|x64\"" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );
			SetCurrentDirectoryWithCheck( "..\\.." );

			if ( bSingleCommand ) goto endPoint;
		}
	}

	// VS2015 template (to check it works, does not contribute to the final build)
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Compile template_windows_vs2015 project", indexCheck );
		else
		{
			SetCurrentDirectoryWithCheck( "apps\\template_windows_vs2015" );
			int status = 0;
			status = RunCmd( indexCheck, szVS2015, "Template.sln /rebuild Release" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );
			SetCurrentDirectoryWithCheck( "..\\.." );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}

	// VS2015 template 64-bit (to check it works, does not contribute to the final build)
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Compile template_windows_vs2015_64 project", indexCheck );
		else
		{
			SetCurrentDirectoryWithCheck( "apps\\template_windows_vs2015_64" );
			int status = 0;
			status = RunCmd( indexCheck, szVS2015, "Template.sln /rebuild Release" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );
			SetCurrentDirectoryWithCheck( "..\\.." );

			if ( bSingleCommand ) goto endPoint;
		}
	}

	// ***************
	// ANDROID
	// ***************

	// Android lib
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Compile Android lib", indexCheck );
		else
		{
			SetCurrentDirectoryWithCheck( "libprojects\\android_base" );
			int status = 0;
			status = RunCmd( indexCheck, "cmd.exe", "/C jniCompile.bat nopause" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );

			status = RunCmd( indexCheck, "cmd.exe", "/C copylibs-nopause.bat" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );
			SetCurrentDirectoryWithCheck( "..\\.." );

			// copy ARCore libs to IDE build folder
			char dstFolder[ 1024 ];
			strcpy( dstFolder, szDstFolderWin ); 
			strcat( dstFolder, "\\AGK\\Tier 1\\Editor\\data\\android\\lib\\arm64-v8a\\libarcore_sdk.so" ); 
			CopyFile2( "platform\\android\\ARCore\\libs\\arm64-v8a\\libarcore_sdk.so", dstFolder );

			strcpy( dstFolder, szDstFolderWin ); 
			strcat( dstFolder, "\\AGK\\Tier 1\\Editor\\data\\android\\lib\\armeabi-v7a\\libarcore_sdk.so" ); 
			CopyFile2( "platform\\android\\ARCore\\libs\\armeabi-v7a\\libarcore_sdk.so", dstFolder );

			// copy SnapChat libs to IDE build folder
			strcpy( dstFolder, szDstFolderWin ); 
			strcat( dstFolder, "\\AGK\\Tier 1\\Editor\\data\\android\\lib\\arm64-v8a\\libpruneau.so" ); 
			CopyFile2( "platform\\android\\SnapChat\\arm64-v8a\\libpruneau.so", dstFolder );

			strcpy( dstFolder, szDstFolderWin ); 
			strcat( dstFolder, "\\AGK\\Tier 1\\Editor\\data\\android\\lib\\armeabi-v7a\\libpruneau.so" ); 
			CopyFile2( "platform\\android\\SnapChat\\armeabi-v7a\\libpruneau.so", dstFolder );
			
			if ( bSingleCommand ) goto endPoint;
		}
	}

	// Android interpreter
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Compile Android interpreter", indexCheck );
		else
		{
			SetCurrentDirectoryWithCheck( "apps\\interpreter_android_google\\AGKPlayer2\\src\\main" );
			int status = 0;
			status = RunCmd( indexCheck, "cmd.exe", "/C jniCompile.bat nopause" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );

			SetCurrentDirectoryWithCheck( "..\\..\\.." ); // interpreter_android_google

			status = RunCmd( indexCheck, "cmd.exe", "/C copylibs-nopause.bat" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );

			// copy libs to IDE folder
			char srcFolder[ 1024 ];
			char dstFolder[ 1024 ];

			// ARMv8
			GetCurrentDirectory( 1024, dstFolder );
			strcat( dstFolder, "\\..\\..\\IDE\\Geany-1.24.1\\data\\android\\lib\\arm64-v8a\\libandroid_player.so" );
			CopyFile2( "AGKPlayer2\\src\\main\\jniLibs\\arm64-v8a\\libandroid_player.so", dstFolder );

			// ARMv7
			GetCurrentDirectory( 1024, dstFolder );
			strcat( dstFolder, "\\..\\..\\IDE\\Geany-1.24.1\\data\\android\\lib\\armeabi-v7a\\libandroid_player.so" );
			CopyFile2( "AGKPlayer2\\src\\main\\jniLibs\\armeabi-v7a\\libandroid_player.so", dstFolder );

			SetCurrentDirectoryWithCheck( "..\\.." ); // AGKTrunk

			// copy libs to shared folder
			GetCurrentDirectory( 1024, srcFolder );
			strcat( srcFolder, "\\IDE\\Geany-1.24.1\\data\\android\\lib" );
			strcpy( dstFolder, szSharedFolder ); strcat( dstFolder, "\\Classic\\Android\\lib" );
			DeleteFolder( dstFolder );
			CopyFolder( srcFolder, dstFolder );

			if ( bSingleCommand ) goto endPoint;
		}
	}

	// Export Android APK projects
	if ( index <= ++indexCheck )
	{
		iAndroidSigningIndex = indexCheck;

		if ( bListCommands ) Message1( "%d: Export Android Google/Amazon/Ouya interpreter projects", indexCheck );
		else
		{
			const char *szAndroidIDEFolders[] = { "sourceGoogle", "sourceAmazon", "sourceOuya" };
			const char *szAndroidProjects[] = { "interpreter_android_google", "interpreter_android_amazon", "interpreter_android_ouya" };

			char srcFolder[ 1024 ];
			char dstFolder[ 1024 ];

			g_allResources.SetSorted( true, CompareResourceItems );

			for ( int i = 0; i < 3; i++ )
			{
				char msg[ 256 ]; sprintf( msg, "Exporting project %s", szAndroidProjects[i] );
				Message( msg );

				char path[1024];
				strcpy( path, "apps\\" );
				strcat( path, szAndroidProjects[i] );
				SetCurrentDirectoryWithCheck( path );

				// delete old gradle execution log
				DeleteFile( ".gradle\\6.5\\executionHistory\\executionHistory.bin" );
				DeleteFile( ".gradle\\6.5\\executionHistory\\executionHistory.lock" );
				FILE* pFile = fopen( ".gradle\\6.5\\executionHistory\\executionHistory.bin", "rb" );
				if ( pFile ) Error( "Failed to delete execution history" );

				// export unsigned unaligned apk
				int status = 0;
				status = RunCmd( indexCheck, "cmd.exe", "/C gradlew.bat clean" );
				if ( status != 0 ) Error( "Failed" );
				else Message( "  Success" );

				status = RunCmd( indexCheck, "cmd.exe", "/C gradlew.bat assembleRelease" );
				if ( status != 0 ) Error( "Failed" );
				else Message( "  Success" );

				// sign apk, must be done first when using jarsigner
				SetCurrentDirectoryWithCheck( "AGKPlayer2\\build\\outputs\\apk" );
				char str[ 1000 ];
				sprintf( str, "-sigalg MD5withRSA -digestalg SHA1 -storepass \"%s\" -keystore \"%s\" release\\AGKPlayer2-release-unsigned.apk tgc -keypass \"%s\"", szPassword, szKeyStore, szPassword );
				status = RunCmd( indexCheck, szJarSigner, str );
				if ( status != 0 ) Error( "Failed" );
				else Message( "  Success" );

				// align apk
				DeleteFile( "AGKPlayer2-release-automated.apk" );
				status = RunCmd( indexCheck, szZipAlign, "-f 4 release\\AGKPlayer2-release-unsigned.apk AGKPlayer2-release-automated.apk" );
				if ( status != 0 ) Error( "Failed" );
				else Message( "  Success" );

				// extract classes.dex
				mz_zip_archive zip_archive;
				memset(&zip_archive, 0, sizeof(zip_archive));
				if ( !mz_zip_reader_init_file( &zip_archive, "AGKPlayer2-release-automated.apk", 0 ) ) Error( "Failed to open apk as a zip file" );
				size_t dataLength = 0;
				char* pData = (char*)mz_zip_reader_extract_file_to_heap( &zip_archive, "classes.dex", &dataLength, 0 );
				if ( !pData || dataLength <= 0 ) Error( "Failed to extract classes.dex from APK" );

				// write it to the IDE folder
				SetCurrentDirectoryWithCheck( "..\\..\\..\\..\\..\\.." ); // AGKTrunk
				SetCurrentDirectoryWithCheck( "IDE\\Geany-1.24.1\\data\\android" );
				SetCurrentDirectoryWithCheck( szAndroidIDEFolders[i] );
				FILE *fp = fopen( "classes.dex", "wb" );
				if ( !fp ) Error( "Failed to open classes.dex for writing" );
				fwrite( pData, 1, dataLength, fp );
				fclose( fp );
				free(pData);
				pData = 0;

				SetCurrentDirectoryWithCheck( "..\\..\\..\\..\\.." ); // AGKTrunk

				// copy resMerged folder
				GetCurrentDirectory( 1024, srcFolder );
				strcat( srcFolder, "\\apps\\" );
				strcat( srcFolder, szAndroidProjects[i] );
				strcat( srcFolder, "\\AGKPlayer2\\build\\intermediates\\res\\merged\\release" );
				
				
				GetCurrentDirectory( 1024, dstFolder );
				strcat( dstFolder, "\\IDE\\Geany-1.24.1\\data\\android\\" );
				strcat( dstFolder, szAndroidIDEFolders[i] );
				strcat( dstFolder, "\\resMerged" );

				DeleteFolder( dstFolder );
				CopyFolder( srcFolder, dstFolder );
				
				// collect gradle resources
				strcpy( srcFolder, "apps\\" ); 
				strcat( srcFolder, szAndroidProjects[i] );
				strcat( srcFolder, "\\.gradle\\6.5\\executionHistory\\executionHistory.bin" );
				unsigned char* data = 0;
				size_t length = GetFileContents( srcFolder, (char**) &data );
				if ( length == 0 ) Error( "Failed to open executionHistory.bin" );

				int count = 0;
				unsigned char* ptr = data;
				const char* find = "caches\\transforms-2\\files-2.1\\";
				int findLen = strlen( find );
				while( ptr < (data + length - findLen - 1) )
				{
					if ( strncmp( (char*)ptr, find, findLen ) != 0 ) 
					{
						ptr++;
					}
					else
					{
						count++;
						ptr += findLen;
						
						// look for the end of the string, not null terminated so look for non-ascii character
						unsigned char* slash = ptr;
						while( *slash >= 32 && *slash <= 127 && slash < (data + length - 1) ) slash++;
				
						slash -= 5;
						if ( strncmp( (char*)slash, ".flat", 5 ) == 0 )
						{
							slash += 5;
							unsigned char old = *slash;
							*slash = 0;

							ResourceFile* pNewFile = new ResourceFile();
							pNewFile->SetStr( (char*)ptr );
							*slash = old;

							if ( g_allResources.Add( pNewFile, false ) < 0 ) delete pNewFile;
						}				

						ptr = slash;
					}
				}

				char dstFile[ 1024 ];
				
				for( int i = 0; i < g_allResources.m_iCount; i++ )
				{
					strcpy( srcFolder, szGradleRes );
					strcat( srcFolder, "\\caches\\transforms-2\\files-2.1\\" );
					strcat( srcFolder, g_allResources.m_pData[ i ]->m_pString );

					char* slash = strrchr( g_allResources.m_pData[ i ]->m_pString, '\\' );
					if ( slash ) slash++;
					else slash = g_allResources.m_pData[ i ]->m_pString;

					strcpy( dstFile, dstFolder );
					strcat( dstFile, "\\" );
					strcat( dstFile, slash );

					CopyFile2( srcFolder, dstFile );
				}

				delete [] data;
				g_allResources.Clear();
				
				// create resOrig files
				GetCurrentDirectory( 1024, srcFolder );
				strcat( srcFolder, "\\apps\\" );
				strcat( srcFolder, szAndroidProjects[i] );
				strcat( srcFolder, "\\AGKPlayer2\\build\\intermediates\\incremental\\mergeReleaseResources\\merged.dir\\values\\values.xml" );
				
				GetCurrentDirectory( 1024, dstFolder );
				strcat( dstFolder, "\\IDE\\Geany-1.24.1\\data\\android\\" );
				strcat( dstFolder, szAndroidIDEFolders[i] );
				strcat( dstFolder, "\\resOrig" );
				int strend = strlen(dstFolder);
				CreateDirectory( dstFolder, 0 );
				if ( i == 2 )
				{
					strcpy( dstFolder+strend, "\\drawable-xhdpi-v4" ); CreateDirectory( dstFolder, 0 );
					strcpy( dstFolder+strend, "\\drawable-hdpi-v4" ); CreateDirectory( dstFolder, 0 );
					strcpy( dstFolder+strend, "\\drawable-mdpi-v4" ); CreateDirectory( dstFolder, 0 );
					strcpy( dstFolder+strend, "\\drawable-ldpi-v4" ); CreateDirectory( dstFolder, 0 );
					strcpy( dstFolder+strend, "\\drawable" ); CreateDirectory( dstFolder, 0 );
				}
				else
				{					
					strcpy( dstFolder+strend, "\\drawable-xxxhdpi" ); CreateDirectory( dstFolder, 0 );
					strcpy( dstFolder+strend, "\\drawable-xxhdpi" ); CreateDirectory( dstFolder, 0 );
					strcpy( dstFolder+strend, "\\drawable-xhdpi" ); CreateDirectory( dstFolder, 0 );
					strcpy( dstFolder+strend, "\\drawable-hdpi" ); CreateDirectory( dstFolder, 0 );
					strcpy( dstFolder+strend, "\\drawable-mdpi" ); CreateDirectory( dstFolder, 0 );
					strcpy( dstFolder+strend, "\\drawable-ldpi" ); CreateDirectory( dstFolder, 0 );
				}
				strcpy( dstFolder+strend, "\\values" ); CreateDirectory( dstFolder, 0 );
				strcat( dstFolder, "\\values.xml" );

				CopyFile2( srcFolder, dstFolder );
			}

			// copy Android players to temp folder
			Message( "  Copying Android APKs to temp folder" );
			strcpy( dstFolder, szTemp ); strcat( dstFolder, "\\AppGameKit-Google.apk" );
			CopyFile2( "apps\\interpreter_android_google\\AGKPlayer2\\build\\outputs\\apk\\AGKPlayer2-release-automated.apk", dstFolder );
			strcpy( dstFolder, szTemp ); strcat( dstFolder, "\\AppGameKit-Amazon.apk" );
			CopyFile2( "apps\\interpreter_android_amazon\\AGKPlayer2\\build\\outputs\\apk\\AGKPlayer2-release-automated.apk", dstFolder );
			strcpy( dstFolder, szTemp ); strcat( dstFolder, "\\AppGameKit-Ouya.apk" );
			CopyFile2( "apps\\interpreter_android_ouya\\AGKPlayer2\\build\\outputs\\apk\\AGKPlayer2-release-automated.apk", dstFolder );

			if ( bSingleCommand ) goto endPoint;
		}
	}

	// Build help files
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Build help files", indexCheck );
		else
		{
			Message( "Building help files" );

			// build guides
			SetCurrentDirectoryWithCheck( "AGK Help Generator\\Generator" );
			int status = RunCmd( indexCheck, "Generator.exe", "" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );

			SetCurrentDirectoryWithCheck( "..\\Reference" );
			if ( !SetCurrentDirectory("Output") )
			{
				CreateDirectory( "Output", NULL );
				SetCurrentDirectory( "Output" );
			}

			// clear existing command lists
			char srcFolder[ 1024 ];
			GetCurrentDirectory( 1024, srcFolder );
			DeleteFolder( srcFolder );
			
			// build command lists
			status = RunCmd( indexCheck, "..\\Reference.exe", "" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );

			Message( "Copying commands to main help folder" );

			// copy to main folder
			char dstFolder[ 1024 ];
			GetCurrentDirectory( 1024, dstFolder );
			strcat( dstFolder, "\\..\\..\\AGK Txt\\Reference" );

			DeleteFolder( dstFolder );
			const char *szIgnoreExt[] = { ".txt" };
			CopyFolder( srcFolder, dstFolder, 1, szIgnoreExt );

			// copy keyword files to main folder
			CopyFile2( "keywordlinks.txt", "..\\..\\AGK Txt\\keywordlinks.txt" );

			// edit search.html
			Message( "Editing search.html" );
			char *pSearchData = 0;
			int searchSize = GetFileContents( "..\\..\\AGK Txt\\search.html", &pSearchData );
			if ( !searchSize || !pSearchData ) Error( "Failed to read search.html file" );

			char *szCommands = strstr( pSearchData, "var commands = [" );
			if ( !szCommands ) Error( "Failed to find command list in search.html" );
			szCommands += strlen( "var commands = [" );

			char *szParameters = strstr( pSearchData, "var parameters = [" );
			if ( !szParameters ) Error( "Failed to find parameter list in search.html" );
			szParameters = strstr( szParameters, "];" );
			if ( !szParameters ) Error( "Failed to find end of parameter list in search.html" );

			*szCommands = 0;

			// get new data
			char *pCommandList = 0;
			int commandListSize = GetFileContents( "commands1.txt", &pCommandList );
			if ( !commandListSize || !pCommandList ) Error( "Failed to read new command list from commands1.txt" );

			char *pCategoryList = 0;
			int categoryListSize = GetFileContents( "commands2.txt", &pCategoryList );
			if ( !categoryListSize || !pCategoryList ) Error( "Failed to read new category list from commands2.txt" );

			char *pParameterList = 0;
			int ParameterListSize = GetFileContents( "commands3.txt", &pParameterList );
			if ( !ParameterListSize || !pParameterList ) Error( "Failed to read new parameter list from commands3.txt" );

			// create new search file data
			char *pNewSearchData = new char[ searchSize + 10000 ];
			strcpy( pNewSearchData, pSearchData );
			strcat( pNewSearchData, pCommandList );
			strcat( pNewSearchData, "];\n            var categories = [" );
			strcat( pNewSearchData, pCategoryList );
			strcat( pNewSearchData, "];\n            var parameters = [" );
			strcat( pNewSearchData, pParameterList );
			strcat( pNewSearchData, szParameters );

			FILE *fp = fopen( "..\\..\\AGK Txt\\search.html", "wb" );
			if ( !fp ) Error( "Failed to open earch.html for writing" );
			fwrite( pNewSearchData, 1, strlen(pNewSearchData), fp );
			fclose( fp );

			delete [] pSearchData;
			delete [] pCommandList;
			delete [] pCategoryList;
			delete [] pParameterList;

			Message( "Copying help files to shared folder" );
			
			// copy to shared folder
			SetCurrentDirectoryWithCheck( "..\\..\\AGK Txt" );
			GetCurrentDirectory( 1024, srcFolder );
			
			strcpy( dstFolder, szSharedFolder );
			strcat( dstFolder, "\\Classic\\Help" );

			DeleteFolder( dstFolder );
			CopyFolder( srcFolder, dstFolder, 1, szIgnoreExt );

			// plus skipped files
			strcat( dstFolder, "\\keywordlinks.txt" );
			CopyFile2( "keywordlinks.txt", dstFolder );

			strcpy( dstFolder, szSharedFolder );
			strcat( dstFolder, "\\Classic\\Help\\corekeywordlinks.txt" );
			CopyFile2( "corekeywordlinks.txt", dstFolder );

			Message( "Copying help files to Windows build" );

			// copy to Windows build
			strcpy( srcFolder, szSharedFolder ); strcat( srcFolder, "\\Classic\\Help" );
			strcpy( dstFolder, szDstFolderWin ); strcat( dstFolder, "\\AGK\\Tier 1\\Help" );
			DeleteFolder( dstFolder );
			CopyFolder( srcFolder, dstFolder );

			Message( "Copying help files to Windows Trial build" );

			// copy to Windows build
			strcpy( srcFolder, szSharedFolder ); strcat( srcFolder, "\\Classic\\Help" );
			strcpy( dstFolder, szDstFolderWinTrial ); strcat( dstFolder, "\\AGK\\Tier 1\\Help" );
			DeleteFolder( dstFolder );
			CopyFolder( srcFolder, dstFolder );

			Message( "Copying help files to Mac build" );

			// copy to Mac build
			strcpy( dstFolder, szDstFolderMac ); strcat( dstFolder, "\\AGK\\Tier 1\\Help" );
			DeleteFolder( dstFolder );
			CopyFolder( srcFolder, dstFolder );

			Message( "Copying help files to Linux build" );

			// copy to Linux build
			strcpy( dstFolder, szDstFolderLinux ); strcat( dstFolder, "\\AGK\\Tier1\\Help" ); // purposefully missing a space in Tier1
			DeleteFolder( dstFolder );
			CopyFolder( srcFolder, dstFolder );

			SetCurrentDirectoryWithCheck( "..\\.." ); // AGKTrunk

			if ( bSingleCommand ) goto endPoint;
		}
	}

	// IDE
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Build IDE", indexCheck );
		else
		{
			Message( "Building IDE" );
			SetCurrentDirectoryWithCheck( "IDE\\Geany-1.24.1" );

			Message( "  Compiling" );
			int status = RunCmd( indexCheck, "make", "-f makefile.win32" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );

			Message( "Building IDE - install" );
			status = RunCmd( indexCheck, "make", "install -f makefile.win32" );
			if ( status != 0 ) Error( "Failed" );
			else Message( "  Success" );

			SetCurrentDirectoryWithCheck( "..\\.." );

			if ( bSingleCommand ) goto endPoint;
		}
	}

	// copy to build folders
	if ( index <= ++indexCheck )
	{
		if ( bListCommands ) Message1( "%d: Copy files to build folders", indexCheck );
		else
		{
			char srcFolder[ 1024 ];
			char dstFolder[ 1024 ];
			char backupFolder[ 1024 ];
			FileRecord files;

			const char* szBuildFolder[3] = { szDstFolderWin, szDstFolderMac, szDstFolderLinux };
			const char* szBuildPlatform[3] = { "Windows", "Mac", "Linux" };
			const char* szTier2[3] = { "\\AGK\\Tier 2", "\\AGK\\Tier 2", "\\AGK\\Tier2" };
			const char* szTier1[3] = { "\\AGK\\Tier 1", "\\AGK\\Tier 1", "\\AGK\\Tier1" };

			for ( int b = 0; b < 3; b++ )
			{
				if ( b == 0 ) Message( "Copying to Windows build" );
				if ( b == 1 ) Message( "Copying to Mac build" );
				if ( b == 2 ) Message( "Copying to Linux build" );

				Message( "  Copying Tier 2 files" );

				// Firebase frameworks for iOS are copied separately to the iOS platform Source folder
				
				// update apps folder
				Message( "    Copying apps folder" );
				GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\apps" );
				strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, szTier2[b] ); strcat( dstFolder, "\\apps" );
				// make a copy of the files in case we ever lose the folder structure
				files.Reset(); RecordFiles( dstFolder, &files ); sprintf( backupFolder, "tools\\AGKBuildSystem\\Backup\\%s\\appFiles.txt", szBuildPlatform[b] ); files.Save( backupFolder );
				UpdateFolder( srcFolder, dstFolder );
				
				// update common folder
				Message( "    Copying common folder" );
				GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\common\\include" );
				strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, szTier2[b] ); strcat( dstFolder, "\\common\\include" );
				//files.Reset(); RecordFiles( dstFolder, &files ); sprintf( backupFolder, "tools\\AGKBuildSystem\\Backup\\%s\\commonIncludeFiles.txt", szBuildPlatform[b] ); files.Save( backupFolder );
				DeleteFolder( dstFolder );
				CopyFolder( srcFolder, dstFolder );

				GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\common\\Collision" );
				strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, szTier2[b] ); strcat( dstFolder, "\\common\\Collision" );
				//files.Reset(); RecordFiles( dstFolder, &files ); sprintf( backupFolder, "tools\\AGKBuildSystem\\Backup\\%s\\commonCollisionFiles.txt", szBuildPlatform[b] ); files.Save( backupFolder );
				DeleteFolder( dstFolder );
				const char *szIgnore[] = { ".cpp", ".c", ".CPP", ".C" };
				CopyFolder( srcFolder, dstFolder, 4, szIgnore );

				// update bullet folder
				Message( "    Copying bullet folder" );
				GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\bullet" );
				strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, szTier2[b] ); strcat( dstFolder, "\\bullet" );
				//files.Reset(); RecordFiles( dstFolder, &files ); sprintf( backupFolder, "tools\\AGKBuildSystem\\Backup\\%s\\bulletFiles.txt", szBuildPlatform[b] ); files.Save( backupFolder );
				DeleteFolder( dstFolder );
				CopyFolder( srcFolder, dstFolder, 4, szIgnore );

				// update platform folder
				Message( "    Copying platform folder" );
				GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\platform" );
				strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, szTier2[b] ); strcat( dstFolder, "\\platform" );
				files.Reset(); RecordFiles( dstFolder, &files ); sprintf( backupFolder, "tools\\AGKBuildSystem\\Backup\\%s\\platformFiles.txt", szBuildPlatform[b] ); files.Save( backupFolder );
				UpdateFolder( srcFolder, dstFolder );

				// example plugin
				Message( "    Copying Example Plugin folder" );
				GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\plugins\\Source" );
				strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, "\\AGK\\Example Plugin" );
				UpdateFolder( srcFolder, dstFolder );
				strcat( dstFolder, "\\Commands.txt" );
				CopyFile2( "plugins\\Plugins\\ExamplePlugin\\Commands.txt", dstFolder );

				Message( "  Copying Windows and Android players" );
				strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, "\\AGK\\Players\\Windows\\Windows.exe" );
				CopyFile2( "apps\\interpreter\\Final\\Windows.exe", dstFolder );
				strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, "\\AGK\\Players\\Windows\\Windows64.exe" );
				CopyFile2( "apps\\interpreter\\Final\\Windows64.exe", dstFolder );
				strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, "\\AGK\\Players\\Android\\AppGameKit Player.apk" );
				CopyFile2( "apps\\interpreter_android_google\\AGKPlayer2\\build\\outputs\\apk\\AGKPlayer2-release-automated.apk", dstFolder );
				
				// copy changelog
				Message( "  Copying other files" );
				strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, "\\AGK\\ChangeLog.txt" );
				CopyFile2( "AGK.txt", dstFolder );

				if ( b == 0 )
				{
					// copy compiler
					strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, "\\AGK\\Tier 1\\Compiler\\AGKCompiler.exe" );
					CopyFile2( "CompilerNew\\Final\\AGKCompiler.exe", dstFolder );

					// copy commandlist.dat, no longer needed
					//strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, "\\AGK\\Tier 1\\Compiler\\CommandList.dat" );
					//CopyFile2( "CompilerNew\\CommandList.dat", dstFolder );

					// copy plugins
					GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\plugins\\Plugins" );
					strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, "\\AGK\\Tier 1\\Compiler\\Plugins" );
					DeleteFolder( dstFolder );
					CopyFolder( srcFolder, dstFolder );
					 
					// copy broadcaster
					strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, "\\AGK\\Tier 1\\Compiler\\AGKBroadcaster.exe" );
					CopyFile2( "Broadcaster\\AGKBroadcaster\\Release\\AGKBroadcaster.exe", dstFolder );

					// copy interpreter
					strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, "\\AGK\\Tier 1\\Compiler\\interpreters\\Windows.exe" );
					CopyFile2( "apps\\interpreter\\Final\\Windows.exe", dstFolder );
					strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, "\\AGK\\Tier 1\\Compiler\\interpreters\\Windows64.exe" );
					CopyFile2( "apps\\interpreter\\Final\\Windows64.exe", dstFolder );

					// copy steam files
					strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, "\\AGK\\Tier 1\\Compiler\\interpreters\\steam\\steam_api.dll" );
					CopyFile2( "apps\\interpreter\\Final\\steam\\steam_api.dll", dstFolder );
					strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, "\\AGK\\Tier 1\\Compiler\\interpreters\\steam\\steam_api64.dll" );
					CopyFile2( "apps\\interpreter\\Final\\steam\\steam_api64.dll", dstFolder );
					strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, "\\AGK\\Tier 1\\Compiler\\interpreters\\steam\\steam_appid.txt" );
					CopyFile2( "apps\\interpreter\\Final\\steam\\steam_appid.txt", dstFolder );

					// copy image joiner
					strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, "\\AGK\\Tier 1\\Utilities\\ImageJoiner\\ImageJoiner.exe" );
					CopyFile2( "tools\\ImageJoiner.exe", dstFolder );

					Message( "  Copying IDE" );
					GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\IDE\\Geany_Compiled" );
					strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, "\\AGK\\Tier 1\\Editor" );
					DeleteFolder( dstFolder );
					CopyFolder( srcFolder, dstFolder );

					Message( "  Deleting iOS files" );
					strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, "\\AGK\\Tier 1\\Editor\\data\\ios" ); 
					DeleteFolder( dstFolder );

					// copy HTML5 files (might be old but can replace them later)
					strcpy( srcFolder, szSharedFolder ); strcat( srcFolder, "\\Classic\\HTML5" );
					strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, "\\AGK\\Tier 1\\Editor\\data\\html5" );
					DeleteFolder( dstFolder );
					const char *szIgnore[] = { ".DS_Store" };
					CopyFolder( srcFolder, dstFolder, 1, szIgnore );

					// copy resMerged folder to shared folder
					GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\IDE\\Geany-1.24.1\\data\\android\\sourceGoogle\\resMerged" );
					strcpy( dstFolder, szSharedFolder ); strcat( dstFolder, "\\Classic\\Android\\sourceGoogle\\resMerged" );
					DeleteFolder( dstFolder );
					CopyFolder( srcFolder, dstFolder );

					GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\IDE\\Geany-1.24.1\\data\\android\\sourceAmazon\\resMerged" );
					strcpy( dstFolder, szSharedFolder ); strcat( dstFolder, "\\Classic\\Android\\sourceAmazon\\resMerged" );
					DeleteFolder( dstFolder );
					CopyFolder( srcFolder, dstFolder );

					GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\IDE\\Geany-1.24.1\\data\\android\\sourceOuya\\resMerged" );
					strcpy( dstFolder, szSharedFolder ); strcat( dstFolder, "\\Classic\\Android\\sourceOuya\\resMerged" );
					DeleteFolder( dstFolder );
					CopyFolder( srcFolder, dstFolder );

					// copy resOrig folder to shared folder
					GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\IDE\\Geany-1.24.1\\data\\android\\sourceGoogle\\resOrig" );
					strcpy( dstFolder, szSharedFolder ); strcat( dstFolder, "\\Classic\\Android\\sourceGoogle\\resOrig" );
					DeleteFolder( dstFolder );
					CopyFolder( srcFolder, dstFolder );

					GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\IDE\\Geany-1.24.1\\data\\android\\sourceAmazon\\resOrig" );
					strcpy( dstFolder, szSharedFolder ); strcat( dstFolder, "\\Classic\\Android\\sourceAmazon\\resOrig" );
					DeleteFolder( dstFolder );
					CopyFolder( srcFolder, dstFolder );

					GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\IDE\\Geany-1.24.1\\data\\android\\sourceOuya\\resOrig" );
					strcpy( dstFolder, szSharedFolder ); strcat( dstFolder, "\\Classic\\Android\\sourceOuya\\resOrig" );
					DeleteFolder( dstFolder );
					CopyFolder( srcFolder, dstFolder );

					// copy classes.dex to shared folder
					GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\IDE\\Geany-1.24.1\\data\\android\\sourceGoogle\\classes.dex" );
					strcpy( dstFolder, szSharedFolder ); strcat( dstFolder, "\\Classic\\Android\\sourceGoogle\\classes.dex" );
					CopyFile2( srcFolder, dstFolder );

					GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\IDE\\Geany-1.24.1\\data\\android\\sourceAmazon\\classes.dex" );
					strcpy( dstFolder, szSharedFolder ); strcat( dstFolder, "\\Classic\\Android\\sourceAmazon\\classes.dex" );
					CopyFile2( srcFolder, dstFolder );

					GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\IDE\\Geany-1.24.1\\data\\android\\sourceOuya\\classes.dex" );
					strcpy( dstFolder, szSharedFolder ); strcat( dstFolder, "\\Classic\\Android\\sourceOuya\\classes.dex" );
					CopyFile2( srcFolder, dstFolder );

					// copy manifest to shared folder
					GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\IDE\\Geany-1.24.1\\data\\android\\sourceGoogle\\AndroidManifest.xml" );
					strcpy( dstFolder, szSharedFolder ); strcat( dstFolder, "\\Classic\\Android\\sourceGoogle\\AndroidManifest.xml" );
					CopyFile2( srcFolder, dstFolder );

					GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\IDE\\Geany-1.24.1\\data\\android\\sourceAmazon\\AndroidManifest.xml" );
					strcpy( dstFolder, szSharedFolder ); strcat( dstFolder, "\\Classic\\Android\\sourceAmazon\\AndroidManifest.xml" );
					CopyFile2( srcFolder, dstFolder );

					GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\IDE\\Geany-1.24.1\\data\\android\\sourceOuya\\AndroidManifest.xml" );
					strcpy( dstFolder, szSharedFolder ); strcat( dstFolder, "\\Classic\\Android\\sourceOuya\\AndroidManifest.xml" );
					CopyFile2( srcFolder, dstFolder );

					// copy libs
					GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\IDE\\Geany-1.24.1\\data\\android\\lib" );
					strcpy( dstFolder, szSharedFolder ); strcat( dstFolder, "\\Classic\\Android\\lib" );
					DeleteFolder( dstFolder );
					CopyFolder( srcFolder, dstFolder );

					// copying example projects
					Message( "  Copying Example Projects" );
					GetCurrentDirectory( 1024, srcFolder ); strcat( srcFolder, "\\Examples" );
					strcpy( dstFolder, szBuildFolder[b] ); strcat( dstFolder, "\\AGK\\Projects" );
					DeleteFolder( dstFolder );
					CopyFolder( srcFolder, dstFolder );
				}
			}

			if ( bSingleCommand ) goto endPoint;
		}
	}

	if ( bListCommands ) 
	{
		bListCommands = false;
		goto startPoint;
	}

	// Commit SVN
	int status = 0;
	Message( "Comitting SVN" );
	status = RunCmd( indexCheck, szTortoiseSVN, "/command:commit /path:\".\"" );
	
endPoint:
	system("pause");
	return 0;
}
