#include "gtest/gtest.h"
#include "backend/Algs/Gen2/Trainer.h"


TEST(Trainer, ChanceTest1)
{
	GetByChance test1;
	test1.add(0.1);
	test1.add(0.1);
	test1.add(0.1);
	test1.add(0.1);
	test1.add(0.1);
	test1.add(0.1);
	test1.add(0.1);
	test1.add(0.1);
	test1.add(0.2);

	std::random_device rd;
	std::mt19937 gen(rd());

	int p1count = 0;
	int total = 10000;
	for (size_t i = 0; i < total; i++)
	{
		if (test1.get(gen) == 0.1f)
			p1count++;
	}

	float chs = static_cast<float>(p1count) / static_cast<float>(total);
	ASSERT_LE(chs, 0.7);
}


TEST(Trainer, ChanceTest2)
{
	GetByChance test1;
	test1.add(0.2);
	test1.add(0.1);
	test1.add(0.1);
	test1.add(0.1);
	test1.add(0.1);
	test1.add(0.1);
	test1.add(0.1);
	test1.add(0.1);
	test1.add(0.1);

	std::random_device rd;
	std::mt19937 gen(rd());

	int p1count = 0;
	int total = 10000;
	for (size_t i = 0; i < total; i++)
	{
		if (test1.get(gen) == 0.1f)
			p1count++;
	}

	float chs = static_cast<float>(p1count) / static_cast<float>(total);
	ASSERT_LE(chs, 0.7);
}



TEST(Trainer, ChanceTest3)
{
	GetByChance test1;
	test1.add(1.f);
	test1.add(0.5f);
	test1.add(0.3f);
	test1.add(0.4f);

	test1.add(1.5f);
	test1.add(1.f);


	std::random_device rd;
	std::mt19937 gen(rd());

	int p1count = 0;
	int total = 10000;
	for (size_t i = 0; i < total; i++)
	{
		if (test1.get(gen) == 0.1f)
			p1count++;
	}

	float chs = static_cast<float>(p1count) / static_cast<float>(total);
	ASSERT_LE(chs, 0.7);
}
