#define _CRT_SECURE_NO_WARNINGS
#include "plugin.h"
#include "SkiUI-Lite\SkiUI-Lite\skiui-lite.h"
#include <time.h>
int need_redraw = 0;

int menu_id = 0;
Window *win1 = NULL;
Image *image1;
Image *image2;
Image *image3;
Window *winpop = NULL;
Label *label1 = NULL;
Button *button1 = NULL;
std::vector<Image*> img_vec;
extern HBITMAP ScreenCapture(tstring filename, WORD BitCount, LPRECT lpRect);
typedef Window* (*DllLoadPlugin)(HWND win, int x, int y, int w, int d);
typedef const char* (*DllGetPluginName)();
using namespace std;

tstring mainDir = _T("none");
tstring pluginDir = _T("none");
int xx = 0;
const int record_sec = 40;
const int fps = 5;
double countDown = 3000;
int mutex = 0;
tstring curProject = _T("none");
int forceStop = 0;
tim::timer t;

void start_save()
{

	{//5, 10, 16, 24
		tstring s;
		char sFps[9];
		_itoa_s(int(1000 / fps), sFps, 10);
		tstring cmd = pluginDir + _T("conv.exe 2 ")
			+ pluginDir + _T("output\\") + curProject + _T("\\out.apng ")
			+ pluginDir + _T("output\\") + curProject + _T("\\*.png ") + _TS(sFps) + _T(" 1000");
		char buffer[1024];
		FILE* pipe = _popen(enc::t2a(cmd).c_str(), "r");
		if (!pipe)
		{
			printf("error");
			return;
		}
		char c = 0;
		while (EOF != (c = fgetc(pipe)))
		{
			printf("%c", c);
		}
		_pclose(pipe);
	}
	label1->setText(_T("save files....."));
	{
		tstring cmd = pluginDir + _T("conv.exe 1 ")
			+ pluginDir + _T("output\\") + curProject + _T("\\out.apng ")
			+ pluginDir + _T("output\\") + curProject + _T("\\out.gif");
		char buffer[1024];
		FILE* pipe = _popen(enc::t2a(cmd).c_str(), "r");
		if (!pipe)
		{
			printf("error");
			return;
		}
		char c = 0;
		while (EOF != (c = fgetc(pipe)))
		{
			printf("%c", c);
		}
		_pclose(pipe);
	}
	tstring showDir = _T("");
	showDir = showDir + _T("explorer ");
	showDir = showDir + pluginDir + _T("output\\") + curProject;
	system(enc::t2a(showDir).c_str());
}

VOID(NTAPI s) (PVOID, BOOLEAN)
{
	if (mutex)
		return;
	if (winpop == NULL)
		return;
	if (countDown > 0)
	{
		countDown -= 1000 / fps;
		char sCountDown[9];
		_itoa_s(int(countDown / 1000), sCountDown, 10);
		tstring tss(label1->getText());
		tstring sss(_TS(sCountDown));
		if (tss != sss && tss != _TS("recording"))
		{
			label1->setText(sss);
			if (int(countDown / 1000) == 0)
				label1->setText(_T("recording"));
		}

		return;
	}
	if (xx++ >= fps * record_sec)
		return;
	char ss[256] = { 0 };
	tstring outDir = pluginDir + _T("output\\") + curProject + _T("\\%d.png");
	sprintf_s(ss, enc::t2a(outDir).c_str(), xx);

	RECT rc;
	rc = winpop->getRect();
	rc.left += 10;
	rc.top += 10;
	rc.bottom -= 10 * 3;
	rc.right -= 10 * 3;
	ScreenCapture(_TS(ss), 32, &rc);
	if (xx != fps * record_sec)
		return;
	mutex = 1;
	t.deleteTimer();
	label1->setText(_T("encoding....."));
	start_save();
	xx = 0;

	countDown = 3000;
	label1->setText(_T("countDown"));
	button1->setWindowText(_T("start"));
	mutex = 0;
}


