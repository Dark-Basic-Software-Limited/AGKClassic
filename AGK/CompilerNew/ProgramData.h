#ifndef _H_TOKEN
#define _H_TOKEN

#include "CompilerCommon.h"
#include "../common/include/uString.h"
#include "DynArray.h"
#include "cHashedList.h"
#include "../apps/interpreter/AGKInstructions.h"

// Assumption: Array types are base types + 10
#define AGK_VARIABLE_INTEGER		1
#define AGK_VARIABLE_FLOAT			2
#define AGK_VARIABLE_STRING			3
#define AGK_VARIABLE_TYPE			4
#define AGK_VARIABLE_ARRAY_0		10
#define AGK_VARIABLE_ARRAY_INT		11
#define AGK_VARIABLE_ARRAY_FLOAT	12
#define AGK_VARIABLE_ARRAY_STRING	13
#define AGK_VARIABLE_ARRAY_TYPE		14

#define AGK_VARIABLE_GLOBAL			1
#define AGK_VARIABLE_LOCAL			2
#define AGK_VARIABLE_FUNC_LOCAL		3

const char* getDataTypeName( int var );

namespace AGK_Compiler
{

	enum AGKToken
	{
		AGK_TOKEN_UNKNOWN,

		AGK_TOKEN_INTEGER,
		AGK_TOKEN_FLOAT,
		AGK_TOKEN_STRING,
		AGK_TOKEN_IDENTIFIER, // could be variable, array, function, type, or type field

		AGK_TOKEN_VARIABLE,
		AGK_TOKEN_ARRAY,
		AGK_TOKEN_FUNCTION,
		AGK_TOKEN_COMMAND,

		AGK_TOKEN_DEC_SCOPE, // global or local
		AGK_TOKEN_DEC_AS, // declare as type
		AGK_TOKEN_DEC_VAR_TYPE, // integer, float, or string
		AGK_TOKEN_DEC_REFERENCE, // for passing by reference
		AGK_TOKEN_DEC_FUNCTION,
		AGK_TOKEN_DEC_FUNCTION_END,
		AGK_TOKEN_DEC_ARRAY, // dim
		AGK_TOKEN_DEC_TYPE,
		AGK_TOKEN_DEC_TYPE_END,

		AGK_TOKEN_UNDEC_ARRAY, // undim
		AGK_TOKEN_REDEC_ARRAY, // redim

		AGK_TOKEN_LABEL,
		AGK_TOKEN_INCLUDE,
		AGK_TOKEN_INSERT,
		AGK_TOKEN_CONSTANT,
		AGK_TOKEN_COMPANY_NAME,
		AGK_TOKEN_COMP_DIRECTIVE, // option explicit
		AGK_TOKEN_IMPORT_PLUGIN,
		AGK_TOKEN_IMPORT_MODULE,
		AGK_TOKEN_EXPORT,
		AGK_TOKEN_RENDERER,

		AGK_TOKEN_EQUALS, // =
		AGK_TOKEN_COMPARISON, // <>, <, >, etc
		AGK_TOKEN_UNARY_OP, // - ! NOT
		AGK_TOKEN_BINARY_OP, // + - * / % ^ AND OR XOR
		//AGK_TOKEN_BOOL_OP, // and, or, xor
		//AGK_TOKEN_NOT, // not

		AGK_TOKEN_REMSTART,
		AGK_TOKEN_REMEND,
		AGK_TOKEN_COMMENT, // ` or //

		AGK_TOKEN_DOT,
		AGK_TOKEN_COMMA,
		AGK_TOKEN_EOL, // newline or :
		AGK_TOKEN_OPEN_BRACKET,
		AGK_TOKEN_CLOSE_BRACKET,
		AGK_TOKEN_OPEN_SQR_BRACKET,
		AGK_TOKEN_CLOSE_SQR_BRACKET,

		AGK_TOKEN_IF,
		AGK_TOKEN_THEN,
		AGK_TOKEN_ELSE,
		AGK_TOKEN_ELSEIF,
		AGK_TOKEN_ENDIF,

		AGK_TOKEN_DO,
		AGK_TOKEN_LOOP,
		AGK_TOKEN_WHILE,
		AGK_TOKEN_ENDWHILE,
		AGK_TOKEN_FOR,
		AGK_TOKEN_TO,
		AGK_TOKEN_STEP,
		AGK_TOKEN_NEXT,
		AGK_TOKEN_REPEAT,
		AGK_TOKEN_UNTIL,

