#ifdef USE_MODULE
module;
#undef MEXPORT
#define MEXPORT export
#else
#pragma once
#undef MEXPORT
#define MEXPORT
#endif

#include <vector>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cassert>
#include <memory>
#include <utility>
#include <mutex>
#include "Barcode/PrjBarlib/include/barstrucs.h"
#include "Common.h"

#ifdef __linux__
#include <pthread.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include "../MetadataIO.h"
#include "../ProjectSettings.h"
#include "../MatrImg.h"
#include "../CachedBarcode.h"
#include "../Interfaces/IItem.h"
#include "layerInterface.h"
#include "Rasterlayers.h"

#ifdef USE_MODULE
export module RasterLineLayerModule;

//import BackBind;
// import MatrModule;
// import LayersCore;
// import RasterLayers;
// import MetadataCoreIO;
// import IItemModule;
// import GeoprocessorModule;
// import ProjectSettings;
// import CachedBarcode;
// import ClusterInterface;
#endif

MEXPORT class BarLineWorker
{
protected:
	std::unique_ptr<std::thread> thread;
	int& counter;
	IdGrater parentne;


	bool stopThreadFlag = false;
	bool taskUpdated = false;
	std::atomic<bool> busy = false;

public:
	BarLineWorker(int& counter) : counter(counter)
	{ }

	virtual ~BarLineWorker()
	{
		join();
	}
	void runTask()
	{
		busy = true;
#ifdef _WIN32
		SetThreadPriority(thread->native_handle(), THREAD_PRIORITY_ABOVE_NORMAL);
#elif __linux__
		sched_param params;
		params.sched_priority = 75;
		pthread_setschedparam(thread->native_handle(), SCHED_FIFO, &params);
#endif
		taskUpdated = true;
	}

	void join()
	{
		if (thread && busy)
			thread->join();
	}

	bool isBusy() const
	{
		return busy;
	}

	void stop()
	{
		stopThreadFlag = true;
	}

	void runAsync()
	{
		thread.reset(new std::thread([this] {this->runLoop(); }));
	}

	virtual void runSync() = 0;

	void runLoop()
	{
		while (!stopThreadFlag)
		{
			if (!taskUpdated)
				continue;

			taskUpdated = false;
			runSync();

#ifdef _WIN32
			SetThreadPriority(thread->native_handle(), THREAD_PRIORITY_BELOW_NORMAL);
#elif __linux__
			sched_param params;
			params.sched_priority = 20;
			pthread_setschedparam(thread->native_handle(), SCHED_FIFO, &params);
#endif
		}
	}
};
using BarFunc = std::function<void(int id, TileProvider prov, CachedBarline*)>;




// template<typename TWorker>
class WorkerPool
{
	std::vector<std::unique_ptr<BarLineWorker>> workers;
	std::unique_ptr<BarLineWorker> syncWorker;

public:
	void add(BarLineWorker* worker, bool allowSync)
	{
		if (allowSync && !syncWorker)
		{
			syncWorker.reset(worker);
			return;
		}

		worker->runAsync();
		workers.push_back(std::unique_ptr<BarLineWorker>(worker));
	}

	void addSync(BarLineWorker* worker)
	{
		syncWorker.reset(worker);
	}

	BarLineWorker* getSyncWorker()
	{
		return syncWorker.get();
	}

	BarLineWorker* getFreeWorker(bool& isAsync)
	{
		//while(true)
		{
			for (auto& worker : workers)
			{
				if (!worker->isBusy())
				{
					isAsync = true;
					return worker.get();
				}
			}
		}

		isAsync = false;
		return syncWorker.get();
	}

	void waitForAll(const bool stop)
	{
		using namespace std::chrono_literals;
		bool hasRunning = false;
		do
		{
			hasRunning = false;
			for (auto& worker : workers)
			{
				if (worker->isBusy())
				{
					hasRunning = true;
					std::this_thread::sleep_for(500ms);
					break;
				}
				else if (stop)
				{
					worker->stop();
					worker->join();
				}
			}
		} while (hasRunning);
	}
};



