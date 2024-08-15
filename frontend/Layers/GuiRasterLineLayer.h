#pragma once
#include "../DrawCommon.h"
#include <memory>
#include <future>

#include "Barcode/PrjBarlib/include/barstrucs.h"

#include "../../backend/Interfaces/IItem.h"
#include "../../backend/Layers/Rasterlayers.h"
#include "../../backend/Layers/RasterLineLayer.h"
#include "../Templates/GuiRasterTools.h"
#include "IGuiLayer.h"
#include "../GuiWidgets.h"


//import BackBind;
// import IItemModule;
// import RasterLayers;
import GuiOverlap;
// import RasterLineLayerModule;
//import SimpleLine;

import GuiLayers;
//GuiBackend backend;

import GuiFilter;

class RasterLineGuiLayer : public ITiledRasterGuiLayer<GuiDrawCloudPointClick, RasterLineLayer>
{
	using Base = ITiledRasterGuiLayer<GuiDrawCloudPointClick, RasterLineLayer>;
public:
	BackString debug;
	SimpleLine* selectedLine = nullptr;
	GuiFilter filtere;

	RasterLineGuiLayer(RasterLineLayer* fromCore = nullptr) : ITiledRasterGuiLayer(fromCore)
	{
	}

	float tempVal = 1.f;
	virtual void drawProperty()
	{
		Base::drawProperty();
		ImGui::Separator();
		ImGui::SliderFloat("Прозрачность", &tempVal, 0.f, 1.f);
	}
	virtual void applyPropertyChanges()
	{
		Base::applyPropertyChanges();
		main.opacity = tempVal;
	}

	virtual void toGuiData()
	{
		selectedLine = nullptr;
		Base::toGuiData();
		main.setImage(Base::data->mat, false);
		icon.setImage(Base::data->mat, 32, 32, true);
	}

	virtual void drawToolboxInner(ILayerWorker& context)
	{
		Base::drawToolboxInner(context);
		ImGui::Separator();

		if (ImGui::Button("Update"))
		{
			ImGui::OpenPopup("UpdateImage");
		}

		ImGui::SameLine(0, 30);

		if (ImGui::BeginPopupModal("UpdateImage", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			filtere.draw();

			if (ImGui::Button("Update"))
			{
				selectedLine = nullptr;

				ImGui::CloseCurrentPopup();
				//auto rets = backend.processRaster(context.iol, filtere.getFilter());
				Base::data->collectionToPredict = nullptr;
				auto rets = Base::data->processCachedBarcode(filtere.getFilter());
				this->toGuiData();
				context.setLayers(rets, "barcode");
				//commonValus.onAir = true;
				//commonValus.future = std::async(&GuiBackend::processRaster, std::ref(backend),
			}
			ImGui::SameLine();

			if (ImGui::Button("Close"))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	virtual void draw(const GuiDisplaySystem& ds)
	{
		if (!getCore()->visible)
			return;

		Base::draw(ds);

		ImVec2 realSize(data->displayWidth(), data->displayHeight());
		main.drawPoints(ds, getCore()->cs, realSize);
		drawLineInfoWin(ds);
	}

	virtual void onClick(const GuiDisplaySystem& ds, BackPoint pos)
	{
		ImVec2 posInDisplay = ds.projItemGlobToDisplay(data->cs, pos);
		if (ds.inDisplayRange(posInDisplay))
		{
			// globP =
			// 1. sub point -> display (mat variable) via tileProv
			// 2. Cast to a real img via factor and save for draw

			// (pos - origin) / factor
			// auto ps = data->cs.proj.getThisProj(ds.core.sysProj, pos, false);
			const BackPixelPoint pix = data->cs.toLocal(pos);
			auto points = click(pix.x / data->subToRealFactor, pix.y / data->subToRealFactor);
			setPoints(ds, points);
		}
	}


	void setPoints(const GuiDisplaySystem& ds, const bc::barvector* points)
	{
		main.setPoints(data->cs, points);
	}


	bc::barvector* click(int x, int y)
	{
		if (data->clickResponser.size() == 0)
			return nullptr;

		// x = main.getRealX(x);
		// y = main.getRealY(y);


		if (x < 0 || x >= main.width)
			return nullptr;

		if (y < 0 || y >= main.height)
			return nullptr;

		//std::cout << x << " : " << y << std::endl;
		int index = data->mat.getLineIndex(x, y);
		SimpleLine* line = data->clickResponser[index];
		if (line)
		{
			if (selectedLine == line && line->getParent())
				line = line->getParent();

			selectedLine = line;
			return &(selectedLine->matr);
		}

		return nullptr;
	}


	SimpleLine* moveToParenr()
	{
		return selectedLine = selectedLine->getParent();
	}


	void drawLineInfoWin(const GuiDisplaySystem& ds)
	{
		SimpleLine* line = selectedLine;
		if (!line)
		{
			return;
		}


		int counter = 0;
		if (!ImGui::Begin("Tree"))
		{
			ImGui::End();
			return;
		}
		drawTree(ds, line, counter);
		ImGui::End();


		if (!ImGui::Begin("Propertis"))
		{
			ImGui::End();
			return;
		}

		if (line->depth > 0)
		{
			ImGui::BeginDisabled(line->parent == -1);
			if (ImGui::Selectable("Parent"))
			{
				line = moveToParenr();
				setPoints(ds, &line->matr);
			}
			ImGui::EndDisabled();

			ImGui::Separator();
			BackString s = line->start.text<BackString, toStdStr>();
			s = "Start: " + s;
			ImGui::Text("%s",s.c_str());
			s = line->end.text<BackString, toStdStr>();
			s = "End: " + s;
			ImGui::Text("%s", s.c_str());

			ImGui::Text("Depth %d", line->depth);
			ImGui::Text("Matr size %d", line->matrSrcSize);
		}
		else
		{
			ImGui::Text("The root has been reached");
		}

		ImGui::End();
	}


	void drawTree(const GuiDisplaySystem& ds, SimpleLine* line, int& counter)
	{
		//if (tree.size == -1 && tree.children.size() == 0)
		//	return;

		++counter;

		static int node_clicked = 0;
		ImGuiTreeNodeFlags node_flags = 0;
		if (counter == node_clicked)
		{
			node_flags |= ImGuiTreeNodeFlags_Selected; // ImGuiTreeNodeFlags_Bullet
		}

		int size = line->children.size();
		if (size == 0)
		{
			node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
			ImGui::TreeNodeEx((void*)(intptr_t)counter, node_flags, "%d [%s:%s]", size, line->start.text<BackString, toStdStr>().c_str(), line->end.text<BackString, toStdStr>().c_str());
			if (ImGui::IsItemClicked())
				node_clicked = counter;
		}
		else
		{
			node_flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
			bool isOpen = ImGui::TreeNodeEx((void*)(intptr_t)counter, node_flags, "%d [%s:%s]", size, line->start.text<BackString, toStdStr>().c_str(), line->end.text<BackString, toStdStr>().c_str());
			if (isOpen)
			{
				if (ImGui::IsItemClicked())
					node_clicked = counter;

				for (auto child : line->children)
				{
					drawTree(ds, line->getChild(child), counter);
				}
				ImGui::TreePop();
			}
			else if (ImGui::IsItemClicked())
				node_clicked = counter;
		}
	}

private:
};
