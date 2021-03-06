/*
* cocos2d-x   http://www.cocos2d-x.org
*
* Copyright (c) 2010-2011 - cocos2d-x community
* Copyright (c) 2010-2011 cocos2d-x.org
* Copyright (c) 2008      Apple Inc. All Rights Reserved.
* 
* Portions Copyright (c) Microsoft Open Technologies, Inc.
* All Rights Reserved
* 
* Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
* You may obtain a copy of the License at 
* 
* http://www.apache.org/licenses/LICENSE-2.0 
* 
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an 
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
* See the License for the specific language governing permissions and limitations under the License.
*/

/*
* Support for RGBA_4_4_4_4 and RGBA_5_5_5_1 was copied from:
* https://devforums.apple.com/message/37855#37855 by a1studmuffin
*/

#include "pch.h"

#include "CCTexture2D.h"

#include "ccConfig.h"
#include "ccMacros.h"
#include "CCConfiguration.h"
#include "platform.h"
#include "CCImage.h"
#include "CCGL.h"
#include "support/ccUtils.h"
#include "CCPlatformMacros.h"
#include "CCTexturePVR.h"
#include "CCDirector.h"

#if CC_ENABLE_CACHE_TEXTTURE_DATA
    #include "CCTextureCache.h"
#endif

#include <fstream>
using namespace std;

NS_CC_BEGIN

#if CC_FONT_LABEL_SUPPORT
// FontLabel support
#endif// CC_FONT_LABEL_SUPPORT

//CLASS IMPLEMENTATIONS:

// If the image has alpha, you can create RGBA8 (32-bit) or RGBA4 (16-bit) or RGB5A1 (16-bit)
// Default is: RGBA8888 (32-bit textures)
static CCTexture2DPixelFormat g_defaultAlphaPixelFormat = kCCTexture2DPixelFormat_Default;

// By default PVR images are treated as if they don't have the alpha channel premultiplied
static bool PVRHaveAlphaPremultiplied_ = false;

ID3D11ShaderResourceView* CCTexture2D::getTextureResource()
{
	return m_pTextureResource;
}

CCTexture2D::CCTexture2D()
: m_uPixelsWide(0)
, m_uPixelsHigh(0)
, m_uName(0)
, m_fMaxS(0.0)
, m_fMaxT(0.0)
, m_bHasPremultipliedAlpha(false)
, m_bPVRHaveAlphaPremultiplied(true)
{
	m_pTextureResource=0;
	m_sampleState = 0;
}

CCTexture2D::~CCTexture2D()
{
#if CC_ENABLE_CACHE_TEXTTURE_DATA
    VolatileTexture::removeTexture(this);
#endif

	CCLOGINFO("cocos2d: deallocing CCTexture2D %u.", m_uName);

	// Release the texture resource.
	if(m_pTextureResource)
	{
		m_pTextureResource->Release();
		m_pTextureResource = 0;
	}

	// Release the sampler state.
	if(m_sampleState)
	{
		m_sampleState->Release();
		m_sampleState = 0;
	}
}

CCTexture2DPixelFormat CCTexture2D::getPixelFormat()
{
	return m_ePixelFormat;
}

unsigned int CCTexture2D::getPixelsWide()
{
	return m_uPixelsWide;
}

unsigned int CCTexture2D::getPixelsHigh()
{
	return m_uPixelsHigh;
}

CCuint CCTexture2D::getName()
{
	return m_uName;
}

const CCSize& CCTexture2D::getContentSizeInPixels()
{
	return m_tContentSize;
}

CCSize CCTexture2D::getContentSize()
{
	CCSize ret;
	ret.width = m_tContentSize.width / CC_CONTENT_SCALE_FACTOR();
	ret.height = m_tContentSize.height / CC_CONTENT_SCALE_FACTOR();

	return ret;
}

CCfloat CCTexture2D::getMaxS()
{
	return m_fMaxS;
}

void CCTexture2D::setMaxS(CCfloat maxS)
{
	m_fMaxS = maxS;
}

CCfloat CCTexture2D::getMaxT()
{
	return m_fMaxT;
}

