// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
#include "Application.h"
#include "DrawCommon.h"

#include <cmath>
#include <initializer_list>
#include <memory>
#include <future>
#include <iostream>
#include "../side/implot/implot.h"
#include "Barcode/PrjBarlib/include/barcodeCreator.h"
#include "Json.h"

//#include "sol3/sol.hpp"
//
//#include <GLFW/glfw3.h>
//#include "../libs/glew/include/GL/glew.h"
#include "../../backend/ProjectSettings.h"
#include "../Bind/Framework.h"

#include "../backend/MatrImg.h"
#include "../backend/Layers/layerInterface.h"
#include "../backend/Layers/Rasterlayers.h"
#include "../backend/Layers/RasterLineLayer.h"
#include "../backend/Layers/VectorLayers.h"
#include "../backend/ScreenCapture.h"
#include "Layers/GuiRasterLayers.h"
#include "Layers/IGuiLayer.h"
#include "Layers/GuiRasterLineLayer.h"
#include "GuiWidgets.h"
#include "../backend/project.h"

// import Platform;
import GuiLayers;
// import BarcodeModule;
import GuiOverlap;

import GuiVectorLayers;

// import ProjectSettings;
//import BackBind;
// import RasterLineLayerModule;

// import RasterLayers;
// import LayersCore;
import GuiBlock;
//import Lua;



/// Widget for raster layers


