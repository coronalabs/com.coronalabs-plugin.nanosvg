// ----------------------------------------------------------------------------
// PluginMemBtm.cpp
// Copyright (c) 2016 Corona Labs Inc. All rights reserved.
// This software may be modified and distributed under the terms
// of the MIT license.  See the LICENSE.txt file for details.
// ----------------------------------------------------------------------------


#include "CoronaAssert.h"
#include "CoronaGraphics.h"
#include "PluginSvg.h"
#include "CoronaLog.h"
#include <memory>
#include <algorithm>

#define NANOSVG_ALL_COLOR_KEYWORDS
#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg.h"
#include "nanosvgrast.h"



struct MemBitmap
{
	unsigned w,h;
	unsigned char*data;
};

static unsigned int MemBitmap_GetW(void *context)
{
	return ((MemBitmap*)context)->w;
}

static unsigned int MemBitmap_GetH(void *context)
{
	return ((MemBitmap*)context)->h;
}

static const void* MemBitmap_GetData(void *context)
{
	return ((MemBitmap*)context)->data;
}

static void MemBitmap_Dispose(void *context)
{
	MemBitmap * btm = (MemBitmap*)context;
	if (btm->data) {
		delete[] btm->data;
	}
	delete btm;
}

#ifdef WIN32
#define constexpr 
#endif

constexpr const int& clamp( const int& v, const int& lo, const int& hi )
{
	return v<lo?lo:(v>hi?hi:v);
}


