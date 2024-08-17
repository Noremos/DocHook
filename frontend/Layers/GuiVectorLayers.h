#pragma once
#include "../DrawCommon.h"
#include <vector>
#include <numeric>

#include "../../backend/CSBind.h"
#include "../Bind/Framework.h"
#include "GuiDataLayer.h"
#include "IGuiLayer.h"
#include "../GuiWidgets.h"
#include "../../backend/Layers/VectorLayers.h"

import GuiLayers;

// import Platform;
// import CSBind;

// Vector layer
template<class T>
class VectorBaseLayer : public GuiLayerData<T>
{
	using Base = GuiLayerData<T>;
	ImVec4 propColor;

	enum ChangeState
	{
		none,
		add,
		edit,
		remove
	} checkState = ChangeState::none;

	template<class E, E curval>
	class CheckSate
	{
		bool state = false;
		E& valRef;

	public:
		CheckSate(E& valRef) : valRef(valRef)
		{ }

		void draw(const char* name)
		{
			ImGui::Checkbox(name, &state);
			if (state)
			{
				valRef = curval;
			}
		}
	};

	CheckSate<ChangeState, ChangeState::add> addCB;
	CheckSate<ChangeState, ChangeState::edit> editCB;
	CheckSate<ChangeState, ChangeState::remove> removeCB;
	bool isInChangeMode() const
	{
		return checkState != ChangeState::none;
	}

	std::vector<size_t> orders;

public:
	int selectedId = -1;

	VectorBaseLayer(T* fromCore) : GuiLayerData<T>(fromCore),
		addCB(checkState), editCB(checkState), removeCB(checkState)
	{ }

	virtual ~VectorBaseLayer()
	{ }

	virtual void toGuiData()
	{
		Base::toGuiData();
		propColor.x = static_cast<float>(Base::data->color.r) / 255.f;
		propColor.y = static_cast<float>(Base::data->color.g) / 255.f;
		propColor.z = static_cast<float>(Base::data->color.b) / 255.f;

		orders.resize(Base::data->primitives.size());
		std::iota(orders.begin(), orders.end(), 0);

		std::vector<int> areas;
		areas.reserve(orders.size());

		for (const auto& prim : Base::data->primitives)
		{
			double area = 0;
			for (size_t i = 0; i < prim->points.size(); i++)
			{
				const auto& p1 = prim->points[i];
				const auto& p2 = prim->points[(i + 1) % prim->points.size()];
				area += (p1.x * p2.y - p2.x * p1.y);
			}
			areas.push_back(area);
		}


		if (Base::data->vecType == VectorLayer::VecType::polygons)
		{
			std::sort(orders.begin(), orders.end(), [&](size_t a, size_t b)
			{
				return areas[a] < areas[b];
			});
		}

		// if (Base::data->vecType == VectorLayer::VecType::polygons)
		// {
		// 	std::sort(orders.begin(), orders.end(), [&](size_t a, size_t b) {
		// 		return Base::data->primitives[a]->points.size() < Base::data->primitives[b]->points.size();
		// 	});
		// }
	}

	virtual void draw(const GuiDisplaySystem& ds)
	{
		if (!Base::data->visible)
			return;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
		 | ImGuiWindowFlags_NoScrollWithMouse;


		 ImGuiChildFlags child_flags = ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY;


		ImGui::SetCursorPos(ds.getDrawPos());
		if (!ImGui::BeginChild(Base::data->name.c_str(), ds.getDrawSize(), child_flags, window_flags))
		{
			ImGui::EndChild();
			return;
		}


		switch (Base::data->vecType)
		{
		case VectorLayer::VecType::points:
			drawPoints(ds);
			break;
		case VectorLayer::VecType::polygons:
			drawPolygons(ds);
			break;
		case VectorLayer::VecType::circles:
			drawCircles(ds);
			break;
		default:
			break;
		}

		ImGui::EndChild();
	}


	void drawPoints(const GuiDisplaySystem& ds)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		ImDrawList* list = window->DrawList;

		ImColor bigColor(128, 0, 255);
		ImColor midColor(220, 200, 0);
		float markerSize = 2;//MAX(1, par->displaySize.x / par->width);

		auto& prims = Base::data->primitives;
		if (prims.size() == 0)
			return;

		BackColor cscol = Base::data->color;
		ImColor col(cscol.r, cscol.g, cscol.b);
		ImColor cursorColor(255 - cscol.r, 255 - cscol.g, 255 - cscol.b);

		auto& cs = Base::data->cs;

		BackPoint itemSt = ds.getSysToItemStartPos(cs);
		BackPoint ed = ds.getSysToItemEndPos(cs);

