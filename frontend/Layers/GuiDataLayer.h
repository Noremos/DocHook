#pragma once

#include "../DrawCommon.h"

#include <numeric>
#include <algorithm>
#include "Barcode/PrjBarlib/include/barcodeCreator.h"
#include "Usings.h"

#include "../../backend/CSBind.h"
#include "../backend/project.h"
#include "../Bind/Framework.h"
#include "../../backend/Layers/layerInterface.h"
#include "../../backend/Layers/Rasterlayers.h"
#include "IGuiLayer.h"
#include "../GuiWidgets.h"

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


template<class T>
class GuiLayerData : public IGuiLayer
{
protected:
	T* data;

public:
	GuiImage icon;

	GuiLayerData(T* fromCore = nullptr)
	{
		Project* proj = Project::getProject();

		if (fromCore == nullptr)
			data = proj->addLayerData<T>();
		else
			data = fromCore;

		copiedId = data->id;
	}

	virtual void toGuiData()
	{
		copiedId = data->getSysId();
		strId = data->name;
		strId += intToStr(copiedId);
		projSet.setup(data->cs);
	}

	T* getData()
	{
		return data;
	}

	ILayer* getCore()
	{
		return data;
	}

	virtual GuiImage* getIcon()
	{
		return &icon;
	}

	virtual const char* getName() const
	{
		return data->name.c_str();
	}

	const LayerProvider& getProvider() const
	{
		return data->prov;
	}

	virtual void drawToolboxInner(ILayerWorker&)
	{ }

	virtual void drawToolbox(ILayerWorker& context)
	{
		if (ImGui::Begin("Инструмаенты слоя"))
		{
			drawToolboxInner(context);
		}
		ImGui::End();
	}

	ProjectionSettings projSet;
	virtual void drawProperty()
	{
		projSet.draw();
	}
	virtual void applyPropertyChanges()
	{
		projSet.setup(data->cs);
	}


	void setName(const BackString& name, bool updateOnlyEmpty = false)
	{
		if (data->name.length() != 0 && updateOnlyEmpty)
			return;

		data->name = name;
		strId = name;
		strId += intToStr(data->id);
	}

	virtual ~GuiLayerData()
	{ }
};
