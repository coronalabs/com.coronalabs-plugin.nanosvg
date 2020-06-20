-- Your code here
local svg = require "plugin.nanosvg"


local tex = svg.newTexture {
    scale = "letterbox",
    scale = "zoomEven",
    scaleOffset = 0.5,
    pixelWidth = 256,
    pixelHeight = 256,
    filename = "test.svg",
    -- baseDir = system.ResourceDirectory,
    -- filePath = system.pathForFile( "test.svg" ),
    -- data = '<svg viewBox="0 0 200 200"><circle cx="100" cy="100" r="100"/></svg>',
}

if tex then
	print(tex.width, tex.height)
    display.newRect(  display.contentCenterX, display.contentCenterY, 300, 256 ):setFillColor( 0.5 )
    display.newRect(  display.contentCenterX, display.contentCenterY, tex.width, tex.height ):setFillColor( 0.2 )
    display.newImage( tex.filename, tex.baseDir, display.contentCenterX, display.contentCenterY )
    tex:releaseSelf( )
else
    print("Error rendering SVG")
end


local circle = svg.newImage {
    scale = "letterbox",
    scale = "zoomEven",
    scaleOffset = 0.5,
    pixelWidth = 256,
    pixelHeight = 256,
    filename = "test.svg",
    baseDir = system.ResourceDirectory,
    filePath = system.pathForFile( "test.svg" ),
    data = '<svg viewBox="0 0 200 200"><circle cx="100" cy="100" r="100"/></svg>',
    
    x = 50,
    y = 50,
    width = 100,
    height = 100,
}
circle:setFillColor(1,0,0);