ccResolutionType CCTexture2D::getResolutionType()
{
    return m_eResolutionType; 
}

void CCTexture2D::setResolutionType(ccResolutionType resolution)
{
    m_eResolutionType = resolution;
}

void CCTexture2D::setMaxT(CCfloat maxT)
{
	m_fMaxT = maxT;
}

void CCTexture2D::releaseData(void *data)
{
    free(data);
}

void* CCTexture2D::keepData(void *data, unsigned int length)
{
    CC_UNUSED_PARAM(length);
	//The texture data mustn't be saved becuase it isn't a mutable texture.
	return data;
}

bool CCTexture2D::hasPremultipliedAlpha()
{
	return m_bHasPremultipliedAlpha;
}

bool CCTexture2D::initWithData(const void *data, CCTexture2DPixelFormat pixelFormat, unsigned int pixelsWide, unsigned int pixelsHigh, const CCSize& contentSize)
{
	int formatTmp = DXGI_FORMAT_R8G8B8A8_UNORM;
	int dataSizeByte = 4;
	/*==
	glPixelStorei(CC_UNPACK_ALIGNMENT,1);
	glGenTextures(1, &m_uName);
	glBindTexture(CC_TEXTURE_2D, m_uName);
	==*/
	this->setAntiAliasTexParameters();
	
	// Specify OpenGL texture image
	switch(pixelFormat)
	{
	case kCCTexture2DPixelFormat_RGBA8888:
		formatTmp = DXGI_FORMAT_R8G8B8A8_UNORM;
		dataSizeByte = 4;
		//info.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//=glTexImage2D(CC_TEXTURE_2D, 0, CC_RGBA, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, 0, CC_RGBA, CC_UNSIGNED_BYTE, data);
		break;
	case kCCTexture2DPixelFormat_RGB888:
		dataSizeByte = 4;
		formatTmp = DXGI_FORMAT_R8G8B8A8_UNORM;
		//info.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//=glTexImage2D(CC_TEXTURE_2D, 0, CC_RGB, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, 0, CC_RGB, CC_UNSIGNED_BYTE, data);
		break;
	case kCCTexture2DPixelFormat_RGBA4444:
		dataSizeByte = 4;
		formatTmp = DXGI_FORMAT_B4G4R4A4_UNORM;
		//info.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//=glTexImage2D(CC_TEXTURE_2D, 0, CC_RGBA, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, 0, CC_RGBA, CC_UNSIGNED_SHORT_4_4_4_4, data);
		break;
	case kCCTexture2DPixelFormat_RGB5A1:
		dataSizeByte = 2;
		formatTmp = DXGI_FORMAT_B5G5R5A1_UNORM;
		//info.Format = DXGI_FORMAT_B5G5R5A1_UNORM;
		//=glTexImage2D(CC_TEXTURE_2D, 0, CC_RGBA, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, 0, CC_RGBA, CC_UNSIGNED_SHORT_5_5_5_1, data);
		break;
	case kCCTexture2DPixelFormat_RGB565:
		dataSizeByte = 2;
		formatTmp = DXGI_FORMAT_B5G6R5_UNORM;
		//info.Format = DXGI_FORMAT_B5G6R5_UNORM;
		//=glTexImage2D(CC_TEXTURE_2D, 0, CC_RGB, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, 0, CC_RGB, CC_UNSIGNED_SHORT_5_6_5, data);
		break;
	case kCCTexture2DPixelFormat_AI88:
		dataSizeByte = 2;
		formatTmp = DXGI_FORMAT_R8G8_UNORM;
		//info.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//=glTexImage2D(CC_TEXTURE_2D, 0, CC_LUMINANCE_ALPHA, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, 0, CC_LUMINANCE_ALPHA, CC_UNSIGNED_BYTE, data);
		break;
	case kCCTexture2DPixelFormat_A8:
		dataSizeByte = 1;
		formatTmp = DXGI_FORMAT_A8_UNORM;
		//info.Format = DXGI_FORMAT_A8_UNORM;
		//=glTexImage2D(CC_TEXTURE_2D, 0, CC_ALPHA, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, 0, CC_ALPHA, CC_UNSIGNED_BYTE, data);
		break;
	default:
		CCAssert(0, "NSInternalInconsistencyException");

	}
	ID3D11Device *pdevice = CCDirector::sharedDirector()->getOpenGLView()->GetDevice();
	ID3D11Texture2D *tex;
	D3D11_TEXTURE2D_DESC tdesc;
	D3D11_SUBRESOURCE_DATA tbsd;
	tbsd.pSysMem = data;
	tbsd.SysMemPitch = pixelsWide*dataSizeByte;
	tbsd.SysMemSlicePitch = pixelsWide*pixelsHigh*dataSizeByte; // Not needed since this is a 2d texture

	tdesc.Width = pixelsWide;
	tdesc.Height = pixelsHigh;
	tdesc.MipLevels = 1;
	tdesc.ArraySize = 1;

	tdesc.SampleDesc.Count = 1;
	tdesc.SampleDesc.Quality = 0;
	tdesc.Usage = D3D11_USAGE_DEFAULT;
	tdesc.Format = (DXGI_FORMAT)formatTmp;
	tdesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	tdesc.CPUAccessFlags = 0;
	tdesc.MiscFlags = 0;
	
	if(FAILED(pdevice->CreateTexture2D(&tdesc,&tbsd,&tex)))
	{
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	D3D11_TEXTURE2D_DESC desc;
	// Get a texture description to determine the texture
	// format of the loaded texture.
	tex->GetDesc( &desc );

	// Fill in the D3D11_SHADER_RESOURCE_VIEW_DESC structure.
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;

	// Create the shader resource view.
	pdevice->CreateShaderResourceView( tex, &srvDesc, &m_pTextureResource );
	m_uName = (CCuint)m_pTextureResource;
	if ( tex )
	{
		tex->Release();
		tex = 0;
	}
	
	m_tContentSize = contentSize;
	m_uPixelsWide = pixelsWide;
	m_uPixelsHigh = pixelsHigh;
	m_ePixelFormat = pixelFormat;
	m_fMaxS = contentSize.width / (float)(pixelsWide);
	m_fMaxT = contentSize.height / (float)(pixelsHigh);

	m_bHasPremultipliedAlpha = false;

	m_eResolutionType = kCCResolutionUnknown;

	return true;
}


char * CCTexture2D::description(void)
{
	char *ret = new char[100];
	sprintf(ret, "<CCTexture2D | Name = %u | Dimensions = %u x %u | Coordinates = (%.2f, %.2f)>", m_uName, m_uPixelsWide, m_uPixelsHigh, m_fMaxS, m_fMaxT);
	return ret;
}

// implementation CCTexture2D (Image)

bool CCTexture2D::initWithImage(CCImage * uiImage)
{
	return initWithImage(uiImage, kCCResolutionUnknown);
}

bool CCTexture2D::initWithImage(CCImage * uiImage, ccResolutionType resolution)
{
	unsigned int POTWide, POTHigh;

	if(uiImage == NULL)
	{
		CCLOG("cocos2d: CCTexture2D. Can't create Texture. UIImage is nil");
		this->release();
		return false;
	}

	CCConfiguration *conf = CCConfiguration::sharedConfiguration();

#if CC_TEXTURE_NPOT_SUPPORT
	if( conf->isSupportsNPOT() ) 
	{
		POTWide = uiImage->getWidth();
		POTHigh = uiImage->getHeight();
	}
	else 
#endif
	{
		POTWide = ccNextPOT(uiImage->getWidth());
		POTHigh = ccNextPOT(uiImage->getHeight());
	}

	unsigned maxTextureSize = conf->getMaxTextureSize();
	if( POTHigh > maxTextureSize || POTWide > maxTextureSize ) 
	{
		CCLOG("cocos2d: WARNING: Image (%u x %u) is bigger than the supported %u x %u", POTWide, POTHigh, maxTextureSize, maxTextureSize);
		this->release();
		return NULL;
	}

	m_eResolutionType = resolution;

	// always load premultiplied images
	return initPremultipliedATextureWithImage(uiImage, POTWide, POTHigh);
}
bool CCTexture2D::initPremultipliedATextureWithImage(CCImage *image, unsigned int POTWide, unsigned int POTHigh)
{
	unsigned char*			data = NULL;
	unsigned char*			tempData =NULL;
	unsigned int*			inPixel32 = NULL;
	unsigned short*			outPixel16 = NULL;
	bool					hasAlpha;
	CCSize					imageSize;
	CCTexture2DPixelFormat	pixelFormat;

	hasAlpha = image->hasAlpha();

	size_t bpp = image->getBitsPerComponent();

    // compute pixel format
	if(hasAlpha)
	{
		pixelFormat = g_defaultAlphaPixelFormat;
	}
	else
	{
		if (bpp >= 8)
		{
			pixelFormat = kCCTexture2DPixelFormat_RGB888;
		}
		else
		{
			CCLOG("cocos2d: CCTexture2D: Using RGB565 texture since image has no alpha");
			pixelFormat = kCCTexture2DPixelFormat_RGB565;
		}
	}


	imageSize = CCSizeMake((float)(image->getWidth()), (float)(image->getHeight()));

	switch(pixelFormat) {          
		case kCCTexture2DPixelFormat_RGBA8888:
		case kCCTexture2DPixelFormat_RGBA4444:
		case kCCTexture2DPixelFormat_RGB5A1:
		case kCCTexture2DPixelFormat_RGB565:
		case kCCTexture2DPixelFormat_A8:
			tempData = (unsigned char*)(image->getData());
			CCAssert(tempData != NULL, "NULL image data.");

			if(image->getWidth() == (short)POTWide && image->getHeight() == (short)POTHigh)
			{
				data = new unsigned char[POTHigh * POTWide * 4];
				memcpy(data, tempData, POTHigh * POTWide * 4);
			}
			else
			{
				data = new unsigned char[POTHigh * POTWide * 4];
				memset(data, 0, POTHigh * POTWide * 4);

				unsigned char* pPixelData = (unsigned char*) tempData;
				unsigned char* pTargetData = (unsigned char*) data;

                int imageHeight = image->getHeight();
				for(int y = 0; y < imageHeight; ++y)
				{
					memcpy(pTargetData+POTWide*4*y, pPixelData+(image->getWidth())*4*y, (image->getWidth())*4);
				}
			}

			break;    
		case kCCTexture2DPixelFormat_RGB888:
			tempData = (unsigned char*)(image->getData());
			CCAssert(tempData != NULL, "NULL image data.");
			data = new unsigned char[POTHigh * POTWide * 4];
			memset(data, 0, POTHigh * POTWide * 4);
			if(image->getWidth() == (short)POTWide && image->getHeight() == (short)POTHigh)
			{
				for (int i = 0; i < POTHigh; ++i)
				{
					for (int j = 0; j < POTWide; ++j)
					{
						int m = (i * POTWide * 4) + (j * 4);
						int n = (i * POTWide * 3) + (j * 3);
						data[m+0] = tempData[n+0];
						data[m+1] = tempData[n+1];
						data[m+2] = tempData[n+2];
						data[m+3] = 255;
					}
				}
			}
			else
			{
				int imagewidth = image->getWidth();
				int imageHeight = image->getHeight();
				for (int i = 0; i < imageHeight; ++i)
				{
					for (int j = 0; j < imagewidth; ++j)
					{
						int m = (i * POTWide * 4) + (j * 4);
						int n = (i * imagewidth * 3) + (j * 3);
						data[m+0] = tempData[n+0];
						data[m+1] = tempData[n+1];
						data[m+2] = tempData[n+2];
						data[m+3] = 255;
					}
				}
			}
			break;   
		default:
			CCAssert(0, "Invalid pixel format");
	}

	// Repack the pixel data into the right format

	if(pixelFormat == kCCTexture2DPixelFormat_RGB565) {
		//Convert "RRRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA" to "RRRRRGGGGGGBBBBB"
		tempData = new unsigned char[POTHigh * POTWide * 2];
		inPixel32 = (unsigned int*)data;
		outPixel16 = (unsigned short*)tempData;

		unsigned int length = POTWide * POTHigh;
		for(unsigned int i = 0; i < length; ++i, ++inPixel32)
		{
			*outPixel16++ = 
				((((*inPixel32 >> 0) & 0xFF) >> 3) << 11) |  // R
				((((*inPixel32 >> 8) & 0xFF) >> 2) << 5) |   // G
				((((*inPixel32 >> 16) & 0xFF) >> 3) << 0);   // B
		}

		delete [] data;
		data = tempData;
	}
	else if (pixelFormat == kCCTexture2DPixelFormat_RGBA4444) {
		//Convert "RRRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA" to "RRRRGGGGBBBBAAAA"
		tempData = new unsigned char[POTHigh * POTWide * 2];
		inPixel32 = (unsigned int*)data;
		outPixel16 = (unsigned short*)tempData;

		unsigned int length = POTWide * POTHigh;
		for(unsigned int i = 0; i < length; ++i, ++inPixel32)
		{
			*outPixel16++ = 
			((((*inPixel32 >> 0) & 0xFF) >> 4) << 12) | // R
			((((*inPixel32 >> 8) & 0xFF) >> 4) << 8) | // G
			((((*inPixel32 >> 16) & 0xFF) >> 4) << 4) | // B
			((((*inPixel32 >> 24) & 0xFF) >> 4) << 0); // A
		}

		delete [] data;
		data = tempData;
	}
	else if (pixelFormat == kCCTexture2DPixelFormat_RGB5A1) {
		//Convert "RRRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA" to "RRRRRGGGGGBBBBBA"
		tempData = new unsigned char[POTHigh * POTWide * 2];
		inPixel32 = (unsigned int*)data;
		outPixel16 = (unsigned short*)tempData;

		unsigned int length = POTWide * POTHigh;
		for(unsigned int i = 0; i < length; ++i, ++inPixel32)
		{
 			*outPixel16++ = 
			((((*inPixel32 >> 0) & 0xFF) >> 3) << 11) | // R
			((((*inPixel32 >> 8) & 0xFF) >> 3) << 6) | // G
			((((*inPixel32 >> 16) & 0xFF) >> 3) << 1) | // B
			((((*inPixel32 >> 24) & 0xFF) >> 7) << 0); // A
		}

		delete []data;
		data = tempData;
	}
	else if (pixelFormat == kCCTexture2DPixelFormat_A8)
	{
		// fix me, how to convert to A8
		pixelFormat = kCCTexture2DPixelFormat_RGBA8888;

		/*
		 * The code can not work, how to convert to A8?
		 *
		tempData = new unsigned char[POTHigh * POTWide];
		inPixel32 = (unsigned int*)data;
		outPixel8 = tempData;

		unsigned int length = POTWide * POTHigh;
		for(unsigned int i = 0; i < length; ++i, ++inPixel32)
		{
			*outPixel8++ = (*inPixel32 >> 24) & 0xFF;
		}

		delete []data;
		data = tempData;
		*/
	}

	if (data)
	{
		CCAssert(this->initWithData(data, pixelFormat, POTWide, POTHigh, imageSize), "Create texture failed!");

		// should be after calling super init
		m_bHasPremultipliedAlpha = image->isPremultipliedAlpha();

		//CGContextRelease(context);
		delete [] data;
	}
	return true;
}

// implementation CCTexture2D (Text)
bool CCTexture2D::initWithString(const char *text, const char *fontName, float fontSize)
{
    return initWithString(text,  fontName, fontSize, CCSizeMake(0,0), kCCTextAlignmentCenter, kCCVerticalTextAlignmentTop);
}

bool CCTexture2D::initWithString(const char *text,  const char *fontName, float fontSize, const CCSize& dimensions, CCTextAlignment hAlignment, CCVerticalTextAlignment vAlignment)
{
#if CC_ENABLE_CACHE_TEXTURE_DATA
    // cache the texture data
    VolatileTexture::addStringTexture(this, text, dimensions, hAlignment, vAlignment, fontName, fontSize);
#endif

    bool bRet = false;
    CCImage::ETextAlign eAlign;

    if (kCCVerticalTextAlignmentTop == vAlignment)
    {
        eAlign = (kCCTextAlignmentCenter == hAlignment) ? CCImage::kAlignTop
            : (kCCTextAlignmentLeft == hAlignment) ? CCImage::kAlignTopLeft : CCImage::kAlignTopRight;
    }
    else if (kCCVerticalTextAlignmentCenter == vAlignment)
    {
        eAlign = (kCCTextAlignmentCenter == hAlignment) ? CCImage::kAlignCenter
            : (kCCTextAlignmentLeft == hAlignment) ? CCImage::kAlignLeft : CCImage::kAlignRight;
    }
    else if (kCCVerticalTextAlignmentBottom == vAlignment)
    {
        eAlign = (kCCTextAlignmentCenter == hAlignment) ? CCImage::kAlignBottom
            : (kCCTextAlignmentLeft == hAlignment) ? CCImage::kAlignBottomLeft : CCImage::kAlignBottomRight;
    }
    else
    {
        CCAssert(false, "Not supported alignment format!");
    }
    
    do 
    {
        CCImage* pImage = new CCImage();
        CC_BREAK_IF(NULL == pImage);
        bRet = pImage->initWithString(text, (int)dimensions.width, (int)dimensions.height, eAlign, fontName, (int)fontSize);
        CC_BREAK_IF(!bRet);
        bRet = initWithImage(pImage);
        CC_SAFE_RELEASE(pImage);
    } while (0);
    
    return bRet;
}


// implementation CCTexture2D (Drawing)

void CCTexture2D::drawAtPoint(const CCPoint& point)
{
	CCfloat	coordinates[] = {	
		0.0f,	m_fMaxT,
		m_fMaxS,m_fMaxT,
		0.0f,	0.0f,
		m_fMaxS,0.0f };

	CCfloat	width = (CCfloat)m_uPixelsWide * m_fMaxS,
		height = (CCfloat)m_uPixelsHigh * m_fMaxT;

	CCfloat		vertices[] = {	
		point.x,			point.y,	0.0f,
		width + point.x,	point.y,	0.0f,
		point.x,			height  + point.y,	0.0f,
		width + point.x,	height  + point.y,	0.0f };

	//glBindTexture(GL_TEXTURE_2D, m_uName);
	//glVertexPointer(3, GL_FLOAT, 0, vertices);
	//glTexCoordPointer(2, GL_FLOAT, 0, coordinates);
	//glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void CCTexture2D::drawInRect(const CCRect& rect)
{
	CCfloat	coordinates[] = {	
		0.0f,	m_fMaxT,
		m_fMaxS,m_fMaxT,
		0.0f,	0.0f,
		m_fMaxS,0.0f };

	CCfloat	vertices[] = {	rect.origin.x,		rect.origin.y,							/*0.0f,*/
		rect.origin.x + rect.size.width,		rect.origin.y,							/*0.0f,*/
		rect.origin.x,							rect.origin.y + rect.size.height,		/*0.0f,*/
		rect.origin.x + rect.size.width,		rect.origin.y + rect.size.height,		/*0.0f*/ };

	//glBindTexture(GL_TEXTURE_2D, m_uName);
	//glVertexPointer(2, GL_FLOAT, 0, vertices);
	//glTexCoordPointer(2, GL_FLOAT, 0, coordinates);
	//glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

#ifdef CC_SUPPORT_PVRTC
// implementation CCTexture2D (PVRTC);    
bool CCTexture2D::initWithPVRTCData(const void *data, int level, int bpp, bool hasAlpha, int length, CCTexture2DPixelFormat pixelFormat)
{
	if( !(CCConfiguration::sharedConfiguration()->isSupportsPVRTC()) )
	{
		CCLOG("cocos2d: WARNING: PVRTC images is not supported.");
		this->release();
		return false;
	}

	glGenTextures(1, &m_uName);
	glBindTexture(CC_TEXTURE_2D, m_uName);

	this->setAntiAliasTexParameters();

	GLenum format;
	GLsizei size = length * length * bpp / 8;
	if(hasAlpha) {
		format = (bpp == 4) ? CC_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG : CC_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
	} else {
		format = (bpp == 4) ? CC_COMPRESSED_RGB_PVRTC_4BPPV1_IMG : CC_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
	}
	if(size < 32) {
		size = 32;
	}
	glCompressedTexImage2D(CC_TEXTURE_2D, level, format, length, length, 0, size, data);

	m_tContentSize = CCSizeMake((float)(length), (float)(length));
	m_uPixelsWide = length;
	m_uPixelsHigh = length;
	m_fMaxS = 1.0f;
	m_fMaxT = 1.0f;
    m_bHasPremultipliedAlpha = PVRHaveAlphaPremultiplied_;
    m_ePixelFormat = pixelFormat;

	return true;
}
#endif // CC_SUPPORT_PVRTC

bool CCTexture2D::initWithPVRFile(const char* file)
{
    bool bRet = false;
    // nothing to do with CCObject::init
    
    CCTexturePVR *pvr = new CCTexturePVR;
    bRet = pvr->initWithContentsOfFile(file);
        
    if (bRet)
    {
        pvr->setRetainName(true); // don't dealloc texture on release
        
        m_uName = pvr->getName();
        m_fMaxS = 1.0f;
        m_fMaxT = 1.0f;
        m_uPixelsWide = pvr->getWidth();
        m_uPixelsHigh = pvr->getHeight();
        m_tContentSize = CCSizeMake((float)m_uPixelsWide, (float)m_uPixelsHigh);
        m_bHasPremultipliedAlpha = PVRHaveAlphaPremultiplied_;
        m_ePixelFormat = pvr->getFormat();
                
        this->setAntiAliasTexParameters();
        pvr->release();
    }
    else
    {
        CCLOG("cocos2d: Couldn't load PVR image %s", file);
    }

    return bRet;
}

void CCTexture2D::PVRImagesHavePremultipliedAlpha(bool haveAlphaPremultiplied)
{
    PVRHaveAlphaPremultiplied_ = haveAlphaPremultiplied;
}

    
//
// Use to apply MIN/MAG filter
//
// implementation CCTexture2D (GLFilter)

void CCTexture2D::generateMipmap()
{

	CCAssert( m_uPixelsWide == ccNextPOT(m_uPixelsWide) && m_uPixelsHigh == ccNextPOT(m_uPixelsHigh), "Mimpap texture only works in POT textures");
}

 void CCTexture2D::setTexParameters(ccTexParams *texParams)
{
	
	CCAssert( (m_uPixelsWide == ccNextPOT(m_uPixelsWide) && m_uPixelsHigh == ccNextPOT(m_uPixelsHigh)) ||
		(texParams->wrapS == CC_CLAMP_TO_EDGE && texParams->wrapT == CC_CLAMP_TO_EDGE),
		"CC_CLAMP_TO_EDGE should be used in NPOT textures");

	D3D11_SAMPLER_DESC samplerDesc;
	int filter = -1;
	int u = -1;
	int v = -1;
	bool bcreat = true;
	ZeroMemory(&samplerDesc,sizeof(D3D11_SAMPLER_DESC));
	if ( m_sampleState )
	{
		m_sampleState->GetDesc(&samplerDesc);
		filter = samplerDesc.Filter;
		u = samplerDesc.AddressU;
		v = samplerDesc.AddressV;
	}

	if ( texParams->magFilter == CC_NEAREST )
	{
		switch(texParams->minFilter)
		{
		case CC_NEAREST:
			filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			break;
		case CC_LINEAR:
			filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case CC_NEAREST_MIPMAP_NEAREST:
			filter = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case CC_LINEAR_MIPMAP_NEAREST:
			filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case CC_NEAREST_MIPMAP_LINEAR:
			filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case CC_LINEAR_MIPMAP_LINEAR:
			filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			break;
		}
	}

	if ( texParams->magFilter == CC_LINEAR )
	{
		switch(texParams->minFilter)
		{
		case CC_NEAREST:
			filter = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case CC_LINEAR:
			filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			break;
		case CC_NEAREST_MIPMAP_NEAREST:
			filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case CC_LINEAR_MIPMAP_NEAREST:
			filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case CC_NEAREST_MIPMAP_LINEAR:
			filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case CC_LINEAR_MIPMAP_LINEAR:
			filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			break;
		}
	}
	
	if ( texParams->wrapS == CC_REPEAT )
	{
		u = D3D11_TEXTURE_ADDRESS_WRAP;
	}
	else if ( texParams->wrapS == CC_CLAMP_TO_EDGE )
	{
		u = D3D11_TEXTURE_ADDRESS_CLAMP;
	}
	
	if ( texParams->wrapT == CC_REPEAT )
	{
		v = D3D11_TEXTURE_ADDRESS_WRAP;
	}
	else if ( texParams->wrapT == CC_CLAMP_TO_EDGE )
	{
		v = D3D11_TEXTURE_ADDRESS_CLAMP;
	}

	if ( (filter==samplerDesc.Filter) && (u==samplerDesc.AddressU) && (v==samplerDesc.AddressV) )
	{
		bcreat = false;
	}
	else
	{
		if ( m_sampleState )
		{
			m_sampleState->Release();
			m_sampleState = 0;
		}
	}

	samplerDesc.Filter = (D3D11_FILTER)filter;
	samplerDesc.AddressU = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(u);
	samplerDesc.AddressV = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(v);
	samplerDesc.AddressW = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(u);

	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = samplerDesc.BorderColor[1] = samplerDesc.BorderColor[2] = samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	if ( bcreat )
	{
		CCID3D11Device->CreateSamplerState(&samplerDesc, &m_sampleState);
	}

}

ID3D11SamplerState** CCTexture2D::GetSamplerState()
{
	return &m_sampleState;
}

void CCTexture2D::setAliasTexParameters()
{
	ccTexParams texParams = { CC_NEAREST, CC_NEAREST, CC_CLAMP_TO_EDGE, CC_CLAMP_TO_EDGE };
	this->setTexParameters(&texParams);
}

void CCTexture2D::setAntiAliasTexParameters()
{
	ccTexParams texParams = { CC_LINEAR, CC_LINEAR, CC_CLAMP_TO_EDGE, CC_CLAMP_TO_EDGE };
	this->setTexParameters(&texParams);
}

//
// Texture options for images that contains alpha
//
// implementation CCTexture2D (PixelFormat)

void CCTexture2D::setDefaultAlphaPixelFormat(CCTexture2DPixelFormat format)
{
	g_defaultAlphaPixelFormat = format;
}


CCTexture2DPixelFormat CCTexture2D::defaultAlphaPixelFormat()
{
	return g_defaultAlphaPixelFormat;
}

unsigned int CCTexture2D::bitsPerPixelForFormat()
{
	unsigned int ret = 0;

	switch (m_ePixelFormat) 
	{
		case kCCTexture2DPixelFormat_RGBA8888:
			ret = 32;
			break;
		case kCCTexture2DPixelFormat_RGB565:
			ret = 16;
			break;
		case kCCTexture2DPixelFormat_A8:
			ret = 8;
			break;
		case kCCTexture2DPixelFormat_RGBA4444:
			ret = 16;
			break;
		case kCCTexture2DPixelFormat_RGB5A1:
			ret = 16;
			break;
		case kCCTexture2DPixelFormat_PVRTC4:
			ret = 4;
			break;
		case kCCTexture2DPixelFormat_PVRTC2:
			ret = 2;
			break;
		case kCCTexture2DPixelFormat_I8:
			ret = 8;
			break;
		case kCCTexture2DPixelFormat_AI88:
			ret = 16;
			break;
        case kCCTexture2DPixelFormat_RGB888:
            ret = 24;
            break;
		default:
			ret = -1;
			CCAssert(false, "illegal pixel format");
			CCLOG("bitsPerPixelForFormat: %d, cannot give useful result", m_ePixelFormat);
			break;
	}
	return ret;
}

NS_CC_END
