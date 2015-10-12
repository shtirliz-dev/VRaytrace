#include "RayTracer.h"

float mix(float a, float b, float mix)
{
	return b * mix + a * (1 - mix);
}

vec3d RayTracer::TraceRay(const vec3d &rayorig, const vec3d &raydir, const int &depth)
{
	float tnear = INFINITY;
	const Sphere *sphere = NULL;

	for (int i = 0; i < mSpheres.size(); ++i)
	{
		float t0 = INFINITY, t1 = INFINITY;

		if (mSpheres[i]->intersect(rayorig, raydir, &t0, &t1)) 
		{
			if (t0 < 0) 
				t0 = t1;

			if (t0 < tnear) 
			{
				tnear = t0;
				sphere = mSpheres[i];
			}
		}
	}

	if (!sphere) 
		return vec3d(1.0f);

	vec3d surfaceColor = 0; 
	vec3d phit = rayorig + raydir * tnear;
	vec3d nhit = phit - sphere->center; 

	nhit.Normalize();

	float bias = 1e-4; 
	bool inside = false;

	if (raydir.dot(nhit) > 0) 
	{
		nhit = -nhit;
		inside = true;
	}

	if ((sphere->transparency > 0 || sphere->reflection > 0) && depth < mMaxRayDepth) 
	{
		float facingratio = -raydir.dot(nhit);		
		float fresneleffect = mix(pow(1.0f - facingratio, 3.0f), 1.0f, 0.1); 

		vec3d refldir = raydir - nhit * 2.0f * raydir.dot(nhit);
		refldir.Normalize();
		vec3d reflection = TraceRay(phit + nhit * bias, refldir, depth + 1.0f);
		vec3d refraction = 0.0f;

		if (sphere->transparency) 
		{
			float ior = 1.1, eta = (inside) ? ior : 1 / ior; 
			float cosi = -nhit.dot(raydir);
			float k = 1 - eta * eta * (1 - cosi * cosi);
			vec3d refrdir = raydir * eta + nhit * (eta *  cosi - sqrt(k));
			refrdir.Normalize();
			refraction = TraceRay(phit - nhit * bias, refrdir, depth + 1);
		}

		surfaceColor = (reflection * fresneleffect + refraction * (1 - fresneleffect) * sphere->transparency) * sphere->surfaceColor;
	}
	else 
	{
		for (int i = 0; i < mSpheres.size(); ++i)
		{
			if (mSpheres[i]->emissionColor.x > 0) 
			{
				vec3d transmission = 1;
				vec3d lightDirection = mSpheres[i]->center - phit;
				lightDirection.Normalize();

				for (int j = 0; j < mSpheres.size(); ++j) 
				{
					if (i != j) 
					{
						float t0, t1;

						if (mSpheres[j]->intersect(phit + nhit * bias, lightDirection, &t0, &t1)) 
						{
							transmission = 0;
							break;
						}
					}
				}

				surfaceColor += sphere->surfaceColor * transmission * max(0, nhit.dot(lightDirection)) * mSpheres[i]->emissionColor;
			}
		}
	}

	return surfaceColor + sphere->emissionColor;
}

void RayTracer::Start()
{
	mActive = true;

	for(int i = 0; i < 4; i++)
	{
		RenderThreadParams* params = new RenderThreadParams;
		params->mOffset = i;
		params->mRayTracer = this;
		HANDLE thread = CreateThread(0, 0, DrawBuffer, params, 0, 0);
		mRenderThreads.push_back(thread);
		mRenderThreadFlags.push_back(true);
	}

	mScreenBlitThread = CreateThread(0, 0, DrawScreen, this, 0, 0);
}

RayTracer::RayTracer(HWND pWindow, int pWidth, int pHeight, int pMaxRayDepth)
{
	mWindow = pWindow;
	mHdc = GetDC(mWindow);
	mWidth = pWidth;
	mHeight = pHeight;
	mInvWidth = 1.0f / mWidth;
	mInvHeight = 1.0f / mHeight;
	mFov = 45.0f;
	mAspectratio = (float)mWidth / (float)mHeight;
	mAngle = tan(M_PI * 0.5f * mFov / 180.0f);
	mCamPos = vec3d(0.0f, 0.0f, 16.0f);
	mCamDir = vec3d(0.0f, 0.0f, -1.0f);
	mMaxRayDepth = pMaxRayDepth;
	mBuffer = new unsigned char[mWidth * mHeight * 3];
	mCamRotY = 0.0f;
	mCamRotX = 0.0f;
}

