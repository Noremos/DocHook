// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
#include "Application.h"
#include "DrawCommon.h"

#include <cmath>
#include <initializer_list>
#include <memory>
#include <future>
#include <iostream>
#include "Barcode/PrjBarlib/include/barcodeCreator.h"

#include "../../backend/ProjectSettings.h"
#include "../Bind/Framework.h"

#include "../backend/MatrImg.h"
#include "../backend/Layers/layerInterface.h"
#include "../backend/Layers/Rasterlayers.h"
#include "../backend/Layers/VectorLayers.h"
#include "../backend/ScreenCapture.h"
#include "Layers/GuiRasterLayers.h"
#include "Layers/IGuiLayer.h"
#include "GuiWidgets.h"
#include "../backend/project.h"
#include "../side/clip/clip.h"
#include "Layers/GuiVectorLayers.h"
#include "../backend/CachedBarcode.h"
#include "GuiOverlap.h"
#include "Layers/GuiLayerVals.h"
#include "GuiBlock.h"

// import Platform;
// import GuiLayers;
// import BarcodeModule;
// import GuiOverlap;


// import ProjectSettings;
//import BackBind;
// import RasterLineLayerModule;

// import RasterLayers;
// import LayersCore;
// import GuiBlock;
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

	VectorLayer* vecLayer = nullptr;
	VectorLayer* addVectorLayer()
	{
		if (vecLayer == nullptr)
		{
			vecLayer = proj->addLayerData<VectorLayer>();
		}
		else
		{
			vecLayer->clear();
		}

		vecLayer->vecType = VectorLayer::VecType::polygons;
		return vecLayer;
	}


	std::vector<BarlineWrapper> barlies;
	void process(BackImage& img, VectorLayer* vec, bc::barstruct& constr, IItemFilter* filter)
	{
		// constr.proctype = bc::ProcType::f0t255;

		std::unique_ptr<bc::Baritem> citem(bc::BarcodeCreator::create(img, constr));

		bc::Baritem* item = citem.get();
		int size = (int)item->barlines.size();

		for (int i = 0; i < size; i++)
		{
			bc::barline* line = item->barlines[i];

			if (line->matr.size() < 40)
				continue;
			auto rect = line->getBarRect();

			BarlineWrapper& wrap = barlies.emplace_back(line);
			if (filter)
			{
				if (!filter->pass(&wrap))
					continue;
			}

			DrawPrimitive* priv = vec->addPrimitive({10,100,40});
			priv->addPoint(rect.x, rect.y);
			priv->addPoint(rect.x + rect.width, rect.y);
			priv->addPoint(rect.x + rect.width, rect.y + rect.height);
			priv->addPoint(rect.x, rect.y + rect.height);
		}
	}

	VectorLayer* createCacheBarcode2(IRasterLayer* inLayer, const BarcodeProperies& propertices, IItemFilter* filter = nullptr)
	{
		barlies.clear();
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

		return vec;
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

void edu()
{
	bc::barstruct constr;
	constr.proctype = bc::ProcType::Radius;

	namespace fs = std::filesystem;

	std::string dir = "/Users/sam/Edu/papers/DocHook2024/tests/images/boost/";
	// for (const auto & entry : fs::directory_iterator(path))
	for (size_t i = 1; i <= 3; i++)
	{
		std::string pathSource = dir + std::to_string(i) + ".png";
		std::string pathTemplate = dir + std::to_string(i) + "t.png";

		BackImage imgSource = imread(pathSource);
		BackImage imgTemplate = imread(pathTemplate);




		// greenSide
		// BackImage red = imgTemplate.getChannel(0);
		// std::unique_ptr<bc::Baritem> redItem(bc::BarcodeCreator::create(imgTemplate, constr));

		std::vector<std::pair<bc::BarRect, bool>> activeElements;
		BackImage green = imgTemplate.getChannel(1);
		std::unique_ptr<bc::Baritem> greenItem(bc::BarcodeCreator::create(imgTemplate, constr));

		for (int i = 0, total = greenItem->barlines.size(); i < total; i++)
		{
			bc::barline* line = greenItem->barlines[i];

			if (line->matr.size() < 40)
				continue;

			auto rect = line->getBarRect();
			activeElements.push_back({rect, false});
		}

		// BackImage blue = imgTemplate.getChannel(2);
		// std::unique_ptr<bc::Baritem> blueItem(bc::BarcodeCreator::create(imgTemplate, constr));


		// Side
		std::unique_ptr<bc::Baritem> citem(bc::BarcodeCreator::create(imgSource, constr));


		int truePositive = 0;
		int noise = 0;
		int falsePositive = 0;

		bool found = true;
		bc::Baritem* item = citem.get();
		int size = (int)item->barlines.size();
		for (int i = 0; i < size; i++)
		{
			bc::barline* line = item->barlines[i];

			if (line->matr.size() < 40)
				continue;

			int l, r, t, d;
			r = l = line->matr[0].getX();
			t = d = line->matr[0].getY();

			bool needSkip = true;
			bool equalsToBlue = true;
			for(auto& val : line->matr)
			{
				if (l > val.getX())
					l = val.getX();
				if (r < val.getX())
					r = val.getX();

				if (t > val.getY())
					t = val.getY();
				if (d < val.getY())
					d = val.getY();

				auto templScalr = imgTemplate.get(val.getX(), val.getY());
				if (templScalr[0] != 255)
				{
					needSkip = false;
				}
				if (templScalr[2] != 255)
				{
					equalsToBlue = false;
				}

			}

			if (needSkip)
				continue;

			bool dounf = false;
			bc::BarRect current(l, t, r - l + 1, d - t + 1);
			for (auto &element : activeElements)
			{
				if (element.first.isItemInside(current))
				{
					dounf = true;

					if (element.second)
					{
						++noise;
						break;
					}

					++truePositive;
					element.second = true;
					break;
				}
			}

			if (!dounf)
			{
				++falsePositive;
			}

		}

		std::cout << pathSource << std::endl;
		std::cout << truePositive << " " << noise << " " << falsePositive << std::endl;
	}

}


namespace MyApp
{
	inline int maxThreadCount, minThreadCount;

	LayersVals layersVals;// (backend);
	GuiFilter filter;

	VectorLayer* allBarcodeDisaplyLayer = nullptr;;
	VectorGuiLayer* allBarcodeDisaplyGuiLayer = nullptr;;

	VectorLayer* previewLayer = nullptr;
	VectorGuiLayer* previewGuiLayer = nullptr;

	RasterLayer* windowFrameLayer = nullptr;
	RasterGuiLayer* windowFrameGuiLayer = nullptr;

	void prepare()
	{
		windowFrameLayer = backend.proj->addLayerData<RasterLayer>();
		windowFrameGuiLayer = layersVals.addLayer<RasterGuiLayer, RasterLayer>("Window fram", windowFrameLayer);

		allBarcodeDisaplyLayer = backend.addVectorLayer();
		allBarcodeDisaplyGuiLayer = layersVals.addLayer<VectorGuiLayer, VectorLayer>("all lines vec", allBarcodeDisaplyLayer);

		previewLayer = backend.proj->addLayerData<VectorLayer>();
		previewLayer->vecType = VectorLayer::VecType::polygons;
		previewGuiLayer = layersVals.addLayer<VectorGuiLayer, VectorLayer>("selected vec", previewLayer);
	}

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
		style.FramePadding = ImVec2(5, 7);
		//style.FrameRounding            = 0.0f;
		style.ItemSpacing = ImVec2(5, 5);
		// style.PopupRounding = 1.0;
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
			if (ImGui::Button(BU8("Отмена"), ImVec2(120, 0)))
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

		backend.createCacheBarcode2(inLayer, barset);
		allBarcodeDisaplyGuiLayer->toGuiData();

		previewLayer->clear();
		allBarcodeDisaplyGuiLayer->selectedId = -1;
	}

	int width = 0;
	void createBarLayers(const BackImage& img)
	{
		windowFrameLayer->mat = img;
		windowFrameGuiLayer->toGuiData();
		windowFrameGuiLayer->lockAtThis(layersVals.lastRealSize);
		width = img.width();

		createBarWindow(windowFrameLayer);
	}

	constexpr float itemWidth = 100.0f;
	constexpr float itemHeight = 100.0f;

	std::vector<ImgData> images;
	void drawPreview()
	{
		// Draw image

		const ImGuiWindow* viewport = ImGui::GetCurrentWindow();
		const float windowWidth = viewport->Size.x * 0.8;
		const float windowHeight = viewport->Size.y * 0.8;

		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding = 1.0;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);

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

			float x = margin;
			float y = topMargin;
			for (size_t i = 0; i < images.size(); i++)
			{
				ImgData& data = images[i];
				ImGui::PushID(data.winId);

				auto textWidth = ImGui::CalcTextSize(data.name.c_str()).x;
				int maxWidth = std::max<int>(data.img.width, textWidth);
				int maxHeight = std::max<int>(data.img.height, itemHeight);

				if (x + maxWidth > windowWidth)
				{
					x = margin;
					y += itemWidthWithMargin;
				}

				ImGui::SetCursorPos(ImVec2(x, y));

				int yoff = (maxHeight - data.img.height) / 2;
				ImGui::SetCursorPos(ImVec2(x + (maxWidth - data.img.width) / 2, y + yoff));
				ImVec2 size(data.img.width, data.img.height);
				if (ImGui::ImageButton((intToStr(i) + "tb").data(), data.img.getTexturePtr(), size))
				{
					createBarLayers(data.src);
					ImGui::CloseCurrentPopup();
				}

				ImGui::SetCursorPos(ImVec2(x + (maxWidth - textWidth) / 2, y + size.y + yoff + 20));
				ImGui::Text("%s", data.name.data());
				ImGui::PopID();

				x += maxWidth + margin;
			}

			x = margin;
			y += itemWidthWithMargin;

			ImGui::SetCursorPos(ImVec2(x,y));
			ImGui::Separator();

			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button(BU8("Отмена"), ImVec2(120, 0)))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::PopStyleVar(1);
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
					if (data.height == 0 || data.width == 0 || data.data == nullptr)
						continue;

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
				// BarcodeProperies barset;
				// // RasterFromDiskLayer layer;
				// // layer.open("/Users/sam/1.png");

				// RetLayers ret = backend.createCacheBarcode2(&layer, barset, filter.getFilter());
				// layersVals.setLayers(ret, "barcode");

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
	bool filtered = false;

	class BufferText
	{
	public:
		char* getBuffer()
		{
			return buffer;
		}

		BackString getText() const
		{
			return BackString(buffer);
		}

		void setText(const BackString& text, bool manualy)
		{
			if (buffer[0] == '\0')
				blockAutoGen = false;

			if (!manualy && blockAutoGen)
				return;

			memcpy((char*)buffer, text.data(), text.length());
		}

		void markManualy()
		{
			blockAutoGen = true;
		}

	private:
		char buffer[1000] = "\0";
		bool blockAutoGen = false;
	};

	struct ImageMeta
	{
		BackString name;
		BufferText prefix;
		BufferText altText;

		float hilightColor[4] {0.f, 1.f, 0.5f, 0.f};

		void saveIamge(const BackImage& img)
		{
			BackPathStr path = getSavePath({ "Image Files", "*.png *.jpg *.jpeg *.bmp"});
			if (path.empty())
				return;

			BackImage copyimg = img;
			if (previewLayer->primitives.size() > 0)
			{
				auto* convPrim = previewLayer->primitives[0];
				auto& points = convPrim->points;
				Barscalar color(convPrim->color.r, convPrim->color.g, convPrim->color.b);
				const int thicness = std::max(copyimg.width(), copyimg.height()) * 0.1f;
				for (int i = 0; i < 4; i++)
				{
					copyimg.drawLine(points[i].x, points[i].y, points[(i + 1) % 4].x, points[(i + 1) % 4].y, color, thicness);
				}
			}

			// auto u = backend.proj->addLayerData<RasterLayer>();
			// u->mat = copyimg;
			// layersVals.addLayer<RasterGuiLayer, RasterLayer>("Test", u);

			imwrite(path, copyimg);
			// Remove filename from path
			name = path.filename().string();
			path = path.parent_path();
			prefix.setText(path.string(), false);

		}


		void savePart(const BackImage& img)
		{
			BackPathStr path = getSavePath({ "Image Files", "*.png *.jpg *.jpeg *.bmp"});
			if (path.empty())
				return;

			BackImage copyimg = img;
			if (previewLayer->primitives.size() > 0)
			{
				auto* convPrim = previewLayer->primitives[0];
				auto& points = convPrim->points;
				Barscalar color(convPrim->color.r, convPrim->color.g, convPrim->color.b);

				copyimg = img.getRect(points[0].x, points[0].y, points[1].x - points[0].x, points[3].y - points[0].y);
			}

			// auto u = backend.proj->addLayerData<RasterLayer>();
			// u->mat = copyimg;
			// layersVals.addLayer<RasterGuiLayer, RasterLayer>("Test", u);

			imwrite(path, copyimg);
			// Remove filename from path
			name = path.filename().string();
			path = path.parent_path();
			prefix.setText(path.string(), false);

		}

		BackString getPath() const
		{
			// return (BackPathStr(prefix.getText()) / name).string();
			return prefix.getText() + "/" + name;
		}

		BackString getRstText() const
		{
			using namespace std::string_literals;
			BackString text = ".. image:: "s + getPath();
			text += "\n   :width: "s + std::to_string(width);
			text += "\n   :alt: "s + altText.getText() + "\n"s;
			return text;
		}
		// void genRandomName()
		// {
		// 	static const char* const vowels = "aeiouy";
		// 	static const char* const consonants = "bcdfghjklmnpqrstvwxz";

		// 	std::string name;
		// 	name += consonants[rand() % strlen(consonants)];
		// 	name += vowels[rand() % strlen(vowels)];
		// 	name += consonants[rand() % strlen(consonants)];
		// 	name += vowels[rand() % strlen(vowels)];
		// 	name += consonants[rand() % strlen(consonants)];

		// 	this->name.setText(name, true);
		// }
	};

	ImageMeta imgMeta;
	int sideWidth = 300;

	class TextTemplate
	{
	public:
		std::vector<BackString> templates;

		TextTemplate()
		{
			templates.push_back(getRstTemplate());
			templates.push_back(getLaTexTemplate());
		}

		static BackString getRstTemplate()
		{
			using namespace std::string_literals;
			BackString text =
".. image:: <path>\n"s
"   :width: <width>\n"s
"   :alt: <text>\n"s;
			return text;
		}
		static BackString getLaTexTemplate()
		{
			using namespace std::string_literals;
			BackString text =
			"\begin{figure}[h]\n"s
			"   \\centering\n"s
			"   \\includegraphics[width=<width>\\textwidth]{<path>}\n"s
			"   \\caption{<text>}\n"
			"   \\label{fig:YourLabel}\n"s
			"\\end{figure}"s;
			return text;
		}


		bool replace(std::string& str, const std::string& from, const std::string& to)
		{
			size_t start_pos = str.find(from);
			if(start_pos == std::string::npos)
				return false;
			str.replace(start_pos, from.length(), to);
			return true;
		}


	public:
		BackString getPreparedString(int id)
		{
			BackString ttemplate = templates[id];

			replace(ttemplate, "<path>", imgMeta.getPath());
			replace(ttemplate, "<text>", imgMeta.altText.getText());
			replace(ttemplate, "<width>", std::to_string(width));

			return ttemplate;
		}

	};


	TextTemplate templ;
	SelectableKeyValues<int> templCombo({{0, "RST"}, {0, "LaTeX"}});
	BufferText templateTemplate;

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
		bool selectToNetxTab = false;
		if (centerVals.resizble.Begin("ImagePreview"))
		{
			windowFrameGuiLayer->lockAtThis(layersVals.lastRealSize);
			layersVals.draw(guiDisplay);
			if (centerVals.resizble.clicked)
			{
				layersVals.onClick(guiDisplay, centerVals.resizble.clickedPos);
				if (allBarcodeDisaplyGuiLayer->selectedId != -1)
				{
					previewLayer->clear();
					auto* prim = previewLayer->addPrimitive({ 0, 255, 0 });
					*prim = *backend.vecLayer->primitives[allBarcodeDisaplyGuiLayer->selectedId];
					previewGuiLayer->toGuiData();

					selectToNetxTab = true;
				}
			}

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
		if (ImGui::BeginViewportSideBar("Sidebar", NULL, ImGuiDir_Right, sideWidth, window_flags))
		{
		// if (ImGui::Begin("Sidebar", NULL, window_flags))

			// Draw 3 tabs
			if (ImGui::BeginTabBar("MyTabBar"))
			{
				if (ImGui::BeginTabItem("Фильтр"))
				{
					allBarcodeDisaplyLayer->visible = true;
					previewLayer->visible = false;

					ImGui::Checkbox("Фильтровать", &filtered);
					if (filtered && filter.draw())
					{
						auto* gfilter = filter.getFilter();
						if (gfilter)
						{
							gfilter->imgLen = windowFrameLayer->mat.length();
							for (size_t i = 0; i < backend.barlies.size(); i++)
							{
								allBarcodeDisaplyLayer->primitives[i]->visible = (gfilter->pass(&backend.barlies[i]));
							}
						}
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Вывод", nullptr, selectToNetxTab ? ImGuiTabItemFlags_SetSelected : 0))
				{
					ImGui::BeginDisabled(previewLayer->primitives.size() == 0);
					ImGui::ColorPicker3("Цвет", imgMeta.hilightColor);
					if (previewLayer->primitives.size() == 1)
					{
						previewLayer->primitives[0]->color.r = 255 * imgMeta.hilightColor[0];
						previewLayer->primitives[0]->color.g = 255 * imgMeta.hilightColor[1];
						previewLayer->primitives[0]->color.b = 255 * imgMeta.hilightColor[2];
					}

					allBarcodeDisaplyLayer->visible = false;
					previewLayer->visible = true;
					ImGui::EndDisabled();
					// ImGui::InputText("Имя изображения", imgMeta.name.getBuffer(), 1000);
					// ImGui::SameLine();
					if (previewLayer->primitives.size() == 1)
					{
						if (ImGui::Button("Снять выделение"))
						{
							previewLayer->clear();
							previewGuiLayer->toGuiData();

							allBarcodeDisaplyGuiLayer->selectedId = -1;
						}
					}

					ImGui::LabelText("##Save", "Сохранить");
					if (ImGui::Button("Изображение"))
					{
						imgMeta.saveIamge(windowFrameLayer->mat);
					}
					ImGui::SameLine();
					if (ImGui::Button("Выделенное"))
					{
						imgMeta.savePart(windowFrameLayer->mat);
					}

					ImGui::Separator();
					// get path and pass it to prefix



					templCombo.drawCombobox("Шаблон", 100);
					ImGui::SameLine();
					if (ImGui::Button("Изменить"))
					{
						// open popup
						ImGui::OpenPopup("EditAltText");
						templateTemplate.setText(templ.templates[templCombo.currentIndex], false);
					}

					if (ImGui::BeginPopupModal("EditAltText", NULL, ImGuiWindowFlags_AlwaysAutoResize))
					{
						if (ImGui::Button("Добавить шаблон"))
						{
							templ.templates.emplace_back() = templ.templates[templCombo.currentIndex];
							templateTemplate.setText(templ.templates.back(), false);

							templCombo.add("User Defined", templ.templates.size() - 1);
							templCombo.endAdding();
						}

						ImGui::InputTextMultiline("Альтернативный текст", templateTemplate.getBuffer(), 1000);

						if (ImGui::Button("Закрыть"))
						{
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndPopup();
					}


					if (ImGui::InputText("Префикс", imgMeta.prefix.getBuffer(), 1000))
						imgMeta.prefix.markManualy();

					ImGui::InputInt("Ширина", &width);
					ImGui::InputTextMultiline("Альтернативный текст", imgMeta.altText.getBuffer(), 1000);


					if (ImGui::Button("Скопировать текст"))
					{
						BackString outText;
						outText = templ.getPreparedString(templCombo.currentIndex);
						clip::set_text(outText);
					}

					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}

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
		// layersVals.drawLayersWindow();
		// drawBottomBar();

		commonValus.onAirC();

		// ImGui::ShowDemoWindow();
	}

	constexpr ImVec2 toDVec2(const bc::point& p)
	{
		return ImVec2(static_cast<float>(p.x), static_cast<float>(p.y));
	}

	void Init(const char* root)
	{
		// ::edu();
		// exit(0);
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
		ImGui::StyleColorsDark();
		MyApp::setImGuiStyle(1.0);
		prepare();


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

		LayerFactory::RegisterFactory<RasterGuiLayer, RasterLayer>(RASTER_LAYER_FID);
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
