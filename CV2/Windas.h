#ifndef __WINDAS_H
#define __WINDAS_H

#pragma warning(disable : 26812)

#include <Windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <random>
#include <cmath>
#include <mutex>

#define Allocate malloc
#define AllocateS(StructType) (StructType*)calloc(1, sizeof(StructType))
#define Free free

#define INLINE inline
#define STDSTRING std::string
#define STDVECTOR std::vector
#define STDPAIR std::pair
#define STDMUTEX std::mutex
#define StdFind std::find
#define STATIC_ASSERT static_assert

#define MinVal min
#define MaxVal max

//Maybe make a random class if i need more numbers.
template<typename NumType>
INLINE NumType RndGetRandomNum(NumType Min, NumType Max)
{
	std::random_device RandomDevice;
	std::default_random_engine Generator(RandomDevice());
	std::uniform_int_distribution<NumType> Distribution(Min, Max);
	return Distribution(Generator);
}

#endif