/*
 *   Copyright (c) 2000-2006, Darren Hiebert, Elias Pschernig
 *
 *   This source code is released for free distribution under the terms of the
 *   GNU General Public License.
 *
 *   This module contains functions for generating tags for BlitzBasic
 *   (BlitzMax), PureBasic and FreeBasic language files. For now, this is kept
 *   quite simple - but feel free to ask for more things added any time -
 *   patches are of course most welcome.
 */

/*
 *   INCLUDE FILES
 */
#include "general.h" /* must always come first */

#include <string.h>

#include "parse.h"
#include "read.h"
#include "vstring.h"
#include "main.h"

/*
 *   DATA DEFINITIONS
 */
typedef enum {
	K_CONST,
	K_FUNCTION,
	K_LABEL,
	K_TYPE,
	K_VARIABLE,
	K_MEMBER
} BasicKind;

typedef struct {
	char const *token;
	BasicKind kind;
} KeyWord;

static kindOption BasicKinds[] = {
	{TRUE, 'c', "macro", "constants"},
	{TRUE, 'f', "function", "functions"},
	{TRUE, 'l', "namespace", "labels"},
	{TRUE, 's', "struct", "types"},
	{TRUE, 'v', "variable", "variables"},
	{TRUE, 'm', "member", "members"}
};

static char g_szTypeName[ 50 ];

/*
static KeyWord agk_keywords[] = {
	{"dim", K_VARIABLE}, 
	{"global", K_VARIABLE}, 
	{"local", K_VARIABLE}, 
	{"function", K_FUNCTION},
	{"type", K_TYPE},
	{NULL, 0}
};
*/

/*
 *   FUNCTION DEFINITIONS
 */

int basic_str_n_casecmp(const char *s1, const char *s2, int len)
{
	char *tmp1, *tmp2;
	gint result;

	tmp1 = (char*) malloc( strlen(s1)+1 );
	tmp2 = (char*) malloc( strlen(s2)+1 );

	strcpy( tmp1, s1 );
	strcpy( tmp2, s2 );

	toLowerString(tmp1);
	toLowerString(tmp2);

	result = strncmp(tmp1, tmp2, len);

	free(tmp1);
	free(tmp2);
	return result;
}

gboolean IsIdentifierChar( char c )
{
	if ( c >= '0' && c <= '9' ) return TRUE;
	if ( c >= 'A' && c <= 'Z' ) return TRUE;
	if ( c >= 'a' && c <= 'z' ) return TRUE;
	if ( c == '_' ) return TRUE;
	if ( c == '#' ) return TRUE;
	if ( c == '$' ) return TRUE;

	return FALSE;
}

static int isAGKComment( const char* pos )
{
	if ( *pos == '`' 
	 || (*pos == '/' && *(pos+1) == '/')
	 || (tolower(*pos) == 'r' && tolower(*(pos+1)) == 'e' && tolower(*(pos+2)) == 'm' && !IsIdentifierChar(*(pos+3))) ) return 1;
	else return 0;
}

static int isAGKCommentBlock( const char* pos )
{
	if ( (*pos == '/' && *(pos+1) == '*')
	 || (tolower(*pos) == 'r' && tolower(*(pos+1)) == 'e' && tolower(*(pos+2)) == 'm' && tolower(*(pos+3)) == 's'
	  && tolower(*(pos+4)) == 't' && tolower(*(pos+5)) == 'a' && tolower(*(pos+6)) == 'r' && tolower(*(pos+7)) == 't') ) return 1;
	else if( (*pos == '*' && *(pos+1) == '/')
	 || (tolower(*pos) == 'r' && tolower(*(pos+1)) == 'e' && tolower(*(pos+2)) == 'm' && tolower(*(pos+3)) == 'e'
	  && tolower(*(pos+4)) == 'n' && tolower(*(pos+5)) == 'd') ) return -1;
	else return 0;
}