static int PluginSvg_New(lua_State *L )
{
	int index = 1;
	int res = 0;
	const char *path = nullptr;
	char *data = nullptr;
	int dpi = 96;
	double pixelWidth = -1;
	double pixelHeight = -1;


	if(lua_type( L, index ) != LUA_TTABLE)
	{
		CoronaLuaError(L, "svg.newTexture: expected table as first parameter, got %s", lua_typename(L, lua_type( L, index )));
		return res;
	}

	lua_getfield(L, index, "filePath");
	if(lua_type(L, -1) == LUA_TSTRING)
	{
		path = lua_tostring(L, -1);
	}
	lua_pop(L, 1);

	if(path == nullptr)
	{
		lua_getglobal(L, "system");
		lua_getfield(L, -1, "pathForFile");
		lua_getfield(L, index, "filename");
		lua_getfield(L, index, "baseDir");
		if(lua_type(L, -2) == LUA_TSTRING) // filename
		{
			lua_call(L, 2, 1);
			path = lua_tostring(L, -1);
			lua_pop(L, 2);
		}
		else
		{
			lua_pop(L, 4);
		}
	}

	if(path == nullptr)
	{
		lua_getfield(L, index, "data");
		if(lua_type(L, -1)==LUA_TSTRING)
		{
			const char* constData = lua_tostring(L, -1);
			data = new char[strlen(constData)];
			strcpy(data, constData);
		}
		lua_pop(L, 1);
	}

	if(path == nullptr && data == nullptr)
	{
		CoronaLuaError(L, "svg.newTexture: 'filename', 'filePath' or 'data' in parameters table must exist");
		return res;
	}

	lua_getfield(L, index, "dpi");
	if(lua_type(L, -1) == LUA_TNUMBER)
	{
		dpi = (int)lua_tointeger(L, -1);
	}
	lua_pop(L, 1);

	struct NSVGimage* image = data?nsvgParse(data, "px", dpi):nsvgParseFromFile(path, "px", dpi);
//	std::unique_ptr<NSVGimage, decltype(&nsvgDelete) > imageDestroyer(image, &nsvgDelete);

	if(data)
	{
		delete [] data;
	}

	if (image->height == 0 || image->width == 0 || image->shapes == nullptr)
	{
		CoronaLuaError(L, "svg.newTexture: target (%s) contains empty or invalid SVG", path?path:"<data>");
	}


	lua_getfield(L, index, "pixelWidth");
	if(lua_isnumber(L, -1))
	{
		pixelWidth = lua_tonumber(L, -1);
	}
	lua_pop(L, 1);

	lua_getfield(L, index, "pixelHeight");
	if(lua_isnumber(L, -1))
	{
		pixelHeight = lua_tonumber(L, -1);
	}
	lua_pop(L, 1);

	lua_getglobal(L, "system");
	lua_getfield(L, -1, "getInfo");
	lua_pushliteral(L, "maxTextureSize");
	lua_pcall(L, 1, 1, 0);
	double maxTexSize = lua_tonumber(L, -1);
	lua_pop(L, 2); // result and system
	maxTexSize = maxTexSize ? maxTexSize : 2048;

	bool fullBitmap = true;
	if(pixelHeight <= 0 && pixelWidth <= 0)
	{
		pixelWidth = image->width;
		pixelHeight = image->height;
	}
	else if(pixelWidth > 0 && pixelHeight <= 0)
	{
		pixelHeight = pixelWidth * image->width/image->height;
	}
	else if(pixelHeight > 0 && pixelWidth <= 0)
	{
		pixelWidth = pixelHeight * image->height/image->width;
	}
	else
	{
		fullBitmap = false;
	}

	if(pixelWidth>maxTexSize)
	{
		pixelHeight = pixelHeight * maxTexSize/pixelWidth;
		pixelWidth = maxTexSize;
	}
	if(pixelHeight>maxTexSize)
	{
		pixelWidth = pixelWidth * maxTexSize/pixelHeight;
		pixelHeight = maxTexSize;
	}

	MemBitmap * bitmap = new MemBitmap();
	bitmap->w = ceil(pixelWidth);
	bitmap->h = ceil(pixelHeight);
	size_t sz = bitmap->w*bitmap->h*CoronaExternalFormatBPP(kExternalBitmapFormat_RGBA);
	bitmap->data = new unsigned char[sz]();

	NSVGrasterizer* r = nsvgCreateRasterizer();
//	std::unique_ptr<NSVGrasterizer, decltype(&nsvgDeleteRasterizer) > rasterizerDestroyer(r, &nsvgDeleteRasterizer);

	if (fullBitmap)
	{
		float scale = std::min(bitmap->w/image->width, bitmap->h/image->height);
		nsvgRasterize(r, image, 0, 0, scale, bitmap->data, bitmap->w, bitmap->h, bitmap->w*4);
	}
	else
	{
		float offset = 0.5f;
		lua_getfield(L, index, "scaleOffset");
		if(lua_isnumber(L, -1))
		{
			offset = (float)lua_tonumber(L, -1);
		}
		lua_pop(L, 1);

		lua_getfield(L, index, "scale");
		const char* scaleMode = lua_tostring(L, -1) ? lua_tostring(L, -1) : "";
		int stride = bitmap->w*4;
		if(strcmp(scaleMode, "zoomEven") == 0)
		{
			float scale = std::max(bitmap->w/image->width, bitmap->h/image->height);
			float offsetX = 0;
			float offsetY = 0;
			if(bitmap->w/image->width > bitmap->h/image->height)
			{
				offsetY = offset*(bitmap->h - image->height*scale);
			}
			else
			{
				offsetX = offset*(bitmap->w - image->width*scale);
			}
			nsvgRasterize(r, image, offsetX, offsetY, scale, bitmap->data, bitmap->w, bitmap->h, stride);
		}
		else //default, letterbox
		{
			float scale = std::min(bitmap->w/image->width, bitmap->h/image->height);
			int offsetX = 0;
			int offsetY = 0;
			if(bitmap->w/image->width > bitmap->h/image->height)
			{
				offsetX = clamp(round(offset*(bitmap->w - image->width*scale)), 0, bitmap->w);
			}
			else
			{
				offsetY = clamp(round(offset*(bitmap->h - image->height*scale)), 0, bitmap->h);
			}
			nsvgRasterize(r, image, 0, 0, scale, bitmap->data + stride*offsetY + offsetX*4, bitmap->w - offsetX, bitmap->h - offsetY, stride);
		}

		lua_pop(L, 1);
	}

	nsvgDeleteRasterizer(r);
	nsvgDelete(image);

	// set up callbacks
	CoronaExternalTextureCallbacks callbacks = {};
	callbacks.size = sizeof(CoronaExternalTextureCallbacks);
	callbacks.getWidth = MemBitmap_GetW;
	callbacks.getHeight = MemBitmap_GetH;
	callbacks.onRequestBitmap = MemBitmap_GetData;
	callbacks.onFinalize = MemBitmap_Dispose;
	res = CoronaExternalPushTexture( L, &callbacks,  bitmap );

	return res;
}

