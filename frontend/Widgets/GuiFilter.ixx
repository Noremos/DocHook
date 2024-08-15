module;
#include "../DrawCommon.h"

#include "../backend/Interfaces/IItem.h"
#include "../GuiWidgets.h"

export module GuiFilter;
//import BackBind;
// import IItemModule;

export struct GuiFilter
{
	RangeItemFilter valsFilter;
	//ScriptFilterInfo scriptFilter;
	char text[10000];

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
		ImGui::SliderInt(name1, &rng.first, 0, max, "%d");
		ImGui::SliderInt(name2, &rng.second, 0, max, "%d");
	}

	IItemFilter* getFilter()
	{
		int id = typeCB.currentValue();
		switch (id)
		{
		case 0:
			return nullptr;
		case 1:
			return &valsFilter;
		//case 2:
		//{
		//	scriptFilter.setScript(text);
		//	return &scriptFilter;
		//}
		}
		return nullptr;
	}

	void draw()
	{
		typeCB.drawCombobox("Тип");
		switch (typeCB.currentValue())
		{
		case 0:
			break;
		case 1:
			ImGui::Text("Пороги отсеивания");
			_drawPair("Мин. Начало", "Макс. Начало", valsFilter.start);
			_drawPair("Мин. Длина", "Макс. Длина", valsFilter.len);
			_drawPair("Мин. Размер матрицы в %", "Макс. размер матрицы в %", valsFilter.matrSizeProc, 100);
			_drawPair("Мин. Глубина", "Макс. Глубина", valsFilter.depth, 200);
			ImGui::InputInt("Мин. объем матрицы", &valsFilter.minPixelsSize); // matr size must be more then this

			break;
		//case 2:
		//	ImGui::InputTextMultiline("Lua скрипт", text, 10000, ImVec2(500, 300));
		//	break;
		default:
			break;
		}
	}
};