#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "proximal.h"
#include <iostream>

using namespace utils;

template<class T>
void
print_max_decimal_digits(T value)
{	
	auto flags = std::cout.flags();
	std::cout.flags(std::ios_base::scientific);
	std::cout.precision(std::numeric_limits<T>::digits10+1);
	std::cout << value;
	std::cout.flags(flags);
}

template<class T>
static void
print_hex_scientific(T value)
{	
	auto flags = std::cout.flags();
	std::cout.flags(std::ios_base::fixed | std::ios_base::scientific);
	std::cout << value;
	std::cout.flags(flags);
}


template<class T>
void
print(const std::string& prefix, T value)
{
	std::cout << prefix;
	print_max_decimal_digits(value);
	std::cout << " (";
	print_hex_scientific(value);
	std::cout << ")" << std::endl;

};

TEST_CASE("proximal")
{
	SUBCASE("float (1, 1+1ulp) 1ulp pass")
	{
		proximal<0> close_enough;

		auto a = representation<float>(0,0).value();
		auto b = representation<float>(0,0x00000001).value();
		print("\na=", a);
		print("b=", b);
		print("error=", std::abs(b - a));
		print("margin=", margin<0>(std::max(a, b)));
		print("ulp=", ulp(std::max(a, b)));
		auto c = close_enough(a, b);
		CHECK(c);
	}

	SUBCASE("float (1, 1+2ulp) 1ulp fail")
	{
		proximal<0> close_enough;
		auto a = representation<float>(0,0).value();
		auto b = representation<float>(0,0x00000002).value();
		print("\na=", a);
		print("b=", b);
		print("error=", std::abs(b - a));
		print("margin=", margin<0>(std::max(a, b)));
		print("ulp=", ulp(std::max(a, b)));
		auto c = close_enough(a, b);
		CHECK(!c);
	}

	SUBCASE("double (1, 1+2ulp) 2ulp pass")
	{
		proximal<1> close_enough;
		auto a = representation<double>(0,0).value();
		auto b = representation<double>(0,0x0000000000000002).value();
		print("\na=", a);
		print("b=", b);
		print("error=", std::abs(b - a));
		print("margin=", margin<0>(std::max(a, b)));
		print("ulp=", ulp(std::max(a, b)));
		bool c = close_enough(a, b);
		CHECK(c);
	}

	SUBCASE("double (1, 1+3ulp) 2ulp fail")
	{
		proximal<1> close_enough;
		auto a = representation<double>(0,0).value();
		auto b = representation<double>(0,0x0000000000000003).value();
		print("\na=", a);
		print("b=", b);
		print("error=", std::abs(b - a));
		print("margin=", margin<0>(std::max(a, b)));
		print("ulp=", ulp(std::max(a, b)));
		bool c = close_enough(a, b);
		CHECK(!c);
	}

	SUBCASE("double (1, 1+2ulp) 1ulp fail")
	{
		proximal<0> close_enough;
		
		auto a = representation<double>(0,0).value();
		auto b = representation<double>(0,0x0000000000000002).value();
		print("\na=", a);
		print("b=", b);
		print("error=", std::abs(b - a));
		print("margin=", margin<0>(std::max(a, b)));
		print("ulp=", ulp(std::max(a, b)));
		bool c = close_enough(a, b);
		CHECK(!c);
	}

	SUBCASE ("long double A")
	{
		proximal<1> close_enough;
		
		auto a = representation<long double>(16383,0x8000000000000000).value();
		auto b = representation<long double>(16383,0x8000000000000002).value();
		print("\na=", a);
		print("b=", b);
		print("error=", std::abs(b - a));
		print("margin=", margin<1>(std::max(a, b)));
		print("ulp=", ulp(std::max(a, b)));
		bool c = close_enough(a, b);
		CHECK(c);
	}

	SUBCASE ("long double B") 
	{
		proximal<0> close_enough;
		
		auto a = representation<long double>(-16322,0x8000000000000000).value();
		auto b = representation<long double>(-16322,0x8000000000000002).value();
		print("\na=", a);
		print("b=", b);
		print("error=", std::abs(b - a));
		print("margin=", margin<0>(std::max(a, b)));
		print("ulp=", ulp(std::max(a, b)));
		bool c = close_enough(a, b);
		CHECK(!c);
	}

	SUBCASE ("long double (0,minimum denormalized long double) 1ulp pass") 
	{
		proximal<0> close_enough;
		
		auto a = representation<long double>(-16382,0x0000000000000000).value();
		auto b = representation<long double>(-16382,0x0000000000000001).value();
		print("\na=", a);
		print("b=", b);
		print("error=", std::abs(b - a));
		print("margin=", margin<0>(std::max(a, b)));
		print("ulp=", ulp(std::max(a, b)));
		bool c = close_enough(a, b);
		CHECK(c);
	}
}

