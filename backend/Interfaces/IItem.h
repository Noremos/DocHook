#ifdef USE_MODULE
module;
#else
#pragma once
#endif

#include <assert.h>
#include <functional>
#include <vector>
#include <memory>
#include "Barcode/PrjBarlib/include/barscalar.h"
#include "Barcode/PrjBarlib/include/barline.h"
#include "Usings.h"


#include "../MetadataIO.h"
#ifdef USE_MODULE
export module IItemModule;
#undef MEXPORT
#define MEXPORT export
#else
#undef MEXPORT
#define MEXPORT
#endif

// import BarScalarModule;
// import BarTypes;
// import MetadataCoreIO;
//import BackBind;
// import StateBinIO;


MEXPORT class IClassItem : public IBffIO
{
public:
	virtual size_t getId() const = 0;
	virtual size_t getParentId() const = 0;
	// virtual const bc::barlinevector& getLines() const = 0;

	virtual const bc::barvector& getMatrix() const = 0;
	virtual const size_t getMatrixSize() const = 0;
	bc::barvalue getMatrValue(int id) const
	{
		return getMatrix()[id];
	}

	virtual int getDeath() const = 0;
	virtual Barscalar start() const = 0;
	virtual Barscalar end() const = 0;
	// virtual IClassItem* parent() const = 0;

	virtual ~IClassItem()
	{
	}
};


// Holder - a container to store items
MEXPORT class IClassItemHolder : public IBffIO
{
public:
	virtual IClassItem* getItem(size_t id) = 0;
	virtual const IClassItem* getItem(size_t id) const = 0;
	virtual size_t getItemsCount() const = 0;
	virtual void addItem(const IClassItem& item) = 0;
	virtual void perform()
	{ }

	using ItemCallback = std::function<void(IClassItem* item)>;

	virtual void create(bc::DatagridProvider* img, const bc::barstruct& constr, const ItemCallback& callback) = 0;
	virtual ~IClassItemHolder() {}
};

MEXPORT template<typename T>
class IDataClassItemHolder : public IClassItemHolder
{
protected:
	std::vector<T*> items;

public:
	virtual void addItem(const IClassItem& item)
	{
		//items.push_back(static_cast<T>(&item));
	}

	size_t getItemsCount() const
	{
		return items.size();
	}

	virtual ~IDataClassItemHolder()
	{
		for (size_t var = 0; var < items.size(); ++var)
		{
			delete items[var];
		}
		items.clear();
	}
};


MEXPORT template<typename T>
class IDataClassItemValueHolder : public IClassItemHolder
{
protected:
	std::vector<T> items;

public:

	std::vector<T>& getItems()
	{
		return items;
	}

	const std::vector<T>& getItems() const
	{
		return items;
	}

	virtual void addItem(const IClassItem& item)
	{
		items.push_back(std::move(T(&item)));
	}

	T* getItem(size_t id)
	{
		assert(id < items.size());
		return &items[id];
	}

	const T& getLastItem() const
	{
		return items.back();
	}

	T& getLastItem()
	{
		return items.back();
	}


	const IClassItem* getItem(size_t id) const
	{
		assert(id < items.size());
		return &items[id];
	}

	size_t getItemsCount() const
	{
		return items.size();
	}

	virtual ~IDataClassItemValueHolder()
	{ }
};


MEXPORT class ItemHolderCache
{
protected:
	std::unique_ptr<StateBinFile::BinState> state;

public:
	void openRead(const BackPathStr& str)
	{
		state.reset(new StateBinFile::BinStateReader());
		if (!state->open(str.string()))
			throw;
	}

	void openWrite(const BackPathStr& str)
	{
		state.reset(new StateBinFile::BinStateWriter());
		if (!state->open(str.string()))
			throw;
	}

	// void openRead(std::istringstream& str)
	//{
	//	state.reset(new StateBinFile::BinStateReader(str));
	// }

	// void openWrite(std::ostringstream& str)
	//{
	//	state.reset(new StateBinFile::BinStateWriter(str));
	// }

	int readIndex(int id)
	{
		assert(state->isReading());

		auto* reader = dynamic_cast<StateBinFile::BinStateReader*>(state.get());
		reader->moveIndex(id);
		state->beginItem();

		return reader->pInt(0);
	}

	int getItemsCount()
	{
		assert(state->isReading());

		auto* reader = dynamic_cast<StateBinFile::BinStateReader*>(state.get());
		return (int)reader->getIndexSize();
	}

	IClassItemHolder* load(int& index, IClassItemHolder* t)
	{
		assert(state->isReading());

		index = state->pInt(0);
		t->read(state.get());
		return t;
	}

	IClassItemHolder* loadSpecific(int index, IClassItemHolder* t)
	{
		assert(state->isReading());
		auto* reader = dynamic_cast<StateBinFile::BinStateReader*>(state.get());

		size_t i = 0, total = reader->getIndexSize();
		for (; i < total; ++i)
		{
			reader->moveIndex((int)i);
			int rindex = state->pInt(0);
			if (rindex == index)
				break;
		}
		assert(i != total);

		state->beginItem();
		t->read(state.get());
		return t;
	}

	void save(IClassItemHolder* item, int index)
	{
		assert(!state->isReading());

		state->beginItem();
		state->pInt(index);
		item->write(state.get());
	}

	bool canRead() const
	{
		return !state->ended();
	}

	void close()
	{
		state.reset(nullptr);
	}
};


