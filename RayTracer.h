#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <vector>
#include <cassert>

#define M_PI 3.141592653589793
#define INFINITY 1e8

using namespace std;

class vec3d
{
public:
	float x, y, z;
	vec3d() : x(float(0)), y(float(0)), z(float(0)) {}
	vec3d(float xx) : x(xx), y(xx), z(xx) {}
	void Set(float xx, float yy, float zz)
	{ 
		x = xx; 
		y = yy; 
		z = zz; 
	} 
	vec3d(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {}
	vec3d& Normalize()
	{
		float nor2 = length2();

		if (nor2 > 0) 
		{
			float invNor = 1 / sqrt(nor2);
			x *= invNor, y *= invNor, z *= invNor;
		}

		return *this;
	}
	vec3d operator * (const float &f) const { return vec3d(x * f, y * f, z * f); }
	vec3d operator * (const vec3d &v) const { return vec3d(x * v.x, y * v.y, z * v.z); }
	float dot(const vec3d &v) const { return x * v.x + y * v.y + z * v.z; }
	static vec3d cross(vec3d& p, vec3d& q) { return vec3d((p.y * q.z) - (p.z * q.y),
        (p.z * q.x) - (p.x * q.z),
        (p.x * q.y) - (p.y * q.x)); } 
	vec3d operator - (const vec3d &v) const { return vec3d(x - v.x, y - v.y, z - v.z); }
	vec3d operator + (const vec3d &v) const { return vec3d(x + v.x, y + v.y, z + v.z); }
	vec3d& operator += (const vec3d &v) { x += v.x, y += v.y, z += v.z; return *this; }
	vec3d& operator *= (const vec3d &v) { x *= v.x, y *= v.y, z *= v.z; return *this; }
	vec3d operator - () const { return vec3d(-x, -y, -z); }
	float length2() const { return x * x + y * y + z * z; }
	float length() const { return sqrt(length2()); }
	void RotateY(float angle) 
	{
		//z' = z*cos q - x*sin q
		//x' = z*sin q + x*cos q
		//y' = y

		float ang = angle * (M_PI / 180.0);
		float oldz = z;
		float sina = sinf(ang);
		float cosa = cosf(ang);
		z = z * cosa - x * sina;
		x = oldz * sina + x * cosa;
	}
	void RotateX(float angle) 
	{
		//y' = y*cos q - z*sin q
		//z' = y*sin q + z*cos q
		//x' = x

		float ang = angle * (M_PI / 180.0);
		float oldy = y;
		float sina = sinf(ang);
		float cosa = cosf(ang);
		y = y * cosa - z * sina;
		z = oldy * sina + z * cosa;
	}
};

class Sphere
{
public:
	vec3d center;      
	float radius, radius2;       
	vec3d surfaceColor, emissionColor;   
	float transparency, reflection;  
	
	Sphere(const vec3d &c, const float &r, const vec3d &sc, const float &refl = 0, const float &transp = 0, const vec3d &ec = 0) : 
		center(c), radius(r), radius2(r * r), surfaceColor(sc), emissionColor(ec), transparency(transp), reflection(refl)
	{ }
	
	bool intersect(const vec3d &rayorig, const vec3d &raydir, float *t0 = NULL, float *t1 = NULL) const
	{
		vec3d l = center - rayorig;
		float tca = l.dot(raydir);
		
		if (tca < 0.0f)
			return false;
		
		float d2 = l.dot(l) - tca * tca;
		
		if (d2 > radius2) 
			return false;
		
		if (t0 != NULL && t1 != NULL) 
		{
			float thc = sqrt(radius2 - d2);

			*t0 = tca - thc;
			*t1 = tca + thc;
		}

		return true;
	}
};

class RayTracer;

struct RenderThreadParams
{
	int mOffset;
	RayTracer* mRayTracer;
};

class RayTracer
{
private:
	HDC mHdc;
	HWND mWindow;
	unsigned char* mBuffer;
	int mWidth;
	int mHeight;
	bool mActive;
	float mInvWidth;
	float mInvHeight;
	float mFov;
	vec3d mCamDir;
	vec3d mCamPos;
	float mAspectratio;
	float mAngle;	
	float mCamRotX;
	float mCamRotY;
	vector<HANDLE> mRenderThreads;
	vector<bool> mRenderThreadFlags;
	HANDLE mScreenBlitThread;
	vector<Sphere*> mSpheres;
	
public:	
	int mMaxRayDepth;

	RayTracer(HWND pWindow, int pWidth, int pHeight, int pMaxRayDepth);
	~RayTracer();

	void Start();	
	void Render(int pOffset);
	void MoveCamera(vec3d& pOffset);
	void AddSphere(Sphere* pSphere);
	void RotateCamera(float pAX, float pAY);
	int GetBufferWidth();
	int GetBufferHeight();
	bool IsActive();	
	vec3d GetCameraDir();
	vec3d GetCameraPos();
	vec3d TraceRay(const vec3d &rayorig, const vec3d &raydir, const int &depth);
	
	friend DWORD WINAPI DrawBuffer(LPVOID params);
	friend DWORD WINAPI DrawScreen(LPVOID params);
};