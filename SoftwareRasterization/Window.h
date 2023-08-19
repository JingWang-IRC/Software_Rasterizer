#pragma once


//#include <iostream>
//#include <vector>
//#include <algorithm>
//#include <math.h>
//#include <string>
//#include <queue>
//#include <stack>
//#include <set>
//#include <map>
//#include <unordered_map>
//#include <unordered_set>
//#include <functional>
//#include <atomic>
//#include <mutex>
//#include <memory>



#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>
#include <math.h>
#include <iostream>
#include <chrono>
#include <ctime>
#include <mutex>
#include <tchar.h>
#include <assert.h>

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;

int WINDOWWIDTH = 1080, WINDOWHEIGHT = 720;

BITMAPINFO bitmapInfo;
std::mutex pixelLock;



typedef unsigned int IUINT32;


typedef struct { float m[4][4]; } matrix_t;
typedef struct { float x, y, z, w; } vector_t;
typedef vector_t point_t;



typedef struct {
    int width;                  // 窗口宽度
    int height;                 // 窗口高度
    IUINT32** framebuffer;      // 像素缓存：framebuffer[y] 代表第 y行
    float** zbuffer;            // 深度缓存：zbuffer[y] 为第 y行指针
    IUINT32** texture;          // 纹理：同样是每行索引
    int tex_width;              // 纹理宽度
    int tex_height;             // 纹理高度
    float max_u;                // 纹理最大宽度：tex_width - 1
    float max_v;                // 纹理最大高度：tex_height - 1
    int render_state;           // 渲染状态
    IUINT32 background;         // 背景颜色
    IUINT32 foreground;         // 线框颜色
}	device_t;

#define RENDER_STATE_WIREFRAME      1		// 渲染线框
#define RENDER_STATE_TEXTURE        2		// 渲染纹理
#define RENDER_STATE_COLOR          4		// 渲染颜色

// 设备初始化，fb为外部帧缓存，非 NULL 将引用外部帧缓存（每行 4字节对齐）
void deviceInit(device_t* device, int width, int height, void* fb) {
    int need = sizeof(void*) * (height * 2 + 1024) + width * height * 8;
    char* ptr = (char*)malloc(need + 64);
    char* framebuf, * zbuf;
    int j;
    assert(ptr);
    device->framebuffer = (IUINT32**)ptr;
    device->zbuffer = (float**)(ptr + sizeof(void*) * height);
    ptr += sizeof(void*) * height * 2;
    device->texture = (IUINT32**)ptr;
    ptr += sizeof(void*) * 1024;
    framebuf = (char*)ptr;
    zbuf = (char*)ptr + width * height * 4;
    ptr += width * height * 8;
    if (fb != NULL) framebuf = (char*)fb;
    for (j = 0; j < height; j++) {
        device->framebuffer[j] = (IUINT32*)(framebuf + width * 4 * j);
        device->zbuffer[j] = (float*)(zbuf + width * 4 * j);
    }
    device->texture[0] = (IUINT32*)ptr;
    device->texture[1] = (IUINT32*)(ptr + 16);
    memset(device->texture[0], 0, 64);
    device->tex_width = 2;
    device->tex_height = 2;
    device->max_u = 1.0f;
    device->max_v = 1.0f;
    device->width = width;
    device->height = height;
    device->background = 0xc0c0c0;
    device->foreground = 0;
    device->render_state = RENDER_STATE_WIREFRAME;
}


//=====================================================================
// Win32 窗口及图形绘制：为 device 提供一个 DibSection 的 FB
//=====================================================================
int screen_w, screen_h, screen_exit = 0;
int screen_mx = 0, screen_my = 0, screen_mb = 0;
int screen_keys[512];	// 当前键盘按下状态
static HWND screen_handle = NULL;		// 主窗口 HWND
static HDC screen_dc = NULL;			// 配套的 HDC
static HBITMAP screen_hb = NULL;		// DIB
static HBITMAP screen_ob = NULL;		// 老的 BITMAP
unsigned char* screen_fb = NULL;		// frame buffer
long screen_pitch = 0;

