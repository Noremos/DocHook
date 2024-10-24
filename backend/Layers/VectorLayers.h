#pragma once
#include <memory>
#include <unordered_set>
#include <algorithm>
#include <cassert>
#include "Common.h"


#include "../MetadataIO.h"
#include "../../side/Barcode/PrjBarlib/modules/StateBinFile.h"
#include "../Bind/Framework.h"
#include "layerInterface.h"
//import std.core;
//import BackBind;
// import LayersCore;

// import Platform;

// import MetadataCoreIO;
// import StateBinIO;
// import ClusterInterface;

using LayerMetaProvider = MetadataProvider;


template<class FR>
inline void saveLoadChar(JsonObjectIOState* state, const char* name, FR& val)
{
	int valInt = static_cast<int>(val);
	state->scInt(name, valInt);
	val = static_cast<FR>(valInt);
}


class DrawPrimitive
{
public:
	int id;
	BackColor color;
	std::vector<BackPoint> points;
	bool visible = true;

	DrawPrimitive(int id, const BackColor& bp = BackColor()) : id(id), color(bp)
	{ }

	void addPoint(BackPoint p)
	{
		points.push_back(p);
	}

	void addPoint(double x, double y)
	{
		points.push_back({x,y});
	}


	void setCircle(BackPoint p, BackPoint r)
	{
		points.push_back(p);
		points.push_back(r);
	}

	void clear()
	{
		points.clear();
	}

	// Function to check if a point is inside a polygon
	bool isNearPoint(const BackPoint& point)
	{
		const BackPoint& curp = points[0];
		return (curp.x - 2 < point.x && point.x < curp.x + 2 &&
				curp.y - 2 < point.y && point.y < curp.y + 2);
	}

	// Function to check if a point is inside a polygon
	bool isInsidePolygon(const BackPoint& point)
	{
		int n = points.size();
		bool inside = false;
		for (int i = 0, j = n - 1; i < n; j = i++)
		{
			if (((points[i].y > point.y) != (points[j].y > point.y)) &&
				(point.x < (points[j].x - points[i].x) * (point.y - points[i].y) / (points[j].y - points[i].y) + points[i].x))
			{
				inside = !inside;
			}
		}

		return inside;
	}

	//IClassItemHolder *load(int &index, IClassItemHolder *t)
	//{
	//	assert(state->isReading());

	//	index = state->pInt(0);
	//	t->read(state.get());
	//	return t;
	//}
	BackString pointsAsGeojson() const
	{
		BackString safsd = "\"geometry\": {\"type\":\"Point\", \"coordinates\":";
		safsd += "[";
		safsd += intToStr(points[0].x);
		safsd += ",";
		safsd += intToStr(points[0].y);
		safsd += "]}";
		return safsd;
	}

	BackString polygonAsGeojson() const
	{
		BackString safsd = "\"geometry\": {\"type\":\"Polygon\", \"coordinates\":[[ ";

		asGeojsonCommon(safsd);
		safsd += "]]}";

		return safsd;
	}

	void asGeojsonCommon(BackString& prefix) const
	{
		for (const BackPoint& p : points)
		{
			// prefix += std::format("[{},{}],", p.x, p.y);
			prefix += "[";
			prefix += intToStr(p.x);
			prefix += ",";
			prefix += intToStr(p.y);
			prefix += "],";
		}
		prefix[prefix.length() - 1] = ' ';
	}

	void saveLoadState(StateBinFile::BinState* state)
	{
		state->beginItem();

		color.r = (buchar)state->pInt(color.r);
		color.g = (buchar)state->pInt(color.g);
		color.b = (buchar)state->pInt(color.b);

		int size = state->pInt(points.size());
		if (state->isReading())
		{
			points.resize(size);
		}

		for (int i = 0; i < size; i++)
		{
			points[i].x = state->pFloat(points[i].x);
			points[i].y = state->pFloat(points[i].y);
		}

		state->endItem();
	}
};

class VectorLayer : public IVectorLayer
{
public:
	enum class VecType : short
	{
		points,
		polygons,
		circles,
		multipolygons
	};

	VecType vecType;
	BackColor color;
	std::vector<DrawPrimitive*> primitives;

	void clear()
	{
		for (size_t i = 0; i < primitives.size(); i++)
		{
			delete primitives[i];
		}

		primitives.clear();
	}

	virtual int realWidth() const override
	{
		return 0;
	}
	virtual int realHeight() const override
	{
		return 0;
	}

	~VectorLayer()
	{
		clear();
	}

	DrawPrimitive* addPrimitive(const BackColor& col)
	{
		primitives.push_back(new DrawPrimitive(primitives.size(), col));
		return primitives.back();
	}

	virtual const LFID getFactoryId() const override
	{
		return VECTOR_LAYER_FID;
	}
};
