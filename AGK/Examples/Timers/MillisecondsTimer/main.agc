
// Show the system timer in milliseconds, and show how to reset it per app
// Introduced in build 107

// display a background
backdrop = CreateSprite ( LoadImage ( "background5.jpg" ) )
SetSpriteSize ( backdrop, 100, 100 )

// init code
ResetTimer()

// main loop
do
	// main code
	print("GetMilliseconds() = "+str(GetMilliseconds()))

    // update the screen
    Sync ( )
loop
