#pragma once

#include "../DrawCommon.h"
#include <memory>
#include <future>
#include "Barcode/PrjBarlib/include/barstrucs.h"
#include "../../backend/Layers/layerInterface.h"
#include "../../backend/Layers/Rasterlayers.h"
#include "../../backend/Layers/RasterLineLayer.h"
#include "../../backend/project.h"
#include "../Layers/GuiDataLayer.h"
#include "../Layers/IGuiLayer.h"
#include "../GuiWidgets.h"

//import BackBind;
// import RasterLayers;
import GuiLayers;
import GuiOverlap;
import DynamicSettings;

// import LayersCore;
import GuiFilter;



template<class IM, class T>
class ITiledRasterGuiLayer : public GuiLayerData<T>
{
public:
	IM main;
	GuiTilePreview tilePrview;
	int newTileSize;
	int newOffsetSize;

	ITiledRasterGuiLayer(T* fromCore) : GuiLayerData<T>(fromCore)
	{ }

	virtual ~ITiledRasterGuiLayer()
	{ }

	virtual void toGuiData()
	{
		GuiLayerData<T>::toGuiData();

		newTileSize = getTileSize();
		newOffsetSize = getOffsetSize();
	}

	virtual void draw(const GuiDisplaySystem& ds)
	{
		auto* core = GuiLayerData<T>::data;

		if (!core->visible)
			return;

		BackPixelPoint realSize(core->realWidth(), core->realHeight());

		auto wpos = ds.getWinPos();
		auto start = ds.projItemLocalToDisplay(core->cs, { 0,0 });
		auto end = ds.projItemLocalToDisplay(core->cs, realSize);

		main.drawImage(GuiLayerData<T>::getName(), wpos, ds.getDrawSize(), start, end);
	}

	inline int getTileSize() const
	{
		return GuiLayerData<T>::getProvider().tileSize;
	}
	inline int getOffsetSize() const
	{
		return GuiLayerData<T>::data->tileOffset;
	}

	inline ImVec2 getImageSize() const
	{
		return ImVec2(GuiLayerData<T>::data->realWidth(), GuiLayerData<T>::data->realHeight());
	}

	SelectableKeyValues<int> imgSubImages;

	// Component
	SelectableKeyValues<bc::ComponentType> componentCB =
	{
		{bc::ComponentType::Component, "Компонента"},
		{bc::ComponentType::Hole, "Дыра"}
	};
	// ---

	// Proc Type
	SelectableKeyValues<bc::ProcType> procCB =
	{
		{bc::ProcType::f0t255, "От 0 до 255"},
		{bc::ProcType::f255t0, "От 255 до 0"},
		{bc::ProcType::Radius, "По расстоянию"},
		{bc::ProcType::invertf0, "Инвертировать"},
		{bc::ProcType::experiment, "Радар"},
		// {bc::ProcType::ValueRadius, "Тру расстояние"}
	};

	SelectableKeyValues<bc::ColorType> colorCB =
	{
		{bc::ColorType::native, "Как в изображении"},
		{bc::ColorType::gray, "Серый"},
		{bc::ColorType::rgb, "Цветной"},
	};

	SelectableKeyValues<bc::AttachMode> attachCB =
	{
		{bc::AttachMode::firstEatSecond, "firstEatSecond"},
		{bc::AttachMode::secondEatFirst, "secondEatFirst"},
		{bc::AttachMode::createNew, "createNew"},
		{bc::AttachMode::dontTouch, "dontTouch"},
		{bc::AttachMode::morePointsEatLow, "morePointsEatLow"},
		{bc::AttachMode::closer, "closer"}
	};

	SelectableKeyValues<int> alg =
	{
		{0, "Растровый"},
		{1, "Растр в точки"}
	};
	BarcodeProperies properties;
	GuiFilter filterInfo;
	int cacheMb = 1024;
	int threads = 1;

	void grabSets()
	{
		//properties.barstruct.proctype = procCB.currentValue();
		//properties.barstruct.coltype = colorCB.currentValue();
		//properties.barstruct.comtype = componentCB.currentValue();
		//properties.attachMode = attachCB.currentValue();
		//properties.alg = alg.currentIndex;
	}
	inline RetLayers createCacheBarcode(InOutLayer& iol, const BarcodeProperies& propertices, IItemFilter* filter = nullptr)
	{
		Project* proj = Project::proj;

		IRasterLayer* inLayer = proj->getInRaster(iol);

		RasterLineLayer* layer = proj->addOrUpdateOut<RasterLineLayer>(iol, inLayer->cs.getProjId());
		auto ret = layer->createCacheBarcode(inLayer, propertices, filter);

		proj->saveProject();

		return ret;
	}

	void createBarcode(ILayerWorker& context)
	{
		grabSets();
		RetLayers layerData = createCacheBarcode(context.iol, properties, filterInfo.getFilter());
		context.setLayers(layerData, "barcode");
	}
	StepIntSlider tileSizeSlider, offsetSlider;
	std::vector<SubImgInf> subImgs;

