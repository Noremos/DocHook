#include <iostream>
#include <string>
#ifdef _WIN32
#include <Windows.h>
#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#elif __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif
#include "imgui.h"

// #include <ScreenCaptureKit/ScreenCaptureKit.h>
// #include <CoreGraphics/CoreGraphics.h>
// #include <Foundation/Foundation.h>


// A placeholder structure to hold image data. Replace with your actual image data structure.
struct ImageData {
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
	ImageData imageData;

#ifdef _WIN32
	POINT pt;
	pt.x = static_cast<LONG>(mousePos.x);
	pt.y = static_cast<LONG>(mousePos.y);
	HWND hwnd = WindowFromPoint(pt);
	if (hwnd != NULL) {
		imageData = CaptureWindowWin32(hwnd);
	}
#elif __APPLE__
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
#elif __linux__
	Display *display = XOpenDisplay(NULL);
	if (display) {
		Window root = DefaultRootWindow(display);
		Window retRoot, retChild;
		int rootX, rootY, winX, winY;
		unsigned int mask;

		if (XQueryPointer(display, root, &retRoot, &retChild, &rootX, &rootY, &winX, &winY, &mask)) {
			if (retChild != None) {
				imageData = CaptureWindowLinux(retChild);
			}
		}
		XCloseDisplay(display);
	}
#endif

	return imageData;
}


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
