
// create and display text


// display a background
backdrop = CreateSprite ( LoadImage ( "background5.jpg" ) )
SetSpriteSize ( backdrop, 100, 100 )

// create text
CreateText ( 10, "HELLO AGK!" )

// set its size
SetTextSize ( 10, 6 )


// main loop
do
    // apply a random color to the text entity
    SetTextColor ( 10, Random ( 1, 255 ), Random ( 1, 255 ), Random ( 1, 255 ), 255 )


    // update the screen
    Sync ( )
loop