		AGK_TOKEN_LOOP_EXIT,
		AGK_TOKEN_LOOP_CONT,
		AGK_TOKEN_FUNCTION_EXIT,
		AGK_TOKEN_END,
		AGK_TOKEN_GOTO,
		AGK_TOKEN_GOSUB,
		AGK_TOKEN_RETURN,

		AGK_TOKEN_SELECT,
		AGK_TOKEN_CASE,
		AGK_TOKEN_CASE_DEFAULT,
		AGK_TOKEN_SELECT_END,
		AGK_TOKEN_CASE_END,

		AGK_TOKEN_INC,
		AGK_TOKEN_DEC,

		AGK_TOKEN_FOLDSTART,
		AGK_TOKEN_FOLDEND,

		AGK_TOKEN_PROGRAM_END
	};

	int CompareFunction( const void *a, const void *b );
	int CompareLabel( const void *a, const void *b );
	int CompareVariable( const void *a, const void *b );
	int CompareVariableType( const void *a, const void *b );
	int CompareVariableVarID( const void *a, const void *b );
	int CompareType( const void *a, const void *b );
	int CompareConstant( const void *a, const void *b );
	int CompareCommand( const void *a, const void *b );
	int CompareCommandCompatible( const void *a, const void *b );
	int CompareString( const void *a, const void *b );
	int CompareStringID( const void *a, const void *b );

	int ComparePluginRef( const void *a, const void *b );
	int ComparePlugin( const void *a, const void *b );
	int ComparePluginCommand( const void *a, const void *b );
	int ComparePluginCommandCompatible( const void *a, const void *b );

	int CompareModuleRef( const void *a, const void *b );
	int CompareModule( const void *a, const void *b );
	int CompareModuleFunction( const void *a, const void *b );

	struct stToken
	{
		//uString sFilename;
		int iIncludeFile;
		unsigned int iLine;
		int iValueInt;
		float fValueFloat;
		uString sValueRaw;
		AGKToken iType;
		int iStringIndex;

		stToken() {}
		~stToken() {}

		stToken& operator=( stToken& other )
		{
			sValueRaw.SetStr( other.sValueRaw.GetStr() );
			iIncludeFile = other.iIncludeFile;
			iLine = other.iLine;
			iValueInt = other.iValueInt;
			fValueFloat = other.fValueFloat;
			iType = other.iType;
			iStringIndex = other.iStringIndex;
			return *this;
		}
	};

	struct stVariable
	{
		uString sName;
		int iVarID;
		int iCopyFrom;
		int iTokenID;
		int iNumDimensions; // for arrays
		int *pDefaultLengths; // for arrays
		union
		{
			// only support a single dimension for default values
			int *pDefaultI; // strings also use this from string table indexes
			float *pDefaultF;
		};
		int iTypeIndex; // for user defined types
		int iInitialised;
		unsigned short iTypeOrder; // for user defined types, order the vars were defined in
		unsigned char iScope;
		unsigned char iType;

		stVariable() 
		{
			iTokenID = 0;
			iNumDimensions = 0;
			pDefaultLengths = 0;
			pDefaultI = 0;
			iScope = 0;
			iType = 0;
			iTypeIndex = 0;
			iInitialised = 0;
			iVarID = 0;
			iCopyFrom = 0;
			iTypeOrder = 0;
		}

		~stVariable()
		{
			if ( pDefaultLengths ) delete [] pDefaultLengths;

			switch( iType )
			{
				case AGK_VARIABLE_ARRAY_INT: if ( pDefaultI ) delete [] pDefaultI; break;
				case AGK_VARIABLE_ARRAY_FLOAT: if ( pDefaultF ) delete [] pDefaultF; break;
			}
		}

