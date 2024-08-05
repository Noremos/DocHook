#include "layerInterface.h"


LFID ILayer::counter = 0;

ILayer* CoreLayerFactory::CreateCoreLayer(int id)
{
	auto it = coreLayersCreators.find(id);
	if (it != coreLayersCreators.end())
		return it->second();
	else
		return nullptr;
}
CoreLayerFactory::FunctionCoreHolder<ILayer> CoreLayerFactory::coreLayersCreators;
