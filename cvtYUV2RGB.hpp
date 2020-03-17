#pragma once
#include<cstdint>
#include<optional>
#include<variant>
namespace Kumazuma
{
	using YuvF = struct {
		float Y;
		float U;
		float V;
	};
	using RgbF = struct {
		float R;
		float G;
		float B;
	};
}
namespace Kumazuma::CvtYUV2RGB
{
	struct CvtConstantParamerter
	{
		float kr;
		float kg;
		float kb;
		static CvtConstantParamerter BT601()
		{
			return { 0.299, 0.587, 0.114 };
		}
		static CvtConstantParamerter BT709()
		{
			return { 0.2126,0.7152,0.0722 };
		}
		static CvtConstantParamerter BT2020()
		{
			return { 0.2627, 0.678, 0.0593 };
		}
	};
	struct YUVPlane
	{
		const uint8_t* data;
		uint32_t stride;
	};
	struct YUVImage
	{
		YUVPlane Y;
		YUVPlane U;
		YUVPlane V;
		uint32_t width;
		uint32_t height;
		uint32_t chromaShiftX;
		uint32_t chromaShiftY;
		uint8_t depth;
	};
	class YUVImageBuilder
	{
	public:
		YUVImageBuilder() {}
		YUVImageBuilder& PlaneY(const uint8_t* data, uint32_t stride)
		{
			m_y = YUVPlane{ data, stride };
			return *this;
		}
		YUVImageBuilder& PlaneU(const uint8_t* data, uint32_t stride)
		{
			m_u = YUVPlane{ data, stride };
			return *this;
		}
		YUVImageBuilder& PlaneV(const uint8_t* data, uint32_t stride)
		{
			m_v = YUVPlane{ data, stride };
			return *this;
		}
		YUVImageBuilder& Width(uint32_t val)
		{
			m_width = val;
			return *this;
		}
		YUVImageBuilder& Height(uint32_t val)
		{
			m_height = val;
			return *this;
		}
		YUVImageBuilder& ChromaShiftX(uint32_t val)
		{
			m_chromaShiftX = val;
			return *this;
		}
		YUVImageBuilder& ChromaShiftY(uint32_t val)
		{
			m_chromaShiftY = val;
			return *this;
		}
		YUVImageBuilder& BitDepth(uint8_t val)
		{
			m_bitDepth = val;
			return *this;
		}
		std::variant<YUVImage, std::string> build()
		{
			try
			{
				if (!m_y)
					throw std::string("Y Plane");
				if (!m_u)
					throw std::string("U Plane");
				if (!m_v)
					throw std::string("V Plane");
				if (!m_width)
					throw std::string("Width");
				if (!m_height)
					throw std::string("Height");
				if (!m_chromaShiftX)
					throw std::string("ChromaShiftX");
				if (!m_chromaShiftY)
					throw std::string("ChromaShiftY");
				if (!m_bitDepth)
					throw std::string("BitDepth");
				YUVImage res;
				res.Y = *m_y;
				res.U = *m_u;
				res.V = *m_v;
				res.width = *m_width;
				res.height = *m_height;
				res.chromaShiftX = *m_chromaShiftX;
				res.chromaShiftY = *m_chromaShiftY;
				res.depth = *m_bitDepth;
				return res;
			}
			catch (std::string ex)
			{
				return ex;
			}
		}
	private:
		std::optional<YUVPlane> m_y;
		std::optional<YUVPlane> m_u;
		std::optional<YUVPlane> m_v;
		std::optional<uint32_t> m_width;
		std::optional<uint32_t> m_height;
		std::optional<uint32_t> m_chromaShiftX;
		std::optional<uint32_t> m_chromaShiftY;
		std::optional<uint8_t> m_bitDepth;

	};

