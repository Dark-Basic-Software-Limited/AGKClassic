
// simple multiplayer example

// set a display aspect
SetDisplayAspect ( 4.0 / 3.0 )

// load two images
LoadImage ( 1, "green.png" )
LoadImage ( 2, "red.png" )
LoadImage ( 3, "background5.jpg" )

// backdrop
CreateSprite ( 3, 3 )

// create sprite 11 (the host)
CreateSprite ( 11, 1 )
SetSpritePosition ( 11, 10, 20 )
SetSpriteSize ( 11, 25, -1 )

// create sprite 12 (the joiner)
CreateSprite ( 12, 2 )
SetSpritePosition ( 12, 50, 20 )
SetSpriteSize ( 12, 25, -1 )

// set some variables
State = 0
iType = 0
NetworkID = 0

// main loop
do
    // initial state of game - host or join game
    if State = 0
        // display instructions
        Print ( "Select green square to host a game" )
	    Print ( "Select red square to join a game" )

        // check input
        if GetPointerPressed ( ) = 1
            // find out which sprite was hit
            hit = GetSpriteHit ( GetPointerX ( ), GetPointerY ( ) )

            // green sprite - host game
            if ( hit = 11 )
                NetworkID = HostNetwork ( "AGK Test Game", "Player 1", 1025 )
                iType = 0
                State = 1
            endif

            // red sprite - join game
            if ( hit = 12 )
                randomplayername$="player"+str(random(1,10000))
                NetworkID = JoinNetwork ( "AGK Test Game", randomplayername$ )
                iType = 1
                State = 1
            endif
        endif
    endif

    // in game
    if State = 1 and NetworkID = 0 then print ("Network connection failed")
    if State = 1 and NetworkID > 0
        rem network status
        printc ("Network active:")
        printc (IsNetworkActive ( NetworkID ))
        for activity=1 to activitycount
            printc(".")
        next activity
        print("")
        inc activitycount
        if activitycount>10 then activitycount=0
        rem if active
        if IsNetworkActive ( NetworkID ) <> 0
            // get ID of first player
            id = GetNetworkFirstClient ( NetworkID )

            // cycle through all players and display their names on screen
            while id <> 0
                // show current player
                Print ( GetNetworkClientName ( NetworkID, id ) )

                // find next player
                id = GetNetworkNextClient ( NetworkID )
            endwhile

            // host events
            if iType = 0
                // update position of yellow sprite based on accelerometer
                x# = GetSpriteX ( 11 ) + GetDirectionX ( )
                y# = GetSpriteY ( 11 ) + GetDirectionY ( )

                // update position
                SetSpritePosition ( 11, x#, y# )

                // create a message containing the position to be sent out to the other player
                cmessage = CreateNetworkMessage ( )
                AddNetworkMessageFloat ( cmessage, x# )
                AddNetworkMessageFloat ( cmessage, y# )
                SendNetworkMessage ( NetworkID, 0, cmessage )
            endif

            // client events
            if iType = 1
                // find message from server
                cmessage = GetNetworkMessage ( NetworkID )

                // cycle through all messages
                while cmessage <> 0
                    // get position
                    x# = GetNetworkMessageFloat ( cmessage )
                    y# = GetNetworkMessageFloat ( cmessage )

                    // update green sprite's position
                    SetSpritePosition ( 11, x#, y# )

                    // delete the current message
                    DeleteNetworkMessage ( cmessage )

                    // find the next message
                    cmessage = GetNetworkMessage ( NetworkID )
                endwhile
            endif
        endif
    endif

    // update the screen
    Sync ( )
loop
