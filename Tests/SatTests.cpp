#include "pch.h"
#include "../backend/Clusterizers/LandscapeItem.h"
#include "../backend/ProcBlock/Dataset/DatasetWork.h"
// import ConvertItem;
// import BackBind;


std::vector<float> buildLandscape(std::initializer_list<std::pair<float, float>> list)
{
	int startX = 0;
	float startY = 0;
	std::vector<float> output;
	for (auto& land : list)
	{
		float iter = (land.second - startY) / (land.first * 100 - startX);
		for (size_t i = startX; i < land.first * 100; i++)
		{
			output.push_back(startY);
			startY += iter;
		}
		startX = land.second;
	}

	while (startX < 255)
	{
		output.push_back(0.f);
	}

	return output;
}


struct TestOut
{
	std::vector<std::vector<LandPoint>> expected;

	void compare(LandscapeClass& result)
	{
		EXPECT_EQ(result.landscape.size(), expected.size());

		int minl = std::min(result.landscape.size(), expected.size());
		for (size_t i = 0; i < minl; i++)
		{
			EXPECT_EQ(result.landscape[i].size(), expected[i].size());

			int minn = std::min(result.landscape[i].size(), expected[i].size());
			for (size_t j = 0; j <  minn; j++)
			{
				EXPECT_EQ(result.landscape[i].points[j].x, expected[i][j].x);
				EXPECT_EQ(result.landscape[i].points[j].y, expected[i][j].y);
			}
		}
	}
	void buildexpectedource(std::initializer_list<std::pair<float, float>> list)
	{
		expected.push_back({});
		for (auto& par : list)
		{
			expected.back().push_back({par.first, par.second});
		}
	}
};




TEST(InputLandData, TestBubenick)
{
	LandscapeCollection col;
	col.setTestEnv();

	col.convertLand.addExprRaw(3, 9);
	col.convertLand.addExprRaw(4, 6);
	col.convertLand.addExprRaw(5, 11);
	col.performOnPerform();
	LandscapeClass out;
	col.convertToLandscape(col.convertLand, &out);

	TestOut bubenic;
	bubenic.buildexpectedource({{3,0},{6,3}, {7,2}, {8,3}, 	{11,0}});
	bubenic.buildexpectedource({{4,0},{5,1}, {5.5,0.5}, {7,2},{9,0}});
	bubenic.buildexpectedource({{5,0},{5.5,0.5}, {6,0}});
	bubenic.compare(out);
}

TEST(InputLandData, TestMdpikName)
{
	// https://www.mdpi.com/2073-431X/12/6/110

	LandscapeCollection mdpiTests;
	mdpiTests.setTestEnv();
	mdpiTests.convertLand.addExprRaw(0.5, 2.5);
	mdpiTests.convertLand.addExprRaw(2, 6);
	mdpiTests.convertLand.addExprRaw(3, 5);
	mdpiTests.convertLand.addExprRaw(5, 7);

	mdpiTests.performOnPerform();
	LandscapeClass out;
	mdpiTests.convertToLandscape(mdpiTests.convertLand, &out);

	TestOut mdpiExpected;
	mdpiExpected.buildexpectedource({{0.5,0}, {1.5,1},{2.25,0.25}, {4,2}, {5.5,0.5},{6,1},{7,0}});
	mdpiExpected.buildexpectedource({{2,  0}, {2.25,0.25},{2.5,0},
		{3, 0},  {4,1},  {5,0},
		{5.5, 0.5}, {6,0}});
	mdpiExpected.compare(out);

	// buildexpectedource();
	// buildexpectedource();


	// std::vector < std::pair<float, float>> expected = {
	// 	{3,11},
	// 	{4,9},
	// 	{5,6}
	// };
	// for (size_t i = 0; i < col.getItemsCount(); i++)
	// {
	// 	EXPECT_EQ(col.getRItem(i)->start().getAvgFloat(), expected[i].first);
	// 	EXPECT_EQ(col.getRItem(i)->end().getAvgFloat(), expected[i].second);
	// }
}

