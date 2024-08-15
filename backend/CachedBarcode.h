
#ifdef USE_MODULE
module;
#else
#pragma once
#endif

#include <cassert>
#include <algorithm>
#include <memory>
#include "Barcode/PrjBarlib/include/barcodeCreator.h"
#include "Usings.h"

#include "../../side/Barcode/PrjBarlib/modules/StateBinFile.h"

#ifdef USE_MODULE
export module CachedBarcode;
import IItemModule;
import MHashMap;
#undef MEXPORT
#define MEXPORT export
#else
#include "../Bind/MHashMap.h"
#include "Interfaces/IItem.h"
#undef MEXPORT
#define MEXPORT
#endif

// import BarcodeModule;
//import BackBind;
// import StateBinIO;

MEXPORT class CachedBaritemHolder;

MEXPORT class CachedBarline : public IClassItem
{
	//static unsigned int idCounter;
public:
	buint id;
	buint parentId;
	std::unique_ptr<buint[]> children;
	buint childrenCount;
	Barscalar startl, endl;
	bc::barvector matrix;
	//int matrix;
	unsigned char depth;

	CachedBaritemHolder* root;

	CachedBarline() : IClassItem(),
		id(0), parentId(-1), children(nullptr), childrenCount(0),
		startl(uchar(0)), endl(uchar(0)), matrix(), depth(0), root(nullptr)
	{ }

	CachedBarline(const CachedBarline& other)
	{
		copyFrom(other);
	}

	CachedBarline(const CachedBarline* other)
	{
		copyFrom(*other);
	}

	CachedBarline& operator=(const CachedBarline& other)
	{
		copyFrom(other);
		return *this;
	}


	void copyFrom(const CachedBarline& other)
	{
		id = other.id;
		parentId = other.parentId;
		childrenCount = other.childrenCount;
		if (childrenCount)
		{
			children.reset(new buint[childrenCount]);
			std::copy_n(other.children.get(), childrenCount, children.get());
		}
		startl = other.startl;
		endl = other.endl;
		matrix = other.matrix;
		depth = other.depth;
		root = other.root;
	}

	CachedBarline(CachedBarline&& other)
	{
		moveFrom(std::forward<CachedBarline>(other));
	}

	CachedBarline& operator=(CachedBarline&& other)
	{
		moveFrom(std::forward<CachedBarline>(other));
		return *this;
	}

	void moveFrom(CachedBarline&& other)
	{
		id = other.id;
		parentId = other.parentId;
		childrenCount = std::exchange(other.childrenCount, 0);
		children.reset(other.children.release());
		startl = other.startl;
		endl = other.endl;
		matrix = other.matrix;
		depth = other.depth;
		root = other.root;
	}

	CachedBarline(const IClassItem* item)
	{
		auto* t = dynamic_cast<const CachedBarline*>(item);
		if (t)
			copyFrom(*t);
		else
			assert(false);
	}

	CachedBarline(buint id, bc::barline* line, CachedBaritemHolder* root) : IClassItem()
	{
		update(id, line, root);
	}

	virtual ~CachedBarline()
	{}

	void update(buint id, bc::barline* line, CachedBaritemHolder* root)
	{
		this->id = id;
		parentId = -1;
		assert(line);
		startl = line->start;
		endl = line->end();
		//matrix = (int)line->getPointsSize();
		matrix = std::move(line->getMatrix());
		depth = (buchar)line->getDeath();
		// assert(startl != endl); // Allown for root

		this->root = root;
	}

	virtual size_t getId() const override
	{
		return id;
	}

	virtual size_t getParentId() const override
	{
		return parentId;
	}

	virtual int getDeath() const override
	{
		return (int)depth;
	}

	virtual Barscalar start() const override
	{
		return startl;
	}

	virtual Barscalar end() const override
	{
		return endl;
	}

	Barscalar length() const
	{
		return startl > endl ? endl - startl : startl - endl;
	}

	virtual const bc::barvector& getMatrix() const override
	{
		return matrix;
		//static bc::barvector dummy;
		//return dummy;
	}

	virtual const size_t getMatrixSize() const override
	{
		//return matrix;
		return matrix.size();
	}

	size_t getChildrenCount() const
	{
		return childrenCount;
	}

	CachedBarline* getChild(int id) const;
	CachedBarline* getParent() const;

	virtual void saveLoadState(StateBinFile::BinState* state) override
	{
		state->pType(startl.type); // Only set

		startl = state->pBarscalar(startl);
		endl = state->pBarscalar(endl);
		depth = state->pInt(depth);

		//matrix = state->pInt(matrix);
		int matrSize = matrix.size();
		matrSize = state->pInt(matrSize);
		state->beginArray(matrix, matrSize);
		for (size_t i = 0; i < matrSize; i++)
		{
			auto& barvalue = matrix[i];

			barvalue.x = state->pInt(barvalue.x);
			barvalue.y = state->pInt(barvalue.y);
			barvalue.value = state->pBarscalar(barvalue.value);
		}

		id = state->pInt(id);
		parentId = state->pInt(parentId);

		childrenCount = state->pShort(childrenCount);
		if (childrenCount > 0 && state->isReading())
			children.reset(new buint[childrenCount]);

		for (buint i = 0; i < childrenCount; i++)
		{
			children[i] = state->pInt(children[i]);
		}

	}
};


