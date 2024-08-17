#pragma once
#include "DrawCommon.h"

#include <algorithm>
#include <limits>
#include "Barcode/PrjBarlib/include/barstrucs.h"


#include "../backend/CSBind.h"
#include "../backend/project.h"
#include "DrawUtils.h"

//import BackBind;
// import CSBind;


// struct CDDisplayPS
// {
// 	WindowVec2 csPos;
// 	ImVec2 csSize;
// };

// using LocalDisplayPS = CDDisplayPS;
// using GlobalDisplayPS = CDDisplayPS;


/*
	BackPoint pix = cs.toGlobal(p.getX(), p.getY());

	auto start = ds.getDisplayStartPos();
	auto end = ds.getDisplayEndPos();

	ImVec2 pi = ds.projItemGlobToDisplay(cs, pix);
	if (GuiDisplaySystem::inRange(start, end, pi))
	{
		pi += win->Pos + (pixelSize / 2);
		// do
	}
*/


template<class T>
struct SelectableKeyValues
{
	int currentIndex = 0;

	SelectableKeyValues() : size(0)
	{ }

	SelectableKeyValues(std::initializer_list<std::pair<T, const char*>> l)
	{
		size = l.size();
		items.resize(size);
		values.resize(size);
		int counter = 0;
		for (const auto& kv : l)
		{
			values[counter] = kv.first;
			items[counter] = kv.second;
			++counter;
		}
	}

	void add(const BackString& name, T value)
	{
		holder.push_back(name);
		values.push_back(value);
		++size;
	}

	void remove(int id)
	{
		holder.erase(holder.begin() + id);
		values.erase(values.begin() + id);
		--size;
		endAdding();

		if (currentIndex >= size)
		{
			--currentIndex;
		}
	}

	void updateName(int index, const BackString& name)
	{
		holder[index] = name;
		items[index] = holder[index].c_str();
	}

	void endAdding()
	{
		items.clear();
		for (size_t i = 0; i < holder.size(); i++)
		{
			items.push_back(holder[i].c_str());
		}
	}

	void clear()
	{
		size = 0;
		holder.clear();
		values.clear();
	}

	const char** getItems()
	{
		return items.data();
	}

	T& back()
	{
		return values.back();
	}

	T& currentValue()
	{
		return values[currentIndex];
	}

	const BackString& currentName()
	{
		return holder[currentIndex];
	}

	void drawCombobox(const char* name, int width = 200)
	{
		int oldId = currentIndex;
		const char** itemsl = getItems();
		ImGui::SetNextItemWidth(static_cast<float>(width));
		ImGui::Combo(name, &currentIndex, itemsl, size); //IM_ARRAYSIZE(items)

		hasChange = oldId != currentIndex;
	}

	void drawListBox(const char* name, int width = 200)
	{
		int oldId = currentIndex;
		const char** itemsl = getItems();
		ImGui::SetNextItemWidth(width);
		ImGui::ListBox(name, &currentIndex, itemsl, size); //IM_ARRAYSIZE(items)

		hasChange = oldId != currentIndex;
	}

	T* getValuesIterator()
	{
		return values.data();
	}

	int getSize()
	{
		return size;
	}

	bool hasChanged()
	{
		return hasChange;
	}

private:
	std::vector<const char*> items;
	std::vector<T> values;
	std::vector<BackString> holder;
	int size;
	bool hasChange;
};


class GuiDrawInfo
{
	ImVec2 csPos;
	ImVec2 size;
};

class GuiResizableContainer
{
	ImVec2 zoom = ImVec2(1, 1);
public:
	ImVec2 displaySize;
	WindowVec2 offset = ImVec2(0, 0);
	ItemVec2 clickedPos = ImVec2(0, 0);

	float getZoom()
	{
		return zoom.x;
	}

	bool clicked = false;

	bool floatable = false;
	bool good;
	bool open = false;
	bool Begin(const char* name)
	{
		floatable = true;
		if (open)
		{
			auto window_flags = ImGuiWindowFlags_HorizontalScrollbar;
			good = ImGui::Begin(name, &open, window_flags);
		}
		else
			good = false;

		return good;
	}