MEXPORT struct IItemFilter
{
	int imgLen = 0;

	virtual bool pass(const IClassItem* line) const = 0;
};

MEXPORT struct RangeItemFilter : public IItemFilter
{
	struct FRange
	{
		int first;
		int second;

		// template<class T>
		// bool inRange(const T& val) const
		//{
		//	return val >= first && val <= second;
		// }

		bool inRange(int val) const
		{
			return val >= first && val <= second;
		}

		bool inRange(const Barscalar& val) const
		{
			return val >= first && val <= second;
		}

		bool notInRange(int val) const
		{
			return val < first || val > second;
		}

		bool notInRange(const Barscalar& val) const
		{
			return val < first && val > second;
		}
	};

	FRange start{ 0, 255 };
	FRange len{ 0, 255 };
	FRange matrSizeProc{ 0, 100 };
	FRange depth{ 0, 1000 };

	int minPixelsSize = 0;

	bool pass(const IClassItem* line) const
	{
		size_t matrSize = line->getMatrixSize();
		auto linlen = line->end() - line->start();
		return start.inRange(line->start()) &&
			len.inRange(linlen) &&
			matrSizeProc.inRange(matrSize * 100 / imgLen) &&
			matrSize > minPixelsSize &&
			depth.inRange(line->getDeath());
	}


	//virtual bool passFilter(const IItemFilter& filter) const
	//{
	//	return filter.start.inRange(line->start) &&
	//		filter.len.inRange(line->len()) &&
	//		filter.matrSizeProc.inRange(line->matr.size() * 100 / filter.imgLen) &&
	//		filter.depth.inRange(line->getDeath());
	//}
};

// Ml for machine learning
MEXPORT template <typename ICItem, typename ICHolder, typename IML>
class ImlFactory
{
private:
	template<class Interface>
	using BaseCreator = std::function<std::unique_ptr<Interface>()>;

	// using ItemCreator = BaseCreator<IClassItem>;
	// using HolderCreator = BaseCreator<IClassItemHolder>;
	// using ClassifierCreator = BaseCreator<IBarClassifier>;

	template<class IF>
	using FunctionHolder = std::vector<BaseCreator<IF>>;
	FunctionHolder<ICItem> itemCreators;
	FunctionHolder<ICHolder> holderCreators;
	FunctionHolder<IML> mlCreators;
	std::vector<BackString> names;

	template<class IF>
	std::unique_ptr<IF> Create(int id, FunctionHolder<IF>& creators)
	{
		//auto it = creators.find(id);
		//if (it != creators.end())
		//	return it->second();
		//else
		//	return nullptr;
		assert(id >= 0 && id < creators.size());
		return creators[id]();
	}

public:
	const std::vector<BackString>& getNames() const
	{
		return names;
	}

	size_t getCount() const
	{
		return idCounter;
	}

	std::unique_ptr<ICItem> CreateItem(int id)
	{
		return Create<ICItem>(id, itemCreators);
	}

	std::unique_ptr<ICHolder> CreateHolder(int id)
	{
		return Create<ICHolder>(id, holderCreators);
	}

	std::unique_ptr<IML> CreateML(int id)
	{
		return Create<IML>(id, mlCreators);
	}

	int idCounter = 0;

	template <typename Item, typename Holder, typename ML>
	int Register(BackStringView name)
	{
		itemCreators.push_back([]() { return std::make_unique<Item>(); });
		holderCreators.push_back([]() { return std::make_unique<Holder>(); });
		mlCreators.push_back([]() { return std::make_unique<ML>(); });
		names.push_back(BackString(name.data(), name.length()));
		return idCounter++;
	}
};


MEXPORT template<class TFactory, class TClass, class TClassHolder, class TML>
class GlobalRegister
{
	int id;
public:
	GlobalRegister(TFactory& factory, BackStringView name)
	{
		id = factory.template Register<TClass, TClassHolder, TML>(name);
		printf("%s class registered", name.data());
	}
	int getId() const
	{
		return id;
	}
};