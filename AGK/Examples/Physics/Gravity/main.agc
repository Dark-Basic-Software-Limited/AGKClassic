
// gravitational pull to the right and downwards

// virtual resolution
SetVirtualResolution ( 320, 480 )

// display a background
CreateSprite ( LoadImage ( "background4.jpg" ) )

// load an image
LoadImage ( 1, "small_silver.png" )

// create 5 sprites
CreateSprite ( 1, 1 )
CreateSprite ( 2, 1 )
CreateSprite ( 3, 1 )
CreateSprite ( 4, 1 )
CreateSprite ( 5, 1 )

// set their positions
SetSpritePosition ( 1,   0, 0 )
SetSpritePosition ( 2,  50, 0 )
SetSpritePosition ( 3, 100, 0 )
SetSpritePosition ( 4, 150, 0 )
SetSpritePosition ( 5, 200, 0 )

// turn physics on
SetSpritePhysicsOn ( 1, 2 )
SetSpritePhysicsOn ( 2, 2 )
SetSpritePhysicsOn ( 3, 2 )
SetSpritePhysicsOn ( 4, 2 )
SetSpritePhysicsOn ( 5, 2 )

// alter default gravity
SetPhysicsGravity ( 10, 10 )

// main loop
do
    // update screeen
    Sync ( )
loop