	bool BeginAsChild(const char* name)
	{
		floatable = false;
		auto window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_HorizontalScrollbar;

		good = ImGui::Begin(name, &open, window_flags);
		return good;
	}

	void end(ImVec2, ImVec2 realSize)
	{
		if (!good)
			return;

		ImVec2 wpos = ImGui::GetCurrentWindow()->Pos;
		const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);

		auto& io = ImGui::GetIO();
		clicked = false;
		bool drgged = false;
		bool wheeled = false;
		if (hovered)
		{
			drgged = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
			clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
			wheeled = io.MouseWheel != 0;
		}
		if (clicked)
		{
			clickedPos = io.MousePos - wpos + offset;
		}

		offset.x = ImGui::GetScrollX();
		offset.y = ImGui::GetScrollY();

		if (drgged && !floatable)
		{
			offset.x -= io.MouseDelta.x;
			offset.y -= io.MouseDelta.y;
		}
		if (wheeled)
		{
			float ads = io.MouseWheel * 0.1f;
			zoom.x += ads;
			zoom.y += ads;
			zoom.x = zoom.x > 0.1f ? zoom.x : 0.1f;
			zoom.y = zoom.y > 0.1f ? zoom.y : 0.1f;

			offset.x += abs(ads) * 20;
			offset.y += abs(ads) * 20;
		}


		ImVec2 nsize = ImVec2((float)realSize.x * zoom.x, (float)realSize.y * zoom.y);
		displaySize = nsize;

		if (!floatable)
		{
			ImGui::SetScrollX(offset.x);
			ImGui::SetScrollY(offset.y);
		}

		ImGui::End();
	}
};

// Coord system displayer
class GuiCSDisplayContainer
{
public:

	BackPoint currentPos;
	BackPoint clickedPos;

	GuiCSDisplayContainer()
	{

	}

	bool clicked = false;

	bool good;
	bool Begin(const char* name)
	{
		auto window_flags = ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDocking;

		if (!ImGui::Begin(name, NULL, window_flags))
		{
			good = false;
		}
		else
			good = true;

		return good;
	}

	void end(ImVec2, ImVec2 realSize)
	{
		if (!good)
			return;

		clicked = false;
		const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
		if (hovered)
		{
			Project* proj = Project::getProject();

			auto* win = ImGui::GetCurrentWindow();
			auto& pds = proj->getDisplay();
			// auto wpos = pds.csPos;

			auto& io = ImGui::GetIO();
			auto mp = io.MousePos - win->Pos;
			currentPos = pds.toSysGlob(BackPoint(mp.x, mp.y));

			clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
			if (clicked)
			{
				clickedPos = currentPos;
			}


			// BackPoint& offset = pds.csPos;
			// if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) // Drag
			// {
			// 	offset = offset - toBP(io.MouseDelta) / pds.csScale;
			// }

			// if (io.MouseWheel != 0) // Wheeled
			// {
			// 	float ads = io.MouseWheel * 0.1f;
			// 	// zoom.x += ads;
			// 	// zoom.y += ads;
			// 	// zoom.x = zoom.x > 0.1f ? zoom.x : 0.1f;
			// 	// zoom.y = zoom.y > 0.1f ? zoom.y : 0.1f;
			// 	BackPoint proc = toBP(mp / realSize);
			// 	assert(proc.x <= 1.0);
			// 	assert(proc.y <= 1.0);

			// 	auto displayPrev = realSize / pds.csScale;

			// 	pds.csScale += ads;
			// 	if (pds.csScale <= 0)
			// 	{
			// 		pds.csScale = 0.000001;
			// 	}

			// 	auto displayAfter = realSize / pds.csScale;

			// 	BackPoint displOff = toBP(displayPrev - displayAfter);
			// 	// auto displOff = toBP(realSize / ads);

			// 	offset = offset + displOff * proc;
			// }
		}


		ImGui::End();
	}
};


class GuiDrawImage : public GuiImage
{
public:
	WindowVec2 localDisplayPos;

	ImVec2 displaysBegin;
	ImVec2 displaySize;
	float opacity = 1.f;

