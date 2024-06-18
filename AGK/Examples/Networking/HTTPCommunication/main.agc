
// Call HTTP commands
// Introduced in build 107

// display a background
backdrop = CreateSprite ( LoadImage ( "background5.jpg" ) )
SetSpriteSize ( backdrop, 100, 100 )

// connect to server
http = CreateHTTPConnection ( )
SetHTTPHost( http, "www.appgamekit.com", 0 )

SendHTTPRequestASync( http, "agktest/itworks.html" )
while( GetHTTPResponseReady(http) = 0 )
	print("Waiting for response...")
	Sync()
endwhile

response$ = "Error"
if ( GetHTTPResponseReady(http) > 0 ) then response$ = GetHTTPResponse(http)

CloseHTTPConnection(http)
DeleteHTTPConnection(http)


do
	print(ScreenFPS())
	print(response$)
	sync()
loop


// links to try
// http://www.appgamekit.com/agktest/posttest.php
// http://www.appgamekit.com/agktest/itworks.html
