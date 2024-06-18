
// show all errors
SetErrorMode(2)

// set window properties
SetWindowTitle( "InAppPurchase" )
SetWindowSize( 768, 1024, 0 )
SetVirtualResolution( 768, 1024 )
SetOrientationAllowed( 1, 1, 1, 1 )
UseNewDefaultFonts( 1 )
SetPrintSize( 40 )

// all products must be added before calling InAppPurchaseSetup
if ( GetDeviceBaseName() = "ios" )
	InAppPurchaseAddProductID( "agk_test_once", 0 )
	//InAppPurchaseAddProductID( "agk_test_consumable", 0 )
else
	InAppPurchaseAddProductID( "agk_test", 0 )
endif
InAppPurchaseAddProductID( "test_sub", 2 )
InAppPurchaseSetup()

// add buttons
AddVirtualButton( 1, 384, 250, 100 )
SetVirtualButtonSize( 1, 400, 100 )
SetVirtualButtonText( 1, "Start Purchase" )

AddVirtualButton( 2, 384, 450, 100 )
SetVirtualButtonSize( 2, 400, 100 )
SetVirtualButtonText( 2, "Start Subscription" )

AddVirtualButton( 3, 384, 650, 100 )
SetVirtualButtonSize( 3, 400, 100 )
SetVirtualButtonText( 3, "Reset Purchase" )

AddVirtualButton( 4, 384, 850, 100 )
SetVirtualButtonSize( 4, 400, 100 )
SetVirtualButtonText( 4, "Restore Purchases" )

do
	Print( "FPS: " + str(ScreenFPS()) )
	Print( GetInAppPurchaseToken(0) )
	
	// display some information
    Print( "Item: " + GetInAppPurchaseDescription(0) + ", " + GetInAppPurchaseLocalPrice(0) + ", State: " + str(GetInAppPurchaseAvailable2(0)) )
    Print( "Subscription: " + GetInAppPurchaseDescription(1) + ", " + GetInAppPurchaseLocalPrice(1) + ", State: " + str(GetInAppPurchaseAvailable2(1)) )
    
    if ( GetVirtualButtonReleased(1) ) then InAppPurchaseActivate(0) // purchase item
    if ( GetVirtualButtonReleased(2) ) then InAppPurchaseActivate(1) // purchase subscription
	if ( GetVirtualButtonReleased(3) and GetInAppPurchaseAvailable2(0) = 4 ) // reset purchase 
		InAppPurchaseResetPurchase( GetInAppPurchaseToken(0) )
	endif
	if ( GetVirtualButtonReleased(4) ) then InAppPurchaseRestore()
        	
	Sync()
loop
