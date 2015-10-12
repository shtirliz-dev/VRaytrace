#include <windows.h>
#include <Windowsx.h>
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
#include "RayTracer.h"

#define CM_SET 1001
#define CM_SET2 1003
#define CM_EXIT 1002
#define DLG_BUTTON_OK 1099 
#define DLG_BUTTON_CANCEL 1098 
#define DLG_EDIT_DEPTH 1097 
#define DLG_EDIT_INTENSITY 1096 
#define DLG_CHECK_BOX 999

using namespace std;

LONG WinProc(HWND, UINT,WPARAM,LPARAM);
LRESULT CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);
void CreateDialogBox(HWND hwnd); 

static HWND gParametersDialog, gParamDialogBtnOK, gParamDialogBtnCancel, gParamDialogEditDepth, gParamDialogEditIntensity, gParamDialogCheckBox; 
static HINSTANCE gInstance;
HWND gMainHWND;

struct Keys 
{									
	BOOL keyDown [256];	
};	

Keys gKEY;

static bool dragActive = false;
static bool moveLight = false;

wstring ToString(double value)
{
	wstringstream ss;
	ss << std::fixed << std::setprecision(3) << value;
	return ss.str().c_str();
}

double ToDouble(const wstring& str)
{
	double val = 0.0;
	wstringstream ss;
	ss << str;
	ss >> val;
	return val;
}

wstring ToString(int value)
{
	wstringstream ss;
	ss << std::fixed << std::setprecision(3) << value;
	return ss.str().c_str();
}

int ToInt(const wstring& value)
{
	wstringstream ss;
	ss << value;
	int a = 0;
	ss >> a;
	return a;
}

RayTracer* Tracer = 0;
Sphere* Light = 0;

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{ 
	wchar_t Title[] = L"RayTracer";
	wchar_t ProgName[] = L"OkOk";

	MSG msg; 
	WNDCLASS w;

	w.lpszClassName = ProgName; 
	w.hInstance = hInstance; 
	w.lpfnWndProc = (WNDPROC)MainWndProc;
	w.hCursor = LoadCursor(NULL, IDC_ARROW); 
	w.hIcon = LoadIcon(NULL, IDI_APPLICATION); 
	w.lpszMenuName = NULL; 
	w.hbrBackground = (HBRUSH)(COLOR_WINDOWFRAME);
	w.style = CS_HREDRAW | CS_VREDRAW; 
	w.cbClsExtra = 0; 
	w.cbWndExtra = 0; 

	if(!RegisterClass(&w)) 
		return false;
	
	gMainHWND = CreateWindow(ProgName, Title, WS_CAPTION | WS_SYSMENU, 500, 300, 800, 600, NULL, NULL, hInstance, NULL);
	ShowWindow(gMainHWND, nCmdShow);
	
	ZeroMemory (&gKEY.keyDown, sizeof (Keys));		
	
	bool isMessagePumpActive = TRUE;							
	while (isMessagePumpActive == TRUE)						
	{
		if (PeekMessage (&msg, 0, 0, 0, PM_REMOVE) != 0)
		{
			if(msg.message == WM_CLOSE || msg.message == WM_QUIT || msg.message == WM_DESTROY)
			{
				isMessagePumpActive = FALSE;
				if(Tracer)
					delete Tracer;
				break;	
			}
						
			TranslateMessage(&msg);
			DispatchMessage (&msg);						
		}
		else								
		{
			static DWORD lastTickCount = GetTickCount();
			DWORD tickCount = GetTickCount();
			float dt = (tickCount - lastTickCount) / 1000.0f;
			float SPEED = 35.0f;

			if(gKEY.keyDown['Q'] == true) Tracer->MoveCamera(vec3d(0, SPEED * dt, 0));
			if(gKEY.keyDown['E'] == true) Tracer->MoveCamera(vec3d(0, -SPEED * dt, 0));
			if(gKEY.keyDown['W'] == true) Tracer->MoveCamera(Tracer->GetCameraDir() * SPEED * dt);
			if(gKEY.keyDown['S'] == true) Tracer->MoveCamera(Tracer->GetCameraDir() * -SPEED * dt);
			if(gKEY.keyDown['A'] == true) Tracer->MoveCamera(vec3d::cross(Tracer->GetCameraDir(), vec3d(0, 1, 0)) * -SPEED * dt);
			if(gKEY.keyDown['D'] == true) Tracer->MoveCamera(vec3d::cross(Tracer->GetCameraDir(), vec3d(0, 1, 0)) * SPEED * dt);
			if(moveLight == true) Light->center = Tracer->GetCameraPos() -  Tracer->GetCameraDir() * (Light->radius + 1);

			lastTickCount = tickCount;
		}
	}
	
	return 0;
}

