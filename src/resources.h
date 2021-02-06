/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __RESOURCES_H__
#define __RESOURCES_H__

#include "bitty.h"
#include "audio.h"
#include "image.h"
#include "map.h"
#include "plus.h"
#include "sprite.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef RESOURCES_FONT_DEFAULT_SIZE
#	define RESOURCES_FONT_DEFAULT_SIZE 14
#endif /* RESOURCES_FONT_DEFAULT_SIZE */

// Used internally to begin as raw bytes.
static constexpr const char RESOURCES_BYTES_HEADER[4] = {
	0, 0, 0, 0
};

/* ===========================================================================} */

/*
** {===========================================================================
** Resources
*/

/**
 * @brief Resource manager.
 */
class Resources : public Collectible {
public:
	typedef unsigned Id;

	template<typename P> struct Resource {
	public:
		P pointer;

	protected:
		Id _id = 0;
	};

	struct Async {
	protected:
		Atomic<bool> _processed;

	public:
		Async();

		bool await(void);
	};

	struct Asset : public Resource<Object::Ptr>, public Async, public virtual Object {
	public:
		typedef std::shared_ptr<Asset> Ptr;

		friend class ResourcesImpl;

	public:
		Object::Ptr ref = nullptr;

	private:
		unsigned _target = Object::TYPE();
		std::string _asset;

	public:
		Asset(unsigned target);
		Asset(unsigned target, Object::Ptr ref);
		Asset(unsigned target, Object::Ptr ref, const std::string &asset);
		virtual ~Asset() override;

		BITTY_CLASS_TYPE('A', 'S', 'T', 'R')

		template<typename T> bool operator == (const T &other) const {
			return _id == other._id && _target == other._target && _asset == other._asset;
		}

		virtual unsigned type(void) const override;

		unsigned target(void) const;

		template<typename T> void to(T &other) const {
			decltype(other.pointer) ptr;
			if (pointer)
				ptr = Object::as<decltype(other.pointer)>(pointer);

			other.pointer = ptr;
			other._id = _id;
			other._asset = _asset;
			other._processed = _processed;

			const_cast<Asset*>(this)->pointer = nullptr;
		}
		template<typename T> void from(T &other) {
			pointer = other.pointer;
			_id = other._id;
			_asset = other._asset;
			_processed = other._processed;

			other.pointer = nullptr;
		}

		Object::Ptr unref(void);
	};

	struct Glyph : public Resource<::Texture::Ptr>, public Async, public virtual Object {
		friend class ResourcesImpl;

	private:
		uintptr_t _font = 0;
		Color _color;

	public:
		Glyph(Id cp, const Color* color /* nullable */);
		virtual ~Glyph() override;

		BITTY_CLASS_TYPE('G', 'L', 'Y', 'R')

		virtual unsigned type(void) const override;
	};

	struct Palette : public Resource<::Palette::Ptr>, public Async, public virtual Object {
	public:
		typedef std::shared_ptr<Palette> Ptr;

		friend struct Asset;

		friend class ResourcesImpl;

	public:
		::Palette::Ptr shadow = nullptr;

		Mutex lock;

	private:
		std::string _asset;

	public:
		Palette(const std::string &asset);
		virtual ~Palette() override;

		BITTY_CLASS_TYPE('P', 'L', 'T', 'R')

		virtual unsigned type(void) const override;

		Object::Ptr unref(void);
	};

	struct Texture : public Resource<::Texture::Ptr>, public Async, public virtual Object {
	public:
		typedef std::shared_ptr<Texture> Ptr;

		friend struct Asset;

		friend class ResourcesImpl;

	public:
		::Image::WeakPtr source;

		Palette::Ptr ref = nullptr;

	private:
		std::string _asset;

	public:
		Texture(const std::string &asset);
		Texture(const std::string &asset, Palette::Ptr ref);
		virtual ~Texture() override;

		BITTY_CLASS_TYPE('I', 'M', 'G', 'R')

		virtual unsigned type(void) const override;

		Palette::Ptr unref(void);
	};

	struct Sprite : public Resource<::Sprite::Ptr>, public Async, public virtual Object {
	public:
		typedef std::shared_ptr<Sprite> Ptr;

		friend struct Asset;

		friend class ResourcesImpl;

	public:
		Texture::Ptr ref = nullptr;

		RecursiveMutex lock;

	private:
		std::string _asset;

	public:
		Sprite(const std::string &asset);
		Sprite(const std::string &asset, Texture::Ptr ref);
		virtual ~Sprite() override;

		BITTY_CLASS_TYPE('S', 'P', 'R', 'R')

		virtual unsigned type(void) const override;

		Texture::Ptr unref(void);
	};

	struct Map : public Resource<::Map::Ptr>, public Async, public virtual Object {
	public:
		typedef std::shared_ptr<Map> Ptr;

		friend struct Asset;

		friend class ResourcesImpl;

	public:
		::Map::Ptr shadow = nullptr;

