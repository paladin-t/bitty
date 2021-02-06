/*
** MIT License
**
** For the latest info, see https://github.com/paladin-t/jpath
**
** Copyright (C) 2020 Tony Wang
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
*/

#ifndef __JPATH_H__
#define __JPATH_H__

#include "../rapidjson/include/rapidjson/document.h"
#include <string>

/*
** {===========================================================================
** Jpath
*/

namespace Jpath {

inline bool getValue(const rapidjson::Value &obj, bool &ret) {
	if (!obj.IsBool())
		return false;
	ret = obj.GetBool();

	return true;
}
inline bool getValue(const rapidjson::Value &obj, char &ret) {
	if (!obj.IsInt())
		return false;
	ret = (char)obj.GetInt();

	return true;
}
inline bool getValue(const rapidjson::Value &obj, unsigned char &ret) {
	if (!obj.IsUint())
		return false;
	ret = (unsigned char)obj.GetUint();

	return true;
}
inline bool getValue(const rapidjson::Value &obj, short &ret) {
	if (!obj.IsInt())
		return false;
	ret = (short)obj.GetInt();

	return true;
}
inline bool getValue(const rapidjson::Value &obj, unsigned short &ret) {
	if (!obj.IsUint())
		return false;
	ret = (unsigned short)obj.GetUint();

	return true;
}
inline bool getValue(const rapidjson::Value &obj, int &ret) {
	if (!obj.IsInt())
		return false;
	ret = obj.GetInt();

	return true;
}
inline bool getValue(const rapidjson::Value &obj, unsigned &ret) {
	if (!obj.IsUint())
		return false;
	ret = obj.GetUint();

	return true;
}
inline bool getValue(const rapidjson::Value &obj, long &ret) {
	if (!obj.IsInt64())
		return false;
	ret = (long)obj.GetInt64();

	return true;
}
inline bool getValue(const rapidjson::Value &obj, unsigned long &ret) {
	if (!obj.IsUint64())
		return false;
	ret = (unsigned long)obj.GetUint64();

	return true;
}
inline bool getValue(const rapidjson::Value &obj, long long &ret) {
	if (!obj.IsInt64())
		return false;
	ret = obj.GetInt64();

	return true;
}
inline bool getValue(const rapidjson::Value &obj, unsigned long long &ret) {
	if (!obj.IsUint64())
		return false;
	ret = obj.GetUint64();

	return true;
}
inline bool getValue(const rapidjson::Value &obj, float &ret) {
	if (!obj.IsNumber())
		return false;
	ret = obj.GetFloat();

	return true;
}
inline bool getValue(const rapidjson::Value &obj, double &ret) {
	if (!obj.IsNumber())
		return false;
	ret = obj.GetDouble();

	return true;
}
inline bool getValue(const rapidjson::Value &obj, const char* &ret) {
	if (!obj.IsString())
		return false;
	ret = obj.GetString();

	return true;
}
inline bool getValue(const rapidjson::Value &obj, std::string &ret) {
	if (!obj.IsString())
		return false;
	ret = obj.GetString();

	return true;
}
inline bool getValue(const rapidjson::Value &obj, const rapidjson::Value* &ret) {
	ret = &obj;

	return true;
}
inline void setValue(rapidjson::Value &obj, bool src, rapidjson::Document &) {
	obj.SetBool(src);
}
inline void setValue(rapidjson::Value &obj, char src, rapidjson::Document &) {
	obj.SetInt(src);
}
inline void setValue(rapidjson::Value &obj, unsigned char src, rapidjson::Document &) {
	obj.SetInt(src);
}
inline void setValue(rapidjson::Value &obj, short src, rapidjson::Document &) {
	obj.SetInt(src);
}
inline void setValue(rapidjson::Value &obj, unsigned short src, rapidjson::Document &) {
	obj.SetInt(src);
}
inline void setValue(rapidjson::Value &obj, int src, rapidjson::Document &) {
	obj.SetInt(src);
}
inline void setValue(rapidjson::Value &obj, unsigned src, rapidjson::Document &) {
	obj.SetUint(src);
}
inline void setValue(rapidjson::Value &obj, long src, rapidjson::Document &) {
	obj.SetInt64(src);
}
inline void setValue(rapidjson::Value &obj, unsigned long src, rapidjson::Document &) {
	obj.SetUint64(src);
}
inline void setValue(rapidjson::Value &obj, long long src, rapidjson::Document &) {
	obj.SetInt64(src);
}
inline void setValue(rapidjson::Value &obj, unsigned long long src, rapidjson::Document &) {
	obj.SetUint64(src);
}
inline void setValue(rapidjson::Value &obj, float src, rapidjson::Document &) {
	obj.SetFloat(src);
}
inline void setValue(rapidjson::Value &obj, double src, rapidjson::Document &) {
	obj.SetDouble(src);
}
inline void setValue(rapidjson::Value &obj, const char* src, rapidjson::Document &doc) {
	obj.SetString(src, doc.GetAllocator());
}
inline void setValue(rapidjson::Value &obj, const std::string &src, rapidjson::Document &doc) {
	obj.SetString(src.c_str(), doc.GetAllocator());
}
inline void setValue(rapidjson::Value &obj, const rapidjson::Value &src, rapidjson::Document &doc) {
	obj.CopyFrom(src, doc.GetAllocator());
}

inline bool read(const rapidjson::Value &obj, const rapidjson::Value* &ret, int node) {
	if (ret)
		ret = nullptr;
	if (!obj.IsArray() || node < 0)
		return false;
	if ((rapidjson::SizeType)node >= obj.Size())
		return false;

	ret = &obj[node];

	return true;
}
inline bool read(const rapidjson::Value &obj, const rapidjson::Value* &ret, const char* node) {
	if (ret)
		ret = nullptr;
	if (!obj.IsObject() || !node)
		return false;
	auto entry = obj.FindMember(node);
	if (entry == obj.MemberEnd())
		return false;

	ret = &entry->value;

	return true;
}
inline bool read(const rapidjson::Value &obj, const rapidjson::Value* &ret, const std::string &node) {
	return read(obj, ret, node.c_str());
}
template<typename Car, typename ...Cdr> bool read(const rapidjson::Value &obj, const rapidjson::Value* &ret, Car car, Cdr ...cdr) {
	const rapidjson::Value* tmp = nullptr;
	if (!read(obj, tmp, car))
		return false;
	if (!tmp)
		return false;
	if (!read(*tmp, ret, cdr ...))
		return false;

	return true;
}
inline bool write(rapidjson::Document &doc, rapidjson::Value &obj, rapidjson::Value* &ret, int node) {
	ret = nullptr;
	if (node < 0)
		return false;
	if (!obj.IsArray())
		obj.SetArray();
	while ((rapidjson::SizeType)node >= obj.Size()) {
		rapidjson::Value val;
		val.SetNull();
		obj.PushBack(val, doc.GetAllocator());
	}
	ret = &obj[node];

	return true;
}
inline bool write(rapidjson::Document &doc, rapidjson::Value &obj, rapidjson::Value* &ret, const char* node) {
	ret = nullptr;
	if (!node)
		return false;
	if (!obj.IsObject())
		obj.SetObject();
	auto entry = obj.FindMember(node);
	if (entry == obj.MemberEnd()) {
		rapidjson::Value key, val;
		key.SetString(node, doc.GetAllocator());
		val.SetNull();
		obj.AddMember(key, val, doc.GetAllocator());

		ret = &obj[node];
	} else {
		ret = &entry->value;
	}

	return true;
}
inline bool write(rapidjson::Document &doc, rapidjson::Value &obj, rapidjson::Value* &ret, const std::string &node) {
	return write(doc, obj, ret, node.c_str());
}
template<typename Car, typename ...Cdr> bool write(rapidjson::Document &doc, rapidjson::Value &obj, rapidjson::Value* &ret, Car car, Cdr ...cdr) {
	rapidjson::Value* tmp = nullptr;
	if (!write(doc, obj, tmp, car))
		return false;
	if (!write(doc, *tmp, ret, cdr ...))
		return false;

	return true;
}

template<typename ...Path> bool has(const rapidjson::Value &obj, Path ...path) {
	const rapidjson::Value* tmp = nullptr;
	if (!read(obj, tmp, path ...))
		return false;
	if (!tmp)
		return false;

	return true;
}
template<typename Ret, typename ...Path> bool get(const rapidjson::Value &obj, Ret &ret, Path ...path) {
	const rapidjson::Value* tmp = nullptr;
	if (!read(obj, tmp, path ...))
		return false;
	if (!tmp)
		return false;

	return getValue(*tmp, ret);
}
template<template<typename T, typename A = std::allocator<T> > class Coll, typename Val, typename ...Path> bool get(const rapidjson::Value &obj, Coll<Val> &ret, Path ...path) {
	const rapidjson::Value* tmp = nullptr;
	if (!read(obj, tmp, path ...))
		return false;
	if (!tmp)
		return false;
	if (!tmp->IsArray())
		return false;

	Coll<Val> result;
	for (rapidjson::SizeType i = 0; i < tmp->Size(); ++i) {
		Val val;
		if (!getValue((*tmp)[i], val))
			return false;

		result.push_back(val);
	}
	std::swap(result, ret);

	return true;
}
template<typename Src, typename ...Path> bool set(rapidjson::Document &doc, rapidjson::Value &obj, Src src, Path ...path) {
	rapidjson::Value* tmp = nullptr;
	if (!write(doc, obj, tmp, path ...))
		return false;
	if (!tmp)
		return false;

	setValue(*tmp, src, doc);

	return true;
}
template<template<typename T, typename A = std::allocator<T> > class Coll, class Val, typename ...Path> bool set(rapidjson::Document &doc, rapidjson::Value &obj, const Coll<Val> &src, Path ...path) {
	rapidjson::Value* tmp = nullptr;
	if (!write(doc, obj, tmp, path ...))
		return false;
	if (!tmp)
		return false;

	rapidjson::Value arr;
	arr.SetArray();
	for (auto it = src.begin(); it != src.end(); ++it)
		arr.PushBack(*it, doc.GetAllocator());
	setValue(*tmp, arr, doc);

	return true;
}
template<template<typename T, typename A = std::allocator<T> > class Coll, typename ...Path> bool set(rapidjson::Document &doc, rapidjson::Value &obj, const Coll<std::string> &src, Path ...path) {
	rapidjson::Value* tmp = nullptr;
	if (!write(doc, obj, tmp, path ...))
		return false;
	if (!tmp)
		return false;

	rapidjson::Value arr;
	arr.SetArray();
	for (auto it = src.begin(); it != src.end(); ++it) {
		rapidjson::Value val;
		val.SetString(it->c_str(), doc.GetAllocator());
		arr.PushBack(val, doc.GetAllocator());
	}
	setValue(*tmp, arr, doc);

	return true;
}

}

/* ===========================================================================} */

#endif /* __JPATH_H__ */
