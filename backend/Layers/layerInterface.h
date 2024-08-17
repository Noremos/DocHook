#ifdef USE_MODULE
module;
#undef MEXPORT
#define MEXPORT export
#else
#pragma once
#undef MEXPORT
#define MEXPORT
#endif

#include <algorithm>
#include <functional>
#include <memory>
#include <list>

#include <cassert>
#include "../JsonState.h"
#include "Barcode/PrjBarlib/include/barstrucs.h"

#include "../CSBind.h"
#include "../MatrImg.h"
#include "../Bind/MHashMap.h"
#include "../Bind/Framework.h"


#ifdef USE_MODULE
export module LayersCore;

// import BarTypes;
// import Platform;
// import JsonCore;
// import MetadataCoreIO;
// import CSBind;
// import MHashMap;
//import BackBind;
// import MatrModule;
#endif

constexpr int DEF_TILE_SIZE = 1000;
constexpr int DEF_TILE_OFFSET = 300;

struct TileProvider
{
public:
	float factor;
	int index;
	bc::point offset; // In rastar coords

	TileProvider(float factor, int index) : factor(factor), index(index), offset(0, 0)
	{ }
	TileProvider(float factor, int x, int y) : factor(factor), index(0), offset(x, y)
	{ }

	BackPixelPoint tileToPreview(uint locX, uint locY) const
	{
		return BackPixelPoint((locX + offset.x) / factor, (locY + offset.y) / factor);
	}

	BackPixelPoint tileToFull(uint locX, uint locY) const
	{
		return BackPixelPoint(locX + offset.x, locY + offset.y);
	}

	TileProvider& operator=(const TileProvider& other)
	{
		factor = other.factor;
		index = other.index;
		offset = other.offset;
		return *this;
	}
};

 class TileImgIterator
{
	TileIterator stW, stH;

	uint lastHei;
	bool peakEnd = false;

public:
	TileImgIterator(int tileSize, int tileOffset, int rwid, int rhei) :
		stW(0, tileSize, tileOffset, rwid),
		stH(0, tileSize, tileOffset, rhei)
	{
		lastHei = 0;
		stH.shouldSkip(lastHei);
	}

	void reset()
	{
		stW.reset(0);
		stH.reset(0);

		lastHei = 0;
		stH.shouldSkip(lastHei);

		peakEnd = false;
	}

	bool iter(bc::point& offset, uint& wid, uint& hei)
	{
		while (stW.shouldSkip(wid))
		{
			stW.reset(0);
			stH.accum();
			if (stH.shouldSkip(lastHei))
			{
				return false;
			}
		}

		offset.x = stW.pos();
		offset.y = stH.pos();
		hei = lastHei;

		stW.accum();
		return true;
	}

	// get 2x2 local index
	int getLocRectIndex() const
	{
		return 2 * (stH.locIndex % 2) + (stW.locIndex % 2);
	}

	bool notFintInLocal() const
	{
		return stW.tilesInLine() > 2 || stH.tilesInLine() > 2;
	}
};


 class LayerProvider
{
public:
	int width; // realWid
	int height;
	int tileSize = DEF_TILE_SIZE;
	float displayFactor;
	bc::point layerOffset;


	LayerProvider() :
		width(0), height(0),
		tileSize(DEF_TILE_SIZE),
		displayFactor(1.f),
		layerOffset(0,0)
	{
	}

	//LayerProvider(int realWid, int displayWid, int tileSize) :
	//	width(width), tileSize(tileSize),
	//	layerOffset(0,0), displayFactor(_displayFactor)
	//{
	//}

	int getOptTilesInRow(int offset) const
	{
		return width / tileSize + ((width % tileSize > offset) ? 1 : 0);
	}

	int getOptTilesInCol(int offset) const
	{
		return height / tileSize + ((height % tileSize > offset) ? 1 : 0);
	}

	int getTilesInRow() const
	{
		return getCon(width, tileSize);
	}

