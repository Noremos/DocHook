#include <iostream>
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

// A placeholder structure to hold image data. Replace with your actual image data structure.
struct ImageData {
    std::unique_ptr<char> data;
    int width = 0;
    int height = 0;
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

ImageData CaptureWindowMac(CGWindowID windowID)
{
    ImageData imgData = { nullptr, 0, 0 };

    // Capture the window image
    CGImageRef image = CGWindowListCreateImage(CGRectNull, kCGWindowListOptionIncludingWindow, windowID, kCGWindowImageBoundsIgnoreFraming);
    if (image) {
        imgData.data.reset((char*)ConvertCGImageToData(image, imgData.width, imgData.height));

        // Release the CGImageRef object
        CGImageRelease(image);
    } else {
        std::cerr << "Failed to capture window image." << std::endl;
    }

    return imgData;
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
ImageData CaptureWindowHoveredByCursor() {
    ImVec2 mousePos = ImGui::GetMousePos();
    ImageData imageData = { nullptr, 0, 0 };

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


    for (auto it = windowList; it != nullptr; it++)
	{
        CGRect windowBounds;
        CFTypeRef windowBoundsRef;
        CFDictionaryGetValueIfPresent(it, CFSTR("kCGWindowBounds"), &windowBoundsRef);
        CGRect windowBounds;
        if (windowBoundsRef && CFGetTypeID(windowBoundsRef) == CFDataGetTypeID()) {
            CFDataGetValue((CFDataRef)windowBoundsRef, CFRangeMake(0, sizeof(CGRect)), &windowBounds);
        }
        if (CGRectContainsPoint(windowBounds, cursorPos)) {
            CGWindowID windowID = [window[(NSString *)kCGWindowNumber] unsignedIntValue];
            imageData = CaptureWindowMac(windowID);
            break;
        }
    }
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
