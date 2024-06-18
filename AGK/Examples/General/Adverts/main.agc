
// Monetization : Adverts
// Introduced in build 107

// display a background
backdrop = CreateSprite ( LoadImage ( "background5.jpg" ) )
SetSpriteSize ( backdrop, 100, 100 )

rem InnerActive allows advert positioning (Win/Mac/Android/etc)
SetInnerActiveDetails("insertinneractivecodehere")

rem Create the advert
CreateAdvert(0,1,2,1)

rem Set standard size, centered at bottom advert for this screen (100x100)
SetAdvertPosition(0,89,100)

// main loop
do
    // update the screen
    Print("Advertising commands active...")
    Sync ( )
loop

rem Free usages
DeleteAdvert()

rem launch own website at any time
OpenBrowser("http://www.thegamecreators.com")