void Mouse_Move(HWND hwnd)
{	
	if(dragActive && Tracer)
	{
		POINT mousePos;	
		int mid_x = Tracer->GetBufferWidth()  >> 1;	
		int mid_y = Tracer->GetBufferHeight() >> 1;	
		float angle_y  = 0.0f;				
		float angle_z  = 0.0f;							
		GetCursorPos(&mousePos);
		ScreenToClient(hwnd, &mousePos);
		if( (mousePos.x == mid_x) && (mousePos.y == mid_y) ) return;
		POINT pt; pt.x = mid_x; pt.y = mid_y;
		ClientToScreen(hwnd, &pt);
		SetCursorPos(pt.x, pt.y);					
		angle_y = (float)( (mid_x - mousePos.x) ) / 100;		
		angle_z = (float)( (mid_y - mousePos.y) ) / 100;
		Tracer->RotateCamera(angle_z * 15, angle_y * 15);
	}
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HMENU menu, popup_menu;
	HMENU hMenu;
    PAINTSTRUCT ps;
	RECT rc;
	GetClientRect(hWnd , &rc);			
    switch(msg) 
	{
	case WM_CREATE:
		menu=CreateMenu();
		popup_menu=CreatePopupMenu(); 
		AppendMenu(popup_menu, MF_STRING, CM_SET, TEXT("Настройки")); 
		AppendMenu(popup_menu, MF_SEPARATOR, 0, TEXT("")); 
		AppendMenu(popup_menu, MF_STRING, CM_EXIT, TEXT("Выход")); 
		InsertMenu(menu, -2, MF_STRING | MF_POPUP | MF_BYPOSITION, (UINT)popup_menu, TEXT("Сцена"));
		SetMenu(hWnd,menu);				
		Tracer = new RayTracer(hWnd, rc.right - rc.left, rc.bottom - rc.top, 3);				
		Tracer->AddSphere(new Sphere(vec3d(0, -10004, -20), 10000, vec3d(0.7), 1, 0.0));
		Tracer->AddSphere(new Sphere(vec3d(0, 16, 0), 2, vec3d(1.00, 0, 0), 1, 0.5));
		Light = new Sphere(vec3d(0, 20, 30), 2, vec3d(1,1,1), 0, 0, vec3d(3));
		Tracer->AddSphere(Light);
		for(int i = 0; i <= 360; i += 36)
		{
			Tracer->AddSphere(new Sphere(vec3d(cosf(i) * 10, 0, sinf(i) * 10), 2, vec3d(0.32, 1.00, 0.36), 1, 0.5));
			Tracer->AddSphere(new Sphere(vec3d(cosf(i) * 8, 4, sinf(i) * 8), 1.7, vec3d(0.32, 1.00, 0.36), 1, 0.5));
			Tracer->AddSphere(new Sphere(vec3d(cosf(i) * 6, 8, sinf(i) * 6), 1.4, vec3d(0.32, 1.00, 0.36), 1, 0.5));
			Tracer->AddSphere(new Sphere(vec3d(cosf(i) * 4, 12, sinf(i) * 4), 1.1, vec3d(0.32, 1.00, 0.36), 1, 0.5));
		}
		Tracer->Start();	
		break;	

	case WM_DESTROY: 					
		PostQuitMessage(0);
		break;

	case WM_LBUTTONDOWN:
		dragActive = true;
		ShowCursor(false);
		break;

	case WM_KEYDOWN:
		if(wParam==VK_ESCAPE)
		{
			dragActive = false;
			ShowCursor(true);
		}
		if ((wParam >= 0) && (wParam <= 255))				
		{
			gKEY.keyDown [wParam] = TRUE;				
			return 0;									
		}
		break;															

	case WM_KEYUP:												
		if ((wParam >= 0) && (wParam <= 255))
		{
			gKEY.keyDown [wParam] = FALSE;				
			return 0;	
		}
		break;
		
	case WM_MOUSEMOVE:
		Mouse_Move(hWnd);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case CM_SET: case CM_SET2: 
			CreateDialogBox(gMainHWND);
			break;
		case CM_EXIT:
			exit(-1);
			break;
		}
		break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
        break;
    }
    return NULL;
}

