
#import "ExternalCommands.h"

// All SDKs should be defined here, even if they are not used
ExternalSDK* g_pAppsFlyerSDK __attribute__((weak)) = 0;
ExternalSDK* g_pIDFATrackingSDK __attribute__((weak)) = 0;

ExternalSDK* GetExternalSDK( const char* sdk )
{
    // add all SDKs here, even unused ones
    if ( strcmp( sdk, "appsflyer" ) == 0 ) return g_pAppsFlyerSDK;
    else if ( strcmp( sdk, "idfatracking" ) == 0 ) return g_pIDFATrackingSDK;
    
    return 0;
}


// Do not modify anything below here

extern "C" int ExternExternalSDKSupported( const char* sdk )
{
    ExternalSDK* pSDK = GetExternalSDK( sdk );
    return pSDK ? 1 : 0;
}

extern "C" void ExternExternalCommand( const char* sdk, const char* command, const char* str1, const char* str2 )
{
    ExternalSDK* pSDK = GetExternalSDK( sdk );
    if ( !pSDK ) return;
    
    pSDK->ExternalCommand( command, str1, str2 );
}

extern "C" int ExternExternalCommandInt( const char* sdk, const char* command, const char* str1, const char* str2 )
{
    ExternalSDK* pSDK = GetExternalSDK( sdk );
    if ( !pSDK ) return 0;
    
    return pSDK->ExternalCommandInt( command, str1, str2 );
}

extern "C" float ExternExternalCommandFloat( const char* sdk, const char* command, const char* str1, const char* str2 )
{
    ExternalSDK* pSDK = GetExternalSDK( sdk );
    if ( !pSDK ) return 0;
    
    return pSDK->ExternalCommandFloat( command, str1, str2 );
}

extern "C" char* ExternExternalCommandString( const char* sdk, const char* command, const char* str1, const char* str2 )
{
    ExternalSDK* pSDK = GetExternalSDK( sdk );
    if ( !pSDK )
    {
        char* str = new char[ 1 ];
        *str = 0;
        return str;
    }
    
    return pSDK->ExternalCommandString( command, str1, str2 );
}