		int i = 0;
		for (DrawPrimitive* d : prims)
		{
			if (!d->visible)
				continue;

			auto& points = d->points;
			float tickness = spotId == i ? 3 : 10;

			BackPoint p = points[0];
			if (GuiDisplaySystem::inRange(itemSt, ed, p))
			{
				ImVec2 pi = ds.projItemGlobToDisplay(cs, p);
				if (isInChangeMode() && d->isNearPoint(ds.cursorPos))
				{
					list->AddCircleFilled(pi, tickness, cursorColor);
				}
				else
					list->AddCircleFilled(pi, tickness, col);
			}

			++i;
		}
	}

	//bool areIntersecting(
	//	float v1x1, float v1y1, float v1x2, float v1y2,
	//	float v2x1, float v2y1, float v2x2, float v2y2)
	//{
	//	float d1, d2;
	//	float a1, a2, b1, b2, c1, c2;

	//	// Convert vector 1 to a line (line 1) of infinite length.
	//	// We want the line in linear equation standard form: A*x + B*y + C = 0
	//	// See: http://en.wikipedia.org/wiki/Linear_equation
	//	a1 = v1y2 - v1y1;
	//	b1 = v1x1 - v1x2;
	//	c1 = (v1x2 * v1y1) - (v1x1 * v1y2);

	//	// Every point (x,y), that solves the equation above, is on the line,
	//	// every point that does not solve it, is not. The equation will have a
	//	// positive result if it is on one side of the line and a negative one
	//	// if is on the other side of it. We insert (x1,y1) and (x2,y2) of vector
	//	// 2 into the equation above.
	//	d1 = (a1 * v2x1) + (b1 * v2y1) + c1;
	//	d2 = (a1 * v2x2) + (b1 * v2y2) + c1;

	//	// If d1 and d2 both have the same sign, they are both on the same side
	//	// of our line 1 and in that case no intersection is possible. Careful,
	//	// 0 is a special case, that's why we don't test ">=" and "<=",
	//	// but "<" and ">".
	//	if (d1 > 0 && d2 > 0) return false;
	//	if (d1 < 0 && d2 < 0) return false;

	//	// The fact that vector 2 intersected the infinite line 1 above doesn't
	//	// mean it also intersects the vector 1. Vector 1 is only a subset of that
	//	// infinite line 1, so it may have intersected that line before the vector
	//	// started or after it ended. To know for sure, we have to repeat the
	//	// the same test the other way round. We start by calculating the
	//	// infinite line 2 in linear equation standard form.
	//	a2 = v2y2 - v2y1;
	//	b2 = v2x1 - v2x2;
	//	c2 = (v2x2 * v2y1) - (v2x1 * v2y2);

	//	// Calculate d1 and d2 again, this time using points of vector 1.
	//	d1 = (a2 * v1x1) + (b2 * v1y1) + c2;
	//	d2 = (a2 * v1x2) + (b2 * v1y2) + c2;

	//	// Again, if both have the same sign (and neither one is 0),
	//	// no intersection is possible.
	//	if (d1 > 0 && d2 > 0) return false;
	//	if (d1 < 0 && d2 < 0) return false;

	//	// If we get here, only two possibilities are left. Either the two
	//	// vectors intersect in exactly one point or they are collinear, which
	//	// means they intersect in any number of points from zero to infinite.
	//	if ((a1 * b2) - (a2 * b1) == 0.0f) return true;

	//	// If they are not collinear, they must intersect in exactly one point.
	//	return false;
	//}

	bool drawPolygon(const DrawPrimitive* d, const GuiDisplaySystem& ds, float tick = 3.0)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		ImDrawList* list = window->DrawList;
		ImVec2 offset = window->Pos;

		const CSBinding& dsc = Base::data->cs;
		const BackPoint start = ds.getSysToItemStartPos(dsc);
		const BackPoint end = ds.getSysToItemEndPos(dsc);

		const std::vector<BackPoint>& points = d->points;
		auto cscol = d->color;
		// ImU32 col = ImColor(cscol.r, cscol.g, cscol.b, 200);
		ImU32 col = ImColor(cscol.r, cscol.g, cscol.b, 255);
		ImU32 colRev = ImColor(255 - cscol.r, 255 - cscol.g, 255 - cscol.b);

		if (points.size() < 3)
			return false;

		BackPoint point = ds.cursorPos;
		ImU32 cursorColor = ImColor(255 - cscol.r, 255 - cscol.g, 255 - cscol.b);

		int pointsVisible = 0;
		bool inside = false;
		std::vector<ImVec2> displayPoints;
		int n = (int)points.size();
		for (int i = 0, j = n - 1; i < n; j = i++)
		{
			const BackPoint& a = points[i];
			const BackPoint& c = points[j];

			if (((a.y > point.y) != (c.y > point.y)) &&
				(point.x < (c.x - a.x) * (point.y - a.y) / (c.y - a.y) + a.x))
			{
				inside = !inside;
			}


			if (GuiDisplaySystem::inRange(start, end, a))
			{
				++pointsVisible;
			}

			ImVec2 pi = ds.projItemGlobToDisplay(dsc, a) + offset;
			displayPoints.push_back(pi);
			 //if (inside) // Spot on cursor
			 //	list->AddCircleFilled(pi, 3, cursorColor);
		}

		if (pointsVisible >= 1)
		{
			displayPoints.push_back(displayPoints[0]);

			ImVec2 pi = ds.projItemGlobToDisplay(dsc, points[0]) + offset; // First
			// list->AddConvexPolyFilled(displayPoints.data(), displayPoints.size(), col);
			// list->AddPolyline(displayPoints.data(), displayPoints.size(), colbl, 0, 2.0);
			list->AddPolyline(displayPoints.data(), displayPoints.size(), col, 0, tick);

			if (inside)
			{
				list->AddPolyline(displayPoints.data(), displayPoints.size(), colRev, 0, tick * 1.3);
			}
		}
		displayPoints.clear();
		return pointsVisible >= 1 && inside;
	}

	int spotId = -1;
	void drawPolygons(const GuiDisplaySystem& ds)
	{
		// auto start = ds.getDisplayStartPos();
		// auto end = ds.getDisplayEndPos();

		ImU32 colbl = ImColor(255, 255, 255);

		BackColor cscol = Base::data->color;
		selectedId = -1;
		for (size_t& io : orders)
		{
			const DrawPrimitive* d = Base::data->primitives[io];
			if (!d->visible)
				continue;

			if (drawPolygon(d, ds, io == spotId ? 10.0 : 3.0))
			{
				if (selectedId == -1)
					selectedId = io;
			}
		}
	}

	void drawCircles(const GuiDisplaySystem& ds)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		ImDrawList* list = window->DrawList;
		ImVec2 offset = window->Pos;


		CSBinding& dsc = Base::data->cs;
		const BackPoint start = ds.getSysToItemStartPos(dsc);
		const BackPoint end = ds.getSysToItemEndPos(dsc);

		BackColor cscol = Base::data->color;
		for (const DrawPrimitive* d : Base::data->primitives)
		{
			if (!d->visible)
				continue;

			const std::vector<BackPoint>& points = d->points;
			if (points.size() == 0)
				return;

			assert(points.size() == 2);
			{
				BackPoint rad(points[1].x - points[0].x, points[1].y - points[1].y);

				std::vector<BackPoint> inCheck;
				inCheck.push_back(points[0]);
				inCheck.push_back(BackPoint(points[0].x - rad.x, points[0].y - rad.y));
				inCheck.push_back(BackPoint(points[0].x - rad.x, points[0].y + rad.y));
				inCheck.push_back(BackPoint(points[0].x + rad.x, points[0].y - rad.y));
				inCheck.push_back(points[1]);

				bool inDraw = false;
				for (int i = 0; i < inCheck.size(); ++i)
				{
					const BackPoint& p = inCheck[i];
					if (GuiDisplaySystem::inRange(start, end, p))
					{
						inDraw = true;
					}
				}

				if (!inDraw)
					continue;
			}

			// ImU32 col = ImColor(cscol.r, cscol.g, cscol.b, 200);
			ImU32 col = ImColor(cscol.r, cscol.g, cscol.b, 255);
			ImVec2 pi = ds.projItemGlobToDisplay(dsc, points[0]) + offset; // First
			ImVec2 radDisplay = ds.projItemGlobToDisplay(dsc, points[1]) + offset; // Radius

			const float rX = radDisplay.x - pi.x;
			const float rY = radDisplay.y - pi.y;
			list->AddCircle(pi, rX > rY ? rX : rY, col, 0, 5);
		}
	}

	void drawToolboxInner(ILayerWorker& context)
	{
	}


	void drawPointsToolbox(ILayerWorker& context)
	{
	}

	void drawPolygonToolbox(ILayerWorker& context)
	{}

	virtual void drawProperty()
	{
		Base::drawProperty();
		ImGui::Separator();

		ImGui::ColorPicker4("Color", (float*)&propColor, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs);
	}

	virtual void applyPropertyChanges()
	{
		Base::applyPropertyChanges();
		ImGui::Separator();

		Base::data->color.r = std::min(static_cast<int>(propColor.x * 255), 255);
		Base::data->color.g = std::min(static_cast<int>(propColor.y * 255), 255);
		Base::data->color.b = std::min(static_cast<int>(propColor.z * 255), 255);

		//if (applyForPrimitives)
		{
			for (DrawPrimitive* d : Base::data->primitives)
			{
				d->color = Base::data->color;
			}
		}
	}
};



class VectorGuiLayer : public VectorBaseLayer<VectorLayer>
{
public:
	VectorGuiLayer(VectorLayer* fromCore) : VectorBaseLayer<VectorLayer>(fromCore)
	{ }

	static void getRect(const std::vector<BackPoint>& points, ImVec2& itemSt, ImVec2& end)
	{
		itemSt = end = toIV(points[0]);
		for (const BackPoint& p : points)
		{
			if (p.x < itemSt.x)
			{
				itemSt.x = p.x;
			}

			if (p.y < itemSt.y)
			{
				itemSt.y = p.y;
			}

			if (p.x > end.x)
			{
				end.x = p.x;
			}

			if (p.y < end.y)
			{
				end.y = p.y;
			}
		}
	}

};
