#include "RasterLineLayer.h"

std::vector<Barscalar> RasterLineLayer::colors;

class CreateBarThreadWorker : public BarLineWorker
{
	std::mutex& cacherMutex;

	bc::barstruct constr;
	IRasterLayer* inLayer;
	ItemHolderCache& cacher;

	IClassItemHolder::ItemCallback cacheClass;
	int inde;

	BackImage rect;
	TileProvider tileProv;
public:
	CreateBarThreadWorker(std::mutex& cacherMutex,
		const bc::barstruct& constr,
		IRasterLayer* inLayer,
		ItemHolderCache& cacher,
		int& counter) :
		BarLineWorker(counter),
		cacherMutex(cacherMutex),
		constr(constr),
		inLayer(inLayer),
		cacher(cacher),
		tileProv(0, 0)
	{
	}
	int algMode;

	void setCallback(/*Project* proj, */RasterLineLayer* layer, const IItemFilter* filter)
	{
		cacheClass = [this, /*proj,*/ layer, filter](IClassItem* item)
			{
				if (layer->passLine(item, filter))
				{
					//proj->predictForLayer(item, tileProv, layer->subToRealFactor);
					layer->addLine(inde++, item, tileProv);
				}
			};
	}

	void updateTask(BackImage& rrect, const TileProvider& tileProvider)
	{
		this->rect = std::move(rrect);
		this->tileProv = tileProvider;
	}


	void runSync()
	{
		const auto start = std::chrono::steady_clock::now();

		// Keep this! See lyambda
		inde = 0;
		parentne.clear();
		// -------------

		printf("Start run for tile %d\n", tileProv.index);

		CachedBaritemHolder creator;
		creator.create(&rect, constr, cacheClass);

		{
			std::lock_guard<std::mutex> guard(cacherMutex);
			cacher.save(&creator, tileProv.index);
		}


		const auto end = std::chrono::steady_clock::now();
		const auto diff = end - start;
		const double len = std::chrono::duration<double, std::milli>(diff).count();
		printf("End run for tile %d; Time: %dms\n", tileProv.index, (int)len);

		++counter;
		busy = false;
	}
};


class ProcessCacheBarThreadWorker : public BarLineWorker
{
	const IItemFilter* filter;
	//Project* proj;

	TileProvider tileProv;

public:
	CachedBaritemHolder holder;
	BarFunc func;

	ProcessCacheBarThreadWorker(int& counter, const IItemFilter* filter/*, Project* proj*/) :
		BarLineWorker(counter), filter(filter),// proj(proj),
		tileProv(0, 0)
	{ }

	void updateTask(const TileProvider& itileProv)
	{
		tileProv = itileProv;
	}

	bool addLine = true;
	void runSync()
	{
		const auto start = std::chrono::steady_clock::now();

		printf("Start run for tile %d\n", tileProv.index);

		for (size_t i = 0; i < holder.getItemsCount(); ++i)
		{
			CachedBarline* item = holder.getRItem(i);
			func((int)i, tileProv, item);
		}

		const auto end = std::chrono::steady_clock::now();
		const auto diff = end - start;
		const double len = std::chrono::duration<double, std::milli>(diff).count();
		printf("End run for tile %d; Time: %dms\n", tileProv.index, (int)len);

		++counter;
		busy = false;
	}

};


