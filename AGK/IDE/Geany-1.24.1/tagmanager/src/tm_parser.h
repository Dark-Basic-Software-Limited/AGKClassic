/*
*
*   Copyright (c) 2014, Colomban Wendling
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*/

#ifndef TM_PARSER_H
#define TM_PARSER_H

#ifndef LIBCTAGS_DEFINED
/* from ctags/parse.h */
#	define LANG_AUTO   (-1)
#	define LANG_IGNORE (-2)
#endif


/* keep in sync with ctags/parsers.h */
typedef enum
{
	
	TM_PARSER_NONE = LANG_IGNORE,
	TM_PARSER_AUTO = LANG_AUTO,
	TM_PARSER_C = 0,
	TM_PARSER_CPP,
	TM_PARSER_CONF,
	TM_PARSER_FREEBASIC,
	TM_PARSER_GLSL,
	TM_PARSER_COUNT
} TMParserType;


#endif /* TM_PARSER_H */