	int getCon(int total, int part) const
	{
		return total / part + (total % part == 0 ? 0 : 1);
	}

	void init(int realWid, int realHeight, int displayWid, int tileSize)
	{
		update(realWid, realHeight, displayWid);
		this->tileSize = tileSize;
	}

	void update(int realWid, int realHei, int displayWid)
	{
		this->displayFactor = realWid == displayWid ? 1.0f : static_cast<float>(realWid) / displayWid;
		this->width = realWid;
		this->height = realHei;

		{
			const int maxx = std::max(width, height);
			if (tileSize > maxx)
				tileSize = maxx;
		}
	}

	float getDisplayWidth()
	{
		return static_cast<float>(width) / displayFactor;
	}

	float getDisplayHeight()
	{
		return static_cast<float>(height) / displayFactor;
	}

	TileProvider tileByIndex(uint tileIndex) const
	{
		int tilesInRow = getTilesInRow();
		TileProvider p(displayFactor, tileIndex);
		p.offset.x = (tileIndex % tilesInRow) * tileSize;
		p.offset.y = (tileIndex / tilesInRow) * tileSize;
		return p;
	}

	TileProvider tileByOffset(uint offX, uint offY) const
	{
		int tilesInRow = getTilesInRow();

		TileProvider p(displayFactor, offX, offY);
		p.index = (offY / tileSize) * tilesInRow + offX / tileSize;
		return p;
	}


	bc::point toGlobal(uint locX, uint locY) const
	{
		return bc::point((locX + layerOffset.x) / displayFactor, (locY + layerOffset.y) / displayFactor);
	}

	bc::point toGlobalFactor(uint locX, uint locY) const
	{
		return bc::point(locX / displayFactor, locY / displayFactor);
	}

	bc::point getTileOffset(int index)
	{
		return tileByIndex(index).offset;
	}
};



inline int getCon(int total, int part)
{
	return total / part + (total % part == 0 ? 0 : 1);
}

 using LFID = unsigned int;


 struct InOutLayer
{
	int in;
	int subImgIndex = 0;
	int out;
	bool skipOut = false;

	bool skipOutput() const
	{
		return skipOut;
	}

	bool isOutEmpty() const
	{
		return out == -1 || in == out;
	}
};


 template <class T>
class LayersList
{
	std::list<std::unique_ptr<T>> layers;

public:
	T* at(uint id)
	{
		for (auto& layer : layers)
		{
			if (layer->getSysId() == id)
			{
				return layer.get();
			}
		}

		return nullptr;
		//assert(id < layers.size());
		//auto t = layers.begin();
		//std::advance(t, id);
		//return t->get();
	}

	T* at(const BackString& name)
	{
		for (auto& layer : layers)
		{
			if (layer->name == name)
			{
				return layer.get();
			}
		}

		return nullptr;
	}

	void remove(uint id)
	{
		for (auto it = layers.begin(); it != layers.end(); it++)
		{
			if ((*it)->getSysId() == id)
			{
				layers.erase(it);
				break;
			}
		}
	}

	auto begin()
	{
		return layers.begin();
	}

	auto end()
	{
		return layers.end();
	}

	template<class TReal>
	TReal* add()
	{
		auto t = std::make_unique<TReal>();
		auto tptr = t.get();
		layers.push_back(std::move(t));
		return tptr;
	}

	template<class TReal, class TArg>
	TReal* add(TArg arg)
	{
		auto t = std::make_unique<TReal>(arg);
		auto tptr = t.get();
		layers.push_back(std::move(t));
		return tptr;
	}


	void addMove(T* value)
	{
		std::unique_ptr<T> ptr(value);
		layers.push_back(std::move(ptr));
	}

	bool set(uint id, T* val)
	{
		// auto t = layers.begin();

		for (auto& layer : layers)
		{
			if (layer->getSysId() == id)
			{
				layer.reset(val);
				return true;
			}
		}

		return false;
	}