RetLayers RasterLineLayer::createCacheBarcode(IRasterLayer* inLayer, const BarcodeProperies& propertices, IItemFilter* filter)
{
	// Setup
	bc::barstruct constr;
	constr.createBinaryMasks = true;
	constr.createGraph = true;
	constr.attachMode = propertices.attachMode;
	//constr.maxRadius = 255;
	// constr.attachMode = bc::AttachMode::closer;
	constr.returnType = bc::ReturnType::barcode2d;
	//constr.addStructure(propertices.barstruct);
	//	constr.setStep(stepSB);
	// -------

	constr = propertices.get();

	// Input Layer prepatons
	int tileSize = inLayer->prov.tileSize;
	int tileOffset = inLayer->tileOffset;
	SubImgInf curSize = inLayer->getSubImgInf(); // Cursubimg

	if (filter)
	{
		const buint fullTile = tileSize + tileOffset;
		filter->imgLen = fullTile * fullTile;
	}
	// End Input Layer

	// Setup output layers
	//
	// Line layer
	auto& metaprov = getSettings().getMeta();
	RasterLineLayer* layer = this;
	layer->init(inLayer, getSettings().getMeta());
	layer->initCSFrom(inLayer->cs);
	layer->tileOffset = tileOffset;

	LayerProvider& prov = layer->prov;
	prov.init(curSize.wid, curSize.hei, inLayer->displayWidth(), tileSize);

	if (layer->cacheId == -1)
		layer->cacheId = metaprov.getUniqueId();

	RetLayers ret;
	ret.push_back(layer);

	// Cacher
	ItemHolderCache cacher;
	cacher.openWrite(layer->getCacheFilePath(metaprov));
	// ------


	// Setup tileIterators
	TileImgIterator tileIter(tileSize, tileOffset, curSize.wid, curSize.hei);

	// Threads
	const bool curRunAsync = getSettings().runAsync;
	int curthreadsCount = curRunAsync ? getSettings().threadsCount : 1;
	int counter = 0;
	if (curRunAsync)
	{
		printf("Run in async mode with %d threads\n", curthreadsCount);
	}
	else
	{
		printf("Run in sync mode\n");
	}

	const bool allowSyncInAsync = true;
	WorkerPool wpool;
	std::mutex cacherMutex;
	for (unsigned short i = 0; i < curthreadsCount; i++)
	{
		CreateBarThreadWorker* worker = new CreateBarThreadWorker(cacherMutex, constr, inLayer, cacher, counter);
		worker->setCallback(layer, filter);
		wpool.add(worker, allowSyncInAsync);
	}

	// Run
	buint iwid, ihei;
	bc::point offset;

	const auto start = std::chrono::steady_clock::now();
	if (curRunAsync)
	{
		// How much tiles the offset covers; We skip the conflict tiles;
		// const int maxSteps = 1 + static_cast<int>((tileSize + tileOffset) / tileSize);
		tileIter.reset();
		while (tileIter.iter(offset, iwid, ihei))
		{
			TileProvider tileProv = prov.tileByOffset(offset.x, offset.y);
			auto rect = inLayer->getRect(offset.x, offset.y, iwid, ihei);

			bool isAsyncl;
			CreateBarThreadWorker* worker = static_cast<CreateBarThreadWorker*>(wpool.getFreeWorker(isAsyncl));
			worker->updateTask(rect, tileProv);

			if (isAsyncl)
				worker->runTask();
			else
				worker->runSync();
		}
		wpool.waitForAll(true);
	}
	else
	{
		while (tileIter.iter(offset, iwid, ihei))
		{
			TileProvider tileProv = prov.tileByOffset(offset.x, offset.y);
			auto rect = inLayer->getRect(offset.x, offset.y, iwid, ihei);

			CreateBarThreadWorker* worker = static_cast<CreateBarThreadWorker*>(wpool.getSyncWorker());
			worker->updateTask(rect, tileProv);
			worker->runSync();
		}
	}

	const auto end = std::chrono::steady_clock::now();
	const auto diff = end - start;
	const double len = std::chrono::duration<double, std::milli>(diff).count();
	printf("All works ended in %dms\n", (int)len);

	return ret;
}

RetLayers RasterLineLayer::processCachedBarcode(IItemFilter* filter)
{
	//if (u_displayFactor < 1.0)
	//	throw std::exception();

	RasterLineLayer* inLayer = this;
	int tileSize = inLayer->prov.tileSize;
	int tileOffset = inLayer->tileOffset;

	if (filter)
	{
		const buint fullTile = tileSize + tileOffset;
		filter->imgLen = fullTile * fullTile;
	}

	// -------
	RetLayers ret;
	RasterLineLayer* outLayer = inLayer;

	if (collectionToPredict == nullptr)
		outLayer->clearResponser();

	BarFunc func;
	if (outLayer->collectionToPredict == nullptr)
	{
		func = std::move([&outLayer, &filter](int id, TileProvider prov, CachedBarline* item)
		{
			if (outLayer->passLine(item, filter))
			{
				outLayer->addLine((int)id, item, prov);
			}
		});
	}
	else
	{
		func = std::move([&outLayer, &filter](int id, TileProvider prov, CachedBarline* item)
		{
			if (outLayer->passLine(item, filter))
			{
				outLayer->collectionToPredict->addItem(*item);
			}
		});
	}

	processCachedBarcode(filter, func);
	return ret;
}

void RasterLineLayer::processCachedBarcode(IItemFilter* filter, const BarFunc& func)
{
	RasterLineLayer* inLayer = this;
	int tileSize = inLayer->prov.tileSize;
	int tileOffset = inLayer->tileOffset;

	if (filter)
	{
		const buint fullTile = tileSize + tileOffset;
		filter->imgLen = fullTile * fullTile;
	}
	auto& metaprov = getSettings().getMeta();

	// Cacher
	ItemHolderCache cacher;
	cacher.openRead(inLayer->getCacheFilePath(metaprov));

	// Thread
	auto& prov = inLayer->prov;

	const bool curRunAsync = getSettings().runAsync;
	int curthreadsCount = curRunAsync ? getSettings().threadsCount : 1;
	int counter = 0;
	if (curRunAsync)
	{
		curthreadsCount = getSettings().threadsCount;
		printf("Run in async mode with %d threads\n", curthreadsCount);
	}
	else
	{
		printf("Run in sync mode\n");
	}

	const bool allowSyncInAsync = true;
	WorkerPool wpool;
	for (unsigned short i = 0; i < curthreadsCount; i++)
	{
		ProcessCacheBarThreadWorker* worker = new ProcessCacheBarThreadWorker(counter, filter);
		worker->func = func;
		wpool.add(worker, allowSyncInAsync);
	}

	const auto start = std::chrono::steady_clock::now();
	if (curRunAsync)
	{
		int tileIndex = 0;
		while (cacher.canRead())
		{
			bool isAsync;
			auto* worker = static_cast<ProcessCacheBarThreadWorker*>(wpool.getFreeWorker(isAsync));

			cacher.load(tileIndex, &worker->holder);

			TileProvider tileProv = prov.tileByIndex(tileIndex);

			worker->updateTask(tileProv);

			if (isAsync)
				worker->runTask();
			else
				worker->runSync();
		}

		wpool.waitForAll(true);
	}
	else // Sync
	{
		int tileIndex = 0;
		while (cacher.canRead())
		{
			ProcessCacheBarThreadWorker* worker =  static_cast<ProcessCacheBarThreadWorker*>(wpool.getSyncWorker());
			cacher.load(tileIndex, &worker->holder);

			TileProvider tileProv = inLayer->prov.tileByIndex(tileIndex);

			worker->updateTask(tileProv);
			worker->runSync();
		}
	}

	const auto end = std::chrono::steady_clock::now();
	const auto diff = end - start;
	const double len = std::chrono::duration<double, std::milli>(diff).count();
	printf("All works ended in %dms\n", (int)len);
}