	virtual void drawToolboxInner(ILayerWorker& context)
	{
		if (ImGui::Button("Построить баркод"))
		{
			subImgs = GuiLayerData<T>::data->getSubImageInfos();
			if (subImgs.size() != 0)
			{
				imgSubImages.clear();
				for (size_t i = 0; i < subImgs.size(); i++)
				{
					SubImgInf& sub = subImgs[i];
					BackString s = intToStr(sub.wid) + "x" + intToStr(sub.hei);
					imgSubImages.add(s, i);
				}
				imgSubImages.endAdding();
				imgSubImages.currentIndex = 0;
			}
			else
			{
				subImgs.push_back(BackSize(GuiLayerData<T>::data->realWidth(), GuiLayerData<T>::data->realHeight()));
			}
			ImGui::OpenPopup("SelectMax");
		}

		if (ImGui::BeginPopupModal("SelectMax", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
			if (ImGui::BeginTabBar("Настройки", tab_bar_flags))
			{
				if (ImGui::BeginTabItem("Алгоритм"))
				{
					drawDynamicSettings(properties);

					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Пороги отсеивания"))
				{
					ImGui::Separator();
					ImGui::Text("Пороги отсеивания");
					filterInfo.draw();
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Оптимизация"))
				{
					ImGui::Text("Лимит кэша");

					ImGui::SameLine();
					ImGui::SetNextItemWidth(150);
					ImGui::InputInt("MB", &cacheMb, 1);

					ImGui::Separator();

					if (imgSubImages.getSize() > 1)
					{
						imgSubImages.drawListBox("Размеры");
						if (imgSubImages.hasChanged())
						{
							GuiLayerData<T>::data->setSubImage(imgSubImages.currentIndex);
							SubImgInf& sub = subImgs[imgSubImages.currentIndex];
							int maxSize = std::max(sub.wid, sub.hei);
							if (newTileSize > maxSize)
							{
								newTileSize = maxSize;
							}
						}
						ImGui::SameLine();
					}

					SubImgInf& sub = subImgs[imgSubImages.currentIndex];
					const int maxSize = std::max(sub.wid, sub.hei);
					if (maxSize >= 10)
					{
						if (ImGui::BeginChild("Tile size", ImVec2(300, 360)))
						{
							ImGui::Text("Размер тайла");
							tileSizeSlider.draw("##Tile size", newTileSize, 10, maxSize, 10);

							int maxOffset = newTileSize;
							if (newTileSize + maxOffset > maxSize)
								maxOffset = maxSize - newTileSize;

							ImGui::Text("Доп. наложение тайла");
							offsetSlider.draw("##Offset size", newOffsetSize, 0, maxOffset, 1);

							ImGui::Separator();
							tilePrview.draw(newTileSize, newOffsetSize, ImVec2(sub.wid, sub.hei));
						}
						ImGui::EndChild();
					}
					//else
					//{
					//	newTileSize = maxSize;
					//	newOffsetSize = 0;
					//}
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}

			ImGui::Separator();
			if (ImGui::Button("Run"))
			{
				GuiLayerData<T>::data->prov.tileSize = newTileSize;
				GuiLayerData<T>::data->tileOffset = newOffsetSize;

				GuiLayerData<T>::data->setCache(cacheMb * 1024 * 1024);
				GuiLayerData<T>::data->setSubImage(imgSubImages.currentIndex);

				ImGui::CloseCurrentPopup();
				createBarcode(context);
				GuiLayerData<T>::data->setSubImage(0);
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

			ImGui::Separator();
	}
};

template<class T>
class TiledRasterGuiLayer : public ITiledRasterGuiLayer<GuiDrawImage, T>
{
	using Base = ITiledRasterGuiLayer<GuiDrawImage, T>;
public:
	TiledRasterGuiLayer(T* fromCore) : ITiledRasterGuiLayer<GuiDrawImage, T>(fromCore)
	{ }

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
		Base::main.opacity = tempVal;
	}

	virtual ~TiledRasterGuiLayer()
	{ }
};


template<class T>
class RasterToolsLayer : public TiledRasterGuiLayer<T>
{
	GuiFilter filtere;

	// Proc Type
	SelectableKeyValues<bc::ProcType> procCB =
	{
		{bc::ProcType::f0t255, "От 0 до 255"},
		{bc::ProcType::f255t0, "От 255 до 0"},
		{bc::ProcType::Radius, "По расстоянию"},
		{bc::ProcType::invertf0, "Инвертировать"},
		{bc::ProcType::experiment, "Радар"},
		// {bc::ProcType::ValueRadius, "Тру расстояние"}
	};

public:
	RasterToolsLayer(T* fromCore = nullptr) : TiledRasterGuiLayer<T>(fromCore)
	{ }

	virtual void drawToolboxInner(ILayerWorker& context)
	{
		TiledRasterGuiLayer<T>::drawToolboxInner(context);


		//ImGui::SameLine();
		//ImGui::Checkbox("Переключить вид", &heimap.enable);
		//if (heimap.enable && !heimap.isInit())
		//{
		//	heimap.init(main);
		//}
		ImGui::Separator();
	}
};