		stVariable& operator=( stVariable& other )
		{
			sName.SetStr( other.sName.GetStr() );
			iTokenID = other.iTokenID;
			iScope = other.iScope;
			iType = other.iType;
			iTypeIndex = other.iTypeIndex;
			iVarID = other.iVarID;
			iCopyFrom = other.iCopyFrom;
			iInitialised = other.iInitialised;
			iTypeOrder = other.iTypeOrder;

			if ( pDefaultLengths ) delete [] pDefaultLengths;
			pDefaultLengths = 0;
			iNumDimensions = other.iNumDimensions;
			if ( iNumDimensions > 0 && other.pDefaultLengths )
			{
				pDefaultLengths = new int[ iNumDimensions ];
				for( int i = 0; i < iNumDimensions; i++ ) pDefaultLengths[ i ] = other.pDefaultLengths[ i ];
			}
			
			switch( iType )
			{
				case AGK_VARIABLE_INTEGER:
				case AGK_VARIABLE_STRING:
				case AGK_VARIABLE_ARRAY_INT: 
				case AGK_VARIABLE_ARRAY_STRING:
				{
					if ( pDefaultI ) delete [] pDefaultI; 
					pDefaultI = 0; 
					break;
				}
				case AGK_VARIABLE_FLOAT: 
				case AGK_VARIABLE_ARRAY_FLOAT: 
				{
					if ( pDefaultF ) delete [] pDefaultF; 
					pDefaultF = 0; 
					break;
				}
			}

			if ( other.pDefaultI )
			{
				// variable has default values
				if ( iNumDimensions == 1 )
				{
					// array default values
					if ( pDefaultLengths && pDefaultLengths[0] >= 0 )
					{
						switch( other.iType )
						{
							case AGK_VARIABLE_ARRAY_STRING: 
							case AGK_VARIABLE_ARRAY_INT: 
							{
								pDefaultI = new int[ pDefaultLengths[0]+1 ];
								for ( int i = 0; i <= pDefaultLengths[0]; i++ ) pDefaultI[ i ] = other.pDefaultI[ i ];
								break;
							}
							case AGK_VARIABLE_ARRAY_FLOAT: 
							{
								pDefaultF = new float[ pDefaultLengths[0]+1 ];
								for ( int i = 0; i <= pDefaultLengths[0]; i++ ) pDefaultF[ i ] = other.pDefaultF[ i ];
								break;
							}
						}
					}
				}
				else
				{
					// normal varaible default value
					switch( other.iType )
					{
						case AGK_VARIABLE_STRING: 
						case AGK_VARIABLE_INTEGER: 
						{
							pDefaultI = new int[ 1 ];
							pDefaultI[ 0 ] = other.pDefaultI[ 0 ];
							break;
						}
						case AGK_VARIABLE_FLOAT: 
						{
							pDefaultF = new float[ 1 ];
							pDefaultF[ 0 ] = other.pDefaultF[ 0 ];
							break;
						}
					}
				}
			}

			return *this;
		}
	};

	struct stFunction
	{
		uString sName;
		int iTokenID;
		//int iTokenEndID;
		int iParamsTypes[ 32 ]; // max 32 parameters
		int iParamRef[ 32 ]; // is the parameter passed by reference or value
		unsigned short iReturnType;
		unsigned char iNumParams;
		unsigned char iNumParamStrings;
		int iNumLocalVars;
		int iNumLocalStrings;
		int iInstructionPtr;
		DynArrayObject<stVariable> pFuncVars;
		int valid;

		stFunction() 
		{
			iTokenID = 0;
			//iTokenEndID = 0;
			iReturnType = 0;
			iNumParams = 0;
			iNumParamStrings = 0;
			iNumLocalVars = 0;
			iNumLocalStrings = 0;
			iInstructionPtr = 0;
			pFuncVars.SetSorted( true, CompareVariable );
			valid = 1;
		}
		~stFunction() {}

		stFunction& operator=( stFunction& other )
		{
			sName.SetStr( other.sName.GetStr() );
			iTokenID = other.iTokenID;
			//iTokenEndID = other.iTokenEndID;
			iReturnType = other.iReturnType;
			iNumParams = other.iNumParams;
			iNumParamStrings = other.iNumParamStrings;
			iNumLocalVars = other.iNumLocalVars;
			iNumLocalStrings = other.iNumLocalStrings;
			iInstructionPtr = other.iInstructionPtr;
			pFuncVars = other.pFuncVars;
			valid = other.valid;
			for ( int i = 0; i < iNumParams; i++ ) 
			{
				iParamsTypes[ i ] = other.iParamsTypes[ i ];
				iParamRef[ i ] = other.iParamRef[ i ];
			}
			return *this;
		}