TEST(InputLandData, SameStart)
{
	LandscapeCollection col;
	col.setTestEnv();
	col.convertLand.addExprRaw(60, 62);
	col.convertLand.addExprRaw(60, 84);
	col.convertLand.addExprRaw(60, 186);
	col.performOnPerform();
	LandscapeClass out;
	col.convertToLandscape(col.convertLand, &out);

	TestOut expected;
	expected.buildexpectedource({{60,0},{123,63}, {186,0}});
	expected.buildexpectedource({{60,0},{72,12}, {84,0}});
	expected.buildexpectedource({{60,0},{61,1}, {62,0}});
	expected.compare(out);
}

TEST(LandScape, SameEnd)
{
	LandscapeCollection col;
	col.setTestEnv();

	col.convertLand.addExprRaw(0, 4);
	col.convertLand.addExprRaw(2, 4);

	LandscapeClass out;
	col.convertToLandscape(col.convertLand, &out);
}


TEST(InputLandData, PaperTest)
{
	// https://www.sciencedirect.com/science/article/pii/S0747717116300104
	LandscapeCollection col;
	col.setTestEnv();

	col.convertLand.addExprRaw(1, 5);
	col.convertLand.addExprRaw(2, 8);
	col.convertLand.addExprRaw(3, 4);
	col.convertLand.addExprRaw(5, 9);
	col.convertLand.addExprRaw(6, 7);

	col.performOnPerform();
	LandscapeClass out;
	col.convertToLandscape(col.convertLand, &out);

	TestOut expected;
	expected.buildexpectedource({{1,0}, {3,   2},   {3.5,1.5}, {5,3}, {6.5,1.5}, {7,2}, {9,0}});
	expected.buildexpectedource({{2,0}, {3.5, 1.5}, {5,0},    {6.5, 1.5}, {8,0}});
	expected.buildexpectedource({{3,0}, {3.5, 0.5}, {4,0}, {6,0}, {6.5, 0.5}, {7,0}});
	expected.compare(out);
}


TEST(LandScape, dwq1)
{
	LandscapeCollection col;
	col.setTestEnv();

	col.convertLand.addExprRaw(4, 5);
	// col.convertLand.addExprRaw(144.667, 175.667);
	// col.convertLand.addExprRaw(143.667, 169.333);
	// col.convertLand.addExprRaw(145, 175);
	// col.convertLand.addExprRaw(145.333, 175.667);
	// col.convertLand.addExprRaw(145.667, 180.333);
	// col.convertLand.addExprRaw(144, 179.333);
	col.convertLand.addExprRaw(6, 10);
	col.convertLand.addExprRaw(7, 11);
	col.convertLand.addExprRaw(8, 11);

	LandscapeClass out;
	col.convertToLandscape(col.convertLand, &out);
}


TEST(LandScape, closee)
{
	LandscapeCollection col;
	col.setTestEnv();

	col.convertLand.addExprRaw(144.333, 169);
	// col.convertLand.addExprRaw(144.667, 175.667);
	// col.convertLand.addExprRaw(143.667, 169.333);
	// col.convertLand.addExprRaw(145, 175);
	// col.convertLand.addExprRaw(145.333, 175.667);
	// col.convertLand.addExprRaw(145.667, 180.333);
	// col.convertLand.addExprRaw(144, 179.333);
	col.convertLand.addExprRaw(146, 180);
	col.convertLand.addExprRaw(146.333, 180.333);
	col.convertLand.addExprRaw(146.667, 180.333);

	LandscapeClass out;
	col.convertToLandscape(col.convertLand, &out);
}


