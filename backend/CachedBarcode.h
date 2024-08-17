
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

#include "../Bind/MHashMap.h"
#include "Interfaces/IItem.h"
// import BarcodeModule;
//import BackBind;
// import StateBinIO;


class BarlineWrapper : public IClassItem
{
	//static unsigned int idCounter;
public:
	Barscalar startl, endl;
	bc::barvector matrix;
	//int matrix;
	unsigned char depth = 0;
	int rwidth, rheight;

	BarlineWrapper() : IClassItem(),
		startl(uchar(0)), endl(uchar(0))
	{ }

	BarlineWrapper(const BarlineWrapper& other) = default;
	BarlineWrapper(BarlineWrapper&& other) noexcept = default;

	BarlineWrapper& operator=(const BarlineWrapper& other) = default;
	BarlineWrapper& operator=(BarlineWrapper&& other) noexcept = default;

	BarlineWrapper(bc::barline* line) : IClassItem()
	{
		update(line);
	}

	virtual ~BarlineWrapper()
	{}

	void update(bc::barline* line)
	{
		auto rect = line->getBarRect();
		rwidth = rect.width;
		rheight = rect.height;
		assert(line);
		startl = line->start;
		endl = line->end();
		//matrix = (int)line->getPointsSize();
		matrix = std::move(line->getMatrix());
		depth = (buchar)line->getDeath();
	}

	virtual size_t getId() const override
	{
		return -1;
	}

	virtual size_t getParentId() const override
	{
		return -1;
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

	virtual int width() const override
	{
		return rwidth;
	}

	virtual int height() const override
	{
		return rheight;
	}

	size_t getChildrenCount() const
	{
		return 0;
	}

	virtual void saveLoadState(StateBinFile::BinState* state) override
	{
		assert(false);
	}
};
