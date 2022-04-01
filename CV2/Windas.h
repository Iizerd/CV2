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

//Maybe another time...
//typedef unsigned char U8;
//typedef unsigned char* PU8;
//typedef unsigned short U16;
//typedef unsigned short* PU16;
//typedef unsigned long U32;
//typedef unsigned long* PU32;
//typedef unsigned long long U64;
//typedef unsigned long long* PU64;
//
//typedef char I8;
//typedef char* PI8;
//typedef short I16;
//typedef short* PI16;
//typedef long I32;
//typedef long* PI32;
//typedef long long I64;
//typedef long long* PI64;
//
//typedef unsigned char* PTR;

#define Allocate malloc
#define Realloc realloc
#define AllocateS(StructType) (StructType*)calloc(1, sizeof(StructType))
#define Free free

#define INLINE inline
#define STATIC static
#define STDSTRING std::string
#define STDVECTOR std::vector
#define STDPAIR std::pair
#define STDMUTEX std::mutex
#define StdFind std::find
#define StdSwap std::swap
#define STATIC_ASSERT static_assert

#define MinVal min
#define MaxVal max

//Maybe make a random class if i need more numbers.
template<typename NumType>
INLINE NumType RndGetRandomNum(NumType Min, NumType Max)
{
	static std::random_device RandomDevice;
	static std::default_random_engine Generator(RandomDevice());
	std::uniform_int_distribution<NumType> Distribution(Min, Max);
	return Distribution(Generator);
}

#endif