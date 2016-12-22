/*
MIT License

Copyright © 2016 David Curtis

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef guard_utils_proximal_h
#define guard_utils_proximal_h

#include <cmath>
#include <assert.h>

namespace utils
{
	/*
	 *	Generic implementations of the core functions -- exp2i and ilog2 -- based on
	 *	the standard library. The key constants are derived from numeric limits.
	 *	Unless specializations of the the template functions are provided for specific
	 *	floating point types, the default templates will be used. These should be
	 *	portable and correct for most floating point formats.
	 */

	template<class T>
	static constexpr int fractional_digits = std::numeric_limits<T>::digits - 1;
	
	template<class T>
	static constexpr int min_explicit_exponent = std::numeric_limits<T>::min_exponent - 1;

	template<class T>
	static constexpr int max_explicit_exponent = std::numeric_limits<T>::max_exponent - 1;

	template<class T>
	static constexpr int min_implicit_exponent = min_explicit_exponent<T> - fractional_digits<T>;
	
	template<class T, int N>
	static constexpr int fractional_precision = fractional_digits<T> - N;
	
	template<class T, int N>
	static constexpr int exponent_limit = min_implicit_exponent<T> + N;

	template<class T>
	static inline T exp2i(int exp)
	{
		return exp2(static_cast<T>(exp));
	}
	
	template<class T>
	static inline int ilog2(T x)
	{
		return ilogb(x);
	}

	/*
	 *	Specializations of exp2i and ilog2 templates for IEEE 754 single and
	 *	double precision, and x86 extended precision. These implementations
	 *	use bitwise operations rather than the library functions, and
	 *	typically run about twice as fast as the generic implementations.
	 *	Change the following defines to disable the specializations.
	 */

	#define __USE_FLOAT_IEEE754_SPECIALIZATION__ 1
	#define __USE_DOUBLE_IEEE754_SPECIALIZATION__ 1
	#define __USE_LONG_DOUBLE_X86_EXTENDED_SPECIALIZATION__ 1

	#if (__USE_FLOAT_IEEE754_SPECIALIZATION__) || (__USE_DOUBLE_IEEE754_SPECIALIZATION__) || (__USE_LONG_DOUBLE_X86_EXTENDED_SPECIALIZATION__)

	template<class T>
	int count_leading_zeros(T u) = delete;

	inline int count_leading_zeros(std::uint32_t u)
	{
		if (u == 0) return sizeof(u) * 8; // __builtin_clz(0) is undefined in some implementations
	#if defined(__has_builtin) && __has_builtin(__builtin_clz)
		return __builtin_clz(u);
	#else
		unsigned n = 0;
		int i = reinterpret_cast<std::int32_t&>(u);
		while (1) {
			if (i < 0) break;
			n ++;
			i <<= 1;
		}
		return n;
	#endif
	}
	
	inline int count_leading_zeros(std::uint64_t u)
	{
		if (u == 0) return sizeof(u) * 8; // __builtin_clz(0) is undefined in some implementations
	#if defined(__has_builtin) && __has_builtin(__builtin_clz)
		using bitwords = std::uint32_t[2];
		bitwords& words = reinterpret_cast<bitwords&>(u);
		if (words[1] != 0)
		{
			return __builtin_clz(words[0]) + 32;
		}
		else
		{
			return __builtin_clz(words[1]);
		}
	#else
		unsigned n = 0;
		int i = reinterpret_cast<std::int64_t&>(u);
		while (1) {
			if (i < 0) break;
			n ++;
			i <<= 1;
		}
		return n;
	#endif
	}
	
	template<class T, class S, class U>
	class __representation
	{
	public:
		__representation();
		__representation(T x);
		__representation(int exp, S sig);
		__representation(U bits);
		inline T value() const;
		inline int exponent() const;
		inline S significand() const;
		inline T exp2(int exp);
		inline int ilogb() const;
		inline void negate();
	};
	
	template<class T>
	class representation;
	
	using bits16 = std::uint16_t;
	using bits32 = std::uint32_t;
	using bits64 = std::uint64_t;
	
	#endif // (__USE_FLOAT_IEEE754_SPECIALIZATION__) || (__USE_DOUBLE_IEEE754_SPECIALIZATION__) || (__USE_LONG_DOUBLE_X86_EXTENDED_SPECIALIZATION__)

	#if (__USE_FLOAT_IEEE754_SPECIALIZATION__)

	template<>
	class __representation<float, bits32, bits32>
	{
	public:
		inline __representation()
		:
		data_{}
		{}
		
		inline __representation(float x)
		:
		data_{x}
		{}

		inline __representation(bits32 u)
		:
		data_{u}
		{}
		
		inline __representation(int exp, bits32 sig)
		:
		data_{exp, sig}
		{}
		
		inline float value() const
		{
			return data_.value_;
		}
		
		inline int exponent() const
		{
			return static_cast<int>((data_.bits_ & exp_mask) >> exp_shift) - exp_bias;
		}
		
		inline bits32 significand() const
		{
			return data_.bits_ & sig_mask;
		}
		
		inline float exp2(int exp)
		{
			if (exp < min_explicit_exponent<float>)
			{
				data_.bits_ = sig_integer_bit >> (min_explicit_exponent<float> - exp);
				return data_.value_;
			}
			else
			{
				data_.bits_ = (static_cast<bits32>(exp + exp_bias) << exp_shift) & exp_mask;
				return data_.value_;
			}
		}
		
		inline int ilogb() const
		{
			int exp = exponent();
			if (exp == -exp_bias) // denormalized
			{
				return exp - (count_leading_zeros(data_.bits_ & sig_mask) - sig_offset);
			}
			else
			{
				return exp;
			}
		}
		
		inline void negate()
		{
			data_.value_ = - data_.value_;
		}

	private:
		static constexpr int exp_bias = 127;
		static constexpr int exp_shift = 23;
		static constexpr int sig_offset = 9;
		static constexpr bits32 exp_mask = 0x7F800000;
		static constexpr bits32 sig_mask = 0x007FFFFF;
		static constexpr bits32 sig_integer_bit = 0x00800000;
	
		union data
		{
			inline data()
			:
			bits_{0}
			{}
			
			inline data(float x)
			:
			value_{x}
			{}

			inline data(bits32 u)
			:
			bits_{u}
			{}

			inline data(int exp, bits32 sig)
			:
			bits_{ ((static_cast<bits32>(exp + exp_bias) << exp_shift) & exp_mask) | (sig & sig_mask)}
			{}
			
			inline ~data()
			{}
		
			bits32 bits_;
			float value_;
		} data_;
	};
	
	template<>
	class representation<float> : public __representation<float, bits32, bits32>
	{
	public:
		using base = __representation<float, bits32, bits32>;
	
		inline representation()
		:
		base{}
		{}
		
		inline representation(float x)
		:
		base{x}
		{}

		inline representation(bits32 u)
		:
		base{u}
		{}
		
		inline representation(int exp, bits32 sig)
		:
		base{exp, sig}
		{}
	};
	
	template<>
	inline float exp2i<float>(int exp)
	{
		return representation<float>{}.exp2(exp);
	}

	template<>
	inline int ilog2<float>(float x)
	{
		return representation<float>{x}.ilogb();
	}
	
	#endif // __USE_FLOAT_IEEE754_SPECIALIZATION__
	
	#if (__USE_DOUBLE_IEEE754_SPECIALIZATION__)

	template<>
	class __representation<double, bits64, bits64>
	{
	public:
		inline __representation()
		:
		data_{}
		{}
		
		inline __representation(double x)
		:
		data_{x}
		{}

		inline __representation(bits64 u)
		:
		data_{u}
		{}
		
		inline __representation(int exp, bits64 sig)
		:
		data_{exp, sig}
		{}
		
		inline double value() const
		{
			return data_.value_;
		}
		
		inline int exponent() const
		{
			return static_cast<int>((data_.bits_ & exp_mask) >> exp_shift) - exp_bias;
		}
		
		inline bits64 significand() const
		{
			return data_.bits_ & sig_mask;
		}
		
		inline double exp2(int exp)
		{
			if (exp < min_explicit_exponent<double>)
			{
				data_.bits_ = sig_integer_bit >> (min_explicit_exponent<double> - exp);
				return data_.value_;
			}
			else
			{
				data_.bits_ = (static_cast<bits64>(exp + exp_bias) << exp_shift) & exp_mask;
				return data_.value_;
			}
		}
	
		inline int ilogb() const
		{
			int exp = exponent();
			if (exp == -exp_bias) // denormalized
			{
				return exp - (count_leading_zeros(data_.bits_ & sig_mask) - sig_offset);
			}
			else
			{
				return exp;
			}
		}
		
		inline void negate()
		{
			data_.value_ = - data_.value_;
		}
	
	private:
		static constexpr int exp_bias = 1023;
		static constexpr int exp_shift = 52;
		static constexpr int sig_offset = 12;
		static constexpr bits64 exp_mask = 0x7FF0000000000000;
		static constexpr bits64 sig_mask = 0x000FFFFFFFFFFFFF;
		static constexpr bits64 sig_integer_bit = 0x0010000000000000;
		
		union data
		{
			inline data()
			:
			bits_{0}
			{}
			
			inline data(double x)
			:
			value_{x}
			{}

			inline data(bits64 u)
			:
			bits_{u}
			{}

			inline data(int exp, bits64 sig)
			:
			bits_{ ((static_cast<bits64>(exp + exp_bias) << exp_shift) & exp_mask) | (sig & sig_mask)}
			{}
			
			inline ~data()
			{}
		
			bits64 bits_;
			double value_;
		} data_;
	
	};

	template<>
	class representation<double> : public __representation<double, bits64, bits64>
	{
	public:
		using base = __representation<double, bits64, bits64>;
	
		inline representation()
		:
		base{}
		{}
		
		inline representation(double x)
		:
		base{x}
		{}

		inline representation(bits64 u)
		:
		base{u}
		{}
		
		inline representation(int exp, bits64 sig)
		:
		base{exp, sig}
		{}
	};

	template<>
	inline double exp2i<double>(int exp)
	{
		return representation<double>{}.exp2(exp);
	}

	template<>
	inline int ilog2<double>(double x)
	{
		return representation<double>{x}.ilogb();
	}

	#endif // __USE_DOUBLE_IEEE754_SPECIALIZATION__

	#if (__USE_LONG_DOUBLE_X86_EXTENDED_SPECIALIZATION__)

	struct bits80
	{
		bits80(const bits80& rhs)
		:
		low{rhs.low},
		high{rhs.high}
		{}
		
		bits80(bits16 hi, bits64 lo)
		:
		low{lo},
		high{hi}
		{}
		
		bits80()
		:
		low{0},
		high{0}
		{}
		
		bits64 low;
		bits16 high;
	};

	template<>
	class __representation<long double, bits64, bits80>
	{
	public:
		inline __representation()
		:
		data_{}
		{}
		
		inline __representation(long double x)
		:
		data_{x}
		{}

		inline __representation(const bits80& u)
		:
		data_{u}
		{}
		
		inline __representation(int exp, std::uint64_t sig)
		:
		data_{exp, sig}
		{}
		
		inline long double value() const
		{
			return data_.value_;
		}
		
		inline int exponent() const
		{
			return static_cast<int>((data_.bits_.high & exp_mask) >> exp_shift) - exp_bias;
		}
		
		inline std::uint64_t significand() const
		{
			return data_.bits_.low & sig_mask;
		}
		
		inline long double exp2(int exp)
		{
			if (exp < min_explicit_exponent<long double>)
			{
				data_.bits_.high = 0;
				data_.bits_.low = sig_integer_bit >> (min_explicit_exponent<long double> - exp);
				return data_.value_;
			}
			else
			{
				data_.bits_.high = static_cast<bits16>(exp + exp_bias) & exp_mask;
				data_.bits_.low = sig_integer_bit;
				return data_.value_;
			}
		}
		
		inline int ilogb() const
		{
			int exp = exponent();
			if (exp == -exp_bias) // denormalized
			{
				return exp - (count_leading_zeros(data_.bits_.low & sig_mask) - sig_offset);
			}
			else
			{
				return exp;
			}
		}

		inline void negate()
		{
			data_.value_ = - data_.value_;
		}
	
	private:
		static constexpr int exp_bias = 16383;
		static constexpr int exp_shift = 0;
		static constexpr int sig_offset = 1;
		static constexpr bits16 exp_mask = 0x7FFF;
		static constexpr bits64 sig_mask = 0xFFFFFFFFFFFFFFFF;
		static constexpr bits64 sig_integer_bit = 0x8000000000000000;
	
		union data
		{
			inline data()
			:
			bits_{}
			{}
			
			inline data(long double x)
			:
			value_{x}
			{}

			inline data(const bits80& u)
			:
			bits_{u}
			{}

			inline data(int exp, bits64 sig)
			:
			bits_{static_cast<bits16>(static_cast<bits16>(exp + exp_bias) & exp_mask), sig}
			{}
			
			inline ~data()
			{}
		
			bits80 bits_;
			long double value_;
		} data_;
	
	};

	template<>
	class representation<long double> : public __representation<long double, bits64, bits80>
	{
	public:
		using base = __representation<long double, bits64, bits80>;
	
		inline representation()
		:
		base{}
		{}
		
		inline representation(long double x)
		:
		base{x}
		{}

		inline representation(const bits80& u)
		:
		base{u}
		{}
		
		inline representation(int exp, std::uint64_t sig)
		:
		base{exp, sig}
		{}
	};
	
	template<>
	inline long double exp2i<long double>(int exp)
	{
		return representation<long double>{}.exp2(exp);
	}

	template<>
	inline int ilog2<long double>(long double x)
	{
		return representation<long double>{x}.ilogb();
	}
	
	#endif // __USE_LONG_DOUBLE_X86_EXTENDED_SPECIALIZATION__
	
	template<class T>
	static inline T ulp(T x)
	{
		if (isinf(x) || isnan(x))
		{
			return static_cast<T>(0.0);
		}
		else
		{
			return exp2i<T>(std::max(ilog2(x) - fractional_precision<T, 0>, exponent_limit<T, 0>));
		}
	}
	
	template<int N, class T>
	static inline T margin(T x)
	{
		if (isinf(x) || isnan(x))
		{
			return static_cast<T>(0.0);
		}
		else
		{
			return exp2i<T>(std::max(ilog2(x) - fractional_precision<T, N>, exponent_limit<T, N>));
		}
	}

	template<int N = 1>
	class proximal
	{
	private:
	
		template<class T>
		static inline T _ulp(T x)
		{
			T exp_ulp_x = ilog2(x) - fractional_precision<T, 0>;
			return exp2i<T>(exp_ulp_x > exponent_limit<T, 0> ? exp_ulp_x : exponent_limit<T, 0>);
		}

		template<class T>
		static inline T _margin(T x)
		{
			int margin_exp = ilog2(x) - fractional_precision<T, N>;
			return exp2i<T>(margin_exp > exponent_limit<T, N> ? margin_exp : exponent_limit<T, N>);
		}
	
		template<class T>
		static inline bool _within_margin(T a, T b)
		{
			if (a == b)
			{
				return true;
			}
			
			if (isinf(a) || isinf(b) || isnan(a) || isnan(b))
			{
				return false;
			}
 			return std::abs(a - b) <= _margin(std::max(std::abs(a), std::abs(b)));
		}
		
	public:
	
		inline float ulp(float x) const
		{
			if (isinf(x) || isnan(x))
			{
				return static_cast<float>(0.0);
			}
			if (x == 0.0)
			{
				return exp2i<float>(exponent_limit<float, 0>);
			}
			return _ulp(x);
		}
	
		inline double ulp(double x) const
		{
			if (isinf(x) || isnan(x))
			{
				return static_cast<double>(0.0);
			}
			if (x == 0.0)
			{
				return exp2i<double>(exponent_limit<double, 0>);
			}
			return _ulp(x);
		}
	
		inline long double ulp(long double x) const
		{
			if (isinf(x) || isnan(x))
			{
				return static_cast<long double>(0.0);
			}
			if (x == 0.0)
			{
				return exp2i<long double>(exponent_limit<long double, 0>);
			}
			return _ulp(x);
		}
		
		inline float margin(float x) const
		{
			if (isinf(x) || isnan(x))
			{
				return static_cast<float>(0.0);
			}
			if (x == 0.0)
			{
				return exp2i<float>(exponent_limit<float, N>);
			}
			return _margin(x);
		}
	
		inline double margin(double x) const
		{
			if (isinf(x) || isnan(x))
			{
				return static_cast<double>(0.0);
			}
			if (x == 0.0)
			{
				return exp2i<double>(exponent_limit<double, N>);
			}
			return _margin(x);
		}
	
		inline long double margin(long double x) const
		{
			if (isinf(x) || isnan(x))
			{
				return static_cast<long double>(0.0);
			}
			if (x == 0.0)
			{
				return exp2i<long double>(exponent_limit<long double, N>);
			}
			return _margin(x);
		}
	
		inline bool operator()(float a, float b) const
		{
			return _within_margin(a, b);
		}
		
		inline bool operator()(double a, double b) const
		{
			return _within_margin(a, b);
		}
		
		inline bool operator()(long double a, long double b) const
		{
			return _within_margin(a, b);
		}
		
		template<class T>
		inline T ulp(T value) const = delete;

		template<class T>
		inline T margin(T value) const = delete;

		template<class T, class U>
		inline bool operator()(T a, U b) const = delete;
	};
}

#endif /* guard_utils_proximal_h */
