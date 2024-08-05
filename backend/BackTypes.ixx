module;
#include <algorithm>
#include <cassert>

export module BackTypes;


export template<class T>
struct StaticBuffer
{
private:
	T* m_buffer = nullptr;
	size_t m_size = 0;
	bool drop = true;

public:
	StaticBuffer() {}
	StaticBuffer(const StaticBuffer& other) //: s(other.s)
	{
		allocate(other.m_size);
		drop = other.drop;

		if (drop)
		{
			allocate(other.m_size);
			std::copy(other.m_buffer, other.m_buffer + other.m_size, m_buffer);
		}
		else
		{
			m_buffer = other.m_buffer;
			m_size = other.m_size;
		}
	}
	//std::cout << "move failed!\n";
	StaticBuffer(StaticBuffer&& other) //: s(std::move(o.s))
	{
		m_buffer = std::exchange(other.m_buffer, nullptr); // leave other in valid state
		m_size = std::exchange(other.m_size, 0);
		drop = std::exchange(other.drop, false);
	}

	T* data() const
	{
		return m_buffer;
	}

	size_t size() const { return m_size; }

	T* extract()
	{
		T* temp = std::exchange(m_buffer, nullptr);
		m_size = 0;
		drop = false;

		return temp;
	}

	void allocate(size_t nsize)
	{
		//		assert(size != 0);

		release();
		this->m_size = nsize;
		m_buffer = new T[nsize];
		drop = true;
	}

	void setData(T* newData, size_t size, bool dropData = true)
	{
		release();
		m_size = size;
		m_buffer = newData;
		drop = dropData;
	}
	void copyDataFrom(const T* newData, size_t size)
	{
		allocate(size);
		std::copy(newData, newData + size, m_buffer);
	}

	void setToZero()
	{
		memset(m_buffer, 0, m_size * sizeof(T));
	}

	T& operator[](std::size_t idx)
	{
		assert(idx < m_size);
		return m_buffer[idx];
	}

	// copy assignment
	StaticBuffer& operator=(const StaticBuffer& other)
	{
		// Guard self assignment
		if (this == &other)
			return *this;
		drop = other.drop;

		if (drop)
		{
			allocate(other.m_size);
			std::copy(other.m_buffer, other.m_buffer + other.m_size, m_buffer);
		}
		else
		{
			m_buffer = other.m_buffer;
			m_size = other.m_size;
		}
		return *this;
	}

	// move assignment
	StaticBuffer& operator=(StaticBuffer&& other) noexcept
	{
		// Guard self assignment
		if (this == &other)
			return *this; // delete[]/size=0 would also be ok

		m_buffer = std::exchange(other.m_buffer, nullptr); // leave other in valid state
		m_size = std::exchange(other.m_size, 0);
		drop = std::exchange(other.drop, false);
		return *this;
	}

	void release()
	{
		if (m_buffer && drop)
		{
			delete[] m_buffer;
		}
		m_buffer = nullptr;
		m_size = 0;
		drop = false;
	}

	~StaticBuffer()
	{
		release();
	}
};

export using ubuffer = StaticBuffer<unsigned char>;



//struct FileBuffer
//{
//private:
//	BackString buffer;
//	QFile outfile;
//	QTextStream* stream = nullptr;
//	int maxBufferSize = 10000;
//public:
//	bool openFileStream(BackString path, int maxBufferSize = 10000, QFile::OpenMode openFlags = QFile::WriteOnly | QFile::Truncate)
//	{
//		outfile.setFileName(path);
//
//		//		if (outfile.exists())
//		//		{
//		//			QFile::rename(path,  path.replace("bds.lst", "bds old.lst"));
//		//			//			outfile.setFileName(path);
//		//		}
//
//		if (!outfile.open(openFlags))
//			return false;
//
//		if (stream != nullptr)
//			delete stream;
//		stream = new QTextStream(&outfile);
//		this->maxBufferSize = maxBufferSize;
//
//		return true;
//	}
//
//	void write(const BackString& part)
//	{
//		if (stream == nullptr)
//			return;
//
//		buffer.append(part);
//		if (buffer.size() > maxBufferSize)
//		{
//			dowrite();
//		}
//	}
//
//	void writeLine(const BackString& part = "")
//	{
//		if (stream == nullptr)
//			return;
//
//		buffer.append(part);
//		buffer.append(nl);
//		if (buffer.size() > maxBufferSize)
//		{
//			dowrite();
//		}
//	}
//
//	void dowrite()
//	{
//		stream->operator<<(buffer);
//		buffer.clear();
//	}
//	void close()
//	{
//		if (stream != nullptr)
//		{
//			if (outfile.isOpen())
//			{
//				dowrite();
//				outfile.close();
//			}
//
//			delete stream;
//			stream = nullptr;
//		}
//	}
//	~FileBuffer()
//	{
//		close();
//	}
//};