MEXPORT class RasterLineLayer : public RasterLayer
{
public:
	IClassItemHolder* collectionToPredict = nullptr;
	static std::vector<Barscalar> colors;
	std::vector<SimpleLine*> clickResponser;
	int cacheId = -1;
	int parentlayerId = -1;

	// Sub to display factor
	float subToRealFactor = 1.f;

	//int layerCounter = 0;

	RasterLineLayer()
	{
		if (colors.size() == 0)
		{
			// srand(300);
			colors.push_back(Barscalar(255, 0, 0));
			colors.push_back(Barscalar(0, 0, 0));
			colors.push_back(Barscalar(0, 255, 0));
			colors.push_back(Barscalar(0, 0, 255));
			for (int k = 0; k < 40; ++k)
				colors.push_back(Barscalar(5 + rand() % 250, 5 + rand() % 250, 5 + rand() % 250));

			colors.push_back(Barscalar(255, 255, 255));
		}
	}


	RetLayers createCacheBarcode(IRasterLayer* inLayer, const BarcodeProperies& propertices, IItemFilter* filter = nullptr);
	RetLayers processCachedBarcode(IItemFilter* filter);
	void processCachedBarcode(IItemFilter* filter, const BarFunc& func);

	static const Barscalar& getRandColor(size_t id)
	{
		return colors[id % colors.size()];
	}

	BackPathStr getCacheFilePath(const MetadataProvider& metaFolder)
	{
		MetadataProvider m = metaFolder.getSubMeta(getMetaLayerName());
		return m.getSubFolder("cached.bff");
	}

	virtual void release(const MetadataProvider& metaFolder) override
	{
		// RasterLayer::release(metaFolder);
		// if (cacheId != -1)
		// {
		// 	std::remove(getCacheFilePath(metaFolder).string().c_str()); // delete file
		// }
	}

	virtual const LFID getFactoryId() const override
	{
		return RASTER_LINE_LAYER_FID;
	}

	virtual void saveLoadState(JsonObjectIOState* state, const MetadataProvider& metaFolder) override
	{
		RasterLayer::saveLoadState(state, metaFolder);
		state->scInt("cacheId", cacheId);
		state->scInt("parentLayerId", parentlayerId);
		state->scFloat("subToRealFactor", subToRealFactor); // TODO: rename it to SubToDsiplay
	}

	virtual int realWidth() const override
	{
		return mat.width() * subToRealFactor; // Invalid
	}
	virtual int realHeight() const override
	{
		return mat.height() * subToRealFactor; // Invaild;
	}

	//virtual void readJson(const BackJson& json, const BackDirStr& metaFolder)
	//{
	//	RasterLayer::readJson(json, metaFolder);
	//}

	//virtual void writeJson(BackJson& json, const BackDirStr& metaFolder, int& counter)
	//{
	//	RasterLayer::writeJson(json, metaFolder, counter);

	//	for (int i=0;i < clickResponser.size(); i++)
	//	{
	//		// id, id in bar
	//	}
	//}

	//void init(const BackImage& src, int tileSize = DEF_TILE_SIZE)
	//{
	//	clickResponser.clear();
	//	mat.assignCopyOf(src);
	//	clickResponser.resize(mat.length());
	//	printf("alloced for preview: %d\n", clickResponser.size());
	//	prov.init(src.width(), src.height(), src.width(), tileSize);
	//}

	void init(IRasterLayer* layer, const MetadataProvider& metadata)
	{
		assert(layer != this);

		subToRealFactor = layer->realWidth() / layer->displayWidth(); // SubToDsiplay
		int wid = layer->displayWidth();
		int hei = layer->displayHeight();

		resetResponser(wid, hei);
		parentlayerId = layer->id;
		prov = layer->prov;

		mkDirIfNotExists(metadata.getSubFolder(getMetaLayerName()));
	}

	void clearResponser()
	{
		int wid = mat.width();
		int hei = mat.height();
		assert(mat.length() > 1);
		resetResponser(wid, hei);
	}

	void resetResponser(int wid, int hei)
	{
		mat.reinit(wid, hei, 4);

		clickResponser.clear();
		clickResponser.resize(mat.length());
		printf("alloced for preview: %zd\n", clickResponser.size());
	}
	SimpleLineHolder holder;

	std::mutex addToMapMutex;
	void setMatrPoint(int x, int y, SimpleLine* newLine, const Barscalar& color)
	{
		std::lock_guard<std::mutex> guard(addToMapMutex);

		int indLocal = mat.getLineIndex(x, y);
		SimpleLine* existLine = clickResponser[indLocal];

		if (existLine == nullptr)
		{
			mat.set(x, y, color);
			clickResponser[indLocal] = newLine;
		}
		else if (existLine == newLine) // Might be due to clickResponser[indLocal2] = newLin
		{
			mat.set(x, y, color);
		}
		else if (existLine->getDeath() < newLine->getDeath())
		{
			mat.set(x, y, color);
			clickResponser[indLocal] = newLine;
		}
	}

	// using ColorGrater = std::function<Barscalar(const IClassItem* item, bool& bad)>;

	bool passLine(const IClassItem* item, const IItemFilter* filter) const
	{
		if (item->getMatrixSize() == 0)
			return false;

		if (filter && !filter->pass(item))
			return false;

		return true;
	}

	void addLine(int i, const IClassItem* curLine, const TileProvider& tileProv);

	void addHolder(const IClassItemHolder& items, const TileProvider& tileProv, const IItemFilter* filter)
	{
		for (size_t i = 0; i < items.getItemsCount(); ++i)
		{
			auto curLine = items.getItem(i);
			if (!passLine(curLine, filter))
				continue;

			addLine(i, curLine, tileProv);
		}
	}
};
