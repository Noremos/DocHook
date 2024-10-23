
#pragma once
#include <cmath>
#include "DrawCommon.h"
#include "Barcode/PrjBarlib/include/barscalar.h"

#include "../backend/MatrImg.h"
#include "../backend/Layers/layerInterface.h"
#include "GuiWidgets.h"
// module GuiOverlap;

//import BackBind;
// import MatrModule;
// import LayersCore;
// import BarScalarModule;

struct vec3
{
	short x, y, z;

	vec3(const Barscalar& col)
	{
		x = col.data.b3[0];
		y = col.data.b3[1];
		z = col.data.b3[2];
	}

	vec3(short _x, short _y, short _z) : x(_x), y(_y), z(_z)
	{ }

	vec3 operator+(const vec3& b) const
	{
		return vec3(x + b.x, y + b.y, z + b.z);
	}

	vec3 operator-(const vec3& b) const
	{
		return vec3(x - b.x, y - b.y, z - b.z);
	}

	vec3 operator*(float v) const
	{
		return vec3(x * v, y * v, z * v);
	}

	Barscalar getSc()
	{
		assert(x >= 0);
		assert(x < 256);
		assert(y >= 0);
		assert(y < 256);
		assert(z >= 0);
		assert(z < 256);
		return Barscalar(x, y, z);
	}
	float length()
	{
		return sqrtf(x * x + y * y + z * z);
	}
};


class IOverlapProcess
{
public:
	bool enable = true;
	//virtual void init() = 0;
	virtual void draw(ImVec2 pos, ImVec2 size) = 0;
};

class HeimapOverlap : public IOverlapProcess
{
	GuiDrawImage heimap;
	bool inited = false;
public:

	void init(const BackImage& mainMat)
	{
		Barscalar mi, ma;
		mainMat.maxAndMin(mi, ma);

		vec3 mixc{ mi.data.b3[0], mi.data.b3[1], mi.data.b3[2] };
		vec3 maxc{ ma.data.b3[0], ma.data.b3[1], ma.data.b3[2] };
		vec3 cDiff = maxc - mixc;

		vec3 minHeiCol{ 0, 0, 255 };
		vec3 maxHeiCol{ 255, 0, 0 };
		vec3 colDiff = maxHeiCol - minHeiCol;

		BackImage heim(mainMat.width(), mainMat.height(), 3);
		for (size_t i = 0; i < mainMat.length(); i++)
		{
			vec3 col = mainMat.getLiner(i);
			float t = (col - mixc).length() / cDiff.length();
			vec3 newCol = minHeiCol + colDiff * t;
			heim.setLiner(i, newCol.getSc());
		}

		heimap.setImage(heim);
		inited = true;
	}
	bool isInit()
	{
		return inited;
	}

	virtual void draw(ImVec2 pos, ImVec2 size)
	{
		if (!enable || !inited)
			return;
		//auto p = ds.projItemGlobToLoc(ds.sysCs, ds.pos);
		//auto ps = ds.projItemGlobToLoc(ds.sysCs, ds.pos + ds.size);
		//heimap.drawImage("Heimap", ImVec2(p.x, p.y), ImVec2(ps.x, ps.y));
	}
};


class TilemapOverlap : public IOverlapProcess
{
	GuiDrawImage* img;
	const LayerProvider* lprov;

public:
	void init(GuiDrawImage* src, const LayerProvider* prov)
	{
		img = src;
		lprov = prov;
	}

	virtual void draw(ImVec2 pos, ImVec2 size)
	{
		const LayerProvider& prov = *lprov;
		if (prov.tileSize == 0 || !enable)
			return;

		auto window_flags = ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollbar;
		ImGui::SetCursorPos(pos);
		if (!ImGui::BeginChild("Tilemap", size, false, window_flags))
		{
			ImGui::EndChild();
			return;
		}
		// ImGui::SetCursorPos(cont);
		ImColor lineColor(115, 115, 115);

		ImDrawList* list = ImGui::GetWindowDrawList();
		ImVec2 cont = ImGui::GetCurrentWindow()->Pos + pos;

		float tfc = prov.tileSize / img->scaleFactor;
		ImVec2 drawTileSize(tfc, tfc);

		float newWid = size.x;
		float newHei = size.y;

		ImVec2 tst(0, cont.y);
		ImVec2 ted(0, cont.y + newHei);
		for (TileIterator twid(cont.x, drawTileSize.x, 0, cont.x + newWid); twid.notEnded(); twid.accum())
		{
			tst.x = twid.pos();
			ted.x = twid.pos();

			list->AddLine(tst, ted, lineColor, 3);
		}

		tst.x = cont.x + newWid;
		ted.x = cont.x + newWid;
		list->AddLine(tst, ted, lineColor, 3);


		tst.x = cont.x;
		ted.x = cont.x + newWid;
		for (TileIterator thei(cont.y, drawTileSize.y, 0, cont.y + newHei); thei.notEnded(); thei.accum())
		{
			tst.y = thei.pos();
			ted.y = thei.pos();
			list->AddLine(tst, ted, lineColor, 3);
		}

		tst.y = cont.y + newHei;
		ted.y = cont.y + newHei;
		list->AddLine(tst, ted, lineColor, 3);

		ImGui::EndChild();
	}
};