// Todo.
// 2 режима
class GuiBackend
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

	inline RetLayers createCacheBarcode(InOutLayer& iol, const BarcodeProperies& propertices, IItemFilter* filter = nullptr)
	{
		Project* proj = Project::proj;

		IRasterLayer* inLayer = proj->getInRaster(iol);

		RasterLineLayer* layer = proj->addOrUpdateOut<RasterLineLayer>(iol, inLayer->cs.getProjId());
		auto ret = layer->createCacheBarcode(inLayer, propertices, filter);

		proj->saveProject();

		return ret;
	}


	VectorLayer* addVectorLayer()
	{
		return proj->addLayerData<VectorLayer>();
	}


	void process(BackImage& img, VectorLayer* vec, bc::barstruct& constr, IItemFilter* filter)
	{
		// constr.proctype = bc::ProcType::f0t255;
		bc::BarcodeCreator creator;
		std::unique_ptr<bc::Baritem> citem(bc::BarcodeCreator::create(img, constr));

		bc::Baritem* item = citem.get();
		int size = (int)item->barlines.size();

		for (int i = 0; i < size; i++)
		{
			bc::barline* line = item->barlines[i];

			if (line->matr.size() < 40)
				continue;

			if (filter)
			{
				BarlineWrapper wrap(line);
				if (!filter->pass(&wrap))
					continue;
			}

			auto rect = line->getBarRect();
			DrawPrimitive* priv = vec->addPrimitive({10,100,40});
			priv->addPoint(rect.x, rect.y);
			priv->addPoint(rect.x + rect.width, rect.y);
			priv->addPoint(rect.x + rect.width, rect.y + rect.height);
			priv->addPoint(rect.x, rect.y + rect.height);
		}
	}

	RetLayers createCacheBarcode2(IRasterLayer* inLayer, const BarcodeProperies& propertices, IItemFilter* filter)
	{
		if (filter)
		{
			// Input Layer prepatons
			int tileSize = inLayer->prov.tileSize;
			int tileOffset = inLayer->tileOffset;

			const buint fullTile = tileSize + tileOffset;
			filter->imgLen = fullTile * fullTile;
		}

		BackImage img = inLayer->getImage(100000);

		VectorLayer* vec = addVectorLayer();
		vec->vecType = VectorLayer::VecType::polygons;

		const auto start = std::chrono::steady_clock::now();
		// Setup
		bc::barstruct constr = propertices.get();
		constr.coltype = bc::ColorType::gray;

		// constr.proctype = bc::ProcType::f0t255;
		// process(img, vec, constr, filter);

		// constr.proctype = bc::ProcType::f255t0;
		// process(img, vec, constr, filter);

		constr.proctype = bc::ProcType::Radius;
		process(img, vec, constr, filter);

		const auto end = std::chrono::steady_clock::now();
		const auto diff = end - start;
		const double len = std::chrono::duration<double, std::milli>(diff).count();
		printf("All works ended in %dms\n", (int)len);

		return {vec};
	}

	inline RasterFromDiskLayer* loadImage(const BackPathStr& path, int step)
	{
		Project* proj = Project::proj;

		RasterFromDiskLayer* layer = proj->addLayerData<RasterFromDiskLayer>();
		layer->open(path);

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

GuiBackend backend;

namespace MyApp
{
	inline int maxThreadCount, minThreadCount;

	void setImGuiStyle(float highDPIscaleFactor)
	{
		ImGuiStyle& style = ImGui::GetStyle();

		// https://github.com/ocornut/imgui/issues/707#issuecomment-415097227
		style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
		style.Colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.40f, 0.41f, 0.42f, 1.0f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.40f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 0.67f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.44f, 0.44f, 0.44f, 0.40f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.46f, 0.47f, 0.48f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.70f, 0.70f, 0.70f, 0.31f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.70f, 0.70f, 0.70f, 0.80f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.48f, 0.50f, 0.52f, 1.00f);
		style.Colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.72f, 0.72f, 0.72f, 0.78f);
		style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);
		style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.73f, 0.60f, 0.15f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.87f, 0.87f, 0.87f, 0.35f);
		style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
		style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
		style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

		style.WindowPadding = ImVec2(8, 6);
		style.WindowRounding = 0.0f;
		style.FramePadding = ImVec2(5, 7);
		//style.FrameRounding            = 0.0f;
		style.ItemSpacing = ImVec2(5, 5);
		// style.ItemInnerSpacing         = ImVec2(1, 1);
		// style.TouchExtraPadding        = ImVec2(0, 0);
		// style.IndentSpacing            = 6.0f;
		// style.ScrollbarSize            = 12.0f;
		// style.ScrollbarRounding        = 16.0f;
		// style.GrabMinSize              = 20.0f;
		// style.GrabRounding             = 2.0f;
		// style.WindowTitleAlign.x = 0.50f;
		// style.FrameBorderSize = 0.0f;
		// style.WindowBorderSize = 1.0f;

		style.ScaleAllSizes(highDPIscaleFactor);
	}


	bool useAsync = false;
	struct WindowsValues
	{
		bool onAir = false;

		void onAirC()
		{
			if (onAir && future.valid())
			{
				if (future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
				{
					onAir = false;
				}
			}
		}

		std::future<void> future;
	};

	WindowsValues commonValus;
	LayersVals layersVals;// (backend);



	// Center

	struct ImagesValues
	{
		GuiCSDisplayContainer resizble;
		//GuiResizableContainer resizble;

		// vecotr<IOverlap*>
	};

	ImagesValues centerVals;

	struct BottomBar
	{
		int classId = 0;
		std::string valeExtra = "barclass;";
		BackString debug;
		bool drawPics = true;
		bool showUpdaePopup = false;
	};
	BottomBar bottomVals;

	// Top bar
	int tempThreads;
	bool runAsync;

	void drawProjectSettings()
	{
		if (ImGui::Button(BU8("Project settings")))
		{
			ImGui::OpenPopup("ProjectSetts");
			tempThreads = getSettings().threadsCount;
			runAsync = getSettings().runAsync;
		}

		if (ImGui::BeginPopupModal("ProjectSetts", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::BeginDisabled(minThreadCount < 2);
			ImGui::Checkbox(BU8("Async"), &runAsync);
			ImGui::EndDisabled();

			if (minThreadCount > 1)
			{
				ImGui::SetNextItemWidth(150);

				ImGui::BeginDisabled(!runAsync);
				ImGui::InputInt("##thr", &tempThreads, 2, maxThreadCount);
				ImGui::EndDisabled();
			}

			ImGui::Separator();
			if (ImGui::Button("OK", ImVec2(120, 0)))
			{
				getSettings().threadsCount = tempThreads;
				getSettings().runAsync = runAsync;

				ImGui::CloseCurrentPopup();
			}

			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button(BU8("Cancel"), ImVec2(120, 0)))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	char buffer[1000];
	struct ImgData
	{
		BackImage src;
		GuiDrawImage img;
		std::string name = "";
		uint32_t winId = 0;
	};

	void createBarWindow(IRasterLayer* inLayer)
	{
		BarcodeProperies barset;

		RetLayers ret = backend.createCacheBarcode2(inLayer, barset, nullptr);
		layersVals.setLayers(ret, "barcode");
	}

	void createBarLayers(const BackImage& img)
	{
		RasterLayer* layer = backend.proj->addLayerData<RasterLayer>();
		layer->mat = img;

		RasterGuiLayer* guiLayer = layersVals.addLayer<RasterGuiLayer, RasterLayer>("Loaded", layer);
		guiLayer->lockAtThis(layersVals.lastRealSize);

		createBarWindow(layer);
	}

	constexpr float itemWidth = 100.0f;
	constexpr float itemHeight = 100.0f;

	std::vector<ImgData> images;
	void drawPreview()
	{
		// Draw image
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		const float windowWidth = viewport->Size.x * 0.8;
		const float windowHeight = viewport->Size.y * 0.8;


		ImGuiIO& io = ImGui::GetIO();
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
		if (ImGui::BeginPopupModal("Превью", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
		{
			const float topMargin = 20.0f;
			const float margin = 20.0f;

			const float itemWidthWithMargin = itemWidth + margin;
			const float itemHeightWithMargin = itemHeight + margin + 50; // 50 for text

			// const int itemCount = int(std::ceil(std::sqrt(images.size())));
			const int columns = int(std::ceil((windowWidth - margin) / itemWidthWithMargin));
			const int rows = int(std::ceil(float(images.size()) / columns));

			// ImGui::SetWindowSize(ImVec2(columns * (itemWidth + 10) + 10, rows * (itemHeight + 10) + 10));

			for (int i = 0; i < rows; i++)
			{
				float x = margin;
				float y = topMargin + i * (itemWidthWithMargin);
				// ImGui::BeginChild(("##WrapRow" + std::to_string(i)).c_str(), ImVec2(0, itemHeight * 1.2f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
				for (int j = 0; j < columns && (i + j * rows < images.size()); j++)
				{
					ImgData& data = images[i + j * rows];
					ImGui::PushID(data.winId);

					auto textWidth   = ImGui::CalcTextSize(data.name.c_str()).x;
					int maxWidth = std::max<int>(data.img.width, textWidth);

					ImGui::SetCursorPos(ImVec2(x + (maxWidth - data.img.width) / 2, y));
					ImVec2 size(data.img.width, data.img.height);
					if (ImGui::ImageButton(data.img.getTexturePtr(), size))
					{
						createBarLayers(data.src);
						ImGui::CloseCurrentPopup();
					}

					ImGui::SetCursorPos(ImVec2(x + (maxWidth - textWidth) / 2 , y + size.y + 20));
					ImGui::Text("%s", data.name.data());
					ImGui::PopID();

					x += maxWidth + margin;


					// ImGui::SameLine();
				}
				// ImGui::EndChild();
			}

			// for (ImgData& data : images)
			// {
			// 	data.img.drawImage("##ImgPreview", {0,0}, {0, 0});
			// 	ImGui::Text(data.name.data());
			// }

			ImGui::SetCursorPos(ImVec2(x, topMargin + rows * (itemWidthWithMargin) + 20));
			ImGui::Separator();

			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button(BU8("Cancel"), ImVec2(120, 0)))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void drawTopBar()
	{
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar;
		// float heighto = ImGui::GetFrameHeight();
		if (ImGui::BeginViewportSideBar("##TopMenu", NULL, ImGuiDir_Up, 50, window_flags))
		{
			// static std::queue<RasterFromDiskLayer*> layers;

			// static int val = 0;
			// bool upd = ImGui::SliderInt("Off", &val, -100, 100);

			if (ImGui::Button(BU8("Скриншот")))
			{
				auto imgs = getWindowsPreview();
				images.clear();

				for (ImageData& data : imgs)
				{
					ImgData& guiimg = images.emplace_back();
					guiimg.src = BackImage(data.width, data.height, data.channels, (uchar*)data.data.get());
					BackPoint imgsize(data.width, data.height);
					ResizeImage(imgsize, BackPoint(itemWidth, itemHeight));
					guiimg.img.setImage(guiimg.src, imgsize.x, imgsize.y, false);
					guiimg.name = data.name;
					guiimg.winId = data.winId;
				}
				ImGui::OpenPopup("Превью");
			}

			drawPreview();

			ImGui::SameLine();
			ImGui::SetNextItemWidth(100);
			ImGui::InputText("##name", buffer, 1000);
			ImGui::SameLine();
			if (ImGui::Button(BU8("Скриншот по имени")))
			{
				BarcodeProperies barset;
				RasterFromDiskLayer layer;
				layer.open("/Users/sam/1.png");

				RetLayers ret = backend.createCacheBarcode2(&layer, barset, nullptr);
				layersVals.setLayers(ret, "barcode");

				ImageData img = CaptureWindowByName(buffer);
				if (img.data)
				{
					createBarLayers(BackImage(img.width, img.height, img.channels, (uchar*)img.data.get()));
				}
			}

			ImGui::End();
		}
	}

	int menuBarHeight;

	void drawWorkout()
	{
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImVec2 pos = viewport->WorkPos;
		ImVec2 drawSize = viewport->WorkSize;

		//ImGui::SetNextWindowViewport(viewport->ID);

		auto window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDocking;
			window_flags |= ImGuiWindowFlags_HorizontalScrollbar;

		window_flags |= ImGuiWindowFlags_NoBackground;
		window_flags |= ImGuiWindowFlags_NoTitleBar;

		GuiDisplaySystem guiDisplay(backend.getDS());

		ImGui::SetNextWindowPos(pos);
		ImGui::SetNextWindowSize(drawSize);

		guiDisplay.drawPos = {0,0};
		guiDisplay.drawSize = BackPoint(drawSize.x, drawSize.y);
		guiDisplay.cursorPos = centerVals.resizble.currentPos;
		if (centerVals.resizble.Begin("ImagePreview"))
		{
			layersVals.draw(guiDisplay);
			if (centerVals.resizble.clicked)
			{
				layersVals.onClick(guiDisplay, centerVals.resizble.clickedPos);
			}

			// TiledRasterGuiLayer<RasterFromDiskLayer>* tlay = layersVals.getCastCurrentLayer<TiledRasterGuiLayer<RasterFromDiskLayer>>();
			layersVals.drawOverlap(guiDisplay);
		}

		centerVals.resizble.end(guiDisplay.getWinPos(), guiDisplay.getDrawSize());


		pos.x += drawSize.x;
		// drawSize.x = 200;
		drawSize.y += 20;

		// ImGui::SetNextWindowPos(pos);
		// ImGui::SetNextWindowSize(drawSize);
		//ImGui::SetNextWindowViewport(viewport->ID);

		window_flags = 0;//ImGuiWindowFlags_NoTitleBar;// | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_HorizontalScrollbar;
		if (ImGui::BeginViewportSideBar("Sidebar", NULL, ImGuiDir_Right, 200, window_flags))
		{
		// if (ImGui::Begin("Sidebar", NULL, window_flags))

			ImGui::End();
		}
	}
	// ------

	// Bootom bar
	void drawBottomBar()
	{
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar;
		// https://github.com/bkaradzic/bgfx/blob/master/3rdparty/dear-imgui/widgets/range_slider.inl
		if (ImGui::BeginViewportSideBar("##BottomBar", NULL, ImGuiDir_Down, 50, window_flags))
		{
			ImGui::SameLine(0, 30);

			// GBL
			ImGui::BeginDisabled(commonValus.onAir);

			// Current cursor pos
			ImGui::SameLine();
			const auto& curpos = centerVals.resizble.currentPos;
			ImGui::Text("%f : %f   | %f", curpos.x, curpos.y, backend.getDS().csScale);

			// Progress bar
			ImGui::SameLine();
			ImGui::ProgressBar(0.0, ImVec2(0.0f, 0.0f));
			ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
			ImGui::Text(BU8("Progressbar"));

			// GBL
			ImGui::EndDisabled();
			ImGui::End();
		}
	}

	// Layout
	void drawLayout()
	{
		drawTopBar();
		drawWorkout();
		layersVals.drawLayersWindow();
		// drawBottomBar();

		commonValus.onAirC();

		ImPlot::ShowDemoWindow();
	}

	constexpr ImVec2 toDVec2(const bc::point& p)
	{
		return ImVec2(static_cast<float>(p.x), static_cast<float>(p.y));
	}

	void Init(const char* root)
	{
		srand(time(NULL));
		Variables::setRoot(root);


		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;	   // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;	  // Enable Gamepad Controls


		//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;		   // Enable Docking
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;		 // Enable Multi-Viewport / Platform Windows


		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();
		MyApp::setImGuiStyle(1.0);

		ImFontConfig font_config;
		font_config.OversampleH = 1; //or 2 is the same
		font_config.OversampleV = 1;
		font_config.PixelSnapH = 1;

		static const ImWchar ranges[] =
		{
			0x0020, 0x00FF, // Basic Latin + Latin Supplement
			0x0400, 0x044F, // Cyrillic
			0,
		};

		[[maybe_unused]]
		ImFont* font = io.Fonts->AddFontFromFileTTF(Variables::getDefaultFontPath().string().c_str(), 15.0f, &font_config, ranges);
		IM_ASSERT(font != NULL);

		//setlocale(LC_ALL, "rus");

		float baseFontSize = 20.0f; // 13.0f is the size of the default font. Change to the font size you use.
		float iconFontSize = baseFontSize * 2.0f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly

		// merge in icons from Font Awesome
		static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
		ImFontConfig icons_config;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		icons_config.GlyphMinAdvanceX = iconFontSize;
		io.Fonts->AddFontFromFileTTF((Variables::getFontsDir() / FONT_ICON_FILE_NAME_FAS).string().c_str(), iconFontSize, &icons_config, icons_ranges);
		// use FONT_ICON_FILE_NAME_FAR if you want regular instead of solid

		// in an imgui window somewhere...
		// outputs a paint brush icon and 'Paint' as a string.
		//setlocale(LC_CTYPE, "rus"); // ����� ������� ��������� ������


		//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		//io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;



		maxThreadCount = std::thread::hardware_concurrency();
		if (maxThreadCount < 2)
		{
			minThreadCount = 1;
		}
		else
			minThreadCount = 2;

		backend.getDS().sysProj.init(DEFAULT_PROJECTION);

		LayerFactory::RegisterFactory<RasterGuiLayer, RasterLayer>(RASTER_LAYER_FID);
		LayerFactory::RegisterFactory<RasterLineGuiLayer, RasterLineLayer>(RASTER_LINE_LAYER_FID);
		LayerFactory::RegisterFactory<RasterFromDiskGuiLayer, RasterFromDiskLayer>(RASTER_DISK_LAYER_FID);
		LayerFactory::RegisterFactory<VectorGuiLayer, VectorLayer>(VECTOR_LAYER_FID);
		//classerVals.ioLayer = layersVals.getIoLayer();

		if (useAsync)
		{
			//bc::CloudPointsBarcode::drawLine = drawLine;
			//bc::CloudPointsBarcode::drawPlygon = polyPoint;
		}
	}

	// create a function that takes screenshot of a selected window

	// Main
	void RenderUI()
	{
		drawLayout();
	}

	void Close()
	{
		backend.save();
	}
}
