/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "bytes.h"
#include "datetime.h"
#include "file_handle.h"
#include "font.h"
#include "project.h"
#include "resources.h"
#include "resource/inline_resource.h"
#include <unordered_map>

/*
** {===========================================================================
** Macros and constants
*/

static_assert(!std::numeric_limits<Resources::Id>::is_signed, "Wrong type.");
static_assert(sizeof(Resources::Id) == sizeof(Font::Codepoint), "Wrong size.");

/* ===========================================================================} */

/*
** {===========================================================================
** Resource key
*/

class ResourceKey {
public:
	struct Hash {
		/**
		 * @brief Gets the hash code of the specific key.
		 */
		size_t operator () (const ResourceKey &key) const {
			const size_t result = Math::hash(
				0,
				key.id(),
				(key._size.x << 16) | key._size.y,
				key._color.toRGBA(),
				key.detail()
			);

			return result;
		}
	};
	struct Less {
		/**
		 * @brief Compares two keys and tells whether the first is less than the second one.
		 */
		bool operator () (const ResourceKey &left, const ResourceKey &right) const {
			return left.compare(right) < 0;
		}
	};

private:
	Resources::Id _id = 0;
	Math::Vec2i _size;
	Color _color;
	std::string _detail;

public:
	ResourceKey() {
	}
	ResourceKey(Resources::Id id, uintptr_t ptr, const Color* color) : _id(id) {
		union U {
			uintptr_t in;
			Math::Vec2i out;

			U() {
				out = Math::Vec2i(0, 0);
			}
		};
		static_assert(sizeof(Math::Vec2i) >= sizeof(uintptr_t), "Wrong size.");

		U u;
		u.in = ptr;
		_size = u.out;
		_color = color ? *color : Color(255, 255, 255);
	}
	ResourceKey(Resources::Id id, const Math::Vec2i &size, const Color* color) : _id(id), _size(size) {
		_color = color ? *color : Color(255, 255, 255);
	}
	ResourceKey(Resources::Id id, const Math::Vec2i &size, const Color* color, const std::string &detail) : _id(id), _size(size) {
		if (color)
			_color = *color;

		_detail = detail;
	}

	Resources::Id id(void) const {
		return _id;
	}
	void id(Resources::Id d) {
		_id = d;
	}
	const Math::Vec2i &size(void) const {
		return _size;
	}
	void size(const Math::Vec2i &d) {
		_size = d;
	}
	const Color &color(void) const {
		return _color;
	}
	void color(const Color &d) {
		_color = d;
	}
	const std::string &detail(void) const {
		return _detail;
	}
	void detail(const std::string &d) {
		_detail = d;
	}

	bool operator == (const ResourceKey &other) const {
		return equals(other);
	}

	int compare(const ResourceKey &other) const {
		if (id() < other.id())
			return -1;
		else if (id() > other.id())
			return 1;

		if (size().compare(other.size()) < 0)
			return -1;
		else if (size().compare(other.size()) > 0)
			return 1;

		if (color().toRGBA() < other.color().toRGBA())
			return -1;
		else if (color().toRGBA() > other.color().toRGBA())
			return 1;

		if (detail() < other.detail())
			return -1;
		else if (detail() > other.detail())
			return 1;

		return 0;
	}
	bool equals(const ResourceKey &other) const {
		return id() == other.id() &&
			size() == other.size() &&
			color() == other.color() &&
			detail() == other.detail();
	}
};

/* ===========================================================================} */

/*
** {===========================================================================
** Resources
*/

class ResourcesImpl : public Resources {
private:
	typedef std::unordered_map<ResourceKey, Object::Ptr, ResourceKey::Hash> Dictionary;

private:
	bool _opened = false;

	Font::Ptr _font = nullptr;

	Dictionary _dictionary;

	static Id _idSeed;

public:
	ResourcesImpl() {
		_font = Font::Ptr(Font::create());
	}
	virtual ~ResourcesImpl() {
		cleanup();

		_font = nullptr;
	}

	virtual bool open(void) override {
		if (_opened)
			return false;
		_opened = true;

		font(nullptr);

		fprintf(stdout, "Resources opened.\n");

		return true;
	}
	virtual bool close(void) override {
		if (!_opened)
			return false;
		_opened = false;

		fprintf(stdout, "Resources closed.\n");

		return true;
	}