class CachedBaritemHolder : public IDataClassItemValueHolder<CachedBarline>
{
	using Base = IDataClassItemValueHolder<CachedBarline>;
	int root = 0;
public:
	void create(bc::DatagridProvider* img, const bc::barstruct& constr, const Base::ItemCallback& callback) override
	{
		bc::BarcodeCreator creator;
		std::unique_ptr<bc::Baritem> citem(creator.createBarcode(img, constr));

		auto* item = citem.get();
		int size = (int)item->barlines.size();

		if (item->getRootNode())
			root = item->getRootNode()->id;
		else
		{

			if (item->barlines.size() > 0)
			{
				auto lp = item->barlines[0];
				while (lp->parentId != UINT_MAX)
				{
					lp = item->barlines[lp->parentId];
				}
				root = lp->id;
			}
		}

		// item->relen();
		for (int i = 0; i < size; i++)
		{
			bc::barline* line = item->barlines[i];
			// std::cout << line->start.getAvgFloat() << " " << line->m_end.getAvgFloat() << std::endl;
			assert(line->id == i);

			Base::items.push_back({});
			CachedBarline& id = Base::items.back();

			id.update(i, line, this);
			id.parentId = line->parentId;
			assert(id.parentId != id.id);

			id.childrenCount = line->childrenId.size();
			if (id.childrenCount)
			{
				int k = 0;
				id.children.reset(new buint[id.childrenCount]);
				for (auto child : line->childrenId)
				{
					id.children[k++] = child;
				}
			}
			if (callback)
				callback(&id);
		}
	}


	//void addItem(const IClassItem& item)
	//{
	//	items.push_back(CachedBarline(&item));
	//	items.back().root = this;
	//}
	void addUpdateRoot(const CachedBarline& line)
	{
		CachedBarline nl(line);
		nl.root = this;
		Base::items.push_back(nl);
	}

	CachedBarline* getRItem(int cid)
	{
		return &Base::items[cid];
	}

	const CachedBarline* getRoot() const
	{
		return &Base::items[root];
	}

	virtual void saveLoadState(StateBinFile::BinState* state) override
	{
		// Begin
		root = state->pInt(root);

		int count = Base::items.size();

		count = state->pInt(count);
		state->beginArray(Base::items, count);
		for (int i = 0; i < count; ++i)
		{
			Base::items[i].saveLoadState(state);
			Base::items[i].root = this;
		}
	}
};