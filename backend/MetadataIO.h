#pragma once

#include <memory>
#include <functional>
#include <istream>
#include <ostream>
#include "../Bind/Json.h"
#include "../Bind/Usings.h"


#include "../side/Barcode/PrjBarlib/modules/StateBinFile.h"
#include "JsonState.h"


class MetadataProvider
{
	BackDirStr path;
	int& counter;

public:
	MetadataProvider(const BackDirStr& path, int& counter) :
		path(path), counter(counter)
	{ }

	MetadataProvider(const MetadataProvider& meta) :
		path(meta.path), counter(meta.counter)
	{ }


	void update(const MetadataProvider& mpv)
	{
		path = mpv.path;
		counter = mpv.counter;
	}

	int getUniqueId() const
	{
		return counter++;
	}

// 	MetadataProvider store(const BackImage& img)
// 	{
// 		BackPathStr bps = getUniquePath(".png");
// 		MetadataProvider mp;
// 		mp.path = path / folder;
// 		return mp;
// 	}
};



class UserdefIO
{
public:
	virtual void writeData(BackJson& json) const = 0;
	virtual void readData(const BackJson& json) = 0;
};


inline void ioPoint(JsonArrayIOState* state, int id, BackPoint& p)
{
	auto* obj = state->objectBegin(id);
	obj->scDouble("x", p.x);
	obj->scDouble("y", p.y);
	state->objectEnd();
}

///

class IJsonIO
{
public:
	void read(const JsonObject& obj)
	{
		JsonObjectIOStateReader state(obj);
		saveLoadState(&state);
	}

	void write(JsonObject& obj)
	{
		JsonObjectIOStateWriter state(obj);
		saveLoadState(&state);
	}
private:
	virtual void saveLoadState(JsonObjectIOState* state) = 0;
};


class IBffIO
{
public:
	virtual void read(StateBinFile::BinState* state)
	{
		saveLoadState(state);
	}
	void read(std::istream& stream)
	{
		StateBinFile::BinStateReader reader(stream);
		saveLoadState(&reader);
	}

	void write(StateBinFile::BinState* state)
	{
		saveLoadState(state);
	}
	void write(std::ostream& stream)
	{
		StateBinFile::BinStateWriter writer(stream);
		saveLoadState(&writer);
	}

private:
	virtual void saveLoadState(StateBinFile::BinState* state) = 0;
};
