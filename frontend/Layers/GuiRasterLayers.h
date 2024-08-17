#pragma once
#include "../DrawCommon.h"
#include <memory>
#include <future>

#include "../../backend/MatrImg.h"
#include "../../backend/Interfaces/IItem.h"
#include "../Bind/Framework.h"
#include "../../backend/Layers/layerInterface.h"
#include "../../backend/Layers/Rasterlayers.h"
#include "../Templates/GuiRasterTools.h"
#include "IGuiLayer.h"
#include "../GuiWidgets.h"

//import BackBind;
// import IItemModule;
// import RasterLayers;
import GuiLayers;
import GuiOverlap;
// import Platform
// import LayersCore;



class RasterGuiLayer : public RasterToolsLayer<RasterLayer>
{
public:

	RasterGuiLayer(RasterLayer* fromCore = nullptr) : RasterToolsLayer<RasterLayer>(fromCore)
	{ }

	virtual void toGuiData()
	{
		RasterToolsLayer::toGuiData();

		tempVal = 1.f;
		main.setImage(data->mat, false);
		icon.setImage(data->mat, 32, 32, true);
	}

	virtual void drawToolboxInner(ILayerWorker& context)
	{
		RasterToolsLayer<RasterLayer>::drawToolboxInner(context);

		if (ImGui::Button("Выгрузить"))
		{
			BackPathStr path = getSavePath({ "png", "*.png",
								"jpg", "*.jpg" });
			imwrite(path, data->mat);
		}
		ImGui::Separator();

	}
};


class RasterFromDiskGuiLayer : public RasterToolsLayer<RasterFromDiskLayer>
{
public:
	bool drawHeimap;

	RasterFromDiskGuiLayer(RasterFromDiskLayer* fromCore = nullptr) : RasterToolsLayer<RasterFromDiskLayer>(fromCore)
	{
		drawHeimap = false;
		// heimap.enable = false;
	}

	virtual void toGuiData()
	{
		TiledRasterGuiLayer::toGuiData();

		auto i = *data->getCachedImage();
		main.setImage(i, false);
		icon.setImage(i, 32, 32, true);
	}

	bool isDrawing = false;
	ImVec2 rectMin, rectMax;
	virtual void onClick(const GuiDisplaySystem& ds, BackPoint pos)
	{
		ImVec2 posInDisplay = ds.projItemGlobToDisplay(data->cs, pos);
		if (ds.inDisplayRange(posInDisplay))
		{
			isDrawing = true;
			rectMin = ImGui::GetMousePos();
			rectMax = rectMin;
		}
	}

	GuiResizableContainer selectedRect;
	GuiDrawImage zoomImg;
	virtual void draw(const GuiDisplaySystem& ds)
	{
		if (!getCore()->visible)
			return;

		TiledRasterGuiLayer::draw(ds);

		ImVec2 winpos = ImGui::GetCurrentWindow()->Pos;

		if (selectedRect.Begin("Zoom"))
		{
			ImGuiWindow* win = ImGui::GetCurrentWindow();

			ImVec2 diplsyPos = selectedRect.offset;
			ImVec2 diplsySize = ImVec2(zoomImg.width, zoomImg.height) * selectedRect.getZoom();

			zoomImg.drawImage("Part", ImVec2(0,30), win->Size, ImVec2(0,0), diplsySize);
			selectedRect.end(win->Pos, win->Size);
		}


		// Draw the rectangle if we're currently drawing it
		if (!isDrawing)
			return;

		auto window_flags = ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollWithMouse;
		ImGui::SetCursorPos(ds.getWinPos());
		if (ImGui::BeginChild("Draw rect", ds.getDrawSize(), false, window_flags))
		{
			// Get the draw list for the current window
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			// Check if the left mouse button is released
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
			{
				// Stop drawing the rectangle
				isDrawing = false;
				auto absP = ds.core.toSysGlob(toBP(rectMin - winpos));
				auto ps = absP;
				const BackPixelPoint pixSt = data->cs.toLocal(ps);

				absP = ds.core.toSysGlob(toBP(rectMax - winpos));
				ps = absP;
				const BackPixelPoint pixEnd = data->cs.toLocal(ps);

				data->setSubImage(0);
				zoomImg.setImage(data->getRect(pixSt.x, pixSt.y, pixEnd.x - pixSt.x, pixEnd.y - pixSt.y), false);
				selectedRect.open = true;
			}
			else
			{
				rectMax = ImGui::GetMousePos();
				ImU32 color = IM_COL32(0, 0, 255, 120);
				float thickness = 2.0f;
				drawList->AddRectFilled(rectMin, rectMax, color, 0, ImDrawFlags_RoundCornersAll);
			}
		}
		ImGui::EndChild();
	}


	virtual void drawOverlap(const GuiDisplaySystem& ds)
	{
		if (drawHeimap)
		{
			//heimap.draw(ds);
		}
		else
		{
			// tilemap.init(&main, &getProvider());
			// tilemap.draw(main.displaysBegin, main.displaySize);
		}
	}

	std::vector<SubImgInf> getSubImageInfos()
	{
		return data->getSubImageInfos();
	}
};