	BackPixelPoint getSize()
	{
		return BackPixelPoint(width, height);
	}

	virtual ~GuiDrawImage()
	{ }

	GuiImage& operator=(const GuiImage& other) noexcept = delete;
	GuiImage& operator=(GuiImage&& other) noexcept = delete;


	// int getRealX(int x)
	// {
	// 	return static_cast<float>(x - localDisplayPos.x) * (width / displaySize.x);
	// }
	// int getRealY(int y)
	// {
	// 	return static_cast<float>(y - localDisplayPos.y) * (height / displaySize.y);
	// }

	// int getDisplayX(int x)
	// {
	// 	return static_cast<float>(x) * (displaySize.x / width) + localDisplayPos.x;
	// }
	// int getDisplayY(int y)
	// {
	// 	return static_cast<float>(y) * (displaySize.y / height) + localDisplayPos.y;
	// }

	void drawImage(const char* name, ImVec2 lpos, ImVec2 lsize)
	{
		auto window_flags = ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollWithMouse;

		ImGui::SetCursorPos(lpos);
		if (ImGui::BeginChild(name, lsize, false, window_flags))
		{
			drawTexture(lpos, lsize, ImVec2(0,0), ImVec2(width, height));
		}
		ImGui::EndChild();
	}

	void drawImage(const char* name, ImVec2 lpos, ImVec2 lsize, ImVec2 displayStart, ImVec2 displayEnd)
	{
		auto window_flags = ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollWithMouse;

		ImGui::SetCursorPos(lpos);
		if (ImGui::BeginChild(name, lsize, false, window_flags))
		{
			drawTexture(lpos, lsize, displayStart, displayEnd);
		}
		ImGui::EndChild();
	}

private:
	void drawTexture(ImVec2 winPos, ImVec2 winSize, ImVec2 displayStart, ImVec2 displayEnd)
	{
		if (displayEnd.x < 0 || displayEnd.y < 0)
			return;

		displaysBegin = displayStart;

		ImVec2 winEnd = winSize + winPos;
		if (displayStart.x > winEnd.x || displayStart.y > winEnd.y)
			return;

		if (displayEnd.x < winPos.x || displayEnd.y < winPos.y)
			return;

		auto textId = getTextureId();
		ImDrawList* list = ImGui::GetWindowDrawList();
		if (textId == 0)
		{
			// localDisplayPos.x = 5;
			// localDisplayPos.y = 5;
			// ImVec2 maxPos = csPos;
			// maxPos.x += csSize.x - 10;
			// maxPos.y += csSize.y - 30;

			if (displayEnd.x > winSize.x)
			{
				displayEnd.x = winPos.x + winSize.x;
			}

			if (displayEnd.y > winSize.y)
			{
				displayEnd.y = winPos.y + winSize.y;
			}

			ImVec2 minPos = winPos + displaysBegin;
			ImVec2 maxPos = winPos + displayEnd;


			list->AddRectFilled(minPos, maxPos, ImColor(115, 140, 153));
			//ImGui::Image((void*)0, csSize);
			return;
		}

		displaySize = displayEnd - displayStart;
		// ImVec2 maxSize = displayEnd - displaysBegin;
		// ResizeImage(displaySize, maxSize); // Resize image by available display size

		// this->scaleFactor = width / static_cast<float>(displaySize.x);
		// displaySize.x = newWid;
		// displaySize.y = newHei;

		// localDisplayPos.x = (csSize.x - newWid) / 2;
		// localDisplayPos.y = (csSize.y - newHei) / 2;

		// auto startUV = (winSize / displayStart) / winEnd;
		// auto startUV = ImMin(winSize / displayStart, ImVec2(1.f, 1.f));

		// auto dsrV = [](float winSize, float textSize)
		// {
		// 	if (textSize <= winSize)
		// 	{
		// 		return 1.f;
		// 	}

		// 	return winSize / textSize;
		// };
		// auto endUV = ImVec2(dsrV(winSize.x,  displaySize.x), dsrV(displaySize.y, displaySize.y)); // ImMin(winSize / displaySize, ImVec2(1.f, 1.f));

		// // Normalized coordinates of pixel (10,10) in a 256x256 texture.
		// ImVec2 uv0 = ImVec2(10.0f/256.0f, 10.0f/256.0f);
		// // Normalized coordinates of pixel (110,210) in a 256x256 texture.
		// ImVec2 uv1 = ImVec2((10.0f+100.0f)/256.0f, (10.0f+200.0f)/256.0f);

		ImGui::SetCursorPos(displaysBegin);
		ImVec4 color_multipler(1, 1, 1, opacity);
		ImGui::Image((void*)(intptr_t)textId, displaySize, ImVec2(0.f,0.f), ImVec2(1.0f, 1.0f), color_multipler);
		// list->AddImage((void*)(intptr_t)textId, displayStart + winPos, displayEnd + winPos);
	}
};


