module;
#include "../DrawCommon.h"

#include <numeric>
#include <algorithm>
#include "Barcode/PrjBarlib/include/barcodeCreator.h"
#include "Usings.h"

#include "../../backend/CSBind.h"
#include "../Bind/Framework.h"
#include "../../backend/Layers/layerInterface.h"
#include "../../backend/Layers/Rasterlayers.h"
#include "../../backend/project.h"
#include "IGuiLayer.h"
#include "../GuiWidgets.h"

export module GuiLayers;

// import LayersCore;
// import RasterLayers;
// import BarcodeModule;
import GuiOverlap;
import VectorLayers;
// import CSBind;
// import Platform;
//import BackBind;
//import LuaStates;


//GuiBackend backend;

export class LayersVals : public ILayerWorker
{
public:

	IRasterLayer* getCurrentRasterCore()
	{
		return iol.in < 0 ? nullptr : dynamic_cast<IRasterLayer*>(layers.at(iol.in)->getCore());
	}

	void drawLayersWindow()
	{
		if (!ImGui::Begin("Слои"))
		{
			ImGui::End();
			return;
		}

		ImVec2 pos = ImGui::GetWindowPos();

		auto winsize = ImGui::GetWindowSize();
		winsize.y -= 50;

		bool updateWorkingImage = false;
		int& displayRadioId = iol.out;
		if (layers.size() > 0 && iol.out == -1)
			displayRadioId = (*layers.begin())->getSysId();

		float selHei = 40;
		ImVec2 iconSize(40, selHei / 2);

		bool catchNext = false, incrementCurrent = false, incrementWorkingLayer = false;
		int toMove[2]{ -1, -1 };
		int prevId = -1;
		int delId = -1;

		Project* proj = Project::getProject();


		if (ImGui::BeginListBox("##LayersList", winsize))
		{
			if (displayRadioId == -1)
				displayRadioId = 0;

			buint j = 0;
			for (auto& lay : layers)
			{
				bool fistLayer = j == 0;
				bool lastLayer = j == layers.size() - 1;
				//if (lay->getSysId()s == iol.out)
				//{
				//	ImFont* italicFont = ImGui::GetIO().Fonts->Fonts[1];  // Assuming the second font in the ImFontAtlas is italic
				//	ImGui::PushFont(italicFont);
				//}
				ImGui::PushID(j);

				auto curID = lay->getSysId();
				if (catchNext)
				{
					toMove[1] = curID;
					catchNext = false;
				}

				if (incrementCurrent)
				{
					incrementCurrent = false;
					iol.in = curID;
				}

				if (incrementWorkingLayer)
				{
					incrementWorkingLayer = false;
					displayRadioId = curID;
				}

				auto& icon = *lay->getIcon();

				bool curRadio = (displayRadioId == lay->getSysId());
				updateWorkingImage = ImGui::RadioButton("##Work", curRadio);//, curRadio);

				ImGui::SameLine();
				ImGui::Checkbox("##visible", &lay->getCore()->visible);

				ImGui::SameLine();
				ImGui::Image((void*)(intptr_t)icon.getTextureId(), ImVec2(selHei, selHei));

				ImGui::SameLine();
				auto posBef = ImGui::GetCursorPos();
				ImGui::BeginDisabled(fistLayer);
				if (ImGui::Button(ICON_FA_ANGLE_UP "", iconSize))
				{
					toMove[0] = prevId;
					toMove[1] = curID;
				}
				ImGui::EndDisabled();

				ImGui::SetCursorPos({posBef.x, posBef.y + iconSize.y});
				ImGui::BeginDisabled(lastLayer);
				if (ImGui::Button(ICON_FA_ANGLE_DOWN "", iconSize))
				{
					toMove[0] = curID;
					catchNext = true;
				}
				ImGui::EndDisabled();

				ImGui::SetCursorPos({posBef.x + iconSize.x, posBef.y});
				if (ImGui::Button(ICON_FA_TRASH "", ImVec2(selHei, selHei)))
				{
					delId = curID;
					lay->getCore()->release();
					if (curRadio)
					{
						if (lastLayer)
							displayRadioId = prevId;
						else
							incrementWorkingLayer = true;
					}

					if (iol.in == curID)
					{
						if (lastLayer)
							iol.in = prevId;
						else
							incrementCurrent = true;
					}
				}

				ImGui::SameLine();
				bool seled = ImGui::Selectable(lay->getName(), curID == iol.in, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(winsize.x - 50, selHei));
				prevId = curID;

				if (seled)
				{
					iol.in = lay->getSysId();
					if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					{
						lay->lockAtThis(lastRealSize);
					}
				}
				if (updateWorkingImage)
				{
					if (j == 0)
						displayRadioId = -1;
					else
						displayRadioId  = lay->getSysId();

					updateWorkingImage = false;
				}

				//if (j == iol.out)
				//{
				//	ImGui::PopFont();
				//}
				// ImVec4 color(1.0f, 0.0f, 0.0f, 1.0f); // RGBA color (red in this case)
				ImGui::PopID();
				++j;
			}
			ImGui::EndListBox();
		}

		if (delId != -1)
		{
			layers.remove(delId);
			proj->removeLayer(delId);
		}

		//ImGui::EndGroup();

		if (toMove[0] != -1 && toMove[1] != -1)
		{
			proj->layers.move(toMove[0], toMove[1]);
			layers.move(toMove[0], toMove[1]);
		}
		ImGui::End();
	}
};
