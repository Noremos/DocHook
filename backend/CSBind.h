#ifdef USE_MODULE
#undef MEXPORT
#define MEXPORT export
module;
#else
#pragma once
#undef MEXPORT
#define MEXPORT
#endif

#include <cassert>
#include <iostream>
#include <vector>

#ifdef GEOPROJ
#include <proj.h>
#endif

#include <sqlite3.h>
#include "Usings.h"

#ifdef USE_MODULE
export module CSBind;

import MetadataCoreIO;
//import BackBind;
#else
#include "CSBind.h"
#include "MetadataIO.h"
#endif


#ifdef GEOPROJ

template<typename T>
PJ_COORD getPjCoord(T x, T y)
{
	PJ_COORD cd{};
	cd.xy.x = x;
	cd.xy.y = y;
	return cd;
}

template<typename T>
PJ_COORD getPjCoord(const T& p)
{
	PJ_COORD cd{};
	cd.xy.x = p.x;
	cd.xy.y = p.y;
	return cd;
}
#endif


template<class P>
void ioPoint(JsonObjectIOState* state, BackString name, P& p)
{
	state->scDouble(name + "_x", p.x);
	state->scDouble(name + "_y", p.y);
}

MEXPORT class BackProj
{
private:

#ifdef GEOPROJ
	PJ* proj = nullptr;
	PJ_CONTEXT* ctx = nullptr;
#endif
	friend struct CSBinding;

	int id = -1;
public:

	bool init(int nid)
	{
		if (nid < 0)
			return false;

		// Initialize Proj context
		sqlite3* db = NULL;
		char* strerr = NULL;
		sqlite3_stmt* stmt = prepareSelect(db, "srtext", " WHERE srid = ?");

		auto rc = sqlite3_bind_int(stmt, 1, nid); // bind the rowid value to the first placeholder
		if (rc != SQLITE_OK)
		{
			printf("Failed to bind: %s\n", sqlite3_errmsg(db));
			return false;
		}

		rc = sqlite3_step(stmt);
		if (rc != SQLITE_ROW)
		{
			printf("There is not row with id %d\n", nid);
			return false;
		}

#ifdef GEOPROJ
		const buchar* wtkContext = sqlite3_column_text(stmt, 0);
		const char* cpath = Variables::prodDbPath.c_str();
		ctx = proj_context_create();
		proj_context_set_search_paths(ctx, 1, &cpath);
		proj = proj_create_from_wkt(ctx, (const char*)wtkContext, nullptr, nullptr, nullptr);
#endif

		sqlite3_finalize(stmt); // Do not close before wtkContext is using
		sqlite3_close(db);

		this->id = nid;
		return true;
	}


#ifdef GEOPROJ
	bool isInited() const
	{
		return proj != nullptr;
	}

	bool reinit(int ind)
	{
		if (proj)
		{
			proj = proj_destroy(proj);
			proj = nullptr;
		}

		if (ctx)
		{
			proj_context_destroy(ctx);
			ctx = nullptr;
		}

		return init(ind);
	}
	const char* getName() const
	{
		return proj_get_name(proj);
	}
	BackPoint getThisProj(const BackProj& item, BackPoint itemPos, const bool normalize) const
	{
		if (item.getId() == getId())
			return itemPos;

		PJ* ctc_proj = proj_create_crs_to_crs_from_pj(item.ctx, item.proj, proj, nullptr, nullptr);
		assert(ctc_proj != nullptr);

		if (normalize)
			ctc_proj = proj_normalize_for_visualization(item.ctx, ctc_proj);

		PJ_COORD real = proj_trans(ctc_proj, PJ_IDENT, getPjCoord(itemPos));
		return BackPoint(real.xy.x, real.xy.y);
	}
#else
	bool isInited() const
	{
		return id != -1;
	}

	bool reinit(int ind)
	{
		id = ind;
		return true;
	}
	BackPoint getThisProj(const BackProj& item, BackPoint itemPos, const bool normalize) const
	{
		if (item.getId() == getId())
			return itemPos;

		return itemPos;
	}
#endif


	int getId() const
	{
		return id;
	}

	static sqlite3_stmt* prepareSelect(sqlite3* db, const char* name, const char* where = NULL)
	{
	auto rc = sqlite3_open_v2((Variables::metaPath / "proj.sqlite").string().c_str(), &db, SQLITE_OPEN_READONLY, NULL);
		if (rc != SQLITE_OK)
		{
			std::cerr << "������ ��������/�������� ��: " << sqlite3_errmsg(db) << std::endl;
			return NULL;
		}

		BackString sql = "SELECT ";
		sql += name;
		sql += " FROM spatial_ref_sys ";
		if (where)
			sql += where;

		sqlite3_stmt* stmt;
		rc = sqlite3_prepare_v2(db, sql.c_str(), sql.length(), &stmt, nullptr);
		if (rc != SQLITE_OK)
		{
			printf("Failed to prepare statement: %s\n", sqlite3_errmsg(db));
			return NULL;
		}

		return stmt;
	}

	static std::vector<BackString> getWtkNames()
	{
		sqlite3* db = NULL;
		sqlite3_stmt* stmt = prepareSelect(db, "srid");

		std::vector<BackString> names;
		while (sqlite3_step(stmt) == SQLITE_ROW)
		{
			const buchar* wtkContext = sqlite3_column_text(stmt, 0);
			names.push_back(BackString("EPSG:") + (const char*)wtkContext);
		}

		sqlite3_finalize(stmt);
		sqlite3_close(db);

		return names;
	}

#ifdef GEOPROJ
	~BackProj()
	{
		if (proj)
		{
			proj = proj_destroy(proj);
			proj = nullptr;
		}

		if (ctx)
		{
			proj_context_destroy(ctx);
			ctx = nullptr;
		}
	}
#endif
};


