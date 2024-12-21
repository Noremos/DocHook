#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
//#include <dwmapi.h>


#include <windows.h>
#include <string>
#include <iostream>
#include <cassert>
#include <codecvt>

//#pragma comment(lib, "Dwmapi.lib")
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#elif __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <vector>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
// #include <X11/extensions/Xcomposite.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>
#include <X11/X.h>
#include <string>

#endif

#include <memory>


// #include <ScreenCaptureKit/ScreenCaptureKit.h>
// #include <CoreGraphics/CoreGraphics.h>
// #include <Foundation/Foundation.h>

bool strEquals(std::string winName, std::string supstr)
{
	for (size_t i = 0; i < winName.length(); i++)
	{
		winName[i] = toupper(winName[i]);
	}

	for (size_t i = 0; i < supstr.length(); i++)
	{
		supstr[i] = toupper(supstr[i]);
	}

	return winName.find(supstr) != std::string::npos;
}

// A placeholder structure to hold image data. Replace with your actual image data structure.
struct ImageData
{
	std::unique_ptr<char> data;
	int width = 0;
	int height = 0;
	int channels = 0;

	std::string name = "";
	uint32_t winId = 0;
};

// Platform-specific implementations
#ifdef _WIN32
ImageData CaptureWindowWin32(HWND hwnd) {
	// Your Windows-specific code to capture the window
	ImageData imgData = { nullptr, 0, 0 };
	// Implementation placeholder
	return imgData;
}
void CaptureWindowWin(ImageData& imgData, HWND hwnd);
std::string getWindowMeta(ImageData& data, HWND hwnd);
bool notFullWindow(HWND hwnd);

#elif __APPLE__


bool notFullWindow(CFDictionaryRef window)
{
	CFStringRef windowName = (CFStringRef)CFDictionaryGetValue(window, kCGWindowName);

	if (CFStringGetLength(windowName) == 0)
		return true;

	int layerId;
	CFNumberRef layerRef = (CFNumberRef)CFDictionaryGetValue(window, kCGWindowLayer);
	CFNumberGetValue(layerRef, kCFNumberIntType, &layerId);


	return (layerId != 0);
}

CGRect getWindowBounds(CFDictionaryRef window)
{
	CGRect windowBounds;

	CFTypeRef windowBoundsRef;

	if (CFDictionaryGetValueIfPresent(window, CFSTR("kCGWindowBounds"), &windowBoundsRef))
	{
		CFDictionaryRef windowBoundsDict = (CFDictionaryRef)windowBoundsRef;
		CFNumberRef xNumber = (CFNumberRef)CFDictionaryGetValue(windowBoundsDict, CFSTR("X"));
		CFNumberRef yNumber = (CFNumberRef)CFDictionaryGetValue(windowBoundsDict, CFSTR("Y"));
		CFNumberRef widthNumber = (CFNumberRef)CFDictionaryGetValue(windowBoundsDict, CFSTR("Width"));
		CFNumberRef heightNumber = (CFNumberRef)CFDictionaryGetValue(windowBoundsDict, CFSTR("Height"));


		CFNumberGetValue(xNumber, kCFNumberDoubleType, &(windowBounds.origin.x));
		CFNumberGetValue(yNumber, kCFNumberDoubleType, &(windowBounds.origin.y));
		CFNumberGetValue(widthNumber, kCFNumberDoubleType, &(windowBounds.size.width));
		CFNumberGetValue(heightNumber, kCFNumberDoubleType, &(windowBounds.size.height));
	}

	return windowBounds;
}

