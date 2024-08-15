#pragma once

#include <unordered_set>
#include <random>
#include <assert.h>
#include <chrono>
#include <algorithm>
#include <mutex>
#include <filesystem>
#include "../Bind/Json.h"
#include "Usings.h"
//#include <iostream>

//#include "../side/Barcode/PrjBarlib/include/CellEater.h"


#include "CSBind.h"
#include "ProjectSettings.h"
#include "../Bind/Framework.h"
#include "Layers/layerInterface.h"

template<class T>
inline constexpr T mmmin(T a, T b)
{
	return a > b ? b : a;
}

template<class T>
inline constexpr T mmmax(const T a, const T b)
{
	return a > b ? a : b;
}


enum MarkersShowState { found = 0, ather, barcodeNotPassed, circleNotPassed, boundyNotPassed, holmNotPassed, allExceptRed, all, none };

enum class BackPath
{
	img,
	project,
	root,
	markers,
	classifier,
	classfiles,
	metadata,
	layers
	//classImages,
};


enum class GuiState
{
	Empty = 0,
	Loaded
};


class Project
{
	ProjectSettings& settings;
	DisplaySystem ds;

	int metaCounter = 0;

	void extraRead(const BackJson& json)
	{
		layers.clear();
		JsonObjectIOStateReader reader(json);
		extraReadWrite(&reader);
	}

	void extraWrite(BackJson& json)
	{
		JsonObjectIOStateWriter writer(json);
		extraReadWrite(&writer);
	}

	template<class P>
	static void ioPoint(JsonObjectIOState* state, BackString name, const P& p)
	{
		state->scDouble(name + "_x", p.x);
		state->scDouble(name + "_y", p.y);
	}

	void extraReadWrite(JsonObjectIOState* state)
	{
		const bool isReading = state->isReading();

		int size = layers.size();
		auto iter = layers.begin();
		JsonArrayIOState* arrst = state->arrayBegin("layers", size);

		for (int i = 0; i < size; i++)
		{
			JsonObjectIOState* obj = arrst->objectBegin(i);
			MetadataProvider sub = *settings.metaprov;// Do not reffer!

			int factoryId;
			ILayer* lay;
			if (isReading)
			{
				obj->scInt("factoryId", factoryId);
				lay = CoreLayerFactory::CreateCoreLayer(factoryId);
				if (lay == nullptr)
					continue; // Factory does not exist

				layers.addMove(lay);
			}
			else
			{
				lay = iter->get();
				factoryId = lay->getFactoryId();
				obj->scInt("factoryId", factoryId);
				iter++;
			}

			lay->saveLoadState(obj, sub);
			arrst->objectEnd();
		}
		state->arrayEnd();

		ds.saveLoadState(state, getMeta());
	}
public:
	//	Q_PROPERTY(SeachingSettings* searchSetts READ getSerchSetts)
	//	Q_PROPERTY(SeachingSettings searchSetts MEMBER searchSetts)
	//	SeachingSettings* getSerchSetts(){return &searchSetts;}

	Project() : settings(getSettings())
	{
		projectPath = "";
		// "D:\\Programs\\Barcode\\_bar\\_p2\\";
		settings.getIO().extraRead = [this](const BackJson& json) {extraRead(json);};
		settings.getIO().extraWrite = [this](BackJson& json) {extraWrite(json);};

		ds.csPos = BackPoint(0, 0);
		ds.csScale = 1.0;//BackPoint(1000, 1000);
		// mkDirIfNotExists(u_classCache);
	}

	~Project()
	{
		if (projectPath.has_filename())
		{
			saveProject();
		}

		//closeImages();
	}

	DisplaySystem& getDisplay()
	{
		return ds;
	}

	bool prjCreate = false;
public:
	GuiState state = GuiState::Empty;

	int curLayerIndex;
	LayersList<ILayer> layers;

	bool block = false;

	BackDirStr u_classCache;
	int u_algorithm = 0;

	template<class LDATA>
	LDATA* addLayerData(int projId = -1)
	{
		LDATA* d = layers.add<LDATA>();
		if (projId != -1)
			d->cs.init(projId);

		//d->prov.init(u_displayFactor, tileSize, reader->width());
		d->id = settings.layerCounter++;
		return d;
	}

	// template<class LDATA>
	// LDATA* addLayerData(int keepId, int projId)
	// {
	// 	LDATA* d = layers.add<LDATA>();
	// 	d->cs.init(projId);

	// 	//d->prov.init(u_displayFactor, tileSize, reader->width());
	// 	d->id = keepId;
	// 	return d;
	// }

	template<class LDATA>
	LDATA* changeLayerData(int id, int projId)
	{
		// Replace, dont add!
		LDATA* layer = new LDATA();
		layer->id = id;
		layer->cs.init(projId);
		layers.set(id, layer);
		return layer;
	}

