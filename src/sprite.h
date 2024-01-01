/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __SPRITE_H__
#define __SPRITE_H__

#include "bitty.h"
#include "cloneable.h"
#include "json.h"
#include "texture.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef SPRITE_ANY_KEY
#	define SPRITE_ANY_KEY "*"
#endif /* SPRITE_ANY_KEY */

#ifndef SPRITE_DEFAULT_INTERVAL
#	define SPRITE_DEFAULT_INTERVAL 0.25f
#endif /* SPRITE_DEFAULT_INTERVAL */

/* ===========================================================================} */

/*
** {===========================================================================
** Sprite
*/

/**
 * @brief Sprite resource object.
 */
class Sprite : public Cloneable<Sprite>, public virtual Object {
public:
	typedef std::shared_ptr<Sprite> Ptr;

	typedef std::tuple<int, int> Range;

public:
	BITTY_CLASS_TYPE('S', 'P', 'R', 'A')

	using Cloneable<Sprite>::clone;
	using Object::clone;

	virtual int width(void) const = 0;
	virtual int height(void) const = 0;

	virtual bool hFlip(void) const = 0;
	virtual void hFlip(bool f) = 0;
	virtual bool vFlip(void) const = 0;
	virtual void vFlip(bool f) = 0;

	virtual int count(void) const = 0;
	virtual int indexOf(const std::string &key, int start = 0) const = 0;
	virtual Range rangeOf(const std::string &key, int start = 0) const = 0;

	/**
	 * @param[out] tex
	 * @param[out] area
	 * @param[out] interval
	 * @param[out] key
	 */
	virtual bool get(int index, Texture::Ptr* tex /* nullable */, Math::Recti* area /* nullable */, double* interval /* nullable */, const char** key /* nullable */) const = 0;
	virtual bool set(int index, Texture::Ptr tex /* nullable */, const Math::Recti* area /* nullable */, const double* interval /* nullable */, const char* key /* nullable */) = 0;
	virtual bool set(int index, const Math::Recti* area /* nullable */, const double* interval /* nullable */, const char* key /* nullable */) = 0;
	virtual void add(Texture::Ptr tex, const Math::Recti* area /* nullable */, const double* interval /* nullable */, const char* key /* nullable */) = 0;
	virtual bool insert(int index, Texture::Ptr tex, const Math::Recti* area /* nullable */, const double* interval /* nullable */, const char* key /* nullable */) = 0;
	/**
	 * @param[out] tex
	 * @param[out] area
	 * @param[out] interval
	 * @param[out] key
	 */
	virtual bool remove(int index, Texture::Ptr* tex /* nullable */, Math::Recti* area /* nullable */, double* interval /* nullable */, std::string* key /* nullable */) = 0;

	virtual bool play(int begin, int end, bool reset, bool loop, double* duration /* nullable */) = 0;
	virtual bool play(const std::string &key, bool reset, bool loop, double* duration /* nullable */) = 0;
	virtual void pause(void) = 0;
	virtual void resume(void) = 0;
	virtual void stop(void) = 0;
	/**
	 * @param[out] index
	 * @param[out] tex
	 * @param[out] area
	 * @param[out] interval
	 * @param[out] key
	 */
	virtual bool current(int* index /* nullable */, Texture::Ptr* tex /* nullable */, Math::Recti* area /* nullable */, double* interval /* nullable */, const char** key /* nullable */) const = 0;

	virtual bool update(double delta, unsigned* id /* nullable */) = 0;

	virtual void render(
		class Renderer* rnd,
		int x, int y, int width, int height,
		const double* rotAngle /* nullable */, const Math::Vec2f* rotCenter /* nullable */,
		const Color* color /* nullable */, bool colorChanged, bool alphaChanged
	) const = 0;

	virtual bool load(Texture::Ptr tex, const Math::Recti* fullArea /* nullable */, const Math::Vec2i* frameSize, double interval, bool columnMajorOrder) = 0;
	virtual bool load(int width, int height) = 0;
	virtual void unload(void) = 0;

	/**
	 * @param[out] val
	 * @param[in, out] doc
	 */
	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const = 0;
	/**
	 * @param[in, out] val
	 */
	virtual bool toJson(rapidjson::Document &val) const = 0;
	virtual bool fromJson(Texture::Ptr tex, const rapidjson::Value &val) = 0;
	virtual bool fromJson(Texture::Ptr tex, const rapidjson::Document &val) = 0;

	static Sprite* create(int width, int height);
	static void destroy(Sprite* ptr);
};

/* ===========================================================================} */

#endif /* __SPRITE_H__ */