LONG_PTR popMouseLeftDown2(LONG_PTR x, LONG_PTR y, LONG_PTR state)
{
	if (x > winpop->getSize().cx*0.9 && y > winpop->getSize().cy*0.9)
	{
		if (xx == 0)
		{
			winpop->sendMsg(WM_SYSCOMMAND, FADF_RESERVED, 0);
		}
	}
	else
	{
		winpop->sendMsg(WM_NCLBUTTONDOWN, 2, 0);
	}

	return 1;
}

LONG_PTR popSizeChanged(LONG_PTR x, LONG_PTR y)
{
	HWND hwnd = winpop->_hwnd;
	RECT wndRect;
	::GetWindowRect(hwnd, &wndRect);
	SIZE wndSize = { wndRect.right - wndRect.left,wndRect.bottom - wndRect.top };
	HDC hdc = ::GetDC(hwnd);
	HDC memDC = ::CreateCompatibleDC(hdc);
	HBITMAP memBitmap = ::CreateCompatibleBitmap(hdc, wndSize.cx, wndSize.cy);
	::SelectObject(memDC, memBitmap);
	Gdiplus::Graphics graphics(memDC);
	//graphics.DrawImage(&image, 0, 0, wndSize.cx, wndSize.cy);
	HDC screenDC = GetDC(NULL);
	POINT ptSrc = { 0,0 };
	using namespace Gdiplus;
	Gdiplus::Pen pen(Gdiplus::Color(0xff, 0x2f, 0x3f, 0x5f), 10);
	Gdiplus::Rect rc;
	rc.X = 5;
	rc.Y = 5;
	rc.Width = winpop->getSize().cx - 10;
	rc.Height = winpop->getSize().cy - 10;
	graphics.DrawRectangle(&pen, rc);
	BLENDFUNCTION blendFunction;
	blendFunction.AlphaFormat = AC_SRC_ALPHA;
	blendFunction.BlendFlags = 0;
	blendFunction.BlendOp = AC_SRC_OVER;
	blendFunction.SourceConstantAlpha = 255;
	UpdateLayeredWindow(hwnd, screenDC, NULL, &wndSize, memDC, &ptSrc, 0, &blendFunction, 2);
	::DeleteDC(memDC);
	::DeleteObject(memBitmap);
	return 0;
}

LONG_PTR popMouseMove(LONG_PTR x, LONG_PTR y, LONG_PTR state)
{
	return 0;
}

void stop_record()
{
	if (mutex)
		return;
	if (label1->getText() == _T("recording"))
	{
		mutex = 1;
		t.deleteTimer();
		label1->setText(_T("encoding....."));
		start_save();
		xx = 0;
		countDown = 3000;
		label1->setText(_T("countDown"));
		button1->setWindowText(_T("start"));
		//tstring cmd = pluginDir + _T("output\\") + curProject + _T("\\");
		mutex = 0;
	}
}

void start_record()
{

	if (button1->getWindowText() == _T("stop"))
		return stop_record();
	time_t tick = time(NULL);
	struct tm* currentTime = localtime(&tick);
	char date[256];
	sprintf_s(date, "%02d%02d%02d%02d", currentTime->tm_mday, currentTime->tm_hour, currentTime->tm_min, currentTime->tm_sec);
	curProject = _TS(date);
	t.deleteTimer();
	label1->setText(_T("encoding....."));
	xx = 0;

	countDown = 3000;
	label1->setText(_T("countDown"));
	tstring showDir = pluginDir + _T("output\\") + curProject;
	bool flag = CreateDirectory(_TS(showDir).c_str(), NULL);

	t.CreateTimer(s, 1000 / fps);
	button1->setWindowText(_T("stop"));
}

