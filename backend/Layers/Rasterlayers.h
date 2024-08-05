#ifdef USE_MODULE
#undef MEXPORT
#define MEXPORT export
module;
#else
#pragma once
#undef MEXPORT
#define MEXPORT
#endif


#include <memory>
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <cassert>
#include "Barcode/PrjBarlib/include/barline.h"
#include "Barcode/PrjBarlib/modules/tiffreader.h"
#include "Common.h"


#include "../MetadataIO.h"
#include "../MatrImg.h"

#include "../Bind/MHashMap.h"
#include "../MLSettings.h"
#include "../Bind/Framework.h"

#ifdef USE_MODULE
export module RasterLayers;
import LayersCore;


// import BarTypes;
// import Platform;

// import MetadataCoreIO;

// import ImgReader;
import SimpleImgReaderModule;
// import MHashMap;
// import MLSettings;
//import BackBind;
// import MatrModule;
#else
#include "layerInterface.h"
#include "../SimpleImgReader.h"
#endif

using LayerMetaProvider = MetadataProvider;


MEXPORT enum class ReadType
{
	Tiff,
	Simple
};



inline int getFid(int wid, int s)
{
	return (wid + s - 1) / s;
}

inline BackImage tiffToImg(ImageReader* reader, const BackPathStr& path, int fctor = 10, bool save = false)
{
	const int rwid = reader->width();
	const int rhei = reader->height();
	const bool rgb = reader->getSamples() > 1;
	const float NAN_VALUE = reader->getNullValue();

	if (fctor == 1)
	{
		BackImage out(rwid, rhei, 3);

		auto rp = reader->getRect(0,0, rwid, rhei);

		if (rgb)
		{
			for (size_t i = 0; i < out.length(); i++)
			{
				auto value = rp.data.getValueV(i);
				if (value.avg() == NAN_VALUE)
				{
					out.setLiner(i, Barscalar(0, 0, 0));
				}
				else
				{
					out.setLiner(i, Barscalar(0, 0, 0));

					auto& r = value.val.rgba;
					out.setLiner(i, Barscalar(r.samples[0].s, r.samples[1].s, r.samples[2].s));
				}
			}
		}
		else
		{
			for (size_t i = 0; i < out.length(); i++)
			{
				float value = rp.data.getFloat(i);
				if (value == NAN_VALUE)
				{
					out.setLiner(i, Barscalar(0, 0, 0));
				}
				else
				{
					if (value < 0)
						value = 0;
					else if (value > 255)
						value = 255;

					out.setLiner(i, Barscalar(value, value, value));
				}
			}
		}

		if (save)
			imwrite(path, out);
		return out;
	}
	else
	{
		const int widf = getFid(rwid, fctor);
		const int heif = getFid(reader->height(), fctor);
		BackImage outr(widf, heif, 3);

		float NAN_VALUE = reader->getNullValue();
		for (int h = 0, hr = 0; hr < heif; h += fctor, ++hr)
		{
			const rowptr& rp = reader->getRowData(h);
			if (rgb)
			{
				for (int w = 0, wr = 0; wr < widf; w += fctor, ++wr)
				{
					auto value = rp.getValueV(w);
					if (value.avg() == NAN_VALUE)
					{
						outr.set(wr, hr, Barscalar(0, 0, 0));
					}
					else
					{
						auto& r = value.val.rgba;
						outr.set(wr, hr, Barscalar(r.samples[0].s, r.samples[1].s, r.samples[2].s));
					}
				}
			}
			else
			{
				for (int w = 0, wr = 0; wr < widf; w += fctor, ++wr)
				{
					float value = rp.getFloat(w);
					if (value == NAN_VALUE)
					{
						outr.set(wr, hr, Barscalar(0, 0, 0));
					}
					else
					{
						if (value < 0)
							value = 0;
						else if (value > 255)
							value = 255;

						outr.set(wr, hr, Barscalar(value, value, value));
					}
				}
			}
		}

		if (save)
			imwrite(path, outr);
		return outr;
	}
}