		int AddVariable( stVariable *newVar, int offset=0 )
		{
			if ( offset < 0 ) newVar->iVarID = offset;
			else
			{
				if ( newVar->iType == AGK_VARIABLE_STRING ) newVar->iVarID = (iNumLocalStrings++);
				else newVar->iVarID = (iNumLocalVars++) + 3;
			}

			return pFuncVars.Add( newVar, false );
		}
	};

	struct stLabel
	{
		uString sName;
		int iTokenID;
		int iInstructionPtr;
		stFunction *pInFunction;

		stLabel() { pInFunction = 0; iInstructionPtr = -1; }
		~stLabel() {}

		stLabel& operator=( stLabel& other )
		{
			sName.SetStr( other.sName.GetStr() );
			iTokenID = other.iTokenID;
			return *this;
		}
	};

	struct stType
	{
		uString sName;
		int iTokenID;
		//int iTokenEndID;
		DynArrayObject<stVariable> m_pSubVars;
		int iInitialised;
		uString sFirstVar;
		int valid;

		stType() : m_pSubVars(8) { iInitialised = 0; valid = 1; }
		~stType() { }

		stType& operator=( stType& other )
		{
			sName.SetStr( other.sName.GetStr() );
			iTokenID = other.iTokenID;
			//iTokenEndID = other.iTokenEndID;
			m_pSubVars = other.m_pSubVars;
			iInitialised = other.iInitialised;
			sFirstVar.SetStr( other.sFirstVar.GetStr() );
			valid = other.valid;
			return *this;
		}
	};

	struct stCommandOrig
	{
		const char *szCommandName;
		unsigned char iParamsTypes[ 32 ]; // max 32 parameters
		unsigned char iReturnType;
		unsigned char iNumParams;
	};

	struct stCommand
	{
		AGKInstruction instructionID;
		char szCommandName[ 200 ];
		unsigned char iParamsTypes[ 32 ]; // max 32 parameters
		unsigned char iReturnType;
		unsigned char iNumParams;
	};

	struct stConstant
	{
		uString sName;
		int tokenID;
		int length;
		int valid;

		stConstant() { length = 0; tokenID = 0; valid = 1; }
		~stConstant() {}

		stConstant& operator=( stConstant& other )
		{
			sName.SetStr( other.sName );
			tokenID = other.tokenID;
			length = other.length;
			valid = other.valid;
			return *this;
		}
	};

	struct stInstruction
	{
		AGKInstruction iInstruction;
		int iLineNum;
		union
		{
			int i;
			float f;
		};
		union
		{
			int i2;
			float f2;
		};
		int iIncludeFile;

		void Construct( AGKInstruction instruction, int lineNum, int includeFile, int extra1, int extra2 )
		{
			iInstruction = instruction;
			iLineNum = lineNum;
			iIncludeFile = includeFile;
			i = extra1;
			i2 = extra2;
		}
	};

	struct stString
	{
		uString sName;
		int iID;
		
		stString() { iID = 0; }
		~stString() {}

		stString& operator=( stString& other )
		{
			sName.SetStr( other.sName.GetStr() );
			iID = other.iID;
			return *this;
		}
	};

	// AGK Plugins
	#define AGK_PLUGIN_COMMAND_USED		0x1

	struct stPluginCommand
	{
		uString sTier1Name;

		uString sWindowsName; // mangled DLL function name
		uString sMacName;
		uString sLinuxName;
		uString sAndroidName;
		uString siOSName;
		uString sWindows64Name;

		unsigned char iNumParams;
		unsigned char iParamsTypes[ 32 ]; // max 32 parameters
		unsigned char iReturnType;
		unsigned char iFlags;

		stPluginCommand& operator=( stPluginCommand &other )
		{
			sTier1Name.SetStr( other.sTier1Name );
			sWindowsName.SetStr( other.sWindowsName );
			sMacName.SetStr( other.sMacName );
			sLinuxName.SetStr( other.sLinuxName );
			sAndroidName.SetStr( other.sAndroidName );
			siOSName.SetStr( other.siOSName );
			sWindows64Name.SetStr( other.sWindows64Name );

			iNumParams = other.iNumParams;
			iReturnType = other.iReturnType;
			iFlags = other.iFlags;
			for ( int i = 0; i < iNumParams; i++ )
			{
				iParamsTypes[i] = other.iParamsTypes[i];
			}
			return *this;
		}
	};

	struct stPlugin
	{
		uString sName; // name used to construct the plugin filename

