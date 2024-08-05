#ifdef USE_MODULE
#undef MEXPORT
#define MEXPORT export
module;
#else
#pragma once
#undef MEXPORT
#define MEXPORT
#endif

#include <exception>
#include "Barcode/PrjBarlib/include/barstrucs.h"
#include "Usings.h"
#include "Barcode/PrjBarlib/modules/tiffreader.h"
#include "MatrImg.h"
#include "../Bind/Framework.h"

#ifdef USE_MODULE
import SimpleImgReaderModule;

// import Platform;
// import ImgReader;
//import BackBind;
// import MatrModule;
module;
#endif



MEXPORT class SimpleImgReader : public ImageReader
{
	BackImage main;
	ImageType type;
	int typeSize;
	int samples;
	//	BackImage main;
		// ImageReader interface
public:
	SimpleImgReader() : ImageReader()
	{
		isTile = false;
	}
	rowptr getRowData(int ri) override;
	bool open(const BackString& path) override;
	void close() override;
	ImageType getType() override;
	int getSamples() override;
	int width() override;
	int height() override;
	float getNullValue() override;
	DataRect getRect(int stX, int stRow, int wid, int hei) override;
};


MEXPORT class DataRectBarWrapper : public bc::DatagridProvider
{
	DataRect& baseobj;

	// DatagridProvider interface
public:
	DataRectBarWrapper(DataRect& base) : baseobj(base)
	{
		type = imageTypeToBar(base.data.type, base.data.samples);
	}
	int wid() const override { return baseobj.wid; }
	int hei() const override { return baseobj.hei; }
	int channels() const override { return baseobj.data.samples; }
	void maxAndMin(Barscalar& min, Barscalar& max) const override
	{
		min = getLiner(0);
		max = min;

		for (size_t i = 1; i < this->length(); ++i)
		{
			const Barscalar& s = getLiner(i);
			if (s < min)
			{
				min = s;
			}
			if (s > max)
			{
				max = s;
			}
		}
		baseobj.maxVal = scalarToRow(max);
		baseobj.minVal = scalarToRow(min);
	}
	size_t typeSize() const override { return getImgTypeSize(baseobj.data.type); }
	Barscalar get(int x, int y) const override { return rowToScalar(baseobj.get(x, y)); }
	//BarType getType()
	//{
	//	//return baseobj.data.type;
	//}

	static Barscalar rowToScalar(const roweltype& val)
	{
		Barscalar nva;
		if (val.sampl > 1)
		{
			auto* smaplsr = val.val.rgba.samples;
			switch (val.type)
			{
			case ImageType::int8:
				return Barscalar(smaplsr[0].s, smaplsr[1].s, smaplsr[2].s);
			case ImageType::int16:
				return Barscalar(smaplsr[0].s, smaplsr[1].s, smaplsr[2].s);
			case ImageType::int32:
				return Barscalar(smaplsr[0].i, smaplsr[1].i, smaplsr[2].i);
			case ImageType::float32:
				return Barscalar(smaplsr[0].f, smaplsr[1].f, smaplsr[2].f);
			default:
				std::exception();
			}
		}
		switch (val.type)
		{
		case ImageType::int8:
			nva = Barscalar(val.val.s, BarType::BYTE8_1);
			break;
		case ImageType::int16:
			nva = Barscalar(val.val.s, BarType::FLOAT32_1);
			break;
		case ImageType::int32:
			nva = Barscalar(val.val.i, BarType::FLOAT32_1);
			break;
			//		case ImageType::argb8:
			//			nva = Barscalar(val.val.rgba.rgba[0], val.val.rgba.rgba[1], val.val.rgba.rgba[2]);
			break;
		case ImageType::float32:
			nva = Barscalar(val.val.f, BarType::FLOAT32_1);
			break;
		default:
			std::exception();
		}
		return nva;
	}

	static BarType imageTypeToBar(const ImageType type, int samples)
	{
		switch (samples)
		{
		case 1:
			switch (type)
			{
			case ImageType::int8:
				return BarType::BYTE8_1;
			case ImageType::float32:
				return BarType::FLOAT32_1;
			default:
				throw;
			}
			break;

		case 3:
			switch (type)
			{
			case ImageType::int8:
				return BarType::BYTE8_3;
			case ImageType::float32:
				return BarType::FLOAT32_3;
			default:
				throw;
			}
			break;

		case 4:
			switch (type)
			{
			case ImageType::int8:
				return BarType::BYTE8_4;
			default:
				throw;
			}
			break;

		default:
			throw;
		}
	}

	static roweltype scalarToRow(const Barscalar& val)
	{
		roweltype nva;
		switch (val.type)
		{
		case BarType::BYTE8_1:
			nva = roweltype((buchar)val.data.b1);
			break;
			//		case BarType::BYTE8_3:
			//			nva = roweltype(val.data.b3[0], val.data.b3[1], val.data.b3[2], 255);
			//			break;
		case BarType::FLOAT32_1:
			nva = roweltype(val.data.f);
			break;
		case BarType::NONE:
		default:
			std::exception();
		}
		return nva;
	}

	// DataSetProvider interface
public:
	void set(int x, int y, const Barscalar& val)
	{
		baseobj.data.setValue(y * wid() + x, scalarToRow(val).val);
	}
};
