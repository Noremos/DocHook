#pragma once

#include <filesystem>
#include <string>
#include <string_view>
//#include <random>
#include <fstream>
#include <cmath>

using BackStringView = std::string_view;
using BackString = std::string;
using BackDirStr = std::filesystem::path;
using BackPathStr = std::filesystem::path;
using BackFileReader = std::ifstream;
using BackFileWriter = std::ofstream;

using buint = unsigned int;
using buchar = unsigned char;
using bushort = unsigned short;

inline bool StrEquals(const BackString& str, BackStringView view)
{
	return str == view;
}

inline void WriteFile(const BackPathStr& fileName, const BackString& content)
{
	BackFileWriter w;
	w.open(fileName, std::ios::out | std::ios::trunc);
	w.write(content.c_str(), content.length());
	w.close();
}

struct BackSize
{
	int wid, hei;
	BackSize(int _wid, int _hei)
	{
		wid = _wid;
		hei = _hei;
	}
};


struct BackColor
{
	buchar r = 0;
	buchar g = 0;
	buchar b = 0;
	BackColor operator=(BackColor other)
	{
		r = other.r;
		g = other.g;
		b = other.b;
		return *this;
	}

	static BackColor random()
	{
		BackColor rc;
		rc.r = rand() % 255;
		rc.g = rand() % 255;
		rc.b = rand() % 255;
		return rc;
	}
};

template<typename T>
struct TPoint
{
	T x, y;

	TPoint(T _x = 0, T _y = 0) : x(_x), y(_y) {}

	bool operator==(const TPoint& other) const
	{
		return x == other.x && y == other.y;
	}

	// Point addition operator
	TPoint operator+(const TPoint& other) const {
		return TPoint(x + other.x, y + other.y);
	}

	// TPoint subtraction operator
	TPoint operator-(const TPoint& other) const {
		return TPoint(x - other.x, y - other.y);
	}

	TPoint operator*(const TPoint& other) const {
		return TPoint(x * other.x, y * other.y);
	}

	TPoint operator/(const TPoint& other) const {
		return TPoint(x / other.x, y / other.y);
	}


	TPoint operator*(double scalar) const {
		return TPoint(static_cast<T>(x * scalar), static_cast<T>(y * scalar));
	}

	TPoint operator/(double scalar) const {
		return TPoint(static_cast<T>(x / scalar), static_cast<T>(y / scalar));
	}

	TPoint operator*(float scalar) const {
		return TPoint(static_cast<T>(x * scalar), static_cast<T>(y * scalar));
	}

	TPoint operator/(float scalar) const {
		return TPoint(static_cast<T>(x / scalar), static_cast<T>(y / scalar));
	}

	TPoint operator*(int scalar) const {
		return TPoint(static_cast<T>(x * scalar), static_cast<T>(y * scalar));
	}

	TPoint operator/(int scalar) const {
		return TPoint(static_cast<T>(x / scalar), static_cast<T>(y / scalar));
	}

	TPoint operator-(T scalar) const {
		return TPoint(static_cast<T>(x - scalar), static_cast<T>(y - scalar));
	}

	//// TPoint dot product operator
	//int operator*(const TPoint& other) const {
	//	return x * other.x + y * other.y;
	//}

	//// TPoint cross product operator
	//int operator^(const TPoint& other) const {
	//	return x * other.y - y * other.x;
	//}

	TPoint abs() const
	{
		return TPoint(std::abs(x), std::abs(y));
	}

	// TPoint magnitude
	double mag() const {
		return sqrt(x * x + y * y);
	}

	// Normalize the TPoint to unit magnitude
	TPoint norm() const {
		double m = mag();
		return TPoint((int)(x / m), (int)(y / m));
	}

	// Distance between two TPoints
	double dist(const TPoint& other) const {
		int dx = x - other.x;
		int dy = y - other.y;
		return std::sqrt(dx * dx + dy * dy);
	}
};

using BackPixelPoint = TPoint<int>;
using BackPoint = TPoint<double>;

class Variables
{
public:
	static BackPathStr rootPath;
	static BackPathStr metaPath;
	static BackString prodDbPath;
	static void setRoot(const char* arg)
	{
		rootPath = arg;
		rootPath = rootPath.parent_path();
		metaPath = rootPath / "meta";
		prodDbPath = (metaPath / "proj").string();
	}

	static BackDirStr getFontsDir()
	{
		return rootPath / "fonts";
	}

	static BackPathStr getDefaultFontPath()
	{
		return getFontsDir() / "Arial.ttf";
	}
};


struct toStdStr
{
	using STRTYPE = std::string;

	template<class T>
	static STRTYPE toStr(T val)
	{
		return std::to_string(val);
	}
};


struct TileIterator
{
	buint start;
	buint tileSize;
	buint fullTileSize;

	buint maxLen;
	buint locIndex;

	TileIterator(buint start, buint tileSize, buint offset, buint maxLen) :
		start(start), tileSize(tileSize), fullTileSize(tileSize + offset), maxLen(maxLen),
		locIndex(0)
	{ }

	void reset(buint st = 0)
	{
		start = st;
		locIndex = start / tileSize;
	}

	buint pos()
	{
		return start;
	}

	buint accum()
	{
		++locIndex;
		start += tileSize;
		return start;
	}

	buint tilesInLine() const
	{
		return maxLen / tileSize + (maxLen % tileSize > (fullTileSize - tileSize) ? 1 : 0);
	}


	buint getFullTileSize()
	{
		if (start + fullTileSize <= maxLen)
		{
			return fullTileSize;
		}
		else if (maxLen > start)
		{
			return maxLen - start;
		}
		else
			return 0;
	}

	bool shouldSkip(buint& len)
	{
		len = getFullTileSize();
		return len <= (fullTileSize - tileSize); // len < offset
	}

	bool notEnded()
	{
		return start < maxLen;
	}
};

inline int strToInt(const BackString& string, bool& ready)
{
	char* endptr;
	int numI = std::strtol(string.c_str(), &endptr, 10);
	ready = *endptr == '\0';

	return numI;
}
inline int strToInt(const BackString& string)
{
	char* endptr;
	return std::strtol(string.c_str(), &endptr, 10);
}

template<class T>
inline BackString intToStr(T value)
{
	return std::to_string(value);
}

inline bool pathExists(const BackPathStr& filePath)
{
	return std::filesystem::exists(filePath);
}

template<class StrT>
inline bool mkdir(const StrT& path)
{
	BackDirStr filePath(path);
	return std::filesystem::create_directory(filePath);
}


inline void mkDirIfNotExists(const BackDirStr& dirPath)
{
	if (!pathExists(dirPath))
	{
		mkdir(dirPath);
	}
}

inline void dropDir(const BackDirStr& dirPath)
{
	std::filesystem::remove_all(dirPath);
}

inline void dropDirIfExists(const BackDirStr& dirPath)
{
	if (pathExists(dirPath))
	{
		dropDir(dirPath);
	}
}


inline bool endsWith(const BackString& string, const BackString endl)
{
	signed long long pos = string.length() - endl.length();
	if (pos <= 0)
		return false;

	for (size_t i = 0; i < endl.length(); i++)
	{
		if (string[pos + i] != endl[i])
			return false;
	}

	return true;
}
