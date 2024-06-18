	case AGKI_COMPILE:
	{
		const char* param0 = m_pStrStack[ --m_iStrStackPtr ].GetStr();
		uString sFullPath( param0 );
		if ( strncmp(sFullPath.GetStr(),"raw:",4) == 0 ) sFullPath.SetStr( param0+4 );
		else agk::PlatformGetFullPathWrite( sFullPath );
#ifdef AGK_WINDOWS
		int size = MultiByteToWideChar( CP_UTF8, 0, sFullPath.GetStr(), -1, 0, 0 );
		wchar_t *wzPath = new wchar_t[ size ];
		MultiByteToWideChar( CP_UTF8, 0, sFullPath.GetStr(), -1, wzPath, size );
		BOOL result = SetCurrentDirectoryW( wzPath );
		delete [] wzPath;
#else
		chdir( sFullPath.GetStr() );
#endif
		m_pStack[ m_iStackPtr++ ].i = AGK_Compiler::RunCompiler( "-agk main.agc" );
		break;
	}

	case AGKI_GET_COMPILE_ERRORS:
	{
		char *szReturnStr = AGK_Compiler::GetCompileErrors();
		m_pStrStack[ m_iStrStackPtr++ ].SetStrUTF8( szReturnStr );
		AGK_Compiler::CompilerDeleteString( szReturnStr );
		break;
	}

	case AGKI_RUN_CODE:
	{
		if ( !m_pSubProgram )
		{
			// initial setup
			const char* param0 = m_pStrStack[ --m_iStrStackPtr ].GetStr();
			uString sFullPath( param0 );
			if ( strncmp(sFullPath.GetStr(),"raw:",4) == 0 ) sFullPath.SetStr( param0+4 );
			else agk::PlatformGetFullPathWrite( sFullPath );
			if ( sFullPath.CharAtConst( sFullPath.GetNumChars()-1 ) != '/' ) sFullPath.AppendUTF8("/");

			char *szOldWritePath = agk::GetWritePath();
			m_sOldWritePath.SetStr( szOldWritePath );
			delete [] szOldWritePath;

			// clear out old resources
			agk::MasterReset();
			agk::OverrideDirectories( sFullPath.GetStr(), 0 );
			agk::SetCurrentDir("");
			agk::MakeFolder("media");
			agk::SetCurrentDir("media");
			
			m_pSubProgram = new ProgramData();
			if ( !m_pSubProgram->LoadBytecode( "bytecode.byc" ) )
			{
				goto subprogramcleanup;
			}			
		}

		try
		{
			int result = m_pSubProgram->RunProgram();
			if ( result == 1 )
			{
				// error
				goto subprogramcleanup;
			}
			else if ( result == 3 )
			{
				// program ended
				goto subprogramcleanup;
			}
		}
		catch(...)
		{
			uString err; agk::GetLastError(err);
			err.Prepend( "Error: " );
			agk::Message( err.GetStr() );
			
			goto subprogramcleanup;
		}

		// stay on this instruction until the sub program has finished
		m_iProgramCounter--;
		return 0;

	subprogramcleanup:
		delete m_pSubProgram;
		m_pSubProgram = 0;
		agk::MasterReset();
		agk::OverrideDirectories( m_sOldWritePath.GetStr(), 0 );
		return 0;
	}