// Function to convert a CGImage to raw pixel data (RGBA)
unsigned char* ConvertCGImageToData(CGImageRef image, int& width, int& height)
{
	width = static_cast<int>(CGImageGetWidth(image));
	height = static_cast<int>(CGImageGetHeight(image));
	size_t bitsPerPixel = CGImageGetBitsPerPixel(image);
	size_t bytesPerRow = CGImageGetBytesPerRow(image);

	size_t dataSize = height * bytesPerRow;

	unsigned char* data = new unsigned char[dataSize];

	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	CGContextRef context = CGBitmapContextCreate(data, width, height, 8, bytesPerRow, colorSpace, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);

	CGContextDrawImage(context, CGRectMake(0, 0, width, height), image);

	CGContextRelease(context);
	CGColorSpaceRelease(colorSpace);

	return data;
}

CFStringRef getWindowMeta(ImageData& data, CFDictionaryRef window)
{
	CFStringRef windowName = (CFStringRef)CFDictionaryGetValue(window, kCGWindowOwnerName);

	// Name
	std::string name;
	name.resize(128);

	bool ret = CFStringGetCString(windowName, name.data(), name.size(), kCFStringEncodingUTF8);
	assert(ret);
	name.resize(strlen(name.data())); // Cringe
	data.name = name;

	// Window ID
	CGWindowID windowID = 0;
	CFNumberRef windowNumberRef = (CFNumberRef)CFDictionaryGetValue(window, kCGWindowNumber);
	if (!CFNumberGetValue(windowNumberRef, kCGWindowIDCFNumberType, &windowID))
	{
		std::cerr << "Failed to capture window image." << std::endl;
	}

	data.winId = windowID;
	return windowName;
}

void CaptureWindowMac(ImageData& imgData, CFDictionaryRef window)
{
	CGRect windowBounds = getWindowBounds(window);

	// Capture the window image
	// CGImageRef image = CGWindowListCreateImage(CGRectNull, kCGWindowListOptionIncludingWindow, windowID, kCGWindowImageBoundsIgnoreFraming);
	// CGImageRef image = CGWindowListCreateImage(CGRectNull, kCGWindowListOptionOnScreenOnly, windowID, kCGWindowImageDefault);
	CGImageRef image = CGWindowListCreateImage(windowBounds, kCGWindowListOptionIncludingWindow, imgData.winId, kCGWindowImageBoundsIgnoreFraming);
	if (image) {
		imgData.data.reset((char*)ConvertCGImageToData(image, imgData.width, imgData.height));

		int temp = CGImageGetBitsPerPixel(image);
		imgData.channels = temp / 8;

		temp = CGImageGetBytesPerRow(image);
		imgData.width = temp / imgData.channels;

		// Release the CGImageRef object
		CGImageRelease(image);
	} else {
		std::cerr << "Failed to capture window image." << std::endl;
	}
}
#elif __linux__
ImageData CaptureWindowLinux(Window window) {
	// Your Linux-specific code to capture the window using X11
	ImageData imgData = { nullptr, 0, 0 };
	// Implementation placeholder
	return imgData;
}
#endif



// Main cross-platform function
ImageData CaptureWindowByName(std::string_view searchingName)
{

#ifdef _WIN32

	struct Tempo
	{
		ImageData out;
		std::string winname;
		bool found = false;
	};

	Tempo temp;
	temp.winname = std::string(searchingName.data(), searchingName.length());

	EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL 
	{
		Tempo& temp = *reinterpret_cast<Tempo*>(lParam);
		ImageData& imgData = temp.out;
		if (temp.found)
			return TRUE;

		// Skip minimized or hidden windows
		if (notFullWindow(hwnd)) {
			return TRUE; // Continue enumeration
		}

		// Get window metadata
		getWindowMeta(imgData, hwnd);

		if (!strEquals(imgData.name, temp.winname))
		{
			return TRUE;
		}

		// Capture window image
		CaptureWindowWin(imgData, hwnd);
		temp.found = true;

		return TRUE; // Continue enumeration
	}, reinterpret_cast<LPARAM>(&temp));

	return std::move(temp.out);