TEST(LandScape, Chukanov)
{
	return;
	LandscapeCollection house;
	house.setTestEnv();

	house.convertLand.addExprRaw(0, 1.41);
	house.convertLand.addExprRaw(0, 1.41);
	house.convertLand.addExprRaw(0, 2);
	// house.convertLand.addExprRaw(2, 2.828); // dim1
	house.performOnPerform();
	LandscapeClass outHouse;
	house.convertToLandscape(house.convertLand, &outHouse);


	LandscapeCollection house1;
	house1.setTestEnv();

	house1.convertLand.addExprRaw(0, 2);
	house1.convertLand.addExprRaw(0, 2);
	house1.convertLand.addExprRaw(0, 2);
	house1.convertLand.addExprRaw(0, 2.233);
	// house1.convertLand.addExprRaw(2, 2.828);
	house1.performOnPerform();
	LandscapeClass outHouse1;
	house1.convertToLandscape(house1.convertLand, &outHouse1);

	Landscape& houset = outHouse.landscape;
	Landscape& houset1 = outHouse1.landscape;

	double rest1 = iterLandDistanceMdpiInf(houset, houset1, 1);
	double rest2 = iterLandDistanceMdpiInf(houset, houset1, 0.1);
	double rest3 = iterLandDistanceMdpiInf(houset, houset1, 0.01);
	EXPECT_EQ(rest1, 0.5451);
}



// signature



TEST(LandScape, signatureHard)
{
	return;
	// https://www.sciencedirect.com/science/article/pii/S0747717116300104
	LandscapeCollection col;
	col.setTestEnv();

	col.convertLand.addExprRaw(1, 5);
	col.convertLand.addExprRaw(2, 8);
	col.convertLand.addExprRaw(3, 4);
	col.convertLand.addExprRaw(5, 9);
	col.convertLand.addExprRaw(6, 7);
	col.maxEnd = 9;
	col.performOnPerform();
	col.perform();

	std::vector<std::vector<float>> expectedSignature {
		//1           2          3         4          5         6    7    8,
		{0.0f, 0.5, 1.0f, 1.5, 2.0f, 1.5, 2.f, 2.5, 3.0f, 2.5, 2.0f, 1.5f, 2.0f, 1.5, 1.0, 0.5, 0.0f},
		{0.f, 0.5, 1.0,  1.5, 1.0f, 0.5, 0.0f,
		      0.5, 1.0,  1.5, 1.0f, 0.5, 0.0f},
		{0.0, 0.5,  0.0f,0.f,0.f,0.f,  0.5f, 0.0f}
	};

	// ASSERT_EQ(col.getItemsCount(), expectedSignature.size());
	ASSERT_EQ(col.getItemsCount(), 1);
	LandscapeClass* item = col.getRItem(0);
	ASSERT_EQ(item->landscape.size(), 3);


	for (size_t i = 0; i < 3; i++)
	{
		const LyambdaLine& line = item->landscape[i];
		PointsIterator it{line.points, 0.5};

		std::vector<float> res;
		while (!it.ended())
			res.push_back(it.executeLymbda());

		for (auto &i : res)
		{
			i = std::round(i * 10) / 10;
		}


		EXPECT_EQ(res, expectedSignature[0]);
	}
}


TEST(LandScape, signatureSimple)
{
	return;
	// https://www.sciencedirect.com/science/article/pii/S0747717116300104
	LandscapeCollection col;
	col.setTestEnv();

	col.convertLand.addExprRaw(1, 5);
	col.convertLand.addExprRaw(7, 9);
	col.performOnPerform();
	col.perform();

	std::vector<std::vector<float>> expectedSignature {
		// 0   1      2     3     4    4     5   6  7    8,
		{0.0f, 0.0f, 1.0f, 2.0f, 1.f, 0.0f,  0.f,  0.f,  0.f,  1.f,  0.f},

		{0.0f, 0.0f, 0.0f, 0.5f, 1.0f, 1.5f, 2.0f, 1.5f, 1.0f,  0.5f, 0.f},
		{	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, // 0-1
			0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, //1-2
			1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f, 1.7f, 1.8f, 1.9f, 2.0f,
			1.9f, 1.8f, 1.7f, 1.6f, 1.5f, 1.4f, 1.3f, 1.2f, 1.1f, 1.0f,
			0.9f, 0.8f, 0.7f, 0.6f, 0.5f, 0.4f, 0.3f, 0.2f, 0.1f, 0.0f}
	};

	// ASSERT_EQ(col.getItemsCount(), expectedSignature.size());
	ASSERT_EQ(col.getItemsCount(), 1);
	LandscapeClass* item = col.getRItem(0);
	ASSERT_EQ(item->landscape.size(), 1);
	const LyambdaLine& line = item->landscape[0];


	PointsIterator it{line.points, 1.f};

	std::vector<float> res;
	while (!it.ended())
		res.push_back(it.executeLymbda());

	EXPECT_EQ(res, expectedSignature[0]);


	res.clear();
	it.reset();
	it.t = 0.5;
	while (!it.ended())
		res.push_back(it.executeLymbda());

	for (auto &i : res)
	{
		i = std::round(i * 10) / 10;
	}

	EXPECT_EQ(res, expectedSignature[1]);


	res.clear();
	it.reset();
	it.t = 0.1;
	while (!it.ended())
		res.push_back(it.executeLymbda());

	for (auto &i : res)
	{
		i = std::round(i * 10) / 10;
	}

	EXPECT_EQ(res, expectedSignature[2]);
}