class GuiTilePreview
{
	int getAddnl(int main, int minor)
	{
		//  (ImageWidth + TileWidth - 1) / TileWidth
		//	return (main / minor) + (main % minor == 0 ? 0 : 1);
		return (main + minor - 1) / minor;
	}
public:
	void draw(int tileSize, int offset, ImVec2 imgSize)
	{
		//ImGui::ShowDemoWindow();
		// ImGui::ShowMetricsWindow();

		// ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
		// window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

		ImGuiWindow* win = ImGui::GetCurrentWindow();
		ImDrawList* list = win->DrawList;

		//if (ImGui::BeginChild(parentId))
		{
			int y = getAddnl(imgSize.x, tileSize) * getAddnl(static_cast<int>(imgSize.y), tileSize);
			ImGui::Text("The image will be splitted into %d tiles", y);

			ImVec2 localStartPos = ImGui::GetCursorPos();
			//startPos.x += 5;
			//startPos.y += 5;
			ImVec2 maxPos;
			maxPos.x = win->Size.x;
			maxPos.y = 200;
			int drawWid = imgSize.x, drawHei = static_cast<int>(imgSize.y);
			ResizeImage(drawWid, drawHei, maxPos.x, maxPos.y);
			float scale = imgSize.x / drawWid;

			// Draw List use the coores from top beootm corner of the BASE window. So add the modal wins coords
			ImVec2 absolutePos = win->Pos /* + win->Scroll*/ + localStartPos;
			list->AddRectFilled(absolutePos, ImVec2(absolutePos.x + drawWid, absolutePos.y + drawHei), ImColor(150, 150, 150));

			ImVec2 tileWithOffSize((tileSize + offset) / scale, (tileSize + offset) / scale);
			list->AddRectFilled(absolutePos, ImVec2(absolutePos.x + tileWithOffSize.x, absolutePos.y + tileWithOffSize.y), ImColor(80, 80, 80));


			ImVec2 drawTileSize(tileSize / scale, tileSize / scale);
			list->AddRectFilled(absolutePos, ImVec2(absolutePos.x + drawTileSize.x, absolutePos.y + drawTileSize.y), ImColor(50, 50, 50));
			if (drawTileSize.x >= 1)
			{
				for (float w = 0; w < drawWid; w += drawTileSize.x)
				{
					ImVec2 ts(absolutePos.x + w, absolutePos.y);
					ImVec2 te(absolutePos.x + w, absolutePos.y + drawHei);

					list->AddLine(ts, te, ImColor(15, 15, 15));
				}
			}

			if (drawTileSize.y >= 1)
			{
				for (float h = 0; h < drawHei; h += drawTileSize.y)
				{
					ImVec2 ts(absolutePos.x, absolutePos.y + h);
					ImVec2 te(absolutePos.x + drawWid, absolutePos.y + h);

					list->AddLine(ts, te, ImColor(15, 15, 15));
				}
			}

			localStartPos.y += drawHei + 5;
			ImGui::SetCursorPos(localStartPos);

			//ImGui::End();
			//ImGui::EndChild();
		}
	}
};


class GuiDrawCloudPointClick : public GuiDrawImage
{
	//bool dropPoints = false;
public:

	std::vector<BackPoint> pointsToDraw;

	void unsetPoints()
	{
		pointsToDraw.clear();
	}