#elif __APPLE__
	ImageData imageData;
	CGEventRef event = CGEventCreate(NULL);
	CGPoint cursorPos = CGEventGetLocation(event);
	CFRelease(event);

	CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly, kCGNullWindowID);
	for (CFIndex i = 0; i < CFArrayGetCount(windowList); ++i)
	{
		CFDictionaryRef window = (CFDictionaryRef)CFArrayGetValueAtIndex(windowList, i);

		if (notFullWindow(window))
			continue;

		CFStringRef windowName = getWindowMeta(imageData, window);

		CFStringRef searchingNameRef = CFStringCreateWithCString(kCFAllocatorDefault, searchingName.data(), kCFStringEncodingUTF8);
		if (CFStringCompare(windowName, searchingNameRef, kCFCompareCaseInsensitive) == kCFCompareEqualTo)
		{
			CaptureWindowMac(imageData, window);
			break;
		}

		CFRelease(searchingNameRef);
	}

	CFRelease(windowList);
	return imageData;
#elif __linux__
	{
		ImageData imageData;
		Display* display = XOpenDisplay(NULL);

		Window root = DefaultRootWindow(display);
		Window parent;
		Window* children;
		unsigned int num_children;

		// Get all child windows of the root window
		if (XQueryTree(display, root, &root, &parent, &children, &num_children) == 0)
		{
			return false;
		}

		std::string searchingStrName(searchingName.data(), searchingName.length());
		for (unsigned int i = 0; i < num_children; ++i)
		{
			Window window = children[i];

			// Skip if it's a full-screen window
			if (notFullWindow(display, window)) {
				continue;
			}

			// Check if the window title matches the search name (case insensitive)
			if (strEquals(windowName, searchingStrName))
			{
				// Capture the window
				CaptureWindow(display, window);
				XFree(children);
				break;
			}
		}

		XFree(children);

		return imageData;
	}

#endif

}

#ifdef __APPLE__

std::vector<ImageData> getWindowsPreview()
{
	std::vector<ImageData> out;
	ImageData imageData;

	CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly, kCGNullWindowID);
	for (CFIndex i = 0; i < CFArrayGetCount(windowList); ++i)
	{
		CFDictionaryRef window = (CFDictionaryRef)CFArrayGetValueAtIndex(windowList, i);
		CFTypeRef windowBoundsRef;
		if (notFullWindow(window))
			continue;



		ImageData& img = out.emplace_back();
		getWindowMeta(img, window);
		CaptureWindowMac(img, window);
	}
	CFRelease(windowList);

	return out;
}
#eif _WIN32

struct CGRect {
    struct {
        long x;
        long y;
    } origin;
    struct {
        long width;
        long height;
    } size;
};

// Function to get the window bounds in a format similar to CGRect
CGRect getWindowBounds(HWND hwnd) {
    CGRect windowBounds;
    RECT rect;

    // Get the window rectangle (bounding rectangle)
    if (GetWindowRect(hwnd, &rect)) {
        windowBounds.origin.x = rect.left;
        windowBounds.origin.y = rect.top;
        windowBounds.size.width = rect.right - rect.left;
        windowBounds.size.height = rect.bottom - rect.top;
    } else {
        // Handle error; you might want to initialize to zero or some default values
        windowBounds.origin.x = 0;
        windowBounds.origin.y = 0;
        windowBounds.size.width = 0;
        windowBounds.size.height = 0;
    }

    return windowBounds;
}

bool notFullWindow(HWND hwnd)
{
    // Get the window title
	int length = GetWindowTextLength(hwnd);

	// Get the window style
	LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);

	// Check if the window is minimized
	if (style & WS_MINIMIZE)
		return true;


	if (IsWindowVisible(hwnd) && length != 0)
	{
		//const DWORD TITLE_SIZE = 1024;
		//WCHAR windowTitle[TITLE_SIZE];
		//GetWindowTextW(hwnd, windowTitle, length + 1);
		//std::wstring title(&windowTitle[0]);
		//std::wstring str = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(title.data());


		//std::cout << hwnd << ":  " << str << std::endl;
		return false;
	}


    return true; // Indicates it's not a full window if it passes the above checks
}