	virtual int collect(void) override {
		int result = 0;

		std::map<Object::Ptr*, std::list<ResourceKey> > referenced;
		Dictionary::iterator it = _dictionary.begin();
		while (it != _dictionary.end()) {
			Object::Ptr &ptr = it->second;
			if (unique(ptr)) {
				it = _dictionary.erase(it);
				++result;
			} else {
				Object::Ptr* key = &ptr;
				auto &val = referenced[key];
				val.push_back(it->first);
				++it;
			}
		}

		for (auto kv : referenced) {
			if (kv.first->use_count() == (long)kv.second.size()) {
				for (const ResourceKey &key : kv.second) {
					it = _dictionary.find(key);
					if (it == _dictionary.end())
						continue;

					it = _dictionary.erase(it);
					++result;
				}
			}
		}

		const char* fmt = result > 1 ?
			"Collected %d resources.\n" :
			"Collected %d resource.\n";
		fprintf(stdout, fmt, result);

		return result;
	}
	virtual int cleanup(void) override {
		const int result = (int)_dictionary.size();
		_dictionary.clear();

		return result;
	}

	virtual void reset(void) override {
		font(nullptr);

		const int dictCount = (int)_dictionary.size();
		_dictionary.clear();

		_idSeed = 1;

		const char* fmt = dictCount > 1 ?
			"Resources reset, unloaded %d resources.\n" :
			"Resources reset, unloaded %d resource.\n";
		fprintf(stdout, fmt, dictCount);
	}

	virtual void resetRenderTargets(void) override {
		int resetCount = 0;

		Dictionary::iterator it = _dictionary.begin();
		while (it != _dictionary.end()) {
			Object::Ptr &ptr = it->second;
			if (ptr->type() == ::Map::TYPE()) {
				::Map::Ptr map = Object::as<::Map::Ptr>(ptr);
				if (map) {
					map->cleanup();
					++resetCount;
				}
				++it;
			} else {
				++it;
			}
		}

		const char* fmt = resetCount > 1 ?
			"Resources reset for render targets, cleaned up %d resources.\n" :
			"Resources reset for render targets, cleaned up %d resource.\n";
		fprintf(stdout, fmt, resetCount);
	}

	virtual void font(const class Font* font_) override {
		if (font_)
			_font->fromFont(font_);
		else
			_font->fromBytes(RES_FONT_PROGGY_CLEAN, BITTY_COUNTOF(RES_FONT_PROGGY_CLEAN), RESOURCES_FONT_DEFAULT_SIZE, 0);
	}
	virtual void font(std::nullptr_t) override {
		_font->fromBytes(RES_FONT_PROGGY_CLEAN, BITTY_COUNTOF(RES_FONT_PROGGY_CLEAN), RESOURCES_FONT_DEFAULT_SIZE, 0);
	}

