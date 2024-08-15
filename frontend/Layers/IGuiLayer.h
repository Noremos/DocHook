#pragma once
#include "../DrawCommon.h"
#include <numeric>
#include <algorithm>
#include "Usings.h"

#include "../../backend/CSBind.h"
#include "../../backend/Layers/layerInterface.h"
#include "../../backend/Layers/Rasterlayers.h"
#include "../../backend/project.h"
#include "../DrawUtils.h"

// import LayersCore;
// import RasterLayers;
// import BarcodeModule
// import CSBind;
//import BackBind;
//import LuaStates;

class ILayerWorker;

//GuiBackend backend;

class IGuiLayer
{
protected:
	BackString strId;
	int copiedId = -1; // We need to cpy Id to avoid cases when core was destroid
public:
	virtual void draw(const GuiDisplaySystem& ds) = 0;
	virtual const char* getName() const = 0;
	virtual void setName(const BackString& name, bool updateOnlyEmpty = false) = 0;

	virtual GuiImage* getIcon() = 0;

	virtual void toGuiData() = 0;

	virtual ILayer* getCore() = 0;

	virtual void drawOverlap(const GuiDisplaySystem&)
	{ }

	virtual void drawToolbox(ILayerWorker& context) = 0;

	virtual void drawProperty() = 0;
	virtual void applyPropertyChanges() = 0;

	void lockAtThis(ImVec2 realSize)
	{
		auto* ccore = getCore();
		Project* proj = Project::getProject();
		DisplaySystem& ds = proj->getDisplay();

		BackPoint BPRealSize = toBP(realSize);

		BackPoint start = ds.projItemGlobToSys(ccore->cs, ccore->getNormGlobStart());
		BackPoint IT = ds.projItemGlobToSys(ccore->cs, ccore->getNormGlobSize());

		BackPoint IR = IT;
		ResizeImage(IR, BPRealSize);

		ds.csScale = IR.x / IT.x;

		BackPoint winSizeInSys = ds.toSysGlobRelative(BPRealSize);
		BackPoint itemSize = ds.toSysGlobRelative(IR);
		ds.csPos = start - (winSizeInSys - itemSize) / 2;
	}

	virtual void onClick(const GuiDisplaySystem&, BackPoint)
	{ }

	virtual int getSysId()
	{
		return copiedId;
	}
	virtual const char* getStrId()
	{
		return strId.c_str();
	}

	virtual ~IGuiLayer()
	{ }
};


class LayerFactory : public CoreLayerFactory
{
private:
	static FunctionGuiHolder<IGuiLayer> guiLayersCreators;

public:
	static IGuiLayer* CreateGuiLayer(int id, ILayer* core = nullptr)
	{
		auto it = guiLayersCreators.find(id);
		if (it != guiLayersCreators.end())
			return it->second(core);
		else
			return nullptr;
	}

	static IGuiLayer* CreateGuiLayer(ILayer* core)
	{
		assert(core != nullptr);
		auto it = guiLayersCreators.find(core->getFactoryId());
		if (it != guiLayersCreators.end())
			return it->second(core);
		else
			return nullptr;
	}

	template <typename IGui, typename ICore>
	static void RegisterFactory(int id)
	{
		coreLayersCreators[id] = []() { return new ICore(); };
		guiLayersCreators[id] = [](ILayer* core) { return new IGui(static_cast<ICore*>(core)); };
	}
};


// Contexr

class ILayerWorker
{
public:
	InOutLayer iol;
	LayersList<IGuiLayer> layers;

	//LayersVals(GuiBackend& back) : backend(back)
	//{ }

	InOutLayer* getIoLayer()
	{
		return &iol;
	}

	template<typename T>
	T* getCastCurrentLayer()
	{
		IGuiLayer* l = getCurrentLayer();
		if (l)
			return dynamic_cast<T*>(l);
		else
			return nullptr;
	}

	IGuiLayer* getCurrentLayer()
	{
		return iol.in < 0 ? nullptr : layers.at(iol.in);
	}

	IGuiLayer* getTempLayer()
	{
		return iol.out < 0 ? nullptr : layers.at(iol.out);
	}

	void update()
	{
		if (iol.out == -1)
			return;
	}

	template<typename T>
	T* addLayer(const BackString& name)
	{
		auto t = layers.add<T>();
		settup(t, name);
		return t;
	}

	template<typename TGui, typename TData>
	TGui* addLayer(const BackString& name, TData* core)
	{
		if (core == nullptr)
			return nullptr;

		auto t = layers.add<TGui>(core);
		settup(t, name);
		return t;
	}

	template<typename TGui, typename TData>
	TGui* setLayer(const BackString& name, TData* core)
	{
		if (core == nullptr)
			return nullptr;

		auto ptr = new TGui(core);
		if (layers.set(core->id, ptr))
		{
			settup(ptr, name);
			return nullptr;
		}
		delete ptr;

		return addLayer<TGui, TData>(name, core);
	}

	void settup(IGuiLayer* layer, const BackString& name)
	{
		if (iol.in == -1)
		{
			iol.in = iol.out = layer->getSysId();
		}
		else
			iol.out = layer->getSysId();

		layer->setName(name, true);
		layer->toGuiData();
	}

	void setLayers(const RetLayers& rlayers, const BackString& name)
	{
		for (auto& lay : rlayers)
		{
			auto ptr = LayerFactory::CreateGuiLayer(lay.core);
			if (!layers.set(lay.core->id, ptr))
			{
				layers.addMove(ptr);
			}
			settup(ptr, lay.getName(name));
		}
	}

	void loadLayers()
	{
		Project* proj = Project::getProject();
		layers.clear();
		for (auto& coreLay : proj->layers)
		{
			auto ptr = LayerFactory::CreateGuiLayer(coreLay.get());
			layers.addMove(ptr);
			settup(ptr, coreLay->name);
		}
	}

	ImVec2 lastRealSize;

	void draw(const GuiDisplaySystem& ds)
	{
		lastRealSize = toIV(ds.drawSize);

		buint i = 0;
		for (auto& lay : layers)
		{
			// ImGui::SetNextWindowPos(pos);
			// ImGui::SetNextWindowSize(size);
			// ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushID(i);
			lay->draw(ds);
			ImGui::PopID();
			++i;
		}

		//pos.x += drawSize.x;
		//ImGui::SetNextWindowPos(pos);
		//ImGui::SetNextWindowSize(nextSize);
		//ImGui::SetNextWindowViewport(viewport->ID);
	}

	virtual void drawToolbox()
	{
		auto t = getCurrentLayer();
		if (t)
			t->drawToolbox(*this);
	}

	void drawOverlap(const GuiDisplaySystem& ds)
	{
		auto t = getCurrentLayer();
		if (t)
			t->drawOverlap(ds);
	}

	void onClick(const GuiDisplaySystem& ds, BackPoint click)
	{
		if (iol.in == -1)
			return;
		auto layer = layers.at(iol.in);
		if (layer)
			layer->onClick(ds, click);
	}
};