	void setPoints(const CSBinding& cs, const bc::barvector* _points, bool clear = true)
	{
		// if (dropPoints)
		// 	delete points;

		// points = _point;
		// dropPoints = drop;
		if (clear)
		{
			pointsToDraw.clear();
		}

		if (!_points)
		{
			return;
		}

		const bc::barvector& pointsi = *_points;
		for (auto &p : pointsi)
		{
			// int x = getDisplayX(p.getX());
			// int y = getDisplayY(p.getY());

			BackPoint pix = cs.toGlobal(static_cast<float>(p.getX()), static_cast<float>(p.getY()));
			pointsToDraw.push_back(pix);
		}
	}

	//~GuiDrawCloudPointClick()
	//{
	//	setPoints(nullptr, false);
	//}


	void drawPoints(const GuiDisplaySystem& ds, CSBinding& cs, ImVec2 realImgSize)
	{
		if (pointsToDraw.size() == 0)
			return;

		// TODO: replce it with toDisplay(0.5); Can be with minus
		ImVec2 pixelSize = displaySize / ImVec2(static_cast<float>(width), static_cast<float>(height));

		//const ImVec2 p0 = ImGui::GetItemRectMin();
		//const ImVec2 p1 = ImGui::GetItemRectMax();
		//const char* text_str = "Line 1 hello\nLine 2 clip me!";
		//const ImVec2 text_pos = ImVec2(p0.x + offset.x, p0.y + offset.y);
		//ImDrawList* draw_list = ImGui::GetWindowDrawList();
		//switch (n)
		//{
		//case 0:
		//	ImGui::PushClipRect(p0, p1, true);
		//	draw_list->AddRectFilled(p0, p1, IM_COL32(90, 90, 120, 255));
		//	draw_list->AddText(text_pos, IM_COL32_WHITE, text_str);
		//	ImGui::PopClipRect();
		//	break;
		//case 1:
		//	draw_list->PushClipRect(p0, p1, true);
		//	draw_list->AddRectFilled(p0, p1, IM_COL32(90, 90, 120, 255));
		//	draw_list->AddText(text_pos, IM_COL32_WHITE, text_str);
		//	draw_list->PopClipRect();
		//	break;
		//case 2:
		//	ImVec4 clip_rect(p0.x, p0.y, p1.x, p1.y); // AddText() takes a ImVec4* here so let's convert.
		//	draw_list->AddRectFilled(p0, p1, IM_COL32(90, 90, 120, 255));
		//	draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), text_pos, IM_COL32_WHITE, text_str, NULL, 0.0f, &clip_rect);
		//	break;
		//}
		// ptoj->getVector();

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
		window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

		ImGui::SetCursorPos(ds.getDrawPos());
		if (!ImGui::BeginChild("PointsOverdraw", ds.getDrawSize(), false, window_flags))
		{
			ImGui::EndChild();
			return;
		}

		auto* win = ImGui::GetCurrentWindow();
		ImVec2 offset = win->Pos + (pixelSize / 2);
		ImDrawList* list = win->DrawList;

		ImColor bigColor(128, 0, 255);
		ImColor midColor(220, 200, 0);

		float markerSize = 2.5f;//MAX(1, par->displaySize.x / par->width);

		BackPoint start = ds.getSysToItemStartPos(cs);
		BackPoint end = ds.getSysToItemEndPos(cs);
		for (const BackPoint& p : pointsToDraw)
		{
			if (GuiDisplaySystem::inRange(start, end, p))
			{
				ImVec2 pi = ds.projItemGlobToDisplay(cs, p);
				pi += offset;
				// const auto& pi = ds.projItemGlobToDisplay(cs, bpl);
				list->AddCircleFilled(pi, 1.5f * markerSize, bigColor);
				list->AddCircleFilled(pi, 0.8f * markerSize, midColor);
			}
		}

		ImGui::EndChild();
	}
};

class StepIntSlider
{
public:

	void draw(const char* name, int& val, int minv, int maxv, int step)
	{
		val = val - val % step;
		ImGui::SliderInt(name, &val, minv, maxv, "%d");
		val = val - val % step;
		if (val < minv)
			val = minv;
		else if (val > maxv)
			val = maxv;
	}
};