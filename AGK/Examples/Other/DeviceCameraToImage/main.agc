// show all errors
SetErrorMode(2)

// set window properties
SetWindowTitle( "Device Camera To Image" )
SetWindowSize( 1024, 768, 0 )

// set display properties
SetVirtualResolution( 1024, 768 )
SetOrientationAllowed( 1, 1, 1, 1 )
SetSyncRate( 30, 0 ) // 30fps instead of 60 to save battery
SetPrintSize( 25 )

// use the first camera available
cameraID = 0

// display it to an image and put that image on a sprite
result = SetDeviceCameraToImage( cameraID, 1 )
if ( result > 0 )
	CreateSprite( 1, 1 )
	SetSpritePosition( 1, 10, 50 )
	SetSpriteSize( 1, 1000, 700 )
endif

// get some info about the camera
numCameras = GetNumDeviceCameras()
cameraType = GetDeviceCameraType( cameraID )

do
    if ( result = 0 )
		Print( "Device camera not found or not supported on this platform" )
	else
		Print( "Device has " + str(numCameras) + " camera(s), using camera " + str(cameraID) )
		select ( cameraType ) 
			case 1 : Print( "Using back facing camera, tap to switch" ) : endcase
			case 2 : Print( "Using front facing camera, tap to switch" ) : endcase
		endselect
		
		if ( GetPointerPressed() )
			SetSpriteImage( 1, 0 ) // stop using the image
			SetDeviceCameraToImage( 0,0 ) // stop the camera, also deletes the image
			
			// try the next camera
			inc cameraID
			if ( cameraID >= numCameras ) then cameraID = 0
			SetDeviceCameraToImage( cameraID, 1 )
			cameraType = GetDeviceCameraType( cameraID )
			SetSpriteImage( 1, 1 )
		endif
	endif

    Print( "FPS: " + str(ScreenFPS(),1) )
    Sync()
loop