static int PluginSvg_NewImage( lua_State *L )
{
	int ret = PluginSvg_New( L );
	if(ret == 0 || lua_gettop(L) != 2 || lua_type(L, 2) != LUA_TUSERDATA)
	{
		return ret;
	}

	lua_getglobal(L, "display");
	lua_getfield(L, 2, "filename");
	lua_getfield(L, 2, "baseDir");
	lua_getfield(L, 1, "width");
	lua_getfield(L, 1, "height");
	if(lua_isnumber(L, -1) && lua_isnumber(L, -2))
	{
		lua_getfield(L, 3, "newImageRect");
		lua_insert(L, -5);
		lua_call(L, 4, 1);
	}
	else if(lua_isnumber(L, -2))
	{
		double width = lua_tonumber(L, -2);

		lua_getfield(L, 2, "width");
		double pixelWidth = lua_tonumber(L, -1);
		lua_getfield(L, 2, "height");
		double pixelHeight = lua_tonumber(L, -1);
		lua_pop(L, 2);

		lua_pop(L, 1); // remove non-number height
		lua_pushnumber(L, width * pixelHeight/pixelWidth);

		lua_getfield(L, 3, "newImageRect");
		lua_insert(L, -5);
		lua_call(L, 4, 1);
	}
	else if(lua_isnumber(L, -1))
	{
		double height = lua_tonumber(L, -1);

		lua_getfield(L, 2, "width");
		double pixelWidth = lua_tonumber(L, -1);
		lua_getfield(L, 2, "height");
		double pixelHeight = lua_tonumber(L, -1);
		lua_pop(L, 2);

		lua_pushnumber(L, height * pixelWidth/pixelHeight);
		lua_replace(L, -3);

		lua_getfield(L, 3, "newImageRect");
		lua_insert(L, -5);
		lua_call(L, 4, 1);
	}
	else
	{
		lua_pop(L, 2);
		lua_getfield(L, 3, "newImage");
		lua_insert(L, 4);
		lua_call(L, 2, 1);
	}
	lua_remove(L, 3); // system
	lua_insert(L, -2);
	lua_getfield(L, -1, "releaseSelf");
	lua_insert(L, -2);
	lua_call(L, 1, 0);

	lua_getfield(L, 1, "x");
	lua_getfield(L, 1, "y");
	if(lua_isnumber(L, -1) && lua_isnumber(L, -2) && lua_istable(L, -3))
	{
		lua_getfield(L, -3, "translate");
		lua_insert(L, -3);
		lua_pushvalue(L, 2);
		lua_insert(L, -3);
		lua_call(L, 3, 0);
	}
	else
	{
		lua_pop(L, 2);
	}


	return 1;
}

CORONA_EXPORT int luaopen_plugin_nanosvg( lua_State *L );

CORONA_EXPORT
int luaopen_plugin_nanosvg( lua_State *L )
{
	static const luaL_Reg kVTable[] =
	{
		{ "newTexture", PluginSvg_New },
		{ "newImage", PluginSvg_NewImage },
		{ "newImageRect", PluginSvg_NewImage },
		{ NULL, NULL }
	};

	luaL_openlib( L, "plugin.nanosvg", kVTable, 0 );

	return 1;
}