	virtual ::Texture::Ptr load(class Renderer* rnd, const char* path) override {
		return fromCacheOrFile(rnd, path);
	}
	virtual ::Object::Ptr load(const class Project* project, Asset &req) override {
		Object::Ptr ref = nullptr;
		if (req.ref) {
			switch (req.ref->type()) {
			case Palette::TYPE(): {
					Palette::Ptr pal = Object::as<Palette::Ptr>(req.ref);
					ref = pal->pointer;
				}

				break;
			case Texture::TYPE(): {
					Texture::Ptr tex = Object::as<Texture::Ptr>(req.ref);
					ref = tex->pointer;
				}

				break;
			}
		}

		return fromCacheOrAsset<Object::Ptr, Asset>(
			project,
			[] (::Asset* asset, Asset &/* req */) -> Object::Ptr {
				switch (asset->type()) {
				case ::Image::TYPE(): {
						::Texture::Ptr ptr = asset->texture(::Asset::RUNNING);

						return ptr;
					}
				case ::Sound::TYPE(): {
						Object::Ptr ptr = asset->sound(::Sfx::TYPE());

						return ptr;
					}
				}

				Object::Ptr ptr = asset->object(::Asset::RUNNING);
				switch (asset->type()) {
				case ::Sprite::TYPE(): // Fall through.
				case ::Map::TYPE():
					if (ptr) {
						Object* raw = nullptr;
						if (ptr->clone(&raw))
							ptr = Object::Ptr(raw);
					}

					break;
				}

				return ptr;
			},
			req, ref, req.target()
		);
	}
	virtual ::Texture::Ptr load(class Renderer* rnd, Glyph &req, int* width, int* height) override {
		return fromCacheOrCharacter(rnd, req, width, height);
	}
	virtual ::Palette::Ptr load(const class Project* project, Palette &req) override {
		return fromCacheOrAsset<::Palette::Ptr, Palette>(
			project,
			[] (::Asset* asset, Palette &/* req */) -> ::Palette::Ptr {
				Object::Ptr obj = asset->object(::Asset::RUNNING);
				if (!obj)
					return nullptr;

				::Palette::Ptr ptr = Object::as<::Palette::Ptr>(obj);

				return ptr;
			},
			req, nullptr
		);
	}
	virtual ::Texture::Ptr load(const class Project* project, Texture &req) override {
		Object::Ptr ref = nullptr;
		if (req.ref && req.ref->pointer)
			ref = req.ref->pointer;

		return fromCacheOrAsset<::Texture::Ptr, Texture>(
			project,
			[] (::Asset* asset, Texture &req) -> ::Texture::Ptr {
				Object::Ptr obj = asset->object(::Asset::RUNNING);
				if (obj)
					req.source = Object::as<::Image::Ptr>(obj);

				::Texture::Ptr ptr = asset->texture(::Asset::RUNNING);

				return ptr;
			},
			req, ref, ::Image::TYPE()
		);
	}
	virtual ::Sprite::Ptr load(const class Project* project, Sprite &req) override {
		Object::Ptr ref = nullptr;
		if (req.ref && req.ref->pointer)
			ref = req.ref->pointer;

		return fromCacheOrAsset<::Sprite::Ptr, Sprite>(
			project,
			[] (::Asset* asset, Sprite &/* req */) -> ::Sprite::Ptr {
				Object::Ptr obj = asset->object(::Asset::RUNNING);
				if (!obj)
					return nullptr;

				::Sprite::Ptr ptr = Object::as<::Sprite::Ptr>(obj);
				if (ptr) {
					::Sprite* raw = nullptr;
					if (ptr->clone(&raw))
						ptr = ::Sprite::Ptr(raw);
				}

				return ptr;
			},
			req, ref
		);
	}
	virtual ::Map::Ptr load(const class Project* project, Map &req) override {
		Object::Ptr ref = nullptr;
		if (req.ref && req.ref->pointer)
			ref = req.ref->pointer;

		return fromCacheOrAsset<::Map::Ptr, Map>(
			project,
			[] (::Asset* asset, Map &/* req */) -> ::Map::Ptr {
				Object::Ptr obj = asset->object(::Asset::RUNNING);
				if (!obj)
					return nullptr;

				::Map::Ptr ptr = Object::as<::Map::Ptr>(obj);
				if (ptr) {
					::Map* raw = nullptr;
					if (ptr->clone(&raw))
						ptr = ::Map::Ptr(raw);
				}

				return ptr;
			},
			req, ref
		);
	}
	virtual ::Sfx::Ptr load(const class Project* project, Sfx &req) override {
		return fromCacheOrAsset<::Sfx::Ptr, Sfx>(
			project,
			[] (::Asset* asset, Sfx &/* req */) -> ::Sfx::Ptr {
				Object::Ptr obj = asset->sound(::Sfx::TYPE());
				if (!obj)
					return nullptr;

				::Sfx::Ptr ptr = Object::as<::Sfx::Ptr>(obj);

				return ptr;
			},
			req, nullptr
		);
	}
	virtual ::Music::Ptr load(const class Project* project, Music &req) override {
		return fromCacheOrAsset<::Music::Ptr, Music>(
			project,
			[] (::Asset* asset, Music &/* req */) -> ::Music::Ptr {
				Object::Ptr obj = asset->sound(::Music::TYPE());
				if (!obj)
					return nullptr;

				::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

				return ptr;
			},
			req, nullptr
		);
	}

