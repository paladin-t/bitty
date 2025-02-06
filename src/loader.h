/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __LOADER_H__
#define __LOADER_H__

#include "bitty.h"
#include "cloneable.h"
#include "plus.h"

/*
** {===========================================================================
** Loader
*/

/**
 * @brief Loader middleware. Override this class to make your own loader for
 *   encrypting, pre/post-processing and other customization.
 */
class Loader : public Cloneable<Loader>, public NonCopyable {
public:
	Loader();
	virtual ~Loader();

	/**
	 * @brief Makes a clone of this loader.
	 *
	 * @param[out] ptr
	 */
	virtual bool clone(Loader** ptr) const override;

	/**
	 * @brief Resets this loader.
	 */
	virtual void reset(void);

	/**
	 * @brief Decodes a `Bytes` buffer after loading an asset. The return value
	 *   reuses the input object rather than creating new one.
	 *
	 * @param[in] project The project to work on.
	 * @param[in] asset The asset to decode.
	 * @param[in, out] buf The buffer to decode.
	 * @return The decoded result in place.
	 */
	virtual class Bytes* decode(const class Project* project, const class Asset* asset, class Bytes* buf) const;
	/**
	 * @brief Encodes a `Bytes` buffer before saving an asset. The return value
	 *   reuses the input object rather than creating new one.
	 *
	 * @param[in] project The project to work on.
	 * @param[in] asset The asset to decode.
	 * @param[in, out] buf The buffer to encode.
	 * @return The encoded result in place.
	 */
	virtual class Bytes* encode(const class Project* project, const class Asset* asset, class Bytes* buf) const;
};

/* ===========================================================================} */

#endif /* __LOADER_H__ */
