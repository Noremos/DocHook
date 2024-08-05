#ifdef USE_MODULE
#undef MEXPORT
#define MEXPORT export
module;
#else
#pragma once
#undef MEXPORT
#define MEXPORT
#endif

#include <memory>
#include <functional>
#include <istream>
#include <ostream>
#include "../Bind/Json.h"
#include "Usings.h"


#ifdef USE_MODULE
export module MetadataCoreIO;

// import JsonCore;
import StateBinIO;
import BackBind;
export import JsonState;
#else
#include "../../side/Barcode/PrjBarlib/modules/StateBinFile.h"
#include "JsonState.h"
#endif


MEXPORT class MetadataProvider
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

	BackPathStr getUniquePath(int& id) const
	{
		id = counter++;
		return path / intToStr(id);
	}

	int getUniqueId() const
	{
		return counter++;
	}

	BackPathStr getUniquePath(int& id, const BackString& ext)
	{
		id = counter++;
		return path / (intToStr(id) + (ext[0] == '.' ? ext : ("." + ext)));
	}

	BackPathStr getPath(int id, const BackString& ext) const
	{
		return path / (intToStr(id) + (ext[0] == '.' ? ext : ("." + ext)));
	}

	BackDirStr getSubFolder(const BackString& name) const
	{
		return path / name;
	}

	BackDirStr getFile(const BackString& name) const
	{
		return path / name;
	}

	MetadataProvider getSubMeta(const BackString& folder) const
	{
		return MetadataProvider(path / folder, counter);
	}

	void mkdir()
	{
		mkDirIfNotExists(path);
	}

// 	MetadataProvider store(const BackImage& img)
// 	{
// 		BackPathStr bps = getUniquePath(".png");
// 		MetadataProvider mp;
// 		mp.path = path / folder;
// 		return mp;
// 	}
};



MEXPORT class UserdefIO
{
public:
	virtual void writeData(BackJson& json) const = 0;
	virtual void readData(const BackJson& json) = 0;
};


MEXPORT inline void ioPoint(JsonArrayIOState* state, int id, BackPoint& p)
{
	auto* obj = state->objectBegin(id);
	obj->scDouble("x", p.x);
	obj->scDouble("y", p.y);
	state->objectEnd();
}

///

MEXPORT class IJsonIO
{
public:
	void read(const JsonObject& obj, const MetadataProvider& metaFolder)
	{
		JsonObjectIOStateReader state(obj);
		saveLoadState(&state, metaFolder);
	}

	void write(JsonObject& obj, const MetadataProvider& metaFolder)
	{
		JsonObjectIOStateWriter state(obj);
		saveLoadState(&state, metaFolder);
	}
private:
	virtual void saveLoadState(JsonObjectIOState* state, const MetadataProvider& metaFolder) = 0;
};


MEXPORT class IBffIO
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