struct DictWrap
{
	std::unordered_set<buint> points;
	int wid = 0, hei = 0;

	void set(buint i)
	{
		points.insert(i);
	}

	bool has(buint i) const
	{
		return points.find(i) != points.end();
	}

	bool has(int x, int y) const
	{
		if (x < 0 || y < 0)
			return false;
		return points.find(bc::barvalue::getStatInd(x, y)) != points.end();
	}

	bool hasCorners(int x, int y) const
	{
		return (has(x - 1, y) && has(x - 1, y - 1) && has(x, y - 1) && has(x, y + 1) && has(x + 1, y + 1));
	}
};

void getCountourSimple(const bc::barvector& points, bc::barvector& contur, float changeFactor)
{
	contur.clear();

	DictWrap dictPoints;
	for (auto& p : points)
	{
		dictPoints.set(p.getIndex());
		int x = p.getX();
		int y = p.getY();
		if (x > dictPoints.hei)
		{
			dictPoints.hei = x;
		}
		if (y > dictPoints.wid)
		{
			dictPoints.wid = y;
		}
	}

	for (auto p : points)
	{
		int x = p.getX();
		int y = p.getY();
		if (!dictPoints.hasCorners(x, y))
		{
			p.setXY(x * changeFactor, y * changeFactor);
			contur.push_back(p);
		}
	}
}

void RasterLineLayer::addLine(int i, const IClassItem* curLine, const TileProvider& tileProv)
{
	SimpleLine* sl;
	auto curIdKey = curLine->getId();
	//assert(curIdKey == i);
	// TODO: Will not work with tiles. Use tile + curIdKey to get the hash and use map in holder
	//if (holder.holder.size() <= curIdKey)
	//	holder.holder.resize(curIdKey + 1);

	sl = holder.holder[curIdKey].get();
	if (sl)
	{
		sl->tileId = tileProv.index;
		sl->barlineIndex = i;
	}
	else
	{
		sl = new SimpleLine(tileProv.index, i);
		sl->root = &holder;
		holder.holder[curIdKey].reset(sl);
	}

	{
		sl->parent = curLine->getParentId();
		//if (holder.holder.size() <= sl->parent)
		//	holder.holder.resize(static_cast<size_t>(sl->parent) + 1);

		auto par = holder.holder[sl->parent].get();
		if (par == nullptr)
		{
			par = new SimpleLine(-1, -1);
			par->root = &holder;
			holder.holder[sl->parent].reset(par);
		}
		par->children.push_back(curIdKey);
	}

	const auto& matr = curLine->getMatrix();
	int depth = curLine->getDeath();
	sl->depth = depth;
	sl->start = curLine->start();
	sl->end = curLine->end();
	sl->matrSrcSize = (int)matr.size();

	// Add line
	Barscalar pointCol = RasterLineLayer::colors[rand() % RasterLineLayer::colors.size()];

	// Countur
	DictWrap dictPoints;
	for (auto& p : matr)
	{
		dictPoints.set(p.getIndex());
		int x = p.getX();
		int y = p.getY();
		if (x > dictPoints.hei)
		{
			dictPoints.hei = x;
		}
		if (y > dictPoints.wid)
		{
			dictPoints.wid = y;
		}
	}

	auto& outMatr = sl->matr;
	for (const auto& pm : matr)
	{
		// We have:
		// - a real img size (input)
		// - a sub img size (selected subimg in input)
		// - a display img (for LineRaster).

		// Get a pixels form a sub
		int x = pm.getX();
		int y = pm.getY();


		// Cast sub point to display (mat variable) via tileProv
		BackPixelPoint op = tileProv.tileToPreview(x, y);
		op.x = (std::min)(mat.wid() - 1, op.x);
		op.y = (std::min)(mat.hei() - 1, op.y);

		// Set display point
		setMatrPoint(op.x, op.y, sl, pointCol);

		// Get countur
		if (dictPoints.hasCorners(x, y)) // Skip
		{
			continue;
		}

		// Cast to a real img via factor and save for draw
		outMatr.push_back(bc::barvalue(op.x * subToRealFactor, op.y * subToRealFactor, pm.value));
	}
}