	template<typename T>
	inline const T* GetRow(const uint8_t* plane, const size_t row, const size_t stride)
	{
		return reinterpret_cast<const T*>(&plane[row * stride]);
	}
	template<typename _dataByteType>
	class YUV_t
	{
	public:
		class ConstIter;
		friend class ConstIter;
	public:
		YuvF get(size_t index) const
		{
			if (index >= size())
				return { -1,-1,-1 };
			const auto& image = m_image;
			size_t col = index % image.width;
			size_t row = index / image.width;
			const uint32_t uvY = std::min(static_cast<uint32_t>(row >> image.chromaShiftY), MAX_UVY);
			const uint32_t uvX = std::min(static_cast<uint32_t>(col >> image.chromaShiftX), MAX_UVX);
			float Y = GetRow<_dataByteType>(image.Y.data, row, image.Y.stride)[col] / (float)((1 << image.depth) - 1);
			float U = GetRow<_dataByteType>(image.U.data, uvY, image.U.stride)[uvX] / (float)((1 << image.depth) - 1) - 0.5f;
			float V = GetRow<_dataByteType>(image.V.data, uvY, image.V.stride)[uvX] / (float)((1 << image.depth) - 1) - 0.5f;
			return { Y, U, V };
		}
	public:
		class ConstIter
		{
			friend class YUV_t;
		private:
			YuvF get()
			{
				if (m_index >= m_parent.size())
					return { -1,-1,-1 };
				const auto& image = m_parent.m_image;
				size_t col = m_index % image.width;
				size_t row = m_index / image.width;

				const uint32_t uvX = std::min(static_cast<uint32_t>(col >> image.chromaShiftX), m_parent.MAX_UVX);
				float Y = GetRow<_dataByteType>(image.Y.data, row, image.Y.stride)[col] / (float)((1 << image.depth) - 1);
				float U = GetRow<_dataByteType>(image.U.data, m_uvY, image.U.stride)[uvX] / (float)((1 << image.depth) - 1) - 0.5f;
				float V = GetRow<_dataByteType>(image.V.data, m_uvY, image.V.stride)[uvX] / (float)((1 << image.depth) - 1) - 0.5f;
				return { Y, U, V };
			}
		public:
			ConstIter operator++(int)
			{
				auto rowPrev = m_index / m_parent.m_image.width;
				auto tmp = *this;
				m_index++;
				auto row = m_index / m_image.width;
				if (rowPrev != row)
					m_uvY = std::min(static_cast<uint32_t>(row >> m_parent.m_image.chromaShiftY), m_parent.MAX_UVY);
				if (m_index < m_parent.size())
					m_yuv = get();
				return tmp;
			}
			ConstIter& operator++()
			{
				auto rowPrev = m_index / m_parent.m_image.width;
				m_index++;
				auto row = m_index / m_parent.m_image.width;
				if (rowPrev != row)
					m_uvY = std::min(static_cast<uint32_t>(row >> m_parent.m_image.chromaShiftY), m_parent.MAX_UVY);
				if (m_index < m_parent.size())
					m_yuv = get();
				return *this;
			}
			ConstIter operator+(size_t index)
			{
				return ConstIter(m_parent, m_index + index);
			}
			const YuvF& operator *() const
			{
				return m_yuv;
			}
			const YuvF& operator ->() const
			{
				return m_yuv;
			}
			bool operator == (const ConstIter& obj)
			{
				if (&obj.m_parent != &m_parent)
					return false;
				if (obj.m_index != m_index)
					return false;
				return true;
			}
			bool operator != (const ConstIter& obj)
			{
				return !ConstIter::operator==(obj);
			}
		private:
			ConstIter(const YUV_t& parent, size_t index = 0) :
				m_parent(parent),
				m_index(index),
				m_uvY(std::min(static_cast<uint32_t>((index / parent.m_image.width) >> parent.m_image.chromaShiftY), parent.MAX_UVY)),
				m_yuv(get())

			{

			}

		private:
			const YUV_t& m_parent;
			size_t m_index;
			uint32_t m_uvY;
			YuvF m_yuv;
		};
	public:
		YUV_t(const YUVImage& image) :
			m_image(image),
			MAX_UVX(((image.width + image.chromaShiftX) >> image.chromaShiftX) - 1),
			MAX_UVY(((image.height + image.chromaShiftY) >> image.chromaShiftY) - 1)
		{

		}
		ConstIter begin() const
		{
			return ConstIter(*this);
		}
		ConstIter end() const
		{
			return ConstIter(*this, m_image.width * m_image.height);
		}
		size_t size() const
		{
			return m_image.width * m_image.height;
		}
		YuvF operator[](size_t index)
		{
			return get(index);
		}
	private:

		YUVImage m_image;
		const uint32_t MAX_UVX;
		const uint32_t MAX_UVY;
	};


	template<class YUV_t>
	class RGBFloat
	{
		class ConstIter;
		friend class ConstIter;
	private:
		const YUV_t m_yuvs;
		const CvtConstantParamerter m_param;
		class ConstIter
		{
			friend class RGBFloat;
		private:
			RgbF get()
			{
				auto yuv = *m_yuvIter;
				auto kr = m_parent.m_param.kr;
				auto kg = m_parent.m_param.kg;
				auto kb = m_parent.m_param.kb;

				float R = yuv.Y + (2 * (1 - kr)) * yuv.V;
				float B = yuv.Y + (2 * (1 - kb)) * yuv.U;
				float G = yuv.Y - ((2 * ((kr * (1 - kr) * yuv.V) + (kb * (1 - kb) * yuv.U))) / kg);
				return { R,G, B };
			}
		public:
			ConstIter operator++(int)
			{
				auto tmp = *this;
				m_index++;
				++m_yuvIter;

				m_rgb = get();
				return tmp;
			}
			ConstIter& operator++()
			{
				m_index++;
				++m_yuvIter;
				m_rgb = get();
				return *this;
			}
			const RgbF& operator *() const
			{
				return m_rgb;
			}
			const RgbF& operator ->() const
			{
				return m_rgb;
			}
			bool operator == (const ConstIter& obj)
			{
				if (&obj.m_parent != &m_parent)
					return false;
				if (obj.m_index != m_index)
					return false;
				return true;
			}
			bool operator != (const ConstIter& obj)
			{
				return !ConstIter::operator==(obj);
			}
		private:
			ConstIter(const RGBFloat& parent, size_t index = 0) :
				m_parent(parent),
				m_index(index),
				m_rgb(m_parent.get(0)),
				m_yuvIter(parent.m_yuvs.begin() + index)
			{

			}
		private:
			const RGBFloat& m_parent;
			decltype(m_parent.m_yuvs.begin()) m_yuvIter;
			size_t m_index;
			RgbF m_rgb;
		};
	public:
		RGBFloat(const YUV_t& yuvs, CvtConstantParamerter param) :
			m_yuvs(yuvs), m_param(param)
		{

		}
		RGBFloat(YUV_t&& yuvs, CvtConstantParamerter param) :
			m_yuvs(yuvs), m_param(param)
		{

		}
		ConstIter begin() const
		{
			return ConstIter(*this);
		}
		ConstIter end() const
		{
			return ConstIter(*this, m_yuvs.size());
		}
		size_t size() const
		{
			return m_yuvs.size();
		}
		RgbF operator[](size_t index)
		{
			return get(index);
		}
		RgbF get(size_t index) const
		{
			auto yuv = m_yuvs.get(index);
			auto kr = m_param.kr;
			auto kg = m_param.kg;
			auto kb = m_param.kb;

			float R = yuv.Y + (2 * (1 - kr)) * yuv.V;
			float B = yuv.Y + (2 * (1 - kb)) * yuv.U;
			float G = yuv.Y - ((2 * ((kr * (1 - kr) * yuv.V) + (kb * (1 - kb) * yuv.U))) / kg);
			return { R,G,B };
		}
	};
	template<class YUV>
	void toRGB24(uint8_t* ptr, const RGBFloat<YUV>& rgbs)
	{
		size_t i = 0;
		for (auto& rgb : rgbs)
		{
			ptr[i + 0] = static_cast<uint8_t>(rgb.R * 255);
			ptr[i + 1] = static_cast<uint8_t>(rgb.G * 255);
			ptr[i + 2] = static_cast<uint8_t>(rgb.B * 255);
			i += 3;
		}
	}
}