		Texture::Ptr ref = nullptr;

		Mutex lock;

	private:
		std::string _asset;

	public:
		Map(const std::string &asset);
		Map(const std::string &asset, Texture::Ptr ref);
		virtual ~Map() override;

		BITTY_CLASS_TYPE('M', 'A', 'P', 'R')

		virtual unsigned type(void) const override;

		Texture::Ptr unref(void);
	};

	struct Sfx : public Resource<::Sfx::Ptr>, public Async, public virtual Object {
	public:
		typedef std::shared_ptr<Sfx> Ptr;

		friend struct Asset;

		friend class ResourcesImpl;

	private:
		std::string _asset;

	public:
		Sfx(const std::string &asset);
		virtual ~Sfx() override;

		BITTY_CLASS_TYPE('S', 'F', 'X', 'R')

		virtual unsigned type(void) const override;

		Object::Ptr unref(void);
	};

	struct Music : public Resource<::Music::Ptr>, public Async, public virtual Object {
	public:
		typedef std::shared_ptr<Music> Ptr;

		friend struct Asset;

		friend class ResourcesImpl;

	private:
		std::string _asset;

	public:
		Music(const std::string &asset);
		virtual ~Music() override;

		BITTY_CLASS_TYPE('M', 'U', 'S', 'R')

		virtual unsigned type(void) const override;

		Object::Ptr unref(void);
	};

	template<typename T> struct List {
	public:
		typedef T ValueType;
		typedef std::list<ValueType> Assets;
		typedef typename Assets::iterator Iterator;
		typedef typename Assets::const_iterator ConstIterator;

	public:
		Mutex lock;

	private:
		Assets _assets;

	public:
		Iterator begin(void) {
			return _assets.begin();
		}
		Iterator end(void) {
			return _assets.end();
		}
		ConstIterator begin(void) const {
			return _assets.begin();
		}
		ConstIterator end(void) const {
			return _assets.end();
		}

		int count(void) const {
			return (int)_assets.size();
		}
		bool empty(void) const {
			return _assets.empty();
		}
		void add(ValueType res) {
			_assets.push_back(res);
		}
		void remove(ConstIterator where) {
			_assets.erase(where);
		}
		void clear(void) {
			_assets.clear();
		}
	};

public:
	virtual bool open(void) = 0;
	virtual bool close(void) = 0;

	virtual void reset(void) = 0;

	/**
	 * @brief Sets the data to generate texture of glyph.
	 */
	virtual void font(const class Font* font) = 0;
	/**
	 * @brief Resets to the default font data.
	 */
	virtual void font(std::nullptr_t) = 0;

	/**
	 * @brief Loads texture from an image file.
	 */
	virtual ::Texture::Ptr load(class Renderer* rnd, const char* path) = 0;
	/**
	 * @brief Loads asset from the project.
	 *
	 * @param[in, out] req
	 */
	virtual ::Object::Ptr load(const class Project* project, Asset &req) = 0;
	/**
	 * @brief Loads texture from a glyph.
	 *
	 * @param[in, out] req
	 * @param[out] width
	 */
	virtual ::Texture::Ptr load(class Renderer* rnd, Glyph &req, int* width /* nullable */, int* height /* nullable */) = 0;
	/**
	 * @brief Loads palette from the project.
	 *
	 * @param[in, out] req
	 */
	virtual ::Palette::Ptr load(const class Project* project, Palette &req) = 0;
	/**
	 * @brief Loads image texture from the project.
	 *
	 * @param[in, out] req
	 */
	virtual ::Texture::Ptr load(const class Project* project, Texture &req) = 0;
	/**
	 * @brief Loads sprite from the project.
	 *
	 * @param[in, out] req
	 */
	virtual ::Sprite::Ptr load(const class Project* project, Sprite &req) = 0;
	/**
	 * @brief Loads map from the project.
	 *
	 * @param[in, out] req
	 */
	virtual ::Map::Ptr load(const class Project* project, Map &req) = 0;
	/**
	 * @brief Loads SFX from the project.
	 *
	 * @param[in, out] req
	 */
	virtual ::Sfx::Ptr load(const class Project* project, Sfx &req) = 0;
	/**
	 * @brief Loads music from the project.
	 *
	 * @param[in, out] req
	 */
	virtual ::Music::Ptr load(const class Project* project, Music &req) = 0;

	virtual int unload(const char* path) = 0;
	virtual int unload(const Asset &req) = 0;
	virtual int unload(const Glyph &req) = 0;
	virtual int unload(const Palette &req) = 0;
	virtual int unload(const Texture &req) = 0;
	virtual int unload(const Sprite &req) = 0;
	virtual int unload(const Map &req) = 0;
	virtual int unload(const Sfx &req) = 0;
	virtual int unload(const Music &req) = 0;

	static Resources* create(void);
	static void destroy(Resources* ptr);
};

/* ===========================================================================} */

#endif /* __RESOURCES_H__ */
