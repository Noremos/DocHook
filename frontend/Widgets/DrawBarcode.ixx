module;
#include "../DrawCommon.h"
#include <utility>
#include <vector>
#include "../../backend/Layers/Rasterlayers.h"

export module BarcodeChart;

// import RasterLayers;


class SavedLines
{
	struct DrawLine
	{
		short start, end, depth, lineNumber;
	};
	short minDepth = 0;
	short maxDepth = 0;
	std::vector<DrawLine> lines;
	bool deleted = false;
	float maxLen = 0;
	int ldepts = 1;
	short maxEnd = 0;

	void getLines(const SimpleLine& line, int& ldepts)
	{
		short st = line.start.getAvgFloat();
		short ed = line.end.getAvgFloat();
		if (st > ed)
			std::swap(st, ed);

		if (line.depth > maxDepth)
			maxDepth = line.depth;
		if (line.depth < minDepth)
			minDepth = line.depth;

		//if (ldepts > this->ldepts)
		//	this->ldepts = ldepts;

		//auto len = line.end.absDiff(st).getAvgFloat();
		if (ed > maxEnd)
			maxEnd = ed;

		lines.push_back(DrawLine{ st, ed, (short)line.depth, (short)ldepts});
		int d = ldepts;

		for (size_t i = 0; i < line.children.size(); i++)
		{
			++ldepts;
			getLines(*line.getChild(i), ldepts);
		}

		//ldepts = d;
	}

public:
	void get(const SimpleLine& line)
	{
		lines.clear();
		maxDepth = minDepth = line.depth;
		maxLen = 0;
		ldepts = 0;
		maxEnd = 0;

		//int dldepts = 1;
		getLines(line, ldepts);
	}

	enum DrawOptions
	{
		WithBarcodes = 1,
		WithGraph = 2,
		ShortForm = 4
	};

	ImColor lineColor = { 0.8f, 0.8f, 0.8f };
	ImColor pointColor = { 0.3f, 0.8f, 0.8f };
	ImColor backgroundColor = {0.2f, 0.2f, 0.2f};

	int drawGrapch(ImVec2 start, int height)
	{
		int width = std::max(height, 200);// , (int)maxEnd);
		ImVec2 end(start.x + width, start.y + width);
		float ratio = static_cast<float>(width) / 255.f;
		ImDrawList* list = ImGui::GetWindowDrawList();
		list->AddRectFilled(start, ImVec2(end.x, end.y), backgroundColor);
		list->AddLine({ start.x, end.y }, { end.x, start.y }, ImColor(0.f, 0.f, 0.8f), 2);

		for (auto& pline : lines)
		{
			ImVec2 sp(start.x + pline.start * ratio, end.y - pline.end * ratio);
			ImVec2 spEnd1(start.x + pline.end * ratio, sp.y);
			ImVec2 spEnd2(sp.x, end.y - pline.start * ratio);
			list->AddLine(sp, spEnd1, lineColor, 1);
			list->AddLine(sp, spEnd2, lineColor, 1);

			list->AddCircleFilled(sp, 3, pointColor);
		}

		return width;
	}

	int draw(bool disableDelete, DrawOptions options)
	{
		if (deleted)
			return 0;

		int margingX = 10;
		int margingY = 20;
		int paddingY = 5;
		int hei = 5;


		int widthFull = ImGui::GetWindowWidth() - (margingX * 2) - 50;
		int width = ((options & WithGraph) == 0) || ((options & WithBarcodes) == 0) ? (widthFull) : (widthFull / 2);

		float ratio = static_cast<float>(width) / 255.f;

		ImDrawList* list = ImGui::GetWindowDrawList();

		ImVec2 pos = ImGui::GetCursorScreenPos();
		pos.x += margingX;
		pos.y += margingY;


		ImVec2 start = pos;
		short asd = (options & ShortForm) ? (maxDepth - minDepth + 1) : ldepts;

		ImVec2 end = { start.x + width, start.y+ asd * (hei + 1) + paddingY };

		ImGui::BeginDisabled(disableDelete);
		ImGui::SetCursorScreenPos(ImVec2(3, start.y));
		if (ImGui::Button("D", {20, 40}))
			deleted = true;
		ImGui::EndDisabled();
		int height = end.y - start.y;
		if (options & WithBarcodes)
		{
			ImGui::SetCursorScreenPos(ImVec2(start.x, end.y));
			ImGui::Text("0");
			ImGui::SameLine();

			start.x += 20;
			list->AddRectFilled(start, ImVec2(end.x, end.y), backgroundColor);
			end.y += paddingY;
			for (auto& pline : lines)
			{
				short depth = pline.depth - minDepth + 1;
				short lineNum = (options & ShortForm) ? depth : pline.lineNumber;
				int nyEnd = end.y - (lineNum) * (hei + 1) - paddingY;
				ImColor depCol((19 * depth) % 251, (127), (71 * depth) % 251, options & ShortForm ? 100 : 255);

				list->AddRectFilled(ImVec2(start.x + pline.start * ratio, nyEnd), ImVec2(start.x + pline.end * ratio, nyEnd + hei), depCol);
				//pos.y += hei + 1;
			}

			pos = ImVec2(start.x + width, end.y);
			ImGui::SetCursorScreenPos(pos);
			ImGui::Text("255");
		}

		if (options & WithGraph)
		{
			height = drawGrapch(ImVec2(start.x + width, start.y), end.y - start.y);
		}

		ImGui::SetCursorScreenPos(ImVec2(start.x, start.y + height));
		return height;
	}
};


export class GuiBarcodeChart
{
	static const int len = 300;

	int curOp = 1;
public:
	bool show = false;

	const SimpleLine* last = nullptr;
	SavedLines current;
	std::vector<SavedLines> saved;
	void drawBarlines(const SimpleLine& line)
	{
		if (!show)
			return;

		if (&line != last)
		{
			current.get(line);
			last = &line;
		}

		if (!ImGui::Begin("BarcodeChart"))
		{
			ImGui::End();
		}

		if (ImGui::Button("Save"))
		{
			saved.push_back(current);
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear"))
		{
			saved.clear();
			last = nullptr;
		}
		ImGui::SameLine();
		if (ImGui::Button("Mode"))
		{
			curOp = 1 + (curOp + 1) % 3;
		}

		ImGui::SameLine();
		static bool br = false;
		if (ImGui::Button("barmode"))
		{
			br = !br;
		}

		int dw = curOp;
		if (br)
			dw |= 4;

		current.draw(true, SavedLines::DrawOptions(dw));

		for (int i = saved.size() - 1; i >= 0; i--)
		{
			saved[i].draw(false, SavedLines::DrawOptions(dw));
		}

		ImGui::End();
	}
};
