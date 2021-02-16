/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __MAP_H__
#define __MAP_H__

#include "bitty.h"
#include "cloneable.h"
#include "collectible.h"
#include "json.h"
#include "texture.h"

/*
** {===========================================================================
** Map
*/

/**
 * @brief Map resource object.
 */
class Map : public Collectible, public Cloneable<Map>, public virtual Object {
public:
	typedef std::shared_ptr<Map> Ptr;

	struct Tiles {
		Texture::Ptr texture = nullptr;
		Math::Vec2i count;

		Tiles();
		Tiles(Texture::Ptr texture_, const Math::Vec2i &count_);
	};

public:
	BITTY_CLASS_TYPE('M', 'A', 'P', 'A')

	virtual bool clone(Map** ptr, bool graphical) const = 0;
	using Cloneable<Map>::clone;
	using Object::clone;

	/**
	 * @brief Gets the tile.
	 *
	 * @param[in, out] tiles
	 */
	virtual const Tiles* tiles(Tiles &tiles) const = 0;
	/**
	 * @brief Sets the tile.
	 *
	 * @param[in] tiles
	 */
	virtual void tiles(const Tiles* tiles) = 0;

	virtual int width(void) const = 0;
	virtual int height(void) const = 0;

	/**
	 * @brief This function is slow.
	 */
	virtual Math::Recti aabb(void) const = 0;
	virtual bool resize(int width, int height) = 0;
	/**
	 * @param[out] buf
	 */
	virtual void data(int* buf, size_t len) const = 0;

	virtual int get(int x, int y) const = 0;
	virtual bool set(int x, int y, int v, bool expandable = false) = 0;

	/**
	 * @brief Gets renderable data at a specific tile index.
	 *
	 * @param[out] area
	 * @return The entire tiled texture or `nullptr`.
	 */
	virtual Texture::Ptr at(int index, Math::Recti* area /* nullable */) const = 0;
	/**
	 * @brief Gets renderable data at a specific position.
	 *
	 * @param[out] area
	 * @return The entire tiled texture or `nullptr`.
	 */
	virtual Texture::Ptr at(int x, int y, Math::Recti* area /* nullable */) const = 0;
	/**
	 * @brief Gets sub renderable data at a specific area.
	 *
	 * @return The sub tiled texture or `nullptr`.
	 */
	virtual Texture::Ptr sub(class Renderer* rnd, int x, int y, int width, int height) const = 0;

	virtual bool update(double delta) = 0;

	virtual void render(
		class Renderer* rnd,
		int x, int y,
		const Color* color /* nullable */, bool colorChanged, bool alphaChanged
	) const = 0;

	virtual bool load(const int* cels, int width, int height) = 0;
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
	virtual bool fromJson(Texture::Ptr texture, const rapidjson::Value &val) = 0;
	virtual bool fromJson(Texture::Ptr texture, const rapidjson::Document &val) = 0;

	static int INVALID(void);

	static Map* create(const Tiles* tiles /* nullable */, bool batch);
	static void destroy(Map* ptr);
};

/* ===========================================================================} */

#endif /* __MAP_H__ */