/* Match the name of a dim or const starting at pos. */
/*
static int extract_dim (char const *pos, vString * name, BasicKind kind)
{
	const char *old_pos = pos;
	//while (isspace (*pos))
	//	pos++;

	// create tags only if there is some space between the keyword and the identifier 
	//if (old_pos == pos)
	//	return 0;

	vStringClear (name);

	//if (strncasecmp (pos, "dim ", 4) == 0) pos += 4; 

	while (isspace (*pos))
		pos++;

	int isArray = 0;
	while (*pos 
		&& !isspace (*pos) 
		&& *pos != '(' 
		&& (*pos != ',' || isArray)
		&& *pos != '=' )
	{
		if ( *pos == '[' ) isArray = 1;
		vStringPut (name, *pos);
		pos++;
	}
	vStringTerminate (name);

	if ( strcmp(name->buffer,"dim") == 0 ) 
	{
		vStringClear (name);
		while (isspace (*pos)) pos++;

		isArray = 0;
		while (*pos 
			&& !isspace (*pos) 
			&& *pos != '(' 
			&& (*pos != ',' || isArray)
			&& *pos != '=' )
		{
			if ( *pos == '[' ) isArray = 1;
			vStringPut (name, *pos);
			pos++;
		}
		vStringTerminate (name);
	}

	makeSimpleTag (name, BasicKinds, kind);

	if ( isArray ) 
	{
		vStringDelete (name);
		return 1;
	}

	// if the line contains a ',', we have multiple declarations 
	while (*pos && strchr (pos, ','))
	{
		// skip all we don't need(e.g. "..., new_array(5), " we skip "(5)")
		while (*pos != ',' && !isAGKComment(pos) )
			pos++;

		if ( isAGKComment(pos) )
			return 1; // break if we are in a comment

		while (isspace (*pos) || *pos == ',')
			pos++;

		if ( isAGKComment(pos) )
			return 1; // break if we are in a comment

		vStringClear (name);
		for (; *pos && !isspace (*pos) && *pos != '(' && *pos != ',' && *pos != '='; pos++)
			vStringPut (name, *pos);
		vStringTerminate (name);
		makeSimpleTag (name, BasicKinds, kind);
	}

	vStringDelete (name);
	return 1;
}

// Match the name of a tag (function, variable, type, ...) starting at pos. 
static char const *extract_name (char const *pos, vString * name)
{
	while (isspace (*pos))
		pos++;
	vStringClear (name);
	for (; *pos && !isspace (*pos) && *pos != '(' && *pos != ',' && *pos != '='; pos++)
		vStringPut (name, *pos);
	vStringTerminate (name);
	return pos;
}

// Match a keyword starting at p (case insensitive). 
static int match_keyword (const char *p, KeyWord const *kw)
{
	vString *name;
	size_t i;
	int j;
	const char *old_p;
	for (i = 0; i < strlen (kw->token); i++)
	{
		if (tolower (p[i]) != kw->token[i])
			return 0;
	}
	name = vStringNew ();
	p += i;
	
	old_p = p;
	while (isspace (*p)) p++;

	if (old_p == p) 
	{
		vStringDelete (name);
		return 0;
	}

	if (kw == &agk_keywords[0] ||
		kw == &agk_keywords[1] ||
		kw == &agk_keywords[2])
		return extract_dim (p, name, kw->kind); // extract_dim adds the found tag(s) 

	for (j = 0; j < 1; j++)
	{
		p = extract_name (p, name);
	}
	makeSimpleTag (name, BasicKinds, kw->kind);
	vStringDelete (name);

	return 1;
}
*/

/* Match a "label:" style label. */
static int parse_label( const char* p )
{
	if ( !IsIdentifierChar(*p) ) return 0;

	const char *cur = p;
	while (IsIdentifierChar(*cur))
		cur++;

	if (*cur == ':')
	{
		vString *name = vStringNew ();
		vStringNCatS (name, p, cur - p);
		makeSimpleTag (name, BasicKinds, K_LABEL);
		vStringDelete (name);
		return 1;
	}

	return 0;
}

static int parse_dim( const char* p )
{
	// ignore any global or local qualifiers
	if ( basic_str_n_casecmp(p,"global",6) == 0 && isspace(*(p+6)) ) 
		p += 6;
	else
	{
		if ( basic_str_n_casecmp(p,"local",5) == 0 && isspace(*(p+5)) ) 
			p += 5;
	}

	while (isspace(*p)) 
		p++;

	if ( basic_str_n_casecmp(p,"dim",3) != 0 || !isspace(*(p+3)) ) 
		return 0;

	p += 3;

	while (isspace(*p)) 
		p++;

	if ( !IsIdentifierChar(*p) ) 
		return 0;

	// read array name
	const char *start = p;
	while (IsIdentifierChar(*p))
		p++;

	const char* varend = p-1;

	while (isspace(*p)) 
		p++;
	
	// read array size
	if ( *p != '[' )
		return 0;

	while ( *p && *p != ']' ) 
		p++;
	if ( !*p ) return 0;

	p++;

	int len = (int)(p-start);
	if ( len >= 50 ) len = 50;

	while (isspace(*p)) 
		p++;

	// look for type name
	if ( basic_str_n_casecmp(p,"as ", 3) != 0 ) 
	{
		// arrays do not require explicit types
		while (isspace(*p)) 
			p++;

		if ( !*p || isAGKComment(p) )
		{
			const char *vartype = "integer";
			if ( *varend == '#' ) vartype = "float";
			if ( *varend == '$' ) vartype = "string";

			vString *name = vStringNew ();
			vStringNCatS (name, start, len);
			makeBasicTag( name, BasicKinds, K_VARIABLE, 0, vartype );
			vStringDelete (name);

			return 1;
		}
		
		return 0;
	}

	// array has a type field

	p += 3;
	if ( !IsIdentifierChar(*p) )
		return 0;
	
	// read type name
	const char *start2 = p;
	while (IsIdentifierChar(*p))
		p++;
	if ( p == start2 ) 
		return 0;

	int len2 = (int)(p-start2);
	if ( len2 >= 50 ) len2 = 50;

	vString *name = vStringNew ();
	vStringNCatS (name, start, len);
	
	// scope this var so it frees the stack before recursing
	char vartype[ 50 ];
	strncpy( vartype, start2, len2 );
	vartype[ len2 ] = 0;	
	
	makeBasicTag( name, BasicKinds, K_VARIABLE, 0, vartype );

	vStringDelete (name);

	return 1;
}

