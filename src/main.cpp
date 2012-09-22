
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

extern "C" int lines(unsigned char *in, int w, int h, void (*update)(void *));

HDC hdc, hdcwnd;
void *scan0;

void update(void *data)
{
	BOOL br;

	memcpy(scan0, data, 320*240);

	br = BitBlt(hdcwnd, 0, 0, 320, 240, hdc, 0, 0, SRCCOPY);
	assert(br);
}

int main(int argc, const char *argv[])
{
	FILE *file;
	unsigned char *in;
	int count;
	RECT rect;
	BOOL br;
	WNDCLASS wc;
	ATOM atom;
	HWND hwnd;
	BITMAPINFO *bi;
	int i;
	HBITMAP bitmap, dib;
	PAINTSTRUCT ps;
	MSG msg;
	
	in = (unsigned char *)malloc(320*240);
	assert(in);

	file = fopen(argv[1], "rb");
	assert(file);

	count = fseek(file, 54, SEEK_SET);

	for(i = 0; i < 320*240; i++)
	{
		unsigned char rgb[3];

		count = fread(rgb, 1, 3, file);

		in[i] = (unsigned char)(((int)rgb[0] + (int)rgb[1] + (int)rgb[2]) / 3);
	}

	fclose(file);

	ZeroMemory(&wc, sizeof(wc));

    wc.style          = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc    = DefWindowProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = 0;
	wc.hInstance      = GetModuleHandle(NULL);
    wc.lpszClassName  = "lines";

	atom = RegisterClass(&wc);
	assert(atom);

	rect.left = 0;
	rect.top = 0;
	rect.right = 320;
	rect.bottom = 240;

	br = AdjustWindowRect(&rect, WS_BORDER | WS_CAPTION, FALSE);
	assert(br);

	hwnd = CreateWindow("lines", "lines", WS_BORDER | WS_CAPTION | WS_VISIBLE, 50, 50, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, NULL, NULL);
	assert(hwnd);

	bi = (BITMAPINFO *)malloc(sizeof(BITMAPINFOHEADER) + 256*sizeof(RGBQUAD));
	assert(bi);

	bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi->bmiHeader.biWidth = 320;
	bi->bmiHeader.biHeight = 240;
	bi->bmiHeader.biPlanes = 1;
	bi->bmiHeader.biBitCount = 8;
	bi->bmiHeader.biCompression = BI_RGB;
	bi->bmiHeader.biSizeImage = 0;
	bi->bmiHeader.biXPelsPerMeter = 0;
	bi->bmiHeader.biYPelsPerMeter = 0;
	bi->bmiHeader.biClrUsed = 256;
	bi->bmiHeader.biClrImportant = 256;

	for(i = 0; i < 256; i++)
	{
		bi->bmiColors[i].rgbBlue = i;
		bi->bmiColors[i].rgbGreen = i;
		bi->bmiColors[i].rgbRed = i;
		bi->bmiColors[i].rgbReserved = 0;
	}
	
	hdc = CreateCompatibleDC(NULL);
	assert(hdc);

	bitmap = CreateDIBitmap(hdc, &bi->bmiHeader, CBM_INIT, NULL, bi, DIB_RGB_COLORS);
	assert(bitmap);

	dib = CreateDIBSection(hdc, bi, DIB_RGB_COLORS,&scan0, NULL, 0);
	assert(dib);

	hdcwnd = BeginPaint(hwnd, &ps);
	assert(hdcwnd);

	SelectObject(hdc, dib);

	lines((unsigned char *)in, 320, 240, update);

	br = EndPaint(hwnd, &ps);
	assert(br);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	br = DeleteObject(dib);
	assert(br);

	br = DeleteObject(bitmap);
	assert(br);

	br = DeleteDC(hdc);
	assert(br);
	
	br = DestroyWindow(hwnd);
	assert(br);

	free(bi);

	free(in);

	return 0;
}