void RegisterDialogClass(HWND hwnd)  
{ 
	WNDCLASSEX wc = {0}; 
	wc.cbSize           = sizeof(WNDCLASSEX); 
	wc.lpfnWndProc      = (WNDPROC) DialogProc; 
	wc.hInstance        = gInstance; 
	wc.hbrBackground    = GetSysColorBrush(COLOR_3DFACE); 
	wc.lpszClassName    = TEXT("Parameters"); 
	RegisterClassEx(&wc); 
} 

void CreateDialogBox(HWND hwnd) 
{ 
	RegisterDialogClass(hwnd);
	RECT rect; 
	GetWindowRect(hwnd, &rect); 
	gParametersDialog = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,  TEXT("Parameters"), 
		TEXT("Параметры"), WS_VISIBLE | WS_SYSMENU | WS_CAPTION | WS_DLGFRAME, rect.left + ((rect.right - rect.left) - 
		180) / 2, rect.top + ((rect.bottom - rect.top) - 150) / 2, 190, 185, hwnd, NULL, gInstance,  NULL);
	CreateWindow(TEXT("static"), TEXT("Глубина отражений:"), WS_VISIBLE | WS_CHILD, 5, 5, 180, 25, gParametersDialog, (HMENU) 2000, NULL, NULL);
	CreateWindow(TEXT("static"), TEXT("Интенсивность света:"), WS_VISIBLE | WS_CHILD, 5, 55, 180, 25, gParametersDialog, (HMENU) 2000, NULL, NULL);
	gParamDialogEditDepth = CreateWindow(TEXT("edit"), ToString(Tracer->mMaxRayDepth).c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | WS_BORDER,	5, 25, 175, 25, gParametersDialog, (HMENU) DLG_EDIT_DEPTH, NULL, NULL);    
	gParamDialogEditIntensity = CreateWindow(TEXT("edit"), ToString(Light->emissionColor.x).c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | WS_BORDER, 5, 75, 175, 25, gParametersDialog, (HMENU) DLG_EDIT_INTENSITY, NULL, NULL);   
	gParamDialogBtnOK = CreateWindowEx(WS_EX_WINDOWEDGE, TEXT("button"), TEXT("Применить"), WS_VISIBLE | WS_CHILD, 5, 130, 95, 25, gParametersDialog, (HMENU) DLG_BUTTON_OK, NULL, NULL);   
	gParamDialogBtnCancel = CreateWindow(TEXT("button"), TEXT("ОК"), WS_VISIBLE | WS_CHILD, 105, 130, 75, 25, gParametersDialog, (HMENU) DLG_BUTTON_CANCEL, NULL, NULL);  
	gParamDialogCheckBox = CreateWindow(TEXT("button"), TEXT("Свет из камеры"), WS_VISIBLE | WS_CHILD|BS_AUTOCHECKBOX, 5, 105, 175, 25, gParametersDialog, (HMENU) DLG_CHECK_BOX, NULL, NULL);
	Button_SetCheck(gParamDialogCheckBox, moveLight); 
	EnableWindow(hwnd, false);
} 

LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{ 
	switch(msg)   
	{ 
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{
		case DLG_BUTTON_OK: 
			{
				wchar_t depth[16], intensity[16];
				GetWindowText(gParamDialogEditDepth, depth, 16);
				GetWindowText(gParamDialogEditIntensity, intensity, 16);
				Tracer->mMaxRayDepth = ToInt(depth);
				Light->emissionColor = vec3d(ToDouble(intensity));
			}
			break; 
		case DLG_CHECK_BOX: 
			{
				LRESULT res = SendMessage (gParamDialogCheckBox, BM_GETCHECK, 0, 0);
				if(res == BST_CHECKED)
					moveLight = true;
				if(res == BST_UNCHECKED)
					moveLight = false;
			}
			break; 
		case DLG_BUTTON_CANCEL: 
			SendMessage(gParametersDialog, WM_CLOSE, 0, 0);
			break; 
		}
		break; 
	case WM_CLOSE: 
		DestroyWindow(hwnd); 
		EnableWindow(gMainHWND, true);
		SetFocus(gMainHWND);
		break;        
	} 
	return (DefWindowProc(hwnd, msg, wParam, lParam)); 
} 