//
//MEXPORT class CoordSystem
//{
//	float stX, stY;
//	float maxX, maxY;
//	float uPerPixX;
//	float uPerPixY;
//
//	bc::point toLocal(float x, float y)
//	{
//		int x_loc = (x - stX) / uPerPixX;
//		int y_loc = (y - stY) / uPerPixY;
//		return { x_loc, y_loc };
//	}
//
//	BackPoint toGlobal(int x, int y)
//	{
//		float x_glob = x * uPerPixX + stX;
//		float y_glob = y * uPerPixY + stY;
//		// y_glob = y_maximum - y * uPerPixY
//		// return QgsPointXY(rectangle.xMinimum() + p[0] * pixelSizeX, rectangle.yMaximum() - p[1] * pixelSizeY)
//		return { x_glob, y_glob };
//	}
//};


MEXPORT struct CSBinding : public IJsonIO
{
	// Projection of the local image for coord system
	BackProj proj;
	BackPoint globOrigin = {0,0};
	double img_transform[6] = {0.0,1.0,0.0,0.0,0.0,1.0};
	// Define glob bottom

	void copyTo(CSBinding& other) const
	{
		other.initProj(proj.id);
		other.globOrigin = globOrigin;
		memcpy(other.img_transform, img_transform, sizeof(img_transform));
	}

	void initProj(const int id)
	{
		proj.init(id);
	}

	void init(const int id)
	{
		proj.init(id);
	}

	void init(const char* id)
	{
		proj.init(strToInt(id));
	}

	void init(const BackString& id)
	{
		proj.init(strToInt(id));
	}

	int getProjId() const
	{
		return proj.getId();
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

		int pid = proj.getId();
		displayObj->scInt("projection", pid);
		if (state->isReading())
		{
			proj.init(pid);
		}

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

MEXPORT using CoordSystem = CSBinding;

MEXPORT struct DisplaySystem : public IJsonIO
{
	BackPoint csPos; // glob
	// static BackPoint
	double csScale;
	// BackPoint csSize; // glob
	BackProj sysProj; // Dest/system coord system

	BackPoint projItemGlobToSys(const CSBinding& itemCs, BackPoint itemPos) const
	{
		auto p = sysProj.getThisProj(itemCs.proj, itemPos, false);
		return p;
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

		int id = sysProj.getId();
		displayObj->scInt("proj_id", id);
		if (state->isReading())
		{
			sysProj.init(id);
		}

		state->objectEnd();
	}
};