	virtual int unload(const char* path) override {
		if (!path)
			return 0;

		const ResourceKey key(0, Math::Vec2i(), nullptr, path);
		Dictionary::iterator it = _dictionary.find(key);
		if (it == _dictionary.end()) {
			return 0;
		} else {
			_dictionary.erase(it);

#if defined BITTY_DEBUG
			if (path)
				fprintf(stdout, "Resources unloaded: file \"%s\".\n", path);
			else
				fprintf(stdout, "Resources unloaded: unknown file.\n");
#endif /* BITTY_DEBUG */

			return 1;
		}
	}
	virtual int unload(const Asset &req) override {
		return dispose(req);
	}
	virtual int unload(const Glyph &req) override {
		const ResourceKey key(req._id, req._font, &req._color);
		Dictionary::iterator it = _dictionary.find(key);
		if (it == _dictionary.end()) {
			return 0;
		} else {
			_dictionary.erase(it);

#if defined BITTY_DEBUG
			fprintf(stdout, "Resources unloaded: glyph '%ud'.\n", req._id);
#endif /* BITTY_DEBUG */

			return 1;
		}
	}
	virtual int unload(const Palette &req) override {
		return dispose(req);
	}
	virtual int unload(const Texture &req) override {
		return dispose(req);
	}
	virtual int unload(const Sprite &req) override {
		return dispose(req);
	}
	virtual int unload(const Map &req) override {
		return dispose(req);
	}
	virtual int unload(const Sfx &req) override {
		return dispose(req);
	}
	virtual int unload(const Music &req) override {
		return dispose(req);
	}