// Function to get window metadata
std::string getWindowMeta(ImageData& data, HWND hwnd) {
    // Get the window title


	int length = GetWindowTextLength(hwnd);

	//std::wstring str = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(title.data());

	const DWORD TITLE_SIZE = 1024;
	WCHAR windowTitle[TITLE_SIZE];
	GetWindowTextW(hwnd, windowTitle, length + 1);
	std::wstring title(&windowTitle[0]);

	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conversion;
	std::u16string str16((char16_t*)title.data());


    // Name
	data.name = conversion.to_bytes(str16.data());
	if (data.name.length() > 21)
		data.name = data.name.substr(0, 8) + "..." + data.name.substr(data.name.length() - 10);

    // Window ID
    data.winId = GetWindowThreadProcessId(hwnd, nullptr);

    return data.name;
}
//
//ImageData takeFullScreen()
//{
//	ImageData imgData;
//
//	HDC hScreenDC = GetDC(NULL);
//	// and a device context to put it in
//	HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
//
//	int width = GetSystemMetrics(SM_CXSCREEN);
//	int height = GetSystemMetrics(SM_CYSCREEN);
//	imgData.width = width;
//	imgData.height = height;
//
//	// hBitmap is a HBITMAP that i declared globally to use within WM_PAINT
//	// maybe worth checking these are positive values
//	auto hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
//
//	// get a new bitmap
//	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);
//
//	BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);
//	hBitmap = (HBITMAP)SelectObject(hMemoryDC, hOldBitmap);
//
//	// clean up
//	DeleteDC(hMemoryDC);
//	ReleaseDC(NULL, hScreenDC);
//
//
//	// Calculate width and height
//
//
//	imgData.channels = hBitmap.bmBitsPixel / 8; // RGBA
//	imgData.data.reset(new char[bitmap.bmWidthBytes * bitmap.bmHeight]); // allocate enough for the data
//	GetBitmapBits(hBitmap, bitmap.bmWidthBytes * bitmap.bmHeight, imgData.data.get());
//}

// Function to capture the window image in Windows
void CaptureWindowWin(ImageData& imgData, HWND hwnd) {
    // Get window rectangle
    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);

    // Calculate width and height
    imgData.width = windowRect.right - windowRect.left;
    imgData.height = windowRect.bottom - windowRect.top;

    // Create a device context for the window
    HDC hdc = GetDC(hwnd);
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, imgData.width, imgData.height);

    SelectObject(hdcMem, hBitmap);

    // Render the window into the memory device context
    BitBlt(hdcMem, 0, 0, imgData.width, imgData.height, hdc, 0, 0, SRCCOPY);

    // Allocate and copy image data to imgData.data
    BITMAP bitmap;
    GetObject(hBitmap, sizeof(bitmap), &bitmap);

	BITMAPINFO bmpInfo;
	ZeroMemory(&bmpInfo, sizeof(bmpInfo));
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfo.bmiHeader.biWidth = bitmap.bmWidth;
	bmpInfo.bmiHeader.biHeight = -bitmap.bmHeight;
	bmpInfo.bmiHeader.biPlanes = bitmap.bmPlanes;
	bmpInfo.bmiHeader.biBitCount = bitmap.bmBitsPixel;
	bmpInfo.bmiHeader.biCompression = BI_RGB;  // Assuming uncompressed bitmap

	// This function retrieves the bitmap data and stores it in imgData.data.get()
	// Assuming a 32-bit bitmap (you can adjust based on your needs)
	imgData.channels = bitmap.bmBitsPixel / 8; // RGBA
	imgData.data.reset(new char[bitmap.bmWidthBytes * bitmap.bmHeight]); // allocate enough for the data
	imgData.width = bitmap.bmWidth;
	imgData.height = bitmap.bmHeight;
	int result = GetDIBits(hdcMem, hBitmap, 0, bitmap.bmHeight, imgData.data.get(), &bmpInfo, DIB_RGB_COLORS);

	char* raw = imgData.data.get();
	for (size_t i = 0; i < imgData.height * imgData.width * imgData.channels; i += imgData.channels)
	{
		// BGR TO RGB
		std::swap(raw[i], raw[i + 2]);
	}

	// Clean up the device context
	ReleaseDC(hwnd, hdc);

    // Cleanup
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
}