int xxxxxxx(HWND parentHwnd, int x, int y, int w, int d)
{
	win1 = Window::create(NULL, _TS("Window1"), 5, 5, 100, 100);
	Edit *edit1 = Edit::create(win1, _TS("插件的控件:edit1"), 000, 60, 200);
	label1 = Label::create(win1, _TS("countDown"), 20, 30);
	button1 = Button::create(win1, _TS("start"), 20, 90);
	button1->_evClick = start_record;
#ifdef xbx
	{
		HDC hDesktopDC = ::GetDC(NULL);
		HDC hTmpDC = CreateCompatibleDC(hDesktopDC);
		HBITMAP hBmp = CreateCompatibleBitmap(hDesktopDC, 351, 250);    //351x250, 示例数据  
		SelectObject(hTmpDC, hBmp);
		BitBlt(hTmpDC, 0, 0, 351, 250, hDesktopDC, 0, 0, SRCCOPY);
		DeleteObject(hTmpDC);

		BITMAP bm;
		PBITMAPINFO bmpInf;
		if (GetObject(hBmp, sizeof(bm), &bm) == 0)
		{
			::ReleaseDC(NULL, hDesktopDC);
			return 1;
		}

		int nPaletteSize = 0;
		if (bm.bmBitsPixel<16)
			nPaletteSize = (int)pow(2, bm.bmBitsPixel);

		bmpInf = (PBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER) +
			sizeof(RGBQUAD)*nPaletteSize + (bm.bmWidth + 7) / 8 * bm.bmHeight*bm.bmBitsPixel);

		BYTE* buf = ((BYTE*)bmpInf) +
			sizeof(BITMAPINFOHEADER) +
			sizeof(RGBQUAD)*nPaletteSize;

		bmpInf->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmpInf->bmiHeader.biWidth = bm.bmWidth;
		bmpInf->bmiHeader.biHeight = bm.bmHeight;
		bmpInf->bmiHeader.biPlanes = bm.bmPlanes;
		bmpInf->bmiHeader.biBitCount = bm.bmBitsPixel;
		bmpInf->bmiHeader.biCompression = BI_RGB;
		bmpInf->bmiHeader.biSizeImage = (bm.bmWidth + 7) / 8 * bm.bmHeight*bm.bmBitsPixel;

		if (!::GetDIBits(hDesktopDC, hBmp, 0, (UINT)bm.bmHeight, buf, bmpInf, DIB_RGB_COLORS))
		{
			ReleaseDC(NULL, hDesktopDC);
			LocalFree(bmpInf);
			return 1;
		}

	}
	//ScreenCapture("C:\\Users\\ftp\\Desktop\\4.BMP", 32, NULL);
	if (0)
	{
		CGifEncoder gifEncoder;
		gifEncoder.SetFrameSize(3840, 2160);
		gifEncoder.SetDelayTime(500);
		wstring s = wstring(L"C:\\Users\\ftp\\Desktop\\11.gif");
		gifEncoder.StartEncoder(s);
		s = wstring(L"C:\\Users\\ftp\\Desktop\\1.bmp");
		gifEncoder.AddFrame(s);
		s = wstring(L"C:\\Users\\ftp\\Desktop\\2.bmp");
		gifEncoder.AddFrame(s);
		s = wstring(L"C:\\Users\\ftp\\Desktop\\3.bmp");
		gifEncoder.AddFrame(s);
		s = wstring(L"C:\\Users\\ftp\\Desktop\\4.bmp");
		gifEncoder.AddFrame(s);
		gifEncoder.FinishEncoder();
	}