static int parse_variable( const char* p )
{
	// ignore any global or local qualifiers
	if ( basic_str_n_casecmp(p,"global",6) == 0 && isspace(*(p+6)) ) 
		p += 6;
	else
	{
		if ( basic_str_n_casecmp(p,"local",5) == 0 && isspace(*(p+5)) ) 
			p += 5;
	}

	while (isspace(*p)) 
		p++;

	if ( !IsIdentifierChar(*p) ) 
		return 0;

	// read var name
	const char *start = p;
	while (IsIdentifierChar(*p))
		p++;
	if ( p == start ) 
		return 0;

	int len = (int)(p-start);
	if ( len >= 50 ) len = 50;

	if ( basic_str_n_casecmp(p," as ", 4) != 0 ) 
	{
		// variables do not require types, but only account for them in types here
		if ( g_szTypeName[0] )
		{
			while (isspace(*p)) 
				p++;
			if ( !*p || *p == ',' )
			{
				const char *vartype = "integer";
				if ( start[len-1] == '#' ) vartype = "float";
				if ( start[len-1] == '$' ) vartype = "string";
				vString *name = vStringNew ();
				vStringNCatS (name, start, len);
				makeBasicTag( name, BasicKinds, K_MEMBER, g_szTypeName, vartype );
				vStringDelete (name);

				if ( *p == ',' ) 
				{
					p++;
					while (isspace(*p)) 
						p++;
					parse_variable( p );
				}

				return 1;
			}
		}
		else
			return 0;
	}

	// variable has a var type field

	p += 4;
	if ( !IsIdentifierChar(*p) )
		return 0;
	
	// read type name
	const char *start2 = p;
	while (IsIdentifierChar(*p))
		p++;
	if ( p == start2 ) 
		return 0;

	int len2 = (int)(p-start2);
	if ( len2 >= 50 ) len2 = 50;

	vString *name = vStringNew ();
	vStringNCatS (name, start, len);

	if ( *p == '[' )
	{
		// variable is an array, append array size to name
		const char* arraystart = p;
		while ( *p && *p != ']' ) 
			p++;
		if ( !*p ) return 0;

		p++;
		int len3 = (int)(p-arraystart);

		vStringNCatS (name, arraystart, len3);
	}
	
	{
		// scope this var so it frees the stack before recursing
		char vartype[ 50 ];
		strncpy( vartype, start2, len2 );
		vartype[ len2 ] = 0;	
		
		makeBasicTag( name, BasicKinds, g_szTypeName[0] ? K_MEMBER : K_VARIABLE, g_szTypeName[0] ? g_szTypeName : 0, vartype );
	}

	vStringDelete (name);

	while (isspace(*p)) 
		p++;

	// look for more variables on the same line
	if ( *p == ',' )
	{
		p++;
		while (isspace(*p)) 
			p++;
		parse_variable( p );
	}

	return 1;
}

static int parse_constant( const char* p )
{
	if ( basic_str_n_casecmp(p,"#constant",9) == 0 && isspace(*(p+9)) ) 
	{
		p += 9;
		while( isspace(*p) ) p++;
		if ( !IsIdentifierChar(*p) ) return 1; // it was a constant, just not formatted correctly

		const char* start = p;
		while( IsIdentifierChar(*p) ) p++;
		if ( p == start ) return 1;
		int len = (int)(p-start);
		if ( len >= 50 ) len = 50;

		vString *name = vStringNew ();
		vStringNCatS (name, start, len);
		makeBasicTag( name, BasicKinds, K_CONST, 0, 0 );
		vStringDelete (name);

		return 1;
	}

	return 0;
}