		int m_iNumCommands;
		stPluginCommand *m_pCommands;

		stPlugin() { m_iNumCommands = 0; m_pCommands = 0; }
		~stPlugin() { if ( m_pCommands ) delete [] m_pCommands; }

		stPlugin& operator=( stPlugin &other )
		{
			sName.SetStr( other.sName );
			m_iNumCommands = other.m_iNumCommands;
			m_pCommands = new stPluginCommand[ m_iNumCommands ];
			for ( int i = 0; i < m_iNumCommands; i++ )
			{
				m_pCommands[i] = other.m_pCommands[i];
			}
			return *this;
		}
	};

	struct stPluginRef
	{
		uString sRefName; // text used to reference this plugin in code
		uString sName; // true plugin name

		stPluginRef& operator=( stPluginRef &other )
		{
			sRefName.SetStr( other.sRefName );
			sName.SetStr( other.sName );
			return *this;
		}
	};

	// AGK Modules

	struct stModuleFunction
	{
		uString sName;

		// bytecode instructions
	};

	struct stModule
	{
		uString sName;

		int m_iNumFunctions;
		stModuleFunction *m_pFunctions;

		stModule() { m_iNumFunctions = 0; m_pFunctions = 0; }
		~stModule() { if ( m_pFunctions ) delete [] m_pFunctions; }
	};

	struct stModuleRef
	{
		uString sRefName;
		uString sName;
	};

	class cProgramData
	{
		public:
			unsigned int m_iNumTokens;
			unsigned int m_iSizeTokens;
			stToken* m_pTokens;

			int m_iCountInts;
			int m_iCountFloats;
			int m_iCountStrings;
			int m_iCountTypes;
			int m_iCountArrays;
			
			DynArrayObject<stLabel> m_pLabels;
			DynArrayObject<stFunction> m_pFunctions;
			DynArrayObject<stVariable> m_pVariables;
			DynArrayObject<stType> m_pTypes;
			DynArrayObject<stConstant> m_pConstants;
			DynArrayObject<stInstruction> m_pInstructions;
			DynArrayObject<stString> m_pStringTable;

			DynArrayObject<stPlugin> m_pPlugins;
			DynArrayObject<stPluginRef> m_pPluginRefs;
			DynArrayObject<stModule> m_pModules;
			DynArrayObject<stModuleRef> m_pModuleRefs;

			uString m_sCompanyName;

			stCommand *m_pCommands;
			int m_iNumCommands;

			int m_iOptionExplicit;
			int m_iEntryPoint;

			cProgramData( int size=0 );
			~cProgramData();

			void TokenWarning( const char* msg, int tokenID );
			void TokenError( const char* msg, int tokenID );

			int AddToken( AGKToken type, const char *raw, int line, int includeFileID );
			
			int AddLabel( const char *raw, int tokenID );
			int FindLabel( const char *name );
			
			int AddFunction( const char *raw, int tokenID );
			int GetFunctionLineNum( const char *name );
			int FindFunction( const char *name );

			int AddType( const char *raw, int tokenID );
			int GetTypeLineNum( const char *name );
			int FindType( const char *name );

			int AddConstant( const char *raw, int tokenID );
			int FindConstant( const char *name );

			int AddVariable( stVariable &var );
			int FindVariable( const char *name );

			int AddInstruction( AGKInstruction instruction, int lineNum, int includeFile, int extra1, int extra2 );
			void EditInstruction( int instructionID, AGKInstruction instruction, int extra1, int extra2 );
			int GetInstructionPosition();

			int AddPlugin( const char *name, const char *asname, uString *error );
			int FindPlugin( const char *name );
			int FindPluginCommandName( int pluginIndex, const char *name );
			int FindPluginCommand( int pluginIndex, stPluginCommand *pCommand );
			int FindPluginCommandCompatible( int pluginIndex, stPluginCommand *pCommand );

			int AddModule( const char *name, const char *asname, uString *error );
			int FindModule( const char *name );

			//bool LoadCommandsArray( const char* filename, uString &error );
			bool LoadCommandsArrayLocal( uString &error );
			int FindCommandName( const char *name );
			int FindCommand( stCommand *pCommand );
			int FindCommandCompatible( stCommand *pCommand );

			int AddString( const char *value );

			int FinalCheck();
			int BuildBytecodeFile( const char *filename, int debugmode=0 );
	};
}

#endif