#endif
	image1->_parent;
	{
		winpop = Window::create(NULL, _TS("winpop"), 300, 300, 405, 405);
		winpop->removeStyle(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_SYSMENU);
		winpop->addExStyle(WS_EX_LAYERED);
		winpop->_evMouseLeftDown = popMouseLeftDown2;
		winpop->_evSizeChanged = popSizeChanged;
		winpop->_evMouseMove = popMouseMove;
		winpop->removeExStyle(WS_EX_APPWINDOW);
		HWND hwnd = winpop->_hwnd;


		RECT wndRect;
		::GetWindowRect(hwnd, &wndRect);
		SIZE wndSize = { wndRect.right - wndRect.left,wndRect.bottom - wndRect.top };
		HDC hdc = ::GetDC(hwnd);
		HDC memDC = ::CreateCompatibleDC(hdc);
		HBITMAP memBitmap = ::CreateCompatibleBitmap(hdc, wndSize.cx, wndSize.cy);
		::SelectObject(memDC, memBitmap);

		//Gdiplus::Image image(L"C:\\Users\\ftp\\Desktop\\Untitled-1.png");
		Gdiplus::Graphics graphics(memDC);
		//graphics.DrawImage(&image, 0, 0, wndSize.cx, wndSize.cy);
		HDC screenDC = GetDC(NULL);
		POINT ptSrc = { 0,0 };
		using namespace Gdiplus;
		Gdiplus::Pen pen(Gdiplus::Color(0xff, 0x2f, 0x3f, 0x5f), 10);
		Gdiplus::Rect rc;
		rc.X = 5;
		rc.Y = 5;
		rc.Width = winpop->getSize().cx - 10;
		rc.Height = winpop->getSize().cy - 10;
		graphics.DrawRectangle(&pen, rc);
		BLENDFUNCTION blendFunction;
		blendFunction.AlphaFormat = AC_SRC_ALPHA;
		blendFunction.BlendFlags = 0;
		blendFunction.BlendOp = AC_SRC_OVER;
		blendFunction.SourceConstantAlpha = 255;
		UpdateLayeredWindow(hwnd, screenDC, NULL, &wndSize, memDC, &ptSrc, 0, &blendFunction, 2);

		::DeleteDC(memDC);
		::DeleteObject(memBitmap);
		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	return 0;
}


BOOL GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT num = 0;
	UINT size = 0;
	using namespace Gdiplus;
	GetImageEncodersSize(&num, &size);

	if (size == 0)
		return FALSE;

	ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)new BYTE[size];
	if (pImageCodecInfo == NULL)
		return FALSE;

	GetImageEncoders(num, size, pImageCodecInfo);
	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			delete pImageCodecInfo;
			return TRUE;
		}
	}

	delete pImageCodecInfo;
	return FALSE;
}