TEST(LandScape, CorrectSignatureHard)
{
	// https://www.sciencedirect.com/science/article/pii/S0747717116300104
	LandscapeCollection col;
	col.setTestEnv();

	col.convertLand.addExprRaw(1, 5);
	col.convertLand.addExprRaw(2, 8);
	col.convertLand.addExprRaw(3, 4);
	col.convertLand.addExprRaw(5, 9);
	col.convertLand.addExprRaw(6, 7);
	col.maxEnd = 9;
	col.performOnPerform();
	LandscapeClass out;
	col.convertToLandscape(col.convertLand, &out);

	std::vector<std::vector<float>> expectedSignature {
		//  0                 1                  2                 3                 4              5               6               7            8,
		{0.0f,     0.0f,     0.0f,     0.5,     1.0f,     1.5,    2.0f,     1.5,    2.f,    2.5,   3.0f,    2.5,   2.0f,  1.5f,   2.0f,   1.5,  1.0,      0.5, 0.0f},
		{0.0f,     0.0f,     0.0f,     0.0f,    0.f,      0.5,    1.0f,     1.5,    1.0f,   0.5,   0.0f,    0.5,   1.0,   1.5f,   1.0f,   0.5, 0.0f},
		{0.0f,     0.0f,     0.0f,     0.0f,    0.f,      0.0f,   0.0f,     0.5,    0.0f,   0.f,    0.f,    0.f,   0.f,   0.5f,   0.0f}
	};

	// ASSERT_EQ(col.getItemsCount(), expectedSignature.size());
	// ASSERT_EQ(out.landscape.size(), 1);
	// LandscapeClass* item = col.getRItem(0);
	ASSERT_EQ(out.landscape.size(), 3);


	for (size_t i = 0; i < 3; i++)
	{
		const LyambdaLine& line = out.landscape[i];
		CorrectPointsIterator it{line.points, 0.5};

		std::vector<float> res;
		while (!it.ended())
			res.push_back(it.executeLymbda());

		for (auto &i : res)
		{
			i = std::round(i * 10) / 10;
		}


		EXPECT_EQ(res, expectedSignature[i]);
	}
}


