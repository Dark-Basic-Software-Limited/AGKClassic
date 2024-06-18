
// Print an image to the connected printer (if OS supports it)
// Introduced in build 107

// display a background
backdrop = CreateSprite ( LoadImage ( "background5.jpg" ) )
SetSpriteSize ( backdrop, 100, 100 )

// main loop
do
	// main code
	Print("Click to print to printer..")
    if GetPointerPressed()=1 and printing=0
        PrintImage(backdrop,50.0)
        printing=1
    endif

    // update the screen
    Sync ( )
loop