HBITMAP ScreenCapture(tstring filename, WORD BitCount, LPRECT lpRect)
{
	HBITMAP hBitmap;
	HDC hScreenDC = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
	HDC hmemDC = CreateCompatibleDC(hScreenDC);
	int ScreenWidth = GetDeviceCaps(hScreenDC, HORZRES);
	int ScreenHeight = GetDeviceCaps(hScreenDC, VERTRES);
	// 旧的BITMAP，用于与所需截取的位置交换  
	HBITMAP hOldBM;
	// 保存位图数据  
	PVOID lpvpxldata;
	// 截屏获取的长宽及起点  
	INT ixStart;
	INT iyStart;
	INT iX;
	INT iY;
	// 位图数据大小  
	DWORD dwBitmapArraySize;
	// 几个大小  
	DWORD nBitsOffset;
	DWORD lImageSize;
	DWORD lFileSize;
	// 位图信息头  
	BITMAPINFO bmInfo;
	// 位图文件头  
	BITMAPFILEHEADER bmFileHeader;
	// 写文件用  
	HANDLE hbmfile;
	DWORD dwWritten;

	// 如果LPRECT 为NULL 截取整个屏幕  
	if (lpRect == NULL)
	{
		ixStart = iyStart = 0;
		iX = ScreenWidth;
		iY = ScreenHeight;
	}
	else
	{
		ixStart = lpRect->left;
		iyStart = lpRect->top;
		iX = lpRect->right - lpRect->left;
		iY = lpRect->bottom - lpRect->top;
	}
	hBitmap = CreateCompatibleBitmap(hScreenDC, iX, iY);
	hOldBM = (HBITMAP)SelectObject(hmemDC, hBitmap);
	// BitBlt屏幕DC到内存DC，根据所需截取的获取设置参数  
	BitBlt(hmemDC, 0, 0, iX, iY, hScreenDC, ixStart, iyStart, SRCCOPY | CAPTUREBLT);
	// 将旧的BITMAP对象选择回内存DC，返回值为被替换的对象，既所截取的位图  
	hBitmap = (HBITMAP)SelectObject(hmemDC, hOldBM);

	{
		int quality = 1;
		using namespace Gdiplus;
		EncoderParameters encoderParameters;
		encoderParameters.Count = 1;
		encoderParameters.Parameter[0].Guid = EncoderQuality;
		encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
		encoderParameters.Parameter[0].NumberOfValues = 1;
		encoderParameters.Parameter[0].Value = &quality;
		CLSID pngClsid;
		int nResult = GetEncoderClsid(L"image/png", &pngClsid);
		Gdiplus::Bitmap bmp(hBitmap, NULL);

		Status status = bmp.Save(enc::t2w(filename).c_str(), &pngClsid, &encoderParameters);
		bool rt = status != Ok;

	}

	DeleteDC(hScreenDC);
	DeleteDC(hmemDC);
	return hBitmap;

	if (filename.length() == 0)
	{
		DeleteDC(hScreenDC);
		DeleteDC(hmemDC);
		return hBitmap;
	}
	// 为位图数据申请内存空间  
	dwBitmapArraySize = ((((iX * 32) + 31) & ~31) >> 3)* iY;
	lpvpxldata = HeapAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE, dwBitmapArraySize);
	ZeroMemory(lpvpxldata, dwBitmapArraySize);

	// 添充 BITMAPINFO 结构  
	ZeroMemory(&bmInfo, sizeof(BITMAPINFO));
	bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmInfo.bmiHeader.biWidth = iX;
	bmInfo.bmiHeader.biHeight = iY;
	bmInfo.bmiHeader.biPlanes = 1;
	bmInfo.bmiHeader.biBitCount = BitCount;
	bmInfo.bmiHeader.biCompression = BI_RGB;

	// 添充 BITMAPFILEHEADER 结构  
	ZeroMemory(&bmFileHeader, sizeof(BITMAPFILEHEADER));
	nBitsOffset = sizeof(BITMAPFILEHEADER) + bmInfo.bmiHeader.biSize;
	lImageSize =
		((((bmInfo.bmiHeader.biWidth * bmInfo.bmiHeader.biBitCount) + 31) & ~31) >> 3)
		* bmInfo.bmiHeader.biHeight;
	lFileSize = nBitsOffset + lImageSize;
	bmFileHeader.bfType = 'B' + ('M' << 8);
	bmFileHeader.bfSize = lFileSize;
	bmFileHeader.bfOffBits = nBitsOffset;


	GetDIBits(hmemDC, hBitmap, 0, bmInfo.bmiHeader.biHeight,
		lpvpxldata, &bmInfo, DIB_RGB_COLORS);

	hbmfile = CreateFile(filename.c_str(),
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hbmfile == INVALID_HANDLE_VALUE)
	{

	}

	WriteFile(hbmfile, &bmFileHeader, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
	WriteFile(hbmfile, &bmInfo, sizeof(BITMAPINFO), &dwWritten, NULL);
	WriteFile(hbmfile, lpvpxldata, lImageSize, &dwWritten, NULL);
	CloseHandle(hbmfile);



	// OPS! DEL BITMAP 
	HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, lpvpxldata);
	ReleaseDC(0, hScreenDC);
	DeleteDC(hmemDC);
	return hBitmap;
}

Window* loadPlugin(HWND parentHwnd, int x, int y, int w, int d)
{
	global::init(NULL,NULL);
	tchar buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	pluginDir = buffer;
	int sz = pluginDir.rfind(_T('\\'), pluginDir.length());

	tstring sxx(_TS(pluginDir));
	sxx.erase(sz + 1);
	mainDir = sxx;
	pluginDir = mainDir + _T("plugin\\record_plugin\\");
	global::init(NULL, NULL);
	xxxxxxx(parentHwnd, x, y, w, d);
    win1->removeStyle(WS_CAPTION | WS_SYSMENU | WS_SIZEBOX);
    win1->addStyle(WS_CHILD);
    win1->addExStyle(WS_EX_MDICHILD);
    SetParent(win1->_hwnd, parentHwnd);
    win1->move(5, 50);
    win1->size(w, d);
	return win1;
}
const char* getPluginName()
{
	return "\nAPNG录制 Version:Alpha --- 已加载\n";
}
