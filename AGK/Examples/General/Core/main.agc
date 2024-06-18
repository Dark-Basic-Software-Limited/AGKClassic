
// showing some of the general commands included within AGK

// get device width and height
width = GetDeviceWidth ( )
height = GetDeviceHeight ( )

// find the current orientation
orientation = GetOrientation ( )

// obtain the platform
platform$ = GetDeviceName ( )

// main loop
do
    // print information on screen
    print ( width )
    print ( height )
    print ( orientation )
    print ( platform$ )

    // update the screen
    Sync ( )
loop