MEXPORT struct BarcodeProperies : public MLSettings
{
	bc::ComponentType comtype;
	bc::ProcType proctype;
	bc::ColorType coltype;

	bc::barstruct barstruct;
	bc::AttachMode attachMode;
	BarcodeProperies() : MLSettings({})
	{
		OptionValue comp("Тип", {});
		comp.data.e->add("Компонента", bc::ComponentType::Component);
		comp.data.e->add("Дыра", bc::ComponentType::Hole);
		values.push_back(comp);

		comp.data.e->clear();
		comp.name = "Сортировка";
		comp.data.e->add("От 0 до 255", bc::ProcType::f0t255);
		comp.data.e->add("От 255 до 0", bc::ProcType::f255t0);
		comp.data.e->add("По расстоянию", bc::ProcType::Radius);
		values.push_back(comp);

		comp.data.e->clear();
		comp.name = "Цвет";
		comp.data.e->add("Как в изображении", bc::ColorType::native);
		comp.data.e->add("Серый", bc::ColorType::gray);
		comp.data.e->add("Цветной", bc::ColorType::rgb);
		values.push_back(comp);

		comp.data.e->clear();
		comp.name = "Присоединение";
		comp.data.e->add("firstEatSecond", bc::AttachMode::firstEatSecond);
		comp.data.e->add("secondEatFirst", bc::AttachMode::secondEatFirst);
		comp.data.e->add("createNew", bc::AttachMode::createNew);
		comp.data.e->add("dontTouch", bc::AttachMode::dontTouch);
		comp.data.e->add("morePointsEatLow", bc::AttachMode::morePointsEatLow);
		comp.data.e->add("closer", bc::AttachMode::closer);
		values.push_back(comp);

		//comp.data.e->clear();
		//comp.name = "Алг";
		//comp.data.e->add("Растровый", 0);
		//comp.data.e->add("Растр в точки", 1);
		// values.push_back(comp);

		values.push_back({ "trueSort", false });
	}

	bc::barstruct get() const
	{
		bc::barstruct st;
		st.comtype = getEnumValue<bc::ComponentType>("Тип");
		st.proctype = getEnumValue<bc::ProcType>("Сортировка");
		st.coltype = getEnumValue<bc::ColorType>("Цвет");
		st.attachMode = getEnumValue<bc::AttachMode>("Присоединение");
		st.createBinaryMasks = true;
		st.createGraph = true;
		st.returnType = bc::ReturnType::barcode2d;
		st.trueSort = *getBool("trueSort");
		return st;
	}
	// ---


	//SelectableKeyValues<int> alg =
	//{
	//	{0, "Растровый"},
	//	{1, "Растр в точки"}
	//};
	int alg = 0; // 0 - raster; 1 - cloud
	bool alg1UseHoles = false;
	bool alg1IgnoreHeight = false;
};


MEXPORT class RasterLayer : public IRasterLayer
{
public:
	BackImage mat;
	float aspect = 1.f;

	virtual const LFID getFactoryId() const override
	{
		return RASTER_LAYER_FID;
	}

	virtual void saveLoadState(JsonObjectIOState* state, const MetadataProvider& metaFolder) override
	{
		IRasterLayer::saveLoadState(state, metaFolder);

		LayerMetaProvider layerMeta(getLayerMeta(metaFolder));
		layerMeta.mkdir();
		// int imgId = metaFolder.getUniqueId();
		const BackPathStr path = layerMeta.getSubFolder("mat.png");

		if (state->isReading())
		{
			mat = imread(path);
			// std::remove(path.string().c_str());
		}
		else
			imwrite(path, mat);
	}

	//void release(MetadataProvider mprov)
	//{
	//	const BackPathStr path = mprov.getPath(imgId, ".png");
	//	std::remove(path.string().c_str());
	//}

	void init(int wid, int hei, int chls, int mtileSize = DEF_TILE_SIZE)
	{
		mat.reinit(wid, hei, chls);
		prov.init(wid, hei, wid, mtileSize);
	}

	void init(const BackImage& src, int mtileSize = DEF_TILE_SIZE)
	{
		mat.assignCopyOf(src);
		prov.init(src.width(), src.height(), src.width() * aspect, mtileSize);
	}

	void init(IRasterLayer* layer)
	{
		int wid = layer->displayWidth();
		int hei = layer->displayHeight();

		mat.reinit(wid, hei, 4);
		prov = layer->prov;
	}

	bc::point minmax(const bc::point& p) const
	{
		return { std::min(p.x, mat.width() - 1), std::min(p.y, mat.height() - 1) };
	}


	// ������������ ����� IRasterLayer
	virtual void setCache(size_t) override
	{ }

	virtual int realWidth() const override
	{
		return mat.width() * aspect;
	}
	virtual int realHeight() const override
	{
		return mat.height() * aspect;
	}

	virtual float displayWidth() const override
	{
		return mat.width();
	}
	virtual float displayHeight() const override
	{
		return mat.height();
	}

	virtual BackImage getRect(int stX, int stRow, int wid, int hei) override
	{
		return mat.getRect(stX, stRow, wid, hei);
	}
	virtual BackImage getImage(const int max) const override
	{
		return mat;
	}
	virtual const BackImage* getCachedImage() const override
	{
		return &mat;
	}

	virtual SubImgInf getSubImgInf() override
	{
		return SubImgInf(mat.width(), mat.height());
	}

	void setSubImage(int,bool = false) override {}
	int getSubImage() override {return 0;}
	int getFirstSmallIndex(const int maxSize = 2000) override {return 0;}
	std::vector<SubImgInf> getSubImageInfos() override
	{
		std::vector<SubImgInf> sub;
		//sub.push_back({ mat.width(), mat.height()});
		return sub;
	}
};

