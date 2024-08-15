module;
#include <algorithm>
#include "DrawCommon.h"
#include "../backend/Interfaces/IBlock.h"
#include "../backend/project.h"

#include "../backend/MatrImg.h"
#include "../Bind/Framework.h"
#include "Layers/IGuiLayer.h"
#include "GuiWidgets.h"

export module GuiBlock;

//import BackBind;
// import MatrModule;

// import Platform;
// import IBlock;

import DynamicSettings;

export class GuiBlock
{
public:
	bool show = false;


	//int getClassImagesSize()
	//{
	//	return classImages.size();
	//}
	//ClassTextures& getClassImagesData()
	//{
	//	return classImages[classesLB.currentValue().vecId];
	//}


	int selectedId = 0;
	void draw(ILayerWorker& context)
	{
		if (!show)
			return;

		if (!ImGui::Begin("Генератор", &show))
		{
			ImGui::End();
			return;
		}

		bool found = false;
		int foundId = -1;
		BlockFactory& factory = getBlockFactory();
		auto& names = factory.getNames();
		if (ImGui::BeginCombo("Algs", names[selectedId].data()))
		{
			for (size_t i = 0; i < factory.getCount(); i++)
			{
				if (foundId == -1)
					foundId = i;

				bool is_selected = (selectedId == i); // You can store your selection however you want, outside or inside your objects
				if (ImGui::Selectable(names[i].data(), is_selected))
					selectedId = i;

				if (is_selected)
				{
					found = true;
					ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
				}
			}
			if (!found)
			{
				selectedId = foundId;
			}
			ImGui::EndCombo();
		}

		IBlock* block = factory.get(selectedId);
		if (block == nullptr)
		{
			ImGui::End();
			return;
		}

		drawDynamicRefSettings(block->settings);

		if (ImGui::Button("Добавить к выборке"))
		{
			block->addInput(context.iol);
		}

		ImGui::SameLine();
		if (ImGui::Button("Выполнить"))
		{
			auto ret = block->execute(context.iol);
			context.setLayers(ret, names[selectedId].data());
		}

		if (ImGui::Button("Очистить"))
		{
			block->clear();
		}
		ImGui::End();
	}
};