	template<class LDATA>
	LDATA* addOrUpdateOut(InOutLayer& iol, int projId)
	{
		LDATA* layer;
		if (iol.isOutEmpty())
		{
			layer = addLayerData<LDATA>(projId);
			iol.out = layer->id;
		}
		else
		{
			// Replace, dont add!
			layer = changeLayerData<LDATA>(iol.out, projId);
		}

		return layer;
	}

	// template<class LDATA>
	// LDATA* addOrUpdateOut(InOutLayer& iol)
	// {
	// 	LDATA* layer = addOrUpdateOut(iol);
	// 	layer->init(getInRaster(iol));
	// 	return layer;
	// }

	IRasterLayer* getInRaster(int inId, int subImgIndex = -1)
	{
		ILayer* layer = layers.at(inId);
		auto rlayer = dynamic_cast<IRasterLayer*>(layer);
		assert(rlayer != nullptr);
		//rlayer->setSubImage(subImgIndex);
		return rlayer;
	}

	IRasterLayer* getInRaster(const InOutLayer& iol)
	{
		return getInRaster(iol.in, iol.subImgIndex);
	}


	template<class T>
	T* getInTRaster(const int in)
	{
		return dynamic_cast<T*>(getInRaster(in, 0));
	}

	template<class T>
	T* getInTRaster(const InOutLayer& iol)
	{
		return dynamic_cast<T*>(getInRaster(iol.in, iol.subImgIndex));
	}

	void removeLayer(int delId)
	{
		layers.remove(delId);
	}

	static Project* proj;

	BackString status;
public:
	void setProjectPath(const BackPathStr& path, bool recreateMeta = false)
	{
		projectPath = std::filesystem::absolute(path).parent_path();
		settupMeta(recreateMeta);
		prjCreate = true;

		/*		projectPath = (char*)dir.c_str();
				projectPath.string
				char last = projectPath[projectPath.filename().string().length() - 1];
				if (last != '\\' || last != '/')
					projectPath += '/';
		*/
	}

	static Project* getProject()
	{
		if (proj == nullptr)
			proj = new Project();

		return proj;
	}
	static void dropProject()
	{
		if (proj != nullptr)
		{
			delete proj;
			proj = nullptr;
		}
	}

	static BackPathStr getPathSt(BackPath pType)
	{
		return getProject()->getPath(pType);
	}

private:

	BackDirStr projectPath;

public:
	// bool isTileCached(int ind)
	// {
	// 	BackPathStr path = getTilePath(ind);
	// 	return pathExists(path);
	// }

	// BackPathStr getTilePath(int ind)
	// {
	// 	return getPath(BackPath::tiles) / (intToStr(ind) + ".png");
	// }

	BackPathStr getPath(BackPath pathI) const
	{
		switch (pathI)
		{
		case BackPath::project:
			return projectPath / "proj.qwr";
		case BackPath::root:
			return projectPath;
		case BackPath::metadata:
			return projectPath / "Metadata";
		case BackPath::layers:
			return getPath(BackPath::metadata) / "layers.json";
		default:
			throw;
		}
	}

	MetadataProvider& getMeta()
	{
		return *settings.metaprov.get();
	}

	BackPathStr getMetaPath(const BackString& item) const
	{
		return getPath(BackPath::metadata) / item;
	}

	BackPathStr getMetaPath() const
	{
		return getPath(BackPath::metadata);
	}

	void settupMeta(bool recreateMeta)
	{
		auto path = getMetaPath();
		if (pathExists(path))
		{
			if (recreateMeta)
			{
				dropDirIfExists(path);
				mkdir(path);
			}
		}
		else
			mkdir(path);

		settings.metaprov.reset(new MetadataProvider(path, metaCounter));
	}

	bool loadProject(const BackPathStr& prjFilepath)
	{
		setProjectPath(prjFilepath);

		BackJson loadDoc = jsonFromFile(prjFilepath);
		settings.getIO().read(loadDoc);
		prjCreate = true;

		return true;
	}

	bool saveProject()
	{
		if (!prjCreate)
			return false;

		JsonObject sets;
		settings.getIO().write(sets);
		jsonToFile(sets, getPath(BackPath::project));

		// mkDirIfNotExists(getPath(BackPath::classfiles));
		// mkDirIfNotExists(getPath(BackPath::tiles));
		return true;
	}

	BackPoint toGlob(const CSBinding& cs, const TileProvider& tileProv, float diplsayToRealFactor, BackPoint p)
	{
		BackPixelPoint op = tileProv.tileToPreview(p.x, p.y); // To display
		BackPoint iglob((static_cast<float>(op.x) + 0.5f), static_cast<float>(op.y) + 0.5f);
		return cs.toGlobal(iglob.x * diplsayToRealFactor, iglob.y * diplsayToRealFactor); // To real
	}
};