TEST(LandScape, CorrectSignatureSimple)
{
	// https://www.sciencedirect.com/science/article/pii/S0747717116300104
	LandscapeCollection col;
	col.setTestEnv();

	col.convertLand.addExprRaw(1, 5);
	col.convertLand.addExprRaw(7, 9);
	col.performOnPerform();
	col.perform();

	std::vector<std::vector<float>> expectedSignature {
		// 0   1      2     3     4   5       6     7    8,     9
		{0.0f, 0.f, 1.0f, 2.0f, 1.f, 0.0f,  0.f,  0.f,  1.f,  0.f},

		// 0          1           2           3           4           5       6         7       8,      9
		{0.0f, 0.0f, 0.0f, 0.5f, 1.0f, 1.5f, 2.0f, 1.5f, 1.0f,  0.5f, 0.f, 0, 0,   0.0, 0, 0.5, 1, 0.5, 0},
		{
			0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, // 0 -1
			0.0, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, // 1 -2
			1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f, 1.7f, 1.8f, 1.9f, // 2- 3
			2.0f, 1.9f, 1.8f, 1.7f, 1.6f, 1.5f, 1.4f, 1.3f, 1.2f, 1.1f, // 3-4
			1.0f, 0.9f, 0.8f, 0.7f, 0.6f, 0.5f, 0.4f, 0.3f, 0.2f, 0.1f, // 4-5
			0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, // 5-6
			0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, // 6-7
			0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f,// 7-8
			1.0f, 0.9f, 0.8f, 0.7f, 0.6f, 0.5f, 0.4f, 0.3f, 0.2f, 0.1f, 0.0f} // 8-9
	};

	// ASSERT_EQ(col.getItemsCount(), expectedSignature.size());
	ASSERT_EQ(col.getItemsCount(), 1);
	LandscapeClass* item = col.getRItem(0);
	ASSERT_EQ(item->landscape.size(), 1);
	const LyambdaLine& line = item->landscape[0];


	CorrectPointsIterator it0{line.points, 1.f};

	std::vector<float> res;
	while (!it0.ended())
		res.push_back(it0.executeLymbda());

	EXPECT_EQ(res, expectedSignature[0]);


	res.clear();
	CorrectPointsIterator it1{line.points, 0.5};
	while (!it1.ended())
		res.push_back(it1.executeLymbda());

	for (auto &i : res)
	{
		i = std::round(i * 10) / 10;
	}

	EXPECT_EQ(res, expectedSignature[1]);


	res.clear();
	CorrectPointsIterator it2{line.points, 0.1};
	while (!it2.ended())
		res.push_back(it2.executeLymbda());

	for (auto &i : res)
	{
		i = std::round(i * 10) / 10;
	}

	EXPECT_EQ(res.size(), expectedSignature[2].size());
	for (size_t i = 0; i < res.size(); i+=20)
	{
		int left = std::min<int>(res.size() - i, 20);
		std::vector<float> resPart = {res.begin() + i, res.begin() + i + left};
		std::vector<float> expectedPart = {expectedSignature[2].begin() + i, expectedSignature[2].begin() + i + left};
		EXPECT_EQ(resPart, expectedPart);
	}
}


vector<int> replace(vector<int> vec, std::unordered_map<int, int> rplmap)
{
	for (size_t i = 0; i < vec.size(); ++i)
	{
		vec[i] = rplmap[vec[i]];
	}

	return vec;
}


TEST(Side, hangarianSimple)
{
	vector<int> true_labels = {0, 0, 0, 1, 1, 1};
	vector<int> predicted_labels = {1, 1, 1, 0, 0, 0};
	auto expected = replace(predicted_labels, {{1,0}, {0,1}});

	int num_classes = 2;  // Количество классов

	auto assigns = DatasetWork::mapIds(true_labels, predicted_labels, num_classes);
	EXPECT_EQ(assigns, expected);
}

TEST(Side, hangarianHard)
{
	vector<int> true_labels =      {0,0,0, 0,0,0, 0,0,0,  1,1,1, 1,1,1, 1,1,1};
	vector<int> predicted_labels = {1,1,1, 0,0,0, 1,1,1,  0,0,0, 1,0,0, 0,0,0 };

	auto expected = replace(predicted_labels, {{1,0}, {0,1}});

	int num_classes = 2;  // Количество классов

	auto assigns = DatasetWork::mapIds(true_labels, predicted_labels, num_classes);
	EXPECT_EQ(assigns, expected);
}

TEST(Side, hangarianSame)
{
	vector<int> true_labels =      {0,0,0, 0,0,0, 0,0,0,  1,1,1, 1,1,1, 1,1,1};
	vector<int> predicted_labels = {1,1,1, 0,0,0, 1,1,1,  0,0,0, 1,1,1, 0,0,0 };
	auto expected = replace(predicted_labels, {{1,0}, {0,1}});

	int num_classes = 2;  // Количество классов

	auto assigns = DatasetWork::mapIds(true_labels, predicted_labels, num_classes);
	EXPECT_EQ(assigns, expected);
}