	static Id getId(void) {
		Id ret = 0;
		while (ret == 0)
			ret = _idSeed++;

		return ret;
	}

private:
	::Texture::Ptr fromCacheOrFile(class Renderer* rnd, const char* path) {
		if (!rnd)
			return nullptr;
		if (!path)
			return nullptr;

		const ResourceKey key(0, Math::Vec2i(), nullptr, path);
		Dictionary::iterator it = _dictionary.find(key);
		if (it == _dictionary.end()) {
			File* file = File::create();
			Bytes* bytes = Bytes::create();
			::Image* img = ::Image::create(nullptr);
			if (file->open(path, Stream::READ)) {
				file->readBytes(bytes);
				file->close();
			}
			img->fromBytes(bytes);
			::Texture::Ptr ptr(::Texture::create());
			ptr->fromBytes(rnd, ::Texture::STATIC, img->pixels(), img->width(), img->height(), 0);
			ptr->blend(::Texture::BLEND);
			::Image::destroy(img);
			Bytes::destroy(bytes);
			File::destroy(file);

			_dictionary[key] = ptr;

			return ptr;
		} else {
			::Texture::Ptr ptr = Object::as<::Texture::Ptr>(it->second);

			return ptr;
		}
	}
	::Texture::Ptr fromCacheOrCharacter(class Renderer* rnd, Glyph &req, int* outWidth, int* outHeight) {
		if (outWidth)
			*outWidth = -1;
		if (outHeight)
			*outHeight = -1;

		if (!rnd)
			return nullptr;

		::Texture::Ptr ptr = req.pointer;
		if (ptr) {
			if (outWidth)
				*outWidth = ptr->width();
			if (outHeight)
				*outHeight = ptr->height();

			return ptr;
		}

		if (req._processed)
			return nullptr;

		const uintptr_t pointer = (uintptr_t)_font->pointer();
		if (req._font == 0)
			req._font = pointer;
		const ResourceKey key(req._id, req._font, &req._color);
		Dictionary::iterator it = _dictionary.find(key);
		if (it == _dictionary.end()) {
			int width = -1;
			int height = -1;
			Bytes* bytes = Bytes::create();
			if (!_font->render(req._id, bytes, &req._color, &width, &height)) {
				Bytes::destroy(bytes);

				req._processed = true;

				return nullptr;
			}
			assert((int)bytes->count() == width * height * sizeof(Color));
			ptr = ::Texture::Ptr(::Texture::create());
			ptr->fromBytes(rnd, ::Texture::STATIC, bytes->pointer(), width, height, 0);
			ptr->blend(::Texture::BLEND);
			Bytes::destroy(bytes);

			if (outWidth)
				*outWidth = width;
			if (outHeight)
				*outHeight = height;

			_dictionary[key] = ptr;
			req.pointer = ptr;
			req._processed = true;

			return ptr;
		} else {
			ptr = Object::as<::Texture::Ptr>(it->second);
			req.pointer = ptr;
			req._processed = true;

			return ptr;
		}
	}
	template<typename P, typename Q, typename R> P fromCacheOrAsset(const class Project* project, std::function<P(::Asset*, Q &)> getObj, Q &req, R r, unsigned y = P::element_type::TYPE()) {
		if (!project)
			return nullptr;

		P ptr = req.pointer;
		if (ptr)
			return ptr;

		if (req._processed)
			return nullptr;

		auto retrieveObj = [getObj] (::Asset* asset, Q &req) -> P {
			if (!asset || !asset->prepare(::Asset::RUNNING, true))
				return nullptr;

			return getObj(asset, req);
		};
		auto linkAsset = [retrieveObj] (Project* prj, Q &req, R r, unsigned y) -> P {
			// Prepare.
			P ptr = nullptr;
			Bytes* buf = nullptr;

			unsigned type = y;
			if (type == ::Sfx::TYPE() || type == ::Music::TYPE())
				type = Sound::TYPE();

			// Get by entry name.
			::Asset* asset = prj->get(req._asset.c_str());
			if (asset) // Got.
				ptr = retrieveObj(asset, req);
			if (ptr)
				return ptr;

			// Try to link dynamically as bytes buffer.
			constexpr size_t SIZE = BITTY_COUNTOF(RESOURCES_BYTES_HEADER);
			if (req._asset.length() > SIZE && memcmp(req._asset.c_str(), RESOURCES_BYTES_HEADER, SIZE) == 0) {
				buf = Bytes::create();
				buf->writeBytes(
					(Byte*)req._asset.c_str() + SIZE,
					req._asset.length() - SIZE
				);
				buf->poke(0);
				asset = prj->factory().create(prj);
				if (asset->link(type, buf, nullptr, r)) // Linked successfully.
					ptr = retrieveObj(asset, req);
				prj->factory().destroy(asset);
				Bytes::destroy(buf);
				if (ptr)
					return ptr;
			}

			// Try to link dynamically as anonymous string buffer.
			if (type != Sound::TYPE()) {
				buf = Bytes::create();
				buf->writeString(req._asset);
				buf->poke(0);
				asset = prj->factory().create(prj);
				if (asset->link(type, buf, nullptr, r)) // Linked successfully.
					ptr = retrieveObj(asset, req);
				prj->factory().destroy(asset);
				Bytes::destroy(buf);
				if (ptr)
					return ptr;
			}

			// Try to link dynamically as file path.
			File* file = File::create();
			buf = Bytes::create();
			if (file->open(req._asset.c_str(), Stream::READ)) {
				file->readBytes(buf);
				file->close();
			}
			buf->poke(0);
			File::destroy(file);
			asset = prj->factory().create(prj);
			if (asset->link(type, buf, req._asset.c_str(), r)) // Linked successfully.
				ptr = retrieveObj(asset, req);
			prj->factory().destroy(asset);
			Bytes::destroy(buf);
			if (ptr)
				return ptr;

			return nullptr;
		};

		const ResourceKey key(req._id, Math::Vec2i(), nullptr, req._asset);
		Dictionary::iterator it = _dictionary.find(key);
		if (it == _dictionary.end()) {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj)
				return nullptr;

			ptr = linkAsset(prj, req, r, y);

			acquired.reset();

			if (!ptr) {
				req._processed = true;

				return nullptr;
			}

			_dictionary[key] = ptr;
			req.pointer = ptr;
			req._processed = true;

			return ptr;
		} else {
			ptr = Object::as<P>(it->second);
			req.pointer = ptr;
			req._processed = true;

			return ptr;
		}
	}

	template<typename Q> int dispose(const Q &req) {
		const ResourceKey key(req._id, Math::Vec2i(), nullptr, req._asset);
		Dictionary::iterator it = _dictionary.find(key);
		if (it == _dictionary.end()) {
			return 0;
		} else {
			_dictionary.erase(it);

#if defined BITTY_DEBUG
			fprintf(stdout, "Resources unloaded: asset \"%s\".\n", req._asset.c_str());
#endif /* BITTY_DEBUG */

			return 1;
		}
	}
};

Resources::Id ResourcesImpl::_idSeed = 1;

Resources::Async::Async() {
	_processed = false;
}

