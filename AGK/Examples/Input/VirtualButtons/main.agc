rem
rem AGK Application
rem

rem Portrait App
SetDisplayAspect( 0.666 )

rem Nice backdrop
CreateSprite ( 1, LoadImage ( "background5.jpg" ) )
SetSpriteSize ( 1, 100, 100 )

rem Create Virtual Buttons
AddVirtualButton ( 1, 50, 30, 30 )
AddVirtualButton ( 2, 50, 70, 30 )

rem Main loop
do
 Print ( "Virtual Buttons Are Multi-Touch" )
 Print ( "" )
 PrintC ( "Button One = " )
 Print ( GetVirtualButtonState ( 1 ) )
 PrintC ( "Button Two = " )
 Print ( GetVirtualButtonState ( 2 ) )
 Sync()
loop
