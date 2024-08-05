module;

#include <unordered_set>
#include <random>
#include <memory>

#include "PortFileDialog.h"
#include "sago/platform_folders.h"

#include "../frontend/DrawCommon.h"
#include "Barcode/PrjBarlib/include/barscalar.h"
#include "Barcode/PrjBarlib/include/barline.h"
#include "Barcode/PrjBarlib/include/barcodeCreator.h"


#include "../backend/CSBind.h"
#include "../backend/Interfaces/IItem.h"
#include "../Bind/Framework.h"
#include "../backend/Layers/layerInterface.h"
#include "../backend/Layers/Rasterlayers.h"
#include "../backend/Layers/RasterLineLayer.h"
export module FrontendBind;

import ProjectModule;
// import Platform;
// import BarcodeModule;
// import RasterLayers;

import GuiWidgets;
import VectorLayers;


// Todo.
// 2 режима
export class GuiBackend
{
	static int opens;

public:
	Project* proj = nullptr;
	GuiBackend()
	{
		proj = Project::getProject();
		opens++;
	}
	~GuiBackend()
	{
		opens--;
		if (opens == 0)
			Project::dropProject();
	}

	DisplaySystem& getDS()
	{
		return proj->getDisplay();
	}

	bool isLoaded() const
	{
		return proj->state >= GuiState::Loaded;
	}

	MetadataProvider& getMeta()
	{
		return proj->getMeta();
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




	inline RasterFromDiskLayer* loadImage(const BackPathStr& path, int step)
	{
		Project* proj = Project::proj;

		RasterFromDiskLayer* layer = proj->addLayerData<RasterFromDiskLayer>();
		layer->open(path, proj->getMeta());

		proj->saveProject();

		return layer;
	}


	// Gui
	void createProject(const BackPathStr& path, const BackString& name, const BackPathStr& imgPath)
	{
		BackPathStr fullPath = path / name;
		proj->setProjectPath(fullPath);
		dropDirIfExists(proj->getPath(BackPath::metadata));
		loadImage(imgPath, 1);
		proj->state = GuiState::Loaded;
	}

	//RetLayers processRaster(InOutLayer& layer, IItemFilter* filter)
	//{
	//	if (!created)
	//		return RetLayers();

	//	return proj->processCachedBarcode(layer, filter);
	//}

	VectorLayer* addVectorLayer()
	{
		return proj->addLayerData<VectorLayer>();
	}

	void removeLayer(buint id)
	{
		proj->layers.remove(id);
	}

	RasterFromDiskLayer* loadImageOrProject(const BackPathStr& path)
	{
		RasterFromDiskLayer* layer = nullptr;
		GuiState newState = proj->state;
		bool setProc = false;
		if (path.extension() == ".qwr")
		{
			if (!proj->loadProject(path))
				return nullptr;
			//		return;
			setProc = true;
			newState = GuiState::Loaded;
		}
		else
		{
			if (state == GuiState::Empty)
			{
				proj->setProjectPath(path, true);
			}

			newState = GuiState::Loaded;
			layer = loadImage(path, 1);
		}


		proj->state = newState;
		return layer;
	}


	void save()
	{
		proj->saveProject();
	}


private:


private:

private:
	GuiState state = GuiState::Empty;
};

int GuiBackend::opens = 0;
