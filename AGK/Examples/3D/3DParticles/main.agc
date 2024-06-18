// show all errors
SetErrorMode(2)

// set window properties
SetWindowTitle( "3DParticles" )
SetWindowSize( 1024, 768, 0 )
SetWindowAllowResize( 1 ) // allow the user to resize the window

// set display properties
SetVirtualResolution( 1024, 768 ) // doesn't have to match the window
SetOrientationAllowed( 1, 1, 1, 1 ) // allow both portrait and landscape on mobile devices
SetSyncRate( 30, 0 ) // 30fps instead of 60 to save battery
SetScissor( 0,0,0,0 ) // use the maximum available screen space, no black borders
UseNewDefaultFonts( 1 ) // since version 2.0.22 we can use nicer default fonts

// create floor
ground = CreateObjectPlane( 100,100 )
SetObjectRotation( ground, 90,0,0 )
SetObjectColor( ground, 128,128,128,255 )

// create marker to show emitter position
CreateObjectSphere( 1, 1, 10, 10 )
SetObjectPosition( 1, 0,1,0 )

freq# = 100 // per second
life# = 5.0 // seconds
additive = 1 // additive or alpha transparent particles

// create 3D particle emitter
Create3DParticles( 1, 0,0,0 )
Set3DParticlesPosition( 1, 0,1,0 )
Set3DParticlesDirection( 1, 0,3,0, 0 )
Set3DParticlesFrequency( 1, freq# )
Set3DParticlesLife( 1, life# )
Set3DParticlesSize( 1, 2 )
Set3DParticlesVelocityRange( 1, 0.5, 3.0 )
//Set3DParticlesStartZone( 1, -5,-5,-5, 5,5,5 ) // appear anywhere in this box
Set3DParticlesDirectionRange( 1, 180, 180 ) // anywhere in a hemisphere

// two types of transparency, additive and alpha
if ( additive = 1 )
	LoadImage( 1, "particleBlack.png" )
	Set3DParticlesImage( 1, 1 )
	Set3DParticlesTransparency( 1, 2 )
	
	// fade the particles out before they die
	Add3DParticlesColorKeyFrame( 1, life#-1, 255,255,255,255 )
	Add3DParticlesColorKeyFrame( 1, life#, 0,0,0,255 )
else
	LoadImage( 1, "particle.png" )
	Set3DParticlesImage( 1, 1 )
	Set3DParticlesTransparency( 1, 1 )
	
	// fade the particles out before they die
	Add3DParticlesColorKeyFrame( 1, life#-1, 255,255,255,255 )
	Add3DParticlesColorKeyFrame( 1, life#, 255,255,255,0 )
endif

// position the camera to look at the particles
SetCameraPosition( 1, 0,15,-25 )
SetCameraLookAt( 1, 0,1,0, 0 )

// controls setup
addvirtualbutton( 1, 970,715,100 )
addvirtualbutton( 2, 970,610,100 )
startx# = GetPointerX()
starty# = GetPointerY()

do
    // control the camera
    if ( GetRawKeyState( 87 ) ) then MoveCameraLocalZ( 1, 0.2 )
	if ( GetRawKeyState( 83 ) ) then MoveCameraLocalZ( 1, -0.2 )

	if ( GetRawKeyState( 65 ) ) then MoveCameraLocalX( 1, -0.2 )
	if ( GetRawKeyState( 68 ) ) then MoveCameraLocalX( 1, 0.2 )

	if ( GetRawKeyState( 81 ) ) then MoveCameraLocalY( 1, -0.2 )
	if ( GetRawKeyState( 69 ) ) then MoveCameraLocalY( 1, 0.2 )
	
	if ( getvirtualbuttonstate(1) = 1 ) then MoveCameraLocalZ( 1, -0.6 )
    if ( getvirtualbuttonstate(2) = 1 ) then MoveCameraLocalZ( 1, 0.6 )
	
    if ( GetPointerPressed() )
        startx# = GetPointerX()
        starty# = GetPointerY()
        angx# = GetCameraAngleX(1)
        angy# = GetCameraAngleY(1)
        pressed = 1
    endif

    if ( GetPointerState() = 1 )
        fDiffX# = (GetPointerX() - startx#)/4.0
        fDiffY# = (GetPointerY() - starty#)/4.0

        newX# = angx# + fDiffY#
        if ( newX# > 89 ) then newX# = 89
        if ( newX# < -89 ) then newX# = -89
        SetCameraRotation( 1, newX#, angy# + fDiffX#, 0 )
    endif
   
	Print( ScreenFPS() )	
	Sync()
loop