MEXPORT struct SimpleLine;
MEXPORT struct SimpleLineHolder
{
	//std::vector<std::unique_ptr<SimpleLine>> holder;
	std::unordered_map<size_t, std::unique_ptr<SimpleLine>> holder;
};

struct SimpleLine
{
	int tileId, barlineIndex;
	SimpleLine(int tileId = 0, int barlineIndex = 0) :
	tileId(tileId), barlineIndex(barlineIndex), start(uchar(0)), end(uchar(0)), depth(0), matrSrcSize(0)
	{}
	//	bushort counter = 0;
	Barscalar start, end;

	int depth;
	int matrSrcSize;

	buint parent = -1;
	std::vector<buint> children;

	bc::barvector matr;
	SimpleLineHolder* root;

	SimpleLine* getParent() const
	{
		if (parent == -1)
			return nullptr;

		return root->holder[parent].get();
	}
	SimpleLine* getChild(buint id) const
	{
		return root->holder[children[id]].get();
	}
	int getDeath() const
	{
		return depth;
	}
};

MEXPORT using IdGrater = MMMAP<size_t, SimpleLine*>;
MEXPORT class RasterFromDiskLayer : public IRasterLayer
{
public:
	ImageReader* reader = nullptr;

	ReadType imgType;
	BackPathStr imgPath;

	// notSave
	// MetadataProvider* mprov;

	int subImageIndex = 0;
	std::vector<BackImage> images;
	int subImgSize;
	BackPixelPoint realSize;

	~RasterFromDiskLayer()
	{
		closeImages();
	}

	virtual const LFID getFactoryId() const override
	{
		return RASTER_DISK_LAYER_FID;
	}

	virtual void saveLoadState(JsonObjectIOState* state, const MetadataProvider& metaFolder) override
	{
		// this->mprov = &metaFolder;
		IRasterLayer::saveLoadState(state, metaFolder);

		state->scPath("imgPath", imgPath);
		state->scInt("subImgSize", subImgSize);
		state->scInt("subImageIndex", subImageIndex);
		int iimgType = (int)imgType;
		state->scInt("imgType", iimgType);
		imgType = (ReadType)iimgType;

		if (state->isReading())
		{
			openReader();
			readImagesFromCache(metaFolder);
			setSubImage(subImageIndex);
		}
	}

	void open(const BackPathStr& path, MetadataProvider metaPath)
	{
		closeReader();

		imgPath = path;
		name = imgPath.filename().string();
		// this->mprov = &metaPath;

		openReader();

		setSubImage(0, true);
		if (!reader->ready)
			return;
	}

	void cache(const MetadataProvider& metaPath)
	{
		writeImages(metaPath);
	}

	int getSubImage() override
	{
		return subImageIndex;
	}

	SubImgInf getSubImgInf() override
	{
		return SubImgInf(reader->width(), reader->height());
	}

	void setSubImage(int imgIndex, bool overrideCs = false) override
	{
		int iwid = 2000;

		if (imgType == ReadType::Tiff)
		{
			TiffReader* treader = static_cast<TiffReader*>(reader);
			for (int i = imgIndex; i < treader->getSubImageSize(); ++i)
			{
				//int factor = 1;
				int tegWid = treader->getSubImg(i)->tags.ImageWidth;
				if (tegWid <= 2000)
				{
					iwid = tegWid;
					break;
				}
			}

			treader->setCurrentSubImage(imgIndex);

			if (overrideCs)
			{
				const auto& tags = treader->getTags();

				auto& p = tags.ModelTiepointTag.points[0];
				cs.setOrigin(p.X, p.Y);
				cs.setScale(tags.ModelPixelScaleTag.x, tags.ModelPixelScaleTag.y);
				cs.proj.reinit(treader->getProjection());
			}

			subImageIndex = imgIndex;
		}
		else
		{
			iwid = reader->width();
			subImageIndex = 0;
		}

		prov.update(realSize.x, realSize.y, iwid); // restor it when images will be dropped

		// prov.update(realWidth, realHeight, reader->width()); // restor it when images will be dropped
		int mmax = std::max(prov.width, prov.height);
		if (prov.tileSize + tileOffset > mmax)
		{
			tileOffset = mmax - prov.tileSize;
			if (tileOffset < 0)
				tileOffset = 0;
		}
	}

	int getFirstSmallIndex(const int maxSize = 2000) override
	{
		if (!reader)
			return -1;
		if (imgType != ReadType::Tiff)
			return 0;

		TiffReader* treader = dynamic_cast<TiffReader*>(reader);

		int s = treader->getSubImageSize();
		//BackPathStr tiles = getPath(BackPath::tiles);
		for (int i = 0; i < s; ++i)
		{
			treader->setCurrentSubImage(i);
			if (treader->width() <= maxSize)
			{
				return i;
			}
		}

		return -1;
	}

	std::vector<SubImgInf> getSubImageInfos() override
	{
		std::vector<SubImgInf> info;
		if (imgType != ReadType::Tiff)
		{
			//info.push_back({ realWidth(), realHeight() });
			return info;
		}

		TiffReader* treader = static_cast<TiffReader*>(reader);

		auto osd = treader->getSumImageInfos();
		for (auto& o : osd)
		{
			info.push_back({ o.width, o.height });
		}
		return info;
	}


	void closeReader()
	{
		if (reader)
		{
			reader->close();
			delete reader;
			reader = nullptr;
		}
	}

	void openReader()
	{
		if (!reader)
		{
			const auto& ext = imgPath.extension();
			if (ext == ".tiff" || ext == ".tif")
			{
				imgType = ReadType::Tiff;
				reader = new TiffReader();
			}
			else
			{
				imgType = ReadType::Simple;
				reader = new SimpleImgReader();
			}
		}

		if (!reader->ready)
			reader->open(imgPath.string());

		if (imgType == ReadType::Tiff)
		{
			TiffReader* treader = static_cast<TiffReader*>(reader);
			auto& realTags = treader->getSubImg(0)->tags;
			realSize.x = realTags.ImageWidth;
			realSize.y = realTags.ImageLength;
		}
		else
		{
			realSize.x = reader->width();
			realSize.y = reader->height();
		}
	}

	void writeImages(MetadataProvider metaprov)
	{
		if (!reader)
			return;

		MetadataProvider layerMeta(getLayerMeta(metaprov));
		BackDirStr tiles = layerMeta.getSubFolder("tiles");

		closeImages();
		images.clear();

		layerMeta.mkdir();

		mkDirIfNotExists(tiles);

		if (imgType == ReadType::Tiff)
		{
			TiffReader* trear = static_cast<TiffReader*>(reader);
			subImgSize = trear->getSubImageSize();
			for (int i = 0, k = 0; i < subImgSize; ++i)
			{
				//int factor = 1;
				trear->setCurrentSubImage(i);
				if (reader->width() <= 2000)
				{
					images.push_back(tiffToImg(reader, tiles / (intToStr(k++).append(".png")), 1.0, true));
					break;
				}
			}

			if (images.size() == 0)
			{
				int factor = reader->width() / 2000;
				images.push_back(tiffToImg(reader, tiles / (intToStr(0).append(".png")), factor, true));
			}

			trear->setCurrentSubImage(subImageIndex);
		}
		else
		{
			images.push_back(tiffToImg(reader, tiles / (intToStr(0).append(".png")), 1, true));
		}

		subImgSize = images.size();
	}


	void readImagesFromCache(const MetadataProvider& metaprov)
	{
		if (!reader)
			return;

		LayerMetaProvider layerMeta(getLayerMeta(metaprov));
		BackDirStr tiles = layerMeta.getSubFolder("tiles");

		for (int i = 0; i < subImgSize; ++i)
		{
			// int factor = 1;
			BackPathStr stp = tiles / (intToStr(i).append(".png"));
			images.push_back(imread(stp));
		}
	}


	void closeImages()
	{
		//for (int i = 0; i < images.size(); ++i)
		//	delete images[i];
		images.clear();
	}

	// ������������ ����� IRasterLayer

	virtual void setCache(size_t rowsSize) override
	{
		if (imgType == ReadType::Tiff)
		{
			TiffReader* trear = static_cast<TiffReader*>(reader);
			trear->cachedRows.setMaxSize(rowsSize);
			trear->setMaxTilesCacheSize(rowsSize);
		}
	}

	virtual float displayWidth() const override
	{
		return realSize.x / prov.displayFactor;
	}
	virtual float displayHeight() const override
	{
		return realSize.y / prov.displayFactor;
	}
	virtual int realWidth() const override
	{
		return realSize.x;
	}
	virtual int realHeight() const override
	{
		return realSize.y;
	}
	virtual BackImage getRect(int stX, int stRow, int wid, int hei) override
	{
		DataRect r = reader->getRect(stX, stRow, wid, hei);
		BarType btype = DataRectBarWrapper::imageTypeToBar(r.data.type, r.samples());
		return BackImage(r.wid, r.hei, r.samples(), btype,  r.getData());
	}
	virtual BackImage getImage(const int) const override
	{
		return images[0];
	}
	virtual const BackImage* getCachedImage() const override
	{
		return &images[0];
	}
};
