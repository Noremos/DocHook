#pragma once
#include "../DrawCommon.h"

#include "../backend/Interfaces/IItem.h"
#include "../GuiWidgets.h"

//import BackBind;
// import IItemModule;

struct GuiFilter
{
	RangeItemFilter valsFilter;
	//ScriptFilterInfo scriptFilter;
	char text[10000];
	bool changed = false;

	GuiFilter()
	{
		//const BackString re = ScriptFilterInfo::getScriptTemplate();
		//memcpy(text, re.c_str(), re.length());
	}

	SelectableKeyValues<int> typeCB =
	{
		{0, "Без фильтра"},
		{1, "Простой"},
		{2, "Lua script"}
	};

	void _drawPair(const char* name1, const char* name2, RangeItemFilter::FRange& rng, int max = 256)
	{
		changed |= ImGui::SliderInt(name1, &rng.first, 0, max, "%d");
		changed |= ImGui::SliderInt(name2, &rng.second, 0, max, "%d");
	}

	IItemFilter* getFilter()
	{
		return &valsFilter;
	}

	bool draw()
	{
		changed = false;
		ImGui::Text("Пороги отсеивания");
		_drawPair("Мин. Начало", "Макс. Начало", valsFilter.start);
		_drawPair("Мин. Длина", "Макс. Длина", valsFilter.len);
		_drawPair("Мин. размер в %", "Макс. размер %", valsFilter.matrSizeProc, 100);
		_drawPair("Мин. Глубина", "Макс. Глубина", valsFilter.depth, 30);
		_drawPair("Мин. Ширина", "Макс. Ширина", valsFilter.width, 2000);
		_drawPair("Мин. Высота", "Макс. Высота", valsFilter.height, 2000);
		changed |= ImGui::InputInt("Мин. объем", &valsFilter.minPixelsSize); // matr size must be more then this
		return changed;
	}

};