bool Resources::Async::await(void) {
#if BITTY_MULTITHREAD_ENABLED
	constexpr const int TIMEOUT = 3000; // 3 seconds.
	constexpr const int STEP = 10;
	for (int i = 0; i < TIMEOUT / STEP && !_processed; ++i) // Wait until processed or timeout.
		DateTime::sleep(STEP);
#else /* BITTY_MULTITHREAD_ENABLED */
	assert(_processed);
#endif /* BITTY_MULTITHREAD_ENABLED */

	return _processed;
}

Resources::Asset::Asset(unsigned target) : _target(target) {
}

Resources::Asset::Asset(unsigned target, Object::Ptr ref_) : _target(target), ref(ref_) {
}

Resources::Asset::Asset(unsigned target, Object::Ptr ref_, const std::string &asset) : _target(target), ref(ref_) {
	_asset = asset;
}

Resources::Asset::~Asset() {
}

unsigned Resources::Asset::type(void) const {
	return TYPE();
}

unsigned Resources::Asset::target(void) const {
	return _target;
}

Object::Ptr Resources::Asset::unref(void) {
	Object::Ptr result = nullptr;
	std::swap(result, ref);

	return result;
}

Resources::Glyph::Glyph(Id cp, const Color* col) {
	_id = cp;
	if (col)
		_color = *col;
}

Resources::Glyph::~Glyph() {
}

unsigned Resources::Glyph::type(void) const {
	return TYPE();
}

Resources::Palette::Palette(const std::string &asset) {
	_id = ResourcesImpl::getId();

	_asset = asset;
}

Resources::Palette::~Palette() {
}

unsigned Resources::Palette::type(void) const {
	return TYPE();
}

Object::Ptr Resources::Palette::unref(void) {
	return nullptr;
}

Resources::Texture::Texture(const std::string &asset) {
	_id = ResourcesImpl::getId();

	_asset = asset;
}

Resources::Texture::Texture(const std::string &asset, Palette::Ptr ref_) : ref(ref_) {
	_id = ResourcesImpl::getId();

	_asset = asset;
}

Resources::Texture::~Texture() {
}

unsigned Resources::Texture::type(void) const {
	return TYPE();
}

Resources::Palette::Ptr Resources::Texture::unref(void) {
	Palette::Ptr result = nullptr;
	std::swap(result, ref);

	return result;
}

Resources::Sprite::Sprite(const std::string &asset) {
	_id = ResourcesImpl::getId();

	_asset = asset;
}

Resources::Sprite::Sprite(const std::string &asset, Texture::Ptr ref_) : ref(ref_) {
	_id = ResourcesImpl::getId();

	_asset = asset;
}

Resources::Sprite::~Sprite() {
}

unsigned Resources::Sprite::type(void) const {
	return TYPE();
}

Resources::Texture::Ptr Resources::Sprite::unref(void) {
	Texture::Ptr result = nullptr;
	std::swap(result, ref);

	return result;
}

Resources::Map::Map(const std::string &asset) {
	_id = ResourcesImpl::getId();

	_asset = asset;
}

Resources::Map::Map(const std::string &asset, Texture::Ptr ref_) : ref(ref_) {
	_id = ResourcesImpl::getId();

	_asset = asset;
}

Resources::Map::~Map() {
}

unsigned Resources::Map::type(void) const {
	return TYPE();
}

Resources::Texture::Ptr Resources::Map::unref(void) {
	Texture::Ptr result = nullptr;
	std::swap(result, ref);

	return result;
}

Resources::Sfx::Sfx(const std::string &asset) {
	_id = ResourcesImpl::getId();

	_asset = asset;
}

Resources::Sfx::~Sfx() {
}

unsigned Resources::Sfx::type(void) const {
	return TYPE();
}

Object::Ptr Resources::Sfx::unref(void) {
	return nullptr;
}

Resources::Music::Music(const std::string &asset) {
	_id = ResourcesImpl::getId();

	_asset = asset;
}

Resources::Music::~Music() {
}

unsigned Resources::Music::type(void) const {
	return TYPE();
}

Object::Ptr Resources::Music::unref(void) {
	return nullptr;
}

Resources* Resources::create(void) {
	ResourcesImpl* result = new ResourcesImpl();

	return result;
}

void Resources::destroy(Resources* ptr) {
	ResourcesImpl* impl = static_cast<ResourcesImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
