
// an example showing how to use include files

#include "functions.agc"

do
	Print ( "Calling a function from" )
	Print ( "an include file" )
	Print ( "" )

	myFunction ( 1, 2, 3 )

    Sync ( )
loop