static int parse_function( const char* p )
{
	if ( basic_str_n_casecmp(p,"function",8) == 0 && isspace(*(p+8)) ) 
	{
		p += 8;
		while( isspace(*p) ) p++;
		if ( !IsIdentifierChar(*p) ) return 1; // it was a function, just not formatted correctly

		const char* start = p;
		while( IsIdentifierChar(*p) ) p++;
		if ( p == start ) return 1;
		int len = (int)(p-start);
		if ( len >= 50 ) len = 50;

		// look for args
		while( isspace(*p) ) p++;
		if ( *p != '(' ) return 1; // it was a function, just not formatted correctly
		const char* start2 = p;
		while( *p && *p != ')' ) p++;
		if ( *p != ')' ) return 1;
		int len2 = (int)(p-start2) + 1;
		if ( len2 >= 50 ) len2 = 50;

		vString *name = vStringNew ();
		vString *args = vStringNew ();

		vStringNCatS (name, start, len);		
		vStringNCatS (args, start2, len2);		

		makeBasicFunctionTag( name, BasicKinds, K_FUNCTION, args->buffer );

		vStringDelete (name);
		vStringDelete (args);

		return 1;
	}

	return 0;
}

static int parse_endtype( const char* p )
{
	if ( basic_str_n_casecmp(p,"endtype",7) == 0 && !IsIdentifierChar(*(p+7)) ) 
	{
		g_szTypeName[0] = 0;
		return 1;
	}

	return 0;
}

static int parse_type( const char* p )
{
	if ( basic_str_n_casecmp(p,"type",4) == 0 && isspace(*(p+4)) ) 
	{
		p += 4;
		while (isspace(*p)) p++;
		if ( !IsIdentifierChar(*p) ) return 1; // it was a type, just not formatted correctly

		const char* start = p;
		while( IsIdentifierChar(*p) ) p++;
		if ( p == start ) return 1;
		int len = (int)(p-start);
		if ( len >= 50 ) len = 50;

		strncpy( g_szTypeName, start, len );
		g_szTypeName[ len ] = 0;

		vString *name = vStringNew ();
		vStringNCatS (name, start, len);
		makeBasicTag( name, BasicKinds, K_TYPE, 0, 0 );
		vStringDelete (name);

		return 1;
	}

	return 0;
}

static int parse_line( const char* p )
{
	if ( g_szTypeName[0] )
	{
		if ( parse_endtype( p ) ) return 1;
		
		// if any of these are true then the type is not formatted correctly or is missing its EndType
		if ( parse_function( p )
		  || parse_constant( p )
		  || parse_dim( p )
		  || parse_label( p ) ) 
		{
			g_szTypeName[0] = 0;
			return 1;
		}

		if ( parse_type( p ) ) return 1;
		if ( parse_variable( p ) ) return 1;
	}
	else
	{
		if ( parse_function( p ) ) return 1;
		if ( parse_constant( p ) ) return 1;
		if ( parse_type( p ) ) return 1;
		if ( parse_dim( p ) ) return 1;
		if ( parse_label( p ) ) return 1;
		if ( parse_variable( p ) ) return 1;
	}
	
	return 0;
}

static void findBasicTags (void)
{
	const char *line;
	//KeyWord *keywords;

	//keywords = agk_keywords;
	int inComment = 0;

	while ((line = (const char *) fileReadLine ()) != NULL)
	{
		const char *p = line;
		KeyWord const *kw;

		while (isspace (*p))
			p++;

		/* Empty line or comment? */
		if (!*p || isAGKComment(p) )
			continue;

		// start comment block
		if ( isAGKCommentBlock(p) > 0 ) 
		{
			inComment = 1;
			p += 2; // block comment start is at least 2 characters
		}

		if ( !inComment )
		{
			parse_line( p );
			/*
			for (kw = keywords; kw->token; kw++)
				if (match_keyword (p, kw)) break;

			match_colon_label (p);
			*/
		}
		
		// must check for comment changes
		while ( *p )
		{
			if ( inComment == 0 && isAGKComment(p) ) break;

			// start comment block
			if ( isAGKCommentBlock(p) > 0 ) inComment = 1;

			// end comment block
			if ( isAGKCommentBlock(p) < 0 ) inComment = 0;
			
			p++;
		}
	}
}

parserDefinition *AGKParser (void)
{
	static char const *extensions[] = { "agc", NULL };
	parserDefinition *def = parserNew ("AGK");
	def->kinds = BasicKinds;
	def->kindCount = KIND_COUNT (BasicKinds);
	def->extensions = extensions;
	def->parser = findBasicTags;
	return def;
}


/* vi:set tabstop=4 shiftwidth=4: */