std::vector<ImageData> getWindowsPreview()
{
    std::vector<ImageData> out;

    // Enumerate all top-level windows
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        std::vector<ImageData>& images = *reinterpret_cast<std::vector<ImageData>*>(lParam);

        // Skip minimized or hidden windows
        if (notFullWindow(hwnd)) {
            return TRUE; // Continue enumeration
        }


        ImageData imgData;

        // Get window metadata
        getWindowMeta(imgData, hwnd);

		// Skip dublicates
		for (size_t i = 0; i < images.size(); i++)
		{
			if (images[i].winId == imgData.winId)
				return TRUE;
		}

        // Capture window image
        CaptureWindowWin(imgData, hwnd);


        images.push_back(std::move(imgData));

        return TRUE; // Continue enumeration
    }, reinterpret_cast<LPARAM>(&out));

    return out;
}

#else




bool notFullWindow(Window window, Display* display) {
    // Check if the window should be skipped (e.g., minimized or not visible)
    XWindowAttributes attr;
    XGetWindowAttributes(display, window, &attr);
    return (attr.map_state != IsViewable);
}

void getWindowMeta(ImageData& imgData, Window window, Display* display) {
    imgData.winId = window;
    XWindowAttributes attr;
    XGetWindowAttributes(display, window, &attr);
    imgData.width = attr.width;
    imgData.height = attr.height;
}

void CaptureWindowLinux(ImageData& imgData, Window window, Display* display)
{
	XMapRaised(display, window);
    XImage *image = XGetImage(display, window, 0, 0, imgData.width, imgData.height, AllPlanes, ZPixmap);
    imgData.data.reset(new char[4 * imgData.width * imgData.height]);
    unsigned long red_mask   = image->red_mask;
    unsigned long green_mask = image->green_mask;
    unsigned long blue_mask  = image->blue_mask;

	auto* pixelData = imgData.data.get();
    for (int y = 0; y < imgData.height; ++y)
	{
        for (int x = 0; x < imgData.width; ++x)
		{
            unsigned long pixel = XGetPixel(image, x, y);

            unsigned char blue  = (pixel & blue_mask);
            unsigned char green = (pixel & green_mask) >> 8;
            unsigned char red   = (pixel & red_mask) >> 16;

            pixelData[(y * imgData.width + x) * 4 + 0] = red;
            pixelData[(y * imgData.width + x) * 4 + 1] = green;
            pixelData[(y * imgData.width + x) * 4 + 2] = blue;
            pixelData[(y * imgData.width + x) * 4 + 3] = (char)255; // Alpha
        }
    }
    XDestroyImage(image);
}

std::vector<ImageData> getWindowsPreview()
{
    std::vector<ImageData> out;
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        return out; // Failed to open display
    }

    Window root = DefaultRootWindow(display);
    Window parent;
    Window *children;
    unsigned int nchildren;

    if (XQueryTree(display, root, &root, &parent, &children, &nchildren) == 0) {
        return out; // Failed to query window tree
    }

    for (unsigned int i = 0; i < nchildren; ++i) {
        Window window = children[i];

        if (!notFullWindow(window, display)) {
            ImageData imgData;

            getWindowMeta(imgData, window, display);

            // Skip duplicates
            bool duplicate = false;
            for (const auto& image : out) {
                if (image.winId == imgData.winId) {
                    duplicate = true;
                    break;
                }
            }

            if (duplicate) {
                continue;
            }

            CaptureWindowLinux(imgData, window, display);
            out.push_back(std::move(imgData));
        }
    }
    XFree(children);
    XCloseDisplay(display);

    return out;
}

#endif
