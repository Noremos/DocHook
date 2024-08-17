#pragma once

#include <cassert>
#include <iostream>
#include <vector>

#include "MetadataIO.h"


template<class P>
void ioPoint(JsonObjectIOState* state, BackString name, P& p)
{
	state->scDouble(name + "_x", p.x);
	state->scDouble(name + "_y", p.y);
}


struct CSBinding : public IJsonIO
{
	// Projection of the local image for coord system
	BackPoint globOrigin = {0,0};
	double img_transform[6] = {0.0,1.0,0.0,0.0,0.0,1.0};
	// Define glob bottom

	void copyTo(CSBinding& other) const
	{
		other.globOrigin = globOrigin;
		memcpy(other.img_transform, img_transform, sizeof(img_transform));
	}

	void setScale(double x, double y)
	{
		img_transform[1] = x;
		img_transform[5] = y;
	}

	BackPoint getScale() const
	{
		return BackPoint(img_transform[1], img_transform[5]);
	}

	BackPoint getScaled(const BackPoint& locPoint) const
	{
		return locPoint * getScale();
	}

	double* getScaleX()
	{
		return &img_transform[1];
	}
	double* getScaleY()
	{
		return &img_transform[5];
	}

	void setOrigin(double x, double y)
	{
		globOrigin.x = x;
		globOrigin.y = y;
	}

	BackPixelPoint toLocal(const BackPoint& bp) const
	{
		//int x_loc = (x - stX) / uPerPixX;
		//int y_loc = (y - stY) / uPerPixY;
		//return { x_loc, y_loc };

		// PJ_COORD cd = getPjCoord(bp);


		// PJ_COORD local = proj_trans(proj.proj, PJ_FWD, cd);
		const BackPoint& localtr = bp;
		// const BackPoint localtr(local.xy.x, local.xy.y);

		float pixel_x, pixel_y;
		pixel_x = ((localtr.x - globOrigin.x) / img_transform[1]);
		pixel_y = ((localtr.y - globOrigin.y) / img_transform[5]);

		return BackPixelPoint(pixel_x, pixel_y);
	}

	BackPoint toGlobal(float x, float y) const
	{
		//float x_glob = x * uPerPixX + stX;
		//float y_glob = y * uPerPixY + stY;
		// y_glob = y_maximum - y * uPerPixY
		// return QgsPointXY(rectangle.xMinimum() + p[0] * pixelSizeX, rectangle.yMaximum() - p[1] * pixelSizeY)

		 // Convert the pixel coordinates to the real-world coordinates
		BackPoint r;
		r.x = globOrigin.x + (x * img_transform[1]) + (y * img_transform[2]);
		r.y = globOrigin.y + (x * img_transform[4]) + (y * img_transform[5]);
		// r.y = r.y * -1;
		// r.y = bottom - r.y;

		// PJ_COORD real = proj_trans(proj.proj, PJ_INV, getPjCoord(x, y));
		//return { real.xy.x, real.xy.y };
		return r;
	}

	// Унаследовано через IJsonIO
	virtual void saveLoadState(JsonObjectIOState* state) override
	{
		auto displayObj = state->objectBegin("ClassBinding");

		ioPoint(displayObj, "globOrigin", globOrigin);

		int size = 6;
		auto* arr = displayObj->arrayBegin("transform", size);
		for (size_t i = 0; i < size; i++)
		{
			arr->scDouble(i, img_transform[i]);
		}
		displayObj->arrayEnd();

		state->objectEnd();
	}
};

using CoordSystem = CSBinding;

struct DisplaySystem : public IJsonIO
{
	BackPoint csPos; // glob
	// static BackPoint
	double csScale;
	// BackPoint csSize; // glob

	BackPoint projItemGlobToSys(const CSBinding& itemCs, BackPoint itemPos) const
	{
		return itemPos;
	}

	// S = D * scale
	BackPoint toSysGlob(const BackPoint& display)
	{
		return (display / csScale) + csPos;
	}

	BackPoint toSysGlobRelative(const BackPoint& display)
	{
		return (display / csScale);
	}

	// D = S / scale
	BackPoint toDisplay(const BackPoint& sysGlob) const
	{
		return ((sysGlob - csPos) * csScale);
	}

	BackPoint toDisplayRelative(const BackPoint& sysGlob) const
	{
		return sysGlob * csScale;
	}

	BackPoint getSizeScale() const
	{
		return BackPoint(csScale, csScale);
	}


	// Return sys point; Перед использованием надо добавт Когда конвертим обрано в дисплей, перед выводом надо добавить getDisplayStartPos
	//BackPoint toRelativeSys(const CSBinding& itemCs, const BackPixelPoint& relativeDisplay)
	//{
	//	BackPoint bp(x, y);
	//	BackPoint pix = itemCs.toGlobal(bp.x, bp.y);
	//	return toSysGlob(pix, ds.drawSize);
	//}


	//static ImVec2 tov2(BackPixelPoint p)
	//{
	//	return ImVec2(p.x, p.y);
	//}



	//int getRealX(int x)
	//{
	//	return static_cast<float>(x - csPos) * (width / displaySize.x);
	//}

	// Унаследовано через IJsonIO
	virtual void saveLoadState(JsonObjectIOState* state) override
	{
		auto displayObj = state->objectBegin("display_system");
		ioPoint(displayObj, "csPos", csPos);
		displayObj->scDouble("csScale", csScale);
		state->objectEnd();
	}
};
