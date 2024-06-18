
// place a sprite on screen using the percentage system

// load an image named blue.png and store its ID in
// the variable image
image = LoadImage ( "blue.png" )

// create a sprite using the previously loaded image
// and store its ID in the variable sprite
sprite = CreateSprite ( image )

// set the size of our sprite to be 50% on the X axis
// and work out the Y size automatically
SetSpriteSize ( sprite, 50, -1 )

// our main loop
do
    // update the screen
    sync ( )
loop
