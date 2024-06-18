#ifndef _H_PARSER1
#define _H_PARSER1

#include "ProgramData.h"

namespace AGK_Compiler
{

	struct stKeyword
	{
		const char* szKeyword;
		AGKToken iToken;
	};

	const char* GetTokenString( AGKToken token );

	class Parser1
	{
		protected:
			int iCommentBlock;
			int iFunctionBlock;
			int iSelectBlock;
			int iTypeBlock;
			//uString sFunctionName;
			//uString sTypeName;
			int iFunctionLine;
			int iTypeLine;

			static int g_iSorted;
			static int g_iNumKeywords;
			static stKeyword g_pKeywords[];

		public:
			Parser1();
			~Parser1();

			static void Parser1Init();
			AGKToken ParseKeyword( const char* str );
			AGKToken ParseTokenCommentBlock( AGKToken prevToken, const char* str, int &consumed, uString &error );
			AGKToken ParseToken( AGKToken prevprevToken, AGKToken prevToken, const char* str, int &consumed, uString &error );
			int ParseLine( int includeFileID, const uString &sLine, const uString &sLineRaw, int iLineNum, uString &error, cProgramData *pProgram=0 );
			int ParseFile( const char* filename, int includeFileID, cProgramData *pProgram=0 );
			int ParseSource( const char* source, cProgramData *pProgram=0 );
	};
}

#endif