	bool set(uint id, std::unique_ptr<T>& val)
	{
		auto t = layers.begin();

		for (auto& layer : layers)
		{
			if (layer->getSysId() == id)
			{
				layer = std::move(val);
				return true;
			}
		}
		return false;
	}

	void move(uint oldId, uint newId)
	{
		auto t1 = layers.end();
		auto t2 = layers.end();
		bool foundFirst = false;
		for (auto it = layers.begin(); it != layers.end(); it++)
		{
			if ((*it)->getSysId() == oldId)
			{
				t1 = it;
				if (foundFirst)
					break;
				foundFirst = true;
			}
			else if ((*it)->getSysId() == newId)
			{
				t2 = it;
				if (foundFirst)
					break;
				foundFirst = true;
			}
		}

		if (t1 != layers.end() && t2 != layers.end())
		{
			// auto ptr = std::move(*t1);
			// layers.remove(t1);
			// layers.insert(t2, std::move(ptr));
			std::swap(*t1, *t2);
		}
	}

	void clear()
	{
		layers.clear();
	}

	size_t size()
	{
		return layers.size();
	}
	//template<class TReal>
	//TReal* getOrAdd(int id)
	//{
	//	if (id == -1)
	//	{

	//	}

	//	auto t = std::make_unique<TReal>();
	//	auto tptr = t.get();
	//	layers.push_back(std::move(t));
	//	return tptr;
	//}
};


 class ILayer : public IJsonIO
{
protected:
	enum class Type
	{
		Raster = 1,
		Vector = 2,
		Compinted = Raster | Vector,
		Group
	};

public:
	static LFID counter;
	static LFID getCountId()
	{
		return counter++;
	}

	BackString getMetaLayerName()
	{
		BackString name = intToStr(id);
		return name.append("_layer");
	}

public:
	int id = -1;
	BackString name;
	CSBinding cs;
	bool isSystem = false;
	bool isGroup = false;
	int layerCounter = 0;
	bool visible = true;

	void initCSFrom(const CSBinding& csf)
	{
		cs.globOrigin = csf.globOrigin;
		std::copy(csf.img_transform, csf.img_transform + 6, cs.img_transform);
	}


	virtual void saveLoadState(JsonObjectIOState* state) override
	{
		state->scInt("coreId", id);
		state->scStr("name", name);

		state->scBool("isSystem", isSystem);
		state->scBool("visible", visible);

		cs.saveLoadState(state);

		//state->scInt("layerCounter", layerCounter);

		// const bool isReading = state->isReading();
		// int size = subLayers.size();

		// JsonArrayIOState* arrst = state->arrayBegin("extraLayers", size);
		// auto iter = subLayers.begin();

		// for (int i = 0; i < size; i++)
		// {
		// 	JsonObjectIOState* obj = arrst->objectBegin(i);
		// 	int factoryId;
		// 	ILayer* lay;
		// 	if (isReading)
		// 	{
		// 		obj->scInt("factoryId", factoryId);
		// 		lay = CoreLayerFactory::CreateCoreLayer(factoryId);
		// 		subLayers.addMove(lay);
		// 	}
		// 	else
		// 	{
		// 		lay = iter->get();
		// 		factoryId = lay->getFactoryId();
		// 		obj->scInt("factoryId", factoryId);
		// 		iter++;
		// 	}

		// 	lay->saveLoadState(obj, metaFolder);
		// 	arrst->objectEnd();
		// }
		// state->arrayEnd();
	}

	virtual Type getType() const = 0;
	virtual const LFID getFactoryId() const = 0;
	virtual void release()
	{
		// BackString lname = getMetaLayerName();
		// metaFolder.update(metaFolder.getSubMeta(lname));
	}

	virtual float displayWidth() const = 0;
	virtual float displayHeight() const = 0;

	BackPoint getDisplaySize() const
	{
		return BackPoint(displayWidth(), displayHeight());
	}

	BackPoint getRealSize() const
	{
		return BackPoint(realWidth(), realHeight());
	}