// declare screen related functions
static LRESULT screenEvents(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void screenDispatch(void);
void screenUpdate(void);
int screenClose(void);

// initialise window
int screenInit(int w, int h, const TCHAR* title)
{
    WNDCLASS wc = { CS_BYTEALIGNCLIENT, (WNDPROC)screenEvents, 0, 0, 0,
        NULL, NULL, NULL, NULL, _T("SCREEN3.1415926") };
    BITMAPINFO bi = { { sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB,
        w * h * 4, 0, 0, 0, 0 } };
    RECT rect = { 0, 0, w, h };
    int wx, wy, sx, sy;
    LPVOID ptr;
    HDC hDC;

    screenClose();

    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    if (!RegisterClass(&wc)) return -1;

    screen_handle = CreateWindow(_T("SCREEN3.1415926"), title,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        0, 0, 0, 0, NULL, NULL, wc.hInstance, NULL);
    if (screen_handle == NULL) return -2;

    screen_exit = 0;
    hDC = GetDC(screen_handle);
    screen_dc = CreateCompatibleDC(hDC);
    ReleaseDC(screen_handle, hDC);

    screen_hb = CreateDIBSection(screen_dc, &bi, DIB_RGB_COLORS, &ptr, 0, 0);
    if (screen_hb == NULL) return -3;

    screen_ob = (HBITMAP)SelectObject(screen_dc, screen_hb);
    screen_fb = (unsigned char*)ptr;
    screen_w = w;
    screen_h = h;
    screen_pitch = w * 4;

    AdjustWindowRect(&rect, GetWindowLong(screen_handle, GWL_STYLE), 0);
    wx = rect.right - rect.left;
    wy = rect.bottom - rect.top;
    sx = (GetSystemMetrics(SM_CXSCREEN) - wx) / 2;
    sy = (GetSystemMetrics(SM_CYSCREEN) - wy) / 2;
    if (sy < 0) sy = 0;
    SetWindowPos(screen_handle, NULL, sx, sy, wx, wy, (SWP_NOCOPYBITS | SWP_NOZORDER | SWP_SHOWWINDOW));
    SetForegroundWindow(screen_handle);

    ShowWindow(screen_handle, SW_NORMAL);
    screenDispatch();

    memset(screen_keys, 0, sizeof(int) * 512);
    memset(screen_fb, 0, w * h * 4);

    return 0;
}

// win32 event handler
static LRESULT screenEvents(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CLOSE: screen_exit = 1; break;
    case WM_KEYDOWN: screen_keys[wParam & 511] = 1; break;
    case WM_KEYUP: screen_keys[wParam & 511] = 0; break;
    default: return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// get message from message queue and send to windows procedure
void screenDispatch(void)
{
    MSG msg;
    while (1) {
        if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) break;
        if (!GetMessage(&msg, NULL, 0, 0)) break;
        DispatchMessage(&msg);
    }
}

// update the framebuffer of the window by copying bytes
void screenUpdate(void)
{
    HDC hDC = GetDC(screen_handle);
    BitBlt(hDC, 0, 0, screen_w, screen_h, screen_dc, 0, 0, SRCCOPY);
    ReleaseDC(screen_handle, hDC);
    screenDispatch();
}

int screenClose(void)
{
    if (screen_dc) {
        if (screen_ob) {
            SelectObject(screen_dc, screen_ob);
            screen_ob = NULL;
        }
        DeleteDC(screen_dc);
        screen_dc = NULL;
    }
    if (screen_hb) {
        DeleteObject(screen_hb);
        screen_hb = NULL;
    }
    if (screen_handle) {
        CloseWindow(screen_handle);
        screen_handle = NULL;
    }
    return 0;
}


// clear framebuffer and zbuffer before drawing
void clearBuffers(device_t* device, int mode)
{
    int y, x, height = device->height;
    for (y = 0; y < device->height; y++) {
        IUINT32* dst = device->framebuffer[y];
        IUINT32 cc = (height - 1 - y) * 230 / (height - 1);
        //cc = (cc << 16) | (cc << 8) | cc;
        cc = (0 << 16) | (0 << 8) | int(255);
        //cc = (255 << 24) | (0 << 16) | (255 << 8) | (255);
        if (mode == 0) cc = device->background;
        for (x = 0; x < device->width - 100; dst++, x++) dst[0] = cc;
    }
    for (y = 0; y < device->height; y++) {
        float* dst = device->zbuffer[y];
        for (x = device->width; x > 0; dst++, x--) dst[0] = 0.0f;
    }
}

// print text on window (only print fps at this moment)
void printText(device_t* device, float fps)
{
    char str[30];
    sprintf_s(str, 20, "  FPS:  %4.2f  ", fps);
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0); // get the required size of the new string
    wchar_t* wstr = new wchar_t[size_needed]; // allocate a new wide string
    MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, size_needed); // convert the char* string to wide string
    TextOut(screen_dc, 0, 0, wstr, size_needed);
    delete[] wstr;
}


// present frame buffer
void presentBuffer(device_t* device, const std::vector<glm::vec3>& frameBuffer)
{
    int y, x, height = device->height;
    auto it = frameBuffer.begin();
    for (y = 0; y < device->height; y++) 
    {
        IUINT32* dst = device->framebuffer[y];
        IUINT32 cc = (height - 1 - y) * 230 / (height - 1);
        cc = (0 << 16) | (0 << 8) | int(255);
        for (x = 0; x < device->width; dst++, x++, it++)
        {
            cc = ((int)(*it).x << 16) | ((int)(*it).y << 8) | ((int)(*it).z);
            dst[0] = cc;
        }
    }
}



