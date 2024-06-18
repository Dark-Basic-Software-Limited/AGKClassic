
// position a sprite whenever the use touches / clicks the screen

// set a virtual resolution
SetVirtualResolution ( 320, 480 )

// display a background
CreateSprite ( LoadImage ( "background5.jpg" ) )

// create a sprite
CreateSprite ( 1, LoadImage ( "lime.png" ) )

// main loop
do
    // display instructions
    Print ( "Touch or click the screen" )
    Print ( "to move the sprite" )

    // check for any input
    if ( GetPointerPressed ( ) = 1 )

        // find the input location
        x# = GetPointerX ( )
        y# = GetPointerY ( )

        // set position of our sprite to the input location
        SetSpritePosition ( 1, x#, y# )
    endif

    // update the screen
    sync ( )
loop
