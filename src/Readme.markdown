# Nano SVG example


Usage:

```

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

tex:releaseSelf()


local circle = svg.newImage {
    scale = "letterbox",
    scale = "zoomEven",
    scaleOffset = 0.5,
    pixelWidth = 256,
    pixelHeight = 256,
    -- filename = "test.svg",
    -- baseDir = system.ResourceDirectory,
    -- filePath = system.pathForFile( "test.svg" ),
    data = '<svg viewBox="0 0 200 200"><circle cx="100" cy="100" r="100"/></svg>',
    
    -- x = 100,
    -- y = 100,
    -- width = 100,
    -- height = 100,
}
circle:setFillColor(0.5);

```

