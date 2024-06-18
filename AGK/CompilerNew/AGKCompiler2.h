#ifndef _H_AGKCOMPILER
#define _H_AGKCOMPILER

#include "CompilerCommon.h"
#include "DynArray.h"
#include "../common/include/uString.h"

namespace AGK_Compiler
{
	struct stIncludeFile
	{
		uString sName;
		int type;

		stIncludeFile& operator=( stIncludeFile &other )
		{
			sName.SetStr( other.sName );
			type = other.type;
			return *this;
		}
	};

	extern DynArrayObject<stIncludeFile> g_pIncludeFiles;
	int AddIncludeFile( const char* filename, int type, uString *error );

	extern int iLineCountTotal;
	extern float fTokenTime;

	extern int error_count;
	extern int max_error_count;

	extern uString g_sAGKErrors;
	extern int g_iAllowIncludes;

	char* GetCompileErrors();
	void CompilerDeleteString( char* str );
	int RunCompiler( const char* lpCmdLine );
	int RunCompilerString( const char* szSource );
}

using namespace AGK_Compiler;

#endif