RayTracer::~RayTracer()
{	
	mActive = false;	
	while(mBuffer)
	{
		bool count = 0;
		for(int i = 0; i < mRenderThreadFlags.size(); i++)
			if(mRenderThreadFlags[i] != false)
				count++;
		
		if(!count)
		{
			delete [] mBuffer;
			mBuffer = 0;
		}
	}
}

void RayTracer::Render(int pOffset)
{
	float xx = 0, yy = 0;
	unsigned char* pix = 0;
	vec3d pixel, raydir;
	
	mCamDir.Set((2.0f * ((mWidth / 2.0f + 0.5) * mInvWidth) - 1) * mAngle * mAspectratio, 0.0f, -1.0f);
	mCamDir.RotateX(mCamRotX);
	mCamDir.RotateY(mCamRotY);
	mCamDir.Normalize();

	for (int y = pOffset; y < mHeight; y += 4) 
	{
		yy = (1 - 2 * ((y + 0.5) * mInvHeight)) * mAngle;
		for (int x = 0; x < mWidth; x++) 
		{
			xx = (2 * ((x + 0.5) * mInvWidth) - 1) * mAngle * mAspectratio;					
			vec3d raydir(xx, yy, -1);
			raydir.RotateX(mCamRotX);
			raydir.RotateY(mCamRotY);
			raydir.Normalize();
			pixel = TraceRay(mCamPos, raydir, 0);
			pix = &mBuffer[mWidth * 3 * y + x * 3];
			*pix = (unsigned char)(min(1.0f, pixel.z * 2.0f) * 255);
			*(pix + 1) = (unsigned char)(min(1.0f, pixel.y * 2.0f) * 255);
			*(pix + 2) = (unsigned char)(min(1.0f, pixel.x * 2.0f) * 255);
		}
	}
}

bool RayTracer::IsActive() 
{ 
	return mActive; 
}

int RayTracer::GetBufferWidth() 
{ 
	return mWidth; 
}

int RayTracer::GetBufferHeight() 
{ 
	return mHeight; 
}

void RayTracer::RotateCamera(float pAX, float pAY)
{
	mCamRotX += pAX;
	mCamRotY += pAY;
}

vec3d RayTracer::GetCameraDir()
{
	return mCamDir;
}

vec3d RayTracer::GetCameraPos()
{
	return mCamPos;
}

void RayTracer::MoveCamera(vec3d& pOffset)
{
	mCamPos += pOffset;
}

void RayTracer::AddSphere(Sphere* pSphere)
{
	if(pSphere)
		mSpheres.push_back(pSphere);
}

DWORD WINAPI DrawBuffer(LPVOID params)
{  	
	RenderThreadParams* mParams = (RenderThreadParams*)params;

	int offset = mParams->mOffset;
	RayTracer* tracer = mParams->mRayTracer;

	while(tracer->mActive)
	{
		tracer->Render(offset);
		Sleep(4);
	}
	
	tracer->mRenderThreadFlags[offset] = false;
	delete (RenderThreadParams*)params;
	return 0;
}

DWORD WINAPI DrawScreen(LPVOID params)
{
	RayTracer* tracer = (RayTracer*)params;

	HDC hdcMem;
	hdcMem = CreateCompatibleDC(NULL);
	BITMAPINFO inf;
	memset(&inf, 0, sizeof(BITMAPINFO));

	inf.bmiHeader.biBitCount = 24;
	inf.bmiHeader.biHeight = tracer->mHeight;
	inf.bmiHeader.biWidth = tracer->mWidth;
	inf.bmiHeader.biSizeImage = tracer->mWidth * tracer->mHeight * 3;
	inf.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	inf.bmiHeader.biPlanes = 1;
	inf.bmiHeader.biCompression = BI_RGB;
	
	UINT * pPixels = 0;
	HBITMAP m_HBitmap = CreateDIBSection(hdcMem, (BITMAPINFO *)&
		inf, DIB_RGB_COLORS, (void **)& pPixels , NULL, 0); 

	while(tracer->mActive)
	{
		SetBitmapBits(m_HBitmap, tracer->mWidth * tracer->mHeight * 3, tracer->mBuffer);
		SelectObject(hdcMem, m_HBitmap);
		
		BitBlt(tracer->mHdc, 0, 0, tracer->mWidth, tracer->mHeight, hdcMem, 0, 0, SRCCOPY);
		InvalidateRect(tracer->mWindow, 0, 0);

		Sleep(16);
	}

	DeleteObject(m_HBitmap);
	DeleteDC(hdcMem);
	return 0;
}