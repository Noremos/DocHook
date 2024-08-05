module;
#include <memory>
#include <unordered_set>
#include <algorithm>
#include <cassert>
#include "Common.h"


#include "../MetadataIO.h"
#include "../../side/Barcode/PrjBarlib/modules/StateBinFile.h"
#include "../Bind/Framework.h"
#include "layerInterface.h"
export module VectorLayers;
//import std.core;
//import BackBind;
// import LayersCore;

// import Platform;

// import MetadataCoreIO;
// import StateBinIO;
// import ClusterInterface;

using LayerMetaProvider = MetadataProvider;


template<class FR>
static void saveLoadChar(JsonObjectIOState* state, const char* name, FR& val)
{
	int valInt = static_cast<int>(val);
	state->scInt(name, valInt);
	val = static_cast<FR>(valInt);
}


export class DrawPrimitive
{
public:
	int id;
	BackColor color;
	std::vector<BackPoint> points;

	DrawPrimitive(int id, const BackColor& bp = BackColor()) : id(id), color(bp)
	{ }

	void addPoint(BackPoint p)
	{
		points.push_back(p);
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

export class VectorLayer : public IVectorLayer
{
	int binId = -1;

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

	void savePolygonsAsGeojson(const BackPathStr& savePath) const
	{
		BackString json = "{\"type\":\"FeatureCollection\",";
		json += "\"name\":\"";
		json += name;
		json += "\",";
		json += "\"crs\": { \"type\": \"name\", \"properties\":{\"name\": \"urn:ogc:def:crs:EPSG::";
		json += intToStr(cs.getProjId()); //3857
		json += "\" } },";
		json +=  "\"features\":[ ";

		for (int i = 0, total = primitives.size(); i < total; ++i)
		{
			BackString safsd = "{ \"type\": \"Feature\",";
			safsd += "\"properties\":{\"id\":"; // TODO! Check
			safsd += intToStr(i + 1);
			safsd += "}, ";

			switch (vecType)
			{
			case VecType::points:
				safsd += primitives[i]->pointsAsGeojson();
				break;
			case VecType::polygons:
				safsd += primitives[i]->polygonAsGeojson();
				break;
			default:
				assert(false);
				throw;
			}
			safsd += "},";

			json += safsd;
		}


		json[json.length() - 1] = ']';
		json += "}";

		WriteFile(savePath, json);
	}

	virtual void saveLoadState(JsonObjectIOState* state, const MetadataProvider& metaFolder) override
	{
		IVectorLayer::saveLoadState(state, metaFolder);

		saveLoadChar(state, "vectype", vecType);
		saveLoadChar(state, "col_r", color.r);
		saveLoadChar(state, "col_g", color.g);
		saveLoadChar(state, "col_b", color.b);

		// int imgId = metaFolder.getUniqueId();
		// state->scInt("mat_id", imgId);
		int size = primitives.size();
		state->scInt("primitivesSize", size);


		if (state->isReading())
		{
			state->scInt("binId", binId);

			StateBinFile::BinStateReader binstate;
			binstate.open(metaFolder.getFile(intToStr(binId)).string());

			primitives.resize(size);
			for (int i = 0; i < size; i++)
			{
				primitives[i] = new DrawPrimitive(i);
				primitives[i]->saveLoadState(&binstate);
			}
			binstate.close();
		}
		else
		{
			if (binId == -1)
			{
				binId = metaFolder.getUniqueId();
			}
			state->scInt("binId", binId);

			StateBinFile::BinStateWriter binstate;
			binstate.open(metaFolder.getFile(intToStr(binId)).string());
			for (int i = 0; i < size; i++)
			{
				primitives[i]->saveLoadState(&binstate);
			}
			binstate.close();
		}
	}
};

export class MultiVectorLayer : public IVectorLayer
{
public:
	std::vector<DrawPrimitive> primitives;
	virtual const LFID getFactoryId() const
	{
		return MULTIPOLY_VECTOR_LAYER_FID;
	}
};