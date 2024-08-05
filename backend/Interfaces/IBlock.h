
#ifdef USE_MODULE
#undef MEXPORT
#define MEXPORT export
module;
#else
#pragma once
#undef MEXPORT
#define MEXPORT
#endif

#include <cassert>
#include "Barcode/PrjBarlib/include/barline.h"
#include "Usings.h"

#ifdef USE_MODULE
export module IBlock;
import ProjectSettings;
import RefSettings;
import LayersCore;
#else
#include "../ProjectSettings.h"
#include "../Core/RefSettings.h"
#include "../Layers/layerInterface.h"
#endif

//import BackBind;
// import BarTypes;
//export class IClasterItem
//{
//	IClasterItem(CachedBarline*)
//	{}
//};

MEXPORT class IBlock
{
public:
	BackPathStr dbPath;
	RefSettings settings; // Clustering settings

	void open(const BackPathStr& metaPath)
	{
		this->dbPath = metaPath / name();
	}

	virtual void addInput(InOutLayer iol) = 0;
	virtual RetLayers execute(InOutLayer iol) = 0;
	virtual void clear() = 0;

	virtual const BackString name() const = 0;

	virtual ~IBlock()
	{
	}
};


//using GlobalClusterRegister = GlobalRegister<TClass, TClassHolder, TClassifier, ClusterFactory>;


// Ml for machine learning
MEXPORT class BlockFactory
{
private:
	using BaseHolder = std::unique_ptr<IBlock>;

	// using ItemCreator = BaseCreator<IClassItem>;
	// using HolderCreator = BaseCreator<IClassItemHolder>;
	// using ClassifierCreator = BaseCreator<IBarClassifier>;

	template<class IF>
	using FunctionHolder = std::vector<BaseHolder>;
	FunctionHolder<IBlock> blockHolder;
	std::vector<BackString> names;

public:
	IBlock* get(int id)
	{
		if (id >= 0 && id < blockHolder.size())
			return blockHolder[id].get();
		else
			return nullptr;
	}

	const std::vector<BackString>& getNames() const
	{
		return names;
	}

	size_t getCount() const
	{
		return idCounter;
	}

	int idCounter = 0;

	template <typename Block>
	int Register()
	{
		blockHolder.push_back(std::make_unique<Block>());
		names.push_back(blockHolder.back()->name());
		return idCounter++;
	}
};

MEXPORT inline BlockFactory& getBlockFactory()
{
	static BlockFactory blockFactory;
	return blockFactory;
}

MEXPORT template<class TClass>
class BlockRegister
{
	int id;
public:
	BlockRegister()
	{
		id = getBlockFactory().Register<TClass>();
	}
	int getId() const
	{
		return id;
	}
};