	virtual int realWidth() const = 0;
	virtual int realHeight() const = 0;

	int getSysId() const
	{
		return id;
	}

	BackPoint getNormGlobStart() const
	{
		BackPoint st = getGlobStart();
		BackPoint ed = getGlobEnd();

		if (st.x > ed.x)
		{
			std::swap(st.x, ed.x);
		}

		if (st.y > ed.y)
		{
			std::swap(st.y, ed.y);
		}

		return st;
	}

	BackPoint getNormGlobSize() const
	{
		return cs.getScaled(getRealSize()).abs();
	}

	BackPoint getGlobStart() const
	{
		return cs.globOrigin;
	}

	BackPoint getGlobEnd() const
	{
		return getGlobStart() + getGlobSize();
	}

	BackPoint getGlobSize() const
	{
		return cs.getScaled(getDisplaySize());
	}

	virtual ~ILayer()
	{ }
};

 constexpr LFID RASTER_LAYER_FID = 0;
 constexpr LFID RASTER_LINE_LAYER_FID = 1;
 constexpr LFID RASTER_DISK_LAYER_FID = 2;
 constexpr LFID VECTOR_LAYER_FID = 3;
 constexpr LFID MULTIPOLY_VECTOR_LAYER_FID = 4;// ILayer::getCountId();
 constexpr LFID TREE_VECTOR_LAYER_FID = 5;
 constexpr LFID GROUP_LAYER = 6;
 constexpr LFID TREE_VECTOR_CLASS_LAYER = 7;

 class CoreLayerFactory
{
protected:
	template<class IF>
	using FunctionCoreHolder = MMMAP<LFID, std::function<IF*()> >;

	template<class IF>
	using FunctionGuiHolder = MMMAP<LFID, std::function<IF*(ILayer* core)> >;

	static FunctionCoreHolder<ILayer> coreLayersCreators;

public:
	static ILayer* CreateCoreLayer(int id);
};


 using SubImgInf = BackSize;

 class IRasterLayer : public ILayer
{
public:
	LayerProvider prov;
	int tileOffset = DEF_TILE_OFFSET;

	Type getType() const
	{
		return Type::Raster;
	}

	virtual void saveLoadState(JsonObjectIOState* state)
	{
		ILayer::saveLoadState(state);
		JsonObjectIOState* jobj = state->objectBegin("provider");

		jobj->scInt("width", prov.width);
		jobj->scInt("height", prov.height);
		jobj->scInt("tileSize", prov.tileSize);
		jobj->scFloat("displayFactor", prov.displayFactor);
		jobj->scInt("layerOffsetX", prov.layerOffset.x);
		jobj->scInt("layerOffsetY", prov.layerOffset.y);
		state->objectEnd();
	}

	virtual BackImage getRect(int stX, int stRow, int wid, int hei) = 0;

	virtual BackImage getImage(const int max) const = 0;
	virtual const BackImage* getCachedImage() const = 0;

	virtual void setCache(size_t) = 0;
	virtual void setSubImage(int imgIndex, bool overrideCs = false) = 0;
	virtual int getSubImage() = 0;
	virtual SubImgInf getSubImgInf() = 0;

	virtual int getFirstSmallIndex(const int maxSize = 2000) = 0;
	virtual std::vector<SubImgInf> getSubImageInfos() = 0;
};


 class IVectorLayer : public ILayer
{
public:
	Type getType() const
	{
		return Type::Vector;
	}

	virtual float displayWidth() const
	{
		return 0;
	}

	virtual float displayHeight() const
	{
		return 0;
	}
};


 struct LCoreItems
{
	ILayer* core;
	BackString name;

	LCoreItems(ILayer* core) : core(core)
	{ }

	LCoreItems(ILayer* core, const BackString name) :
		core(core), name(name)
	{ }

	const BackString& getName(const BackString& def) const
	{
		if (name.length() == 0)
			return def;
		else
			return name;
	}
};

 using RetLayers = std::vector<LCoreItems>;