TEST(Side, hangarianSameFull)
{
	vector<int> true_labels =      {0,0,0, 0,0,0, 0,0,0,  1,1,1, 1,1,1, 1,1,1};
	vector<int> predicted_labels = {0,1,1, 0,0,0, 0,0,1,  1,1,1, 1,1,1, 1,1,1 };
	auto expected = replace(predicted_labels, {{0,0}, {1,1}});

	int num_classes = 2;  // Количество классов

	auto assigns = DatasetWork::mapIds(true_labels, predicted_labels, num_classes);
	EXPECT_EQ(assigns, expected);
}


TEST(Side, hangarianMoreClassesStrict)
{
	// 0 -> 3
	// 1 -> 4
	// 2 -> 6
	// 3 -> 1
	// 4 -> 2
	// 5 -> 5
	// 6 -> 0
	vector<int> true_labels =      {0,1,1, 2,2,2, 3,3,3,  3,3,4, 5,5,5, 5,6,6};
	vector<int> predicted_labels = {3,4,4, 6,6,6, 1,1,1,  1,1,2, 5,5,5, 5,0,0};
	auto expected = replace(predicted_labels, {{3,0}, {4,1}, {6,2}, {1, 3}, {2, 4}, {5,5}, {0, 6}});

	int num_classes = 7;  // Количество классов

	auto assigns = DatasetWork::mapIds(true_labels, predicted_labels, num_classes);
	EXPECT_EQ(assigns, expected);
}


TEST(Side, hangarianMoreClassesWithError)
{
	// 0 -> 3
	// 1 -> 4
	// 2 -> 6
	// 3 -> 1
	// 4 -> 2
	// 5 -> 5
	// 6 -> 0
	vector<int> true_labels =      {0,1,1, 2,2,2, 3,3,3,  3,3,4, 5,5,5, 5,6,6};
	vector<int> predicted_labels = {3,4,4, 6,6,5, 1,1,2,  1,1,2, 5,5,3, 5,0,0};
	auto expected = replace(predicted_labels, {{3,0}, {4,1}, {6,2}, {1, 3}, {2, 4}, {5,5}, {0, 6}});

	int num_classes = 7;  // Количество классов

	auto assigns = DatasetWork::mapIds(true_labels, predicted_labels, num_classes);
	EXPECT_EQ(assigns, expected);
}

#include "../backend/ProcBlock/Dataset/LandscapeCalss.h"

TEST(ClusterTest, Hiracy)
{
	LandscapeCluster iterSupCuster;

	LyambdaLine l0;
	l0.points.push_back({1, 1});
	l0.points.push_back({4, 3});
	l0.points.push_back({5, 1});
	LyambdaLine l1;
	l1.points.push_back({1, 1});
	l1.points.push_back({3, 3});
	l1.points.push_back({5, 1});
	Landscape land0;
	land0.push_back(l0);
	Landscape land1;
	land1.push_back(l1);


	LyambdaLine l2;
	l0.points.push_back({5, 1});
	l0.points.push_back({8, 3});
	l0.points.push_back({9, 1});
	LyambdaLine l3;
	l1.points.push_back({5, 1});
	l1.points.push_back({8, 3});
	l1.points.push_back({9, 1});

	Landscape land2;
	land2.push_back(l0);
	Landscape land3;
	land3.push_back(l1);


	std::vector<Landscape> lands;
	lands.push_back(land0);
	lands.push_back(land1);
	lands.push_back(land2);
	lands.push_back(land3);

	iterSupCuster.predict(lands);

	EXPECT_EQ(iterSupCuster.test(0), 0);
	EXPECT_EQ(iterSupCuster.test(1), 0);

	EXPECT_EQ(iterSupCuster.test(2), 1);
	EXPECT_EQ(iterSupCuster.test(3), 1);
}
