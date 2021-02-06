/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "archive.h"
#include "asset.h"
#include "audio.h"
#include "bytes.h"
#include "code.h"
#include "editable.h"
#include "editor_bytes.h"
#include "editor_code.h"
#include "editor_font.h"
#include "editor_image.h"
#include "editor_json.h"
#include "editor_map.h"
#include "editor_palette.h"
#include "editor_plugin.h"
#include "editor_sound.h"
#include "editor_sprite.h"
#include "editor_text.h"
#include "file_handle.h"
#include "filesystem.h"
#include "font.h"
#include "image.h"
#include "loader.h"
#include "map.h"
#include "project.h"
#include "sprite.h"
#include "../lib/jpath/jpath.hpp"

/*
** {===========================================================================
** Asset
*/

Asset::States::~States() {
}

Asset::States::States() {
}

Asset::States::Activity Asset::States::activity(void) const {
	return _activity;
}

void Asset::States::activate(Activity act) {
	if (_activity == EDITABLE && act == INSPECTABLE)
		return;

	_activity = act;
}

void Asset::States::deactivate(void) {
	_activity = CLOSED;
	_focusing = false;
}

bool Asset::States::focusing(void) {
	if (_focusing) {
		_focusing = false;

		return true;
	}

	return false;
}

void Asset::States::focus(void) {
	_focusing = true;
}

bool Asset::States::selected(void) const {
	return _selected;
}

void Asset::States::select(void) {
	_selected = true;

	_focusing = true; // Let the workspace to re-focus it again.
}

void Asset::States::deselect(void) {
	_selected = false;
}

Asset::Asset(class Project* project) : _project(project) {
	states(new States());

	custom(false);
}

Asset::~Asset() {
	delete states();
	states(nullptr);

	unlink();

	unload();
}

bool Asset::link(unsigned y, class Bytes* buf, const char* ent, Object::Ptr ref) {
	int error = 0;

	if (!unlink())
		++error;

	type(y);

	if (ent)
		entry(ent);
	else
		entry("");

	if (buf) {
		if (!load(RUNNING, buf, ref, true))
			++error;
	}

	const char* package = _project->path().empty() ? nullptr : _project->path().c_str();
	connect(package, ent);

	return !error;
}

bool Asset::link(Object::Ptr obj, const char* ent) {
	int error = 0;

	if (!unlink())
		++error;

	if (obj)
		type(obj->type());

	if (ent)
		entry(ent);
	else
		entry("");

	object(RUNNING, obj);

	const char* package = _project->path().empty() ? nullptr : _project->path().c_str();
	connect(package, ent);

	return !error;
}

bool Asset::link(const char* package, const char* ent) {
	int error = 0;

	if (!unlink())
		++error;

	const char* src = ent ? ent : package;
	type(typeOf(src, true));

	if (connect(package, ent))
		entry(ent);
	else
		entry("");

	return !error;
}

bool Asset::unlink(void) {
	type(0);

	if (directoryInfo()) {
		DirectoryInfo::destroy(directoryInfo());
		directoryInfo(nullptr);
	}
	if (fileInfo()) {
		FileInfo::destroy(fileInfo());
		fileInfo(nullptr);
	}
	entry("");

	return true;
}

const char* Asset::package(void) const {
	if (directoryInfo())
		return directoryInfo()->fullPath().c_str();
	if (fileInfo())
		return fileInfo()->fullPath().c_str();

	return "";
}

bool Asset::revertible(void) const {
	Editable* edit = editor();
	if (!edit)
		return true;

	switch (type()) {
	case Code::TYPE(): {
			Object::Ptr obj = (Object::Ptr)edit->post(Editable::GET_BREAKPOINTS);
			IList::Ptr lst = Object::as<IList::Ptr>(obj);
			if (lst->count() != 0)
				return false;
		}

		return true;
	default:
		return true;
	}
}

unsigned Asset::referencing(void) const {
	switch (type()) {
	case Image::TYPE(): {
			std::string ext = extName();
			Text::toLowerCase(ext);
			if (ext == BITTY_IMAGE_EXT)
				return Palette::TYPE();
		}

		break;
	case Sprite::TYPE():
		return Image::TYPE();
	case Map::TYPE():
		return Image::TYPE();
	default: // Do nothing.
		break;
	}

	return 0;
}

Object::Ptr &Asset::object(Usages usage) {
	switch (usage) {
	case RUNNING:
		return _object;
	case EDITING:
		return _editing;
	default: {
			static Object::Ptr nil;

			return nil;
		}
	}
}

Texture::Ptr Asset::texture(Usages usage) {
	switch (usage) {
	case RUNNING:
		if (_texture)
			return _texture;

		break;
	case EDITING:
		if (_painting)
			return _painting;

		break;
	default: // Do nothing.
		break;
	}

	if (!_project->renderer())
		return nullptr;

	prepare(usage, true);
	if (!object(usage))
		return nullptr;

	Image::Ptr img = Object::as<Image::Ptr>(object(usage));
	if (!img)
		return nullptr;

	const Texture::Usages usg = img->blank() ? Texture::TARGET : Texture::STATIC;
	Texture::Ptr tex(Texture::create());
	tex->fromImage(_project->renderer(), usg, img.get());
	tex->blend(Texture::BLEND);

	switch (usage) {
	case RUNNING:
		_texture = tex;

		break;
	case EDITING:
		_painting = tex;

		break;
	default: // Do nothing.
		break;
	}

	return tex;
}

Object::Ptr Asset::sound(unsigned type) {
	if (type != Sfx::TYPE() && type != Music::TYPE())
		return nullptr;

	prepare(RUNNING, true);
	if (!object(RUNNING))
		return nullptr;

	Sound::Ptr snd = Object::as<Sound::Ptr>(object(RUNNING));
	if (!snd)
		return nullptr;

	size_t len = 0;
	const Byte* buf = snd->buffer(&len);
	if (!buf || len == 0)
		return nullptr;

	switch (type) {
	case Sfx::TYPE(): {
			Sfx::Ptr ptr(Sfx::create());
			ptr->fromBytes(buf, len);

			return ptr;
		}
	case Music::TYPE(): {
			Music::Ptr ptr(Music::create());
			ptr->fromBytes(buf, len);

			return ptr;
		}
	default:
		return nullptr;
	}
}

class Editable* Asset::editor(void) const {
	return _editor;
}

bool Asset::readyFor(Usages usage) const {
	return (usage & _readyFor) != NONE;
}

bool Asset::prepare(Usages usage, bool shallow) {
	// Prepare.
	if ((usage & _readyFor) != NONE) {
		if (editor())
			editor()->flush();

		return true;
	}

	// Load object for running.
	do {
		if ((usage & RUNNING) == NONE && (usage & EDITING) == NONE)
			break;

		if (!object(RUNNING))
			load(RUNNING);

		if (editor())
			editor()->flush();
	} while (false);

	// Load object and `editor` for editing.
	do {
		if ((usage & EDITING) == NONE)
			break;

		if (!object(EDITING)) {
			Object::Ptr running = object(RUNNING);

			switch (type()) {
			case Code::TYPE(): // Fall through.
			case Json::TYPE(): // Fall through.
			case Text::TYPE():
				object(EDITING, running); // Reuse the running object for editing, since
				                          // these types of asset cannot be changed during
				                          // running.

				break;
			default:
				if (!load(EDITING)) { // Try to load dedicated editing object.
					Object* clone = nullptr;
					if (running && running->clone(&clone) && clone)
						object(EDITING, Object::Ptr(clone)); // Try to clone the running object.
					else
						object(EDITING, running); // Reuse the running object.
				}

				break;
			}
		}

		if (editor())
			editor()->flush();

		if (editor()) {
			editor()->open(_project, entry().c_str(), object(EDITING), ref().empty() ? nullptr : ref().c_str());

			break;
		}

		Editable* edit = editor();

		if (!shallow) {
			switch (type()) {
			case Palette::TYPE():
				edit = EditorPalette::create();

				break;
			case Image::TYPE():
				edit = EditorImage::create();

				break;
			case Sprite::TYPE():
				edit = EditorSprite::create();

				break;
			case Map::TYPE():
				edit = EditorMap::create();

				break;
			case Sound::TYPE():
				edit = EditorSound::create();

				break;
			case Font::TYPE():
				edit = EditorFont::create();

				break;
			case Code::TYPE():
				if (!edit)
					edit = EditorCode::create();

				break;
			case Json::TYPE():
				edit = EditorJson::create();

				break;
			case Text::TYPE():
				edit = EditorText::create();

				break;
			case Bytes::TYPE():
				if (custom())
					edit = EditorPlugin::create();
				else
					edit = EditorBytes::create();

				break;
			default:
				assert(false && "Not implemented.");

				break;
			}
		}

		if (edit) {
			if (editor())
				finish(EDITING, false);

			edit->open(_project, entry().c_str(), object(EDITING), ref().empty() ? nullptr : ref().c_str());
			editor(edit);
		}
	} while (false);

	// Finish.
	if ((usage & EDITING) != NONE && editor())
		_readyFor = (Usages)(_readyFor | EDITING);
	if ((usage & RUNNING) != NONE && object(RUNNING))
		_readyFor = (Usages)(_readyFor | RUNNING);

	return true;
}

bool Asset::finish(Usages usage, bool shallow) {
	// Prepare.
	bool result = true;
	if ((usage & _readyFor) == NONE) {
		if ((usage & EDITING) != NONE) {
			if (object(EDITING).unique())
				object(EDITING, nullptr);
		}

		return result;
	}

	// Unload object and `editor` to end editing.
	do {
		if ((usage & EDITING) == NONE)
			break;

		Editable* edit = editor();

		if (edit && !shallow) {
			switch (type()) {
			case Palette::TYPE():
				edit->close(_project);

				EditorPalette::destroy(Editable::as<EditorPalette>(edit));
				edit = nullptr;

				break;
			case Image::TYPE():
				edit->close(_project);

				EditorImage::destroy(Editable::as<EditorImage>(edit));
				edit = nullptr;

				break;
			case Sprite::TYPE():
				edit->close(_project);

				EditorSprite::destroy(Editable::as<EditorSprite>(edit));
				edit = nullptr;

				break;
			case Map::TYPE():
				edit->close(_project);

				EditorMap::destroy(Editable::as<EditorMap>(edit));
				edit = nullptr;

				break;
			case Sound::TYPE():
				edit->close(_project);

				EditorSound::destroy(Editable::as<EditorSound>(edit));
				edit = nullptr;

				break;
			case Font::TYPE():
				edit->close(_project);

				EditorFont::destroy(Editable::as<EditorFont>(edit));
				edit = nullptr;

				break;
			case Code::TYPE():
				if (revertible()) {
					edit->close(_project);

					EditorCode::destroy(Editable::as<EditorCode>(edit));
					edit = nullptr;
				} else {
					result = false;
				}

				break;
			case Json::TYPE():
				edit->close(_project);

				EditorJson::destroy(Editable::as<EditorJson>(edit));
				edit = nullptr;

				break;
			case Text::TYPE():
				edit->close(_project);

				EditorText::destroy(Editable::as<EditorText>(edit));
				edit = nullptr;

				break;
			case Bytes::TYPE():
				edit->close(_project);

				if (Editable::is<EditorPlugin>(edit)) {
					EditorPlugin::destroy(Editable::as<EditorPlugin>(edit));
				} else if (Editable::is<EditorBytes>(edit)) {
					EditorBytes::destroy(Editable::as<EditorBytes>(edit));
				} else {
					assert(false && "Impossible.");

					delete edit;
				}
				edit = nullptr;

				break;
			default:
				assert(false && "Not implemented.");

				break;
			}
		}

		editor(edit);

		if (object(EDITING).unique())
			object(EDITING, nullptr);
	} while (false);

	// Unload object to end running.
	do {
		if ((usage & RUNNING) == NONE)
			break;

		if (!object(RUNNING))
			break;

		if (exists()) {
			if (object(RUNNING).unique())
				object(RUNNING, nullptr);
		} else {
			if (object(RUNNING).unique()) {
				const char* usg = usage == RUNNING ? "running" : "editing";
				if (entry().empty())
					fprintf(stdout, "Ignored unloading unpersisted asset object for %s.\n", usg);
				else
					fprintf(stdout, "Ignored unloading unpersisted asset object for %s: \"%s\".\n", usg, entry().c_str());
			}
		}
	} while (false);

	// Finish.
	if (!object(RUNNING))
		_texture = nullptr;
	if (!object(EDITING))
		_painting = nullptr;

	if ((usage & EDITING) != NONE && !editor())
		_readyFor = (Usages)(_readyFor & (~EDITING));
	if ((usage & RUNNING) != NONE && !object(RUNNING))
		_readyFor = (Usages)(_readyFor & (~RUNNING));

	return result;
}

bool Asset::load(Usages usage, class Bytes* buf, Object::Ptr ref, bool implicit) {
	if (object(usage))
		return true;

	return reload(usage, buf, ref, implicit);
}

bool Asset::load(Usages usage) {
	if (object(usage))
		return true;

	Bytes::Ptr buf(Bytes::create());
	if (!toBytes(buf.get()))
		return false;

	buf->poke(0);

	return load(usage, buf.get(), nullptr, false);
}

bool Asset::reload(Usages usage, class Bytes* buf, Object::Ptr ref, bool implicit) {
	if (!buf)
		return false;

	switch (type()) {
	case Palette::TYPE(): {
			std::string str;
			buf->readString(str);

			Json::Ptr json(Json::create());
			if (!json->fromString(str))
				return false;

			rapidjson::Document doc;
			if (!json->toJson(doc))
				return false;

			Palette::Ptr ptr = nullptr;
			if (object(usage))
				ptr = Object::as<Palette::Ptr>(object(usage));
			if (!ptr) {
				ptr = Palette::Ptr(Palette::create(IMAGE_PALETTE_COLOR_COUNT));
				object(usage, ptr);
			}
			if (!ptr->fromJson(doc))
				return false;
		}

		return true;
	case Image::TYPE(): {
			std::string ext = extName();
			Text::toLowerCase(ext);
			if (ext.empty() || ext == BITTY_IMAGE_EXT) {
				rapidjson::Document doc;
				Palette::Ptr refPtr = nullptr;
				Asset* refAsset = nullptr;
				if (toJson(usage, buf, doc, refAsset)) {
					if (refAsset) {
						Object::Ptr objPtr = refAsset->object(usage);
						if (!objPtr)
							return false;
						if (!Object::is<Palette::Ptr>(objPtr))
							return false;
						refPtr = Object::as<Palette::Ptr>(objPtr);
						if (!refPtr)
							return false;
					}
				} else if (ref) {
					if (Object::is<Palette::Ptr>(ref))
						refPtr = Object::as<Palette::Ptr>(ref);
				} else if (!implicit) {
					return false;
				}

				Image::Ptr ptr = nullptr;
				if (object(usage))
					ptr = Object::as<Image::Ptr>(object(usage));
				if (!ptr) {
					ptr = Image::Ptr(Image::create(refPtr));
					object(usage, ptr);
				}
				if (!ptr->fromJson(doc)) {
					if (ptr->fromBytes(buf))
						return true;

					return false;
				}
			} else {
				Image::Ptr ptr = nullptr;
				if (object(usage))
					ptr = Object::as<Image::Ptr>(object(usage));
				if (!ptr) {
					ptr = Image::Ptr(Image::create(nullptr));
					object(usage, ptr);
				}
				if (!ptr->fromBytes(buf))
					return false;
			}
		}

		return true;
	case Sprite::TYPE(): {
			rapidjson::Document doc;
			Asset* refAsset = nullptr;
			Texture::Ptr texPtr = nullptr;
			if (toJson(usage, buf, doc, refAsset)) {
				if (refAsset) {
					texPtr = refAsset->texture(usage);
					if (!texPtr)
						return false;
				}
			} else if (ref) {
				if (Object::is<Texture::Ptr>(ref))
					texPtr = Object::as<Texture::Ptr>(ref);
			} else if (!implicit) {
				return false;
			}

			Sprite::Ptr ptr = nullptr;
			if (object(usage))
				ptr = Object::as<Sprite::Ptr>(object(usage));
			if (!ptr) {
				ptr = Sprite::Ptr(Sprite::create(0, 0));
				object(usage, ptr);
			}
			if (!ptr->fromJson(texPtr, doc))
				return false;
		}

		return true;
	case Map::TYPE(): {
			rapidjson::Document doc;
			Asset* refAsset = nullptr;
			Texture::Ptr texPtr = nullptr;
			if (toJson(usage, buf, doc, refAsset)) {
				if (refAsset) {
					texPtr = refAsset->texture(usage);
					if (!texPtr)
						return false;
				}
			} else if (ref) {
				if (Object::is<Texture::Ptr>(ref))
					texPtr = Object::as<Texture::Ptr>(ref);
			} else if (!implicit) {
				return false;
			}

			Map::Ptr ptr = nullptr;
			if (object(usage))
				ptr = Object::as<Map::Ptr>(object(usage));
			if (!ptr) {
				const bool batch = (_project->strategy() & Project::BATCH_MAP) != Project::NONE;;
				ptr = Map::Ptr(Map::create(nullptr, batch));
				object(usage, ptr);
			}
			if (!ptr->fromJson(texPtr, doc))
				return false;
		}

		return true;
	case Sound::TYPE(): {
			std::string str = fullPath();
			if (str.empty())
				str = entry().name();

			Sound::Ptr ptr = nullptr;
			if (object(usage))
				ptr = Object::as<Sound::Ptr>(object(usage));
			if (!ptr) {
				ptr = Sound::Ptr(Sound::create());
				object(usage, ptr);
			}
			ptr->path(str.c_str(), str.length());
			ptr->fromBytes(buf);
		}

		return true;
	case Font::TYPE(): {
			Bytes::Ptr ptr = nullptr;
			if (object(usage))
				ptr = Object::as<Bytes::Ptr>(object(usage));
			if (!ptr) {
				ptr = Bytes::Ptr(Bytes::create());
				object(usage, ptr);
			}
			ptr->writeBytes(buf->pointer(), buf->count());
		}

		return true;
	case Code::TYPE(): {
			std::string str;
			buf->readString(str);

			Code::Ptr ptr = nullptr;
			if (object(usage))
				ptr = Object::as<Code::Ptr>(object(usage));
			if (!ptr) {
				ptr = Code::Ptr(Code::create());
				object(usage, ptr);
			}
			ptr->text(str.c_str(), str.length());
		}

		return true;
	case Json::TYPE(): {
			std::string str;
			buf->readString(str);

			Json::Ptr ptr = nullptr;
			if (object(usage))
				ptr = Object::as<Json::Ptr>(object(usage));
			if (!ptr) {
				ptr = Json::Ptr(Json::create());
				object(usage, ptr);
			}
			if (!ptr->fromString(str))
				return false;
		}

		return true;
	case Text::TYPE(): {
			std::string str;
			buf->readString(str);

			Text::Ptr ptr = nullptr;
			if (object(usage))
				ptr = Object::as<Text::Ptr>(object(usage));
			if (!ptr) {
				ptr = Text::Ptr(Text::create());
				object(usage, ptr);
			}
			ptr->text(str.c_str(), str.length());
		}

		return true;
	case Bytes::TYPE(): {
			Bytes::Ptr ptr = nullptr;
			if (object(usage))
				ptr = Object::as<Bytes::Ptr>(object(usage));
			if (!ptr) {
				ptr = Bytes::Ptr(Bytes::create());
				object(usage, ptr);
			}
			ptr->writeBytes(buf->pointer(), buf->count());
		}

		return true;
	default:
		assert(false && "Not implemented.");

		return false;
	}
}

bool Asset::reload(Usages usage) {
	Bytes::Ptr buf(Bytes::create());
	toBytes(buf.get());

	buf->poke(0);

	return reload(usage, buf.get(), nullptr, false);
}

bool Asset::save(Usages usage, class Bytes* buf) {
	if (!object(usage))
		return true;

	if (!buf)
		return false;

	switch (type()) {
	case Palette::TYPE(): {
			Palette::Ptr ptr = Object::as<Palette::Ptr>(object(usage));
			if (!ptr)
				return false;

			rapidjson::Document doc;
			if (!ptr->toJson(doc))
				return false;

			Json::Ptr json(Json::create());
			if (!json->fromJson(doc))
				return false;

			std::string str;
			if (!json->toString(str))
				return false;

			buf->writeString(str);
		}

		break;
	case Image::TYPE(): {
			Image::Ptr ptr = Object::as<Image::Ptr>(object(usage));
			if (!ptr)
				return false;

			std::string ext = extName();
			Text::toLowerCase(ext);
			if (ext == BITTY_IMAGE_EXT) {
				rapidjson::Document doc;
				if (!ptr->toJson(doc))
					return false;

				if (!fromJson(usage, buf, doc))
					return false;
			} else {
				if (!ptr->toBytes(buf, ext.c_str()))
					return false;
			}
		}

		break;
	case Sprite::TYPE(): {
			Sprite::Ptr ptr = Object::as<Sprite::Ptr>(object(usage));
			if (!ptr)
				return false;

			rapidjson::Document doc;
			if (!ptr->toJson(doc))
				return false;

			if (!fromJson(usage, buf, doc))
				return false;
		}

		break;
	case Map::TYPE(): {
			Map::Ptr ptr = Object::as<Map::Ptr>(object(usage));
			if (!ptr)
				return false;

			rapidjson::Document doc;
			if (!ptr->toJson(doc))
				return false;

			if (!fromJson(usage, buf, doc))
				return false;
		}

		break;
	case Sound::TYPE(): {
			Sound::Ptr ptr = Object::as<Sound::Ptr>(object(usage));
			if (!ptr)
				return false;

			if (!ptr->toBytes(buf))
				return false;
		}

		break;
	case Font::TYPE(): {
			Bytes::Ptr ptr = Object::as<Bytes::Ptr>(object(usage));
			if (!ptr)
				return false;

			buf->writeBytes(ptr->pointer(), ptr->count());
		}

		break;
	case Code::TYPE(): {
			Code::Ptr ptr = Object::as<Code::Ptr>(object(usage));
			if (!ptr)
				return false;

			const std::string str = ptr->text(nullptr);
			buf->writeString(str);
		}

		break;
	case Json::TYPE(): {
			Json::Ptr ptr = Object::as<Json::Ptr>(object(usage));
			if (!ptr)
				return false;

			std::string str;
			if (!ptr->toString(str))
				return false;

			buf->writeString(str);
		}

		break;
	case Text::TYPE(): {
			Text::Ptr ptr = Object::as<Text::Ptr>(object(usage));
			if (!ptr)
				return false;

			const std::string str = ptr->text(nullptr);
			buf->writeString(str);
		}

		break;
	case Bytes::TYPE(): {
			Bytes::Ptr ptr = Object::as<Bytes::Ptr>(object(usage));
			if (!ptr)
				return false;

			buf->writeBytes(ptr->pointer(), ptr->count());
		}

		break;
	default:
		assert(false && "Not implemented.");

		return false;
	}

	return true;
}

bool Asset::save(Usages usage) {
	if (!object(usage))
		return true;

	Bytes::Ptr buf(Bytes::create());
	if (!save(usage, buf.get()))
		return false;

	buf->poke(0);

	if (!fromBytes(buf.get()))
		return false;

	return true;
}

bool Asset::unload(void) {
	if (editor())
		editor()->post(Editable::CLEAR_BREAKPOINTS);

	object(RUNNING, nullptr);
	object(EDITING, nullptr);

	ref("");

	custom(false);

	_texture = nullptr;
	_painting = nullptr;

	return true;
}

bool Asset::dirty(void) const {
	if (!editor())
		return false;

	return editor()->hasUnsavedChanges() || _dirty;
}

void Asset::dirty(bool val) {
	if (!editor()) {
		_dirty = val;

		return;
	}

	if (val) {
		_dirty = !editor()->hasUnsavedChanges();
	} else {
		editor()->markChangesSaved(_project);
		_dirty = false;
	}
}

std::string Asset::fullPath(void) const {
	if (!fileInfo())
		return "";

	std::string path = fileInfo()->fullPath();
	if (!directoryInfo())
		path = Path::combine(path.c_str(), entry().c_str());

	return path;
}

std::string Asset::extName(void) const {
	return extOf(entry().name());
}

bool Asset::exists(void) const {
	if (!fileInfo() || !fileInfo()->exists())
		return false;

	Archive* arch = _project->archive(Stream::READ);
	if (arch)
		return arch->exists(entry().c_str());

	return true;
}

bool Asset::make(void) {
	if (!fileInfo())
		return false;

	if (!fileInfo()->exists()) {
		if (!fileInfo()->make())
			return false;
	}

	Archive* arch = _project->archive(Stream::APPEND);
	if (arch) {
		if (!arch->make(entry().c_str()))
			return false;
	}

	return true;
}

bool Asset::remove(void) {
	if (!fileInfo())
		return true;

	Archive* arch = _project->archive(Stream::READ_WRITE);
	if (arch && arch->removable()) {
		if (arch->remove(entry().c_str())) // Removed.
			return true;

		return true; // Doesn't exist.
	}

	arch = _project->archive(Stream::READ);
	if (arch) {
		if (!arch->exists(entry().c_str()))
			return true; // Doesn't exist.

		Text::Array entries;
		if (!arch->all(entries))
			return false;

		std::map<std::string, Bytes::Ptr> cache;
		for (const std::string &ent : entries) {
			if (ent == entry().name()) // Filter to remove.
				continue;

			Bytes::Ptr buf(Bytes::create());
			arch->toBytes(buf.get(), ent.c_str());
			cache[ent] = buf;
		}

		arch = _project->archive(Stream::WRITE);
		if (!arch)
			return false;
		for (auto kv : cache) {
			const std::string &ent = kv.first;
			Bytes::Ptr buf = kv.second;
			arch->fromBytes(buf.get(), ent.c_str());
		}

		return true;
	} else {
		if (!fileInfo()->remove(true))
			return false;

		cleanup();

		return true;
	}
}

bool Asset::rename(const char* newNameExt) {
	if (!fileInfo()) {
		entry(newNameExt);

		return true;
	}

	Archive* arch = _project->archive(Stream::READ);
	if (arch) {
		if (!fileInfo()->exists())
			return false;

		Bytes::Ptr buf(Bytes::create());
		bool saved = arch->toBytes(buf.get(), entry().c_str());
		if (!saved)
			saved = object(EDITING) && save(EDITING, buf.get());
		if (!saved)
			return false;

		if (!remove())
			return false;

		arch = _project->archive(Stream::APPEND);
		if (!arch)
			return false;
		if (!arch->fromBytes(buf.get(), newNameExt))
			return false;

		entry(newNameExt);

		return true;
	} else {
		if (!fileInfo()->exists()) {
			std::string path = fileInfo()->parentPath();
			path = Path::combine(path.c_str(), newNameExt);
			FileInfo::destroy(fileInfo());
			fileInfo(FileInfo::create(path.c_str()));

			entry(newNameExt);

			return true;
		}

		Entry newEntry(newNameExt);
		std::string newPath = directoryInfo()->fullPath();
		newPath = Path::combine(newPath.c_str(), newEntry.c_str());
		if (newEntry.parts().size() > 1) {
			FileInfo::Ptr fileInfo(FileInfo::make(newPath.c_str()));
			const std::string dirPath = fileInfo->parentPath();
			if (!Path::existsDirectory(dirPath.c_str()))
				Path::touchDirectory(dirPath.c_str());
		}

		if (!fileInfo()->moveTo(newPath.c_str()))
			return false;

		cleanup();

		entry(newNameExt);

		return true;
	}
}

bool Asset::toBytes(class Bytes* buf) const {
	buf->clear();

	if (!fileInfo() || !fileInfo()->exists())
		return false;

	Archive* arch = _project->archive(Stream::READ);
	if (arch) {
		if (!arch->toBytes(buf, entry().c_str()))
			return false;
	} else {
		File::Ptr file(File::create());
		const std::string path = fullPath();
		if (!file->open(path.c_str(), Stream::READ))
			return false;
		file->readBytes(buf);
		file->close();
	}

	if (_project->loader()) {
		if (!_project->loader()->decode(_project, this, buf))
			return false;
	}

	return true;
}

bool Asset::fromBytes(class Bytes* buf) {
	if (_project->loader()) {
		if (!_project->loader()->encode(_project, this, buf))
			return false;
	}

	if (_project->archived())
		remove();

	Archive* arch = _project->archive(Stream::APPEND);
	if (arch) {
		if (!arch->fromBytes(buf, entry().c_str()))
			return false;
	} else {
		const std::string path = fullPath();
		if (entry().parts().size() > 1) {
			FileInfo::Ptr fileInfo(FileInfo::make(path.c_str()));
			const std::string dirPath = fileInfo->parentPath();
			if (!Path::existsDirectory(dirPath.c_str()))
				Path::touchDirectory(dirPath.c_str());
		}

		File::Ptr file(File::create());
		if (!file->open(path.c_str(), Stream::WRITE))
			return false;
		file->writeBytes(buf);
		file->close();
	}

	return true;
}

bool Asset::toJson(Usages usage, class Bytes* buf, rapidjson::Document &doc, Asset* &refPtr) {
	if (_project->loader()) {
		if (!_project->loader()->decode(_project, this, buf))
			return false;
	}

	refPtr = nullptr;

	std::string str;
	buf->readString(str);

	Json::Ptr json(Json::create());
	if (!json->fromString(str)) {
		ref().clear();

		return false;
	}
	if (!json->toJson(doc)) {
		ref().clear();

		return false;
	}

	std::string refStr;
	if (!Jpath::get(doc, refStr, ASSET_REF_NAME)) {
		ref().clear();

		return false;
	}
	Asset* refAsset = _project->get(refStr.c_str());
	if (!ref().empty() && ref() != refStr) { // Resolve.
		refAsset = _project->get(ref().c_str());
		if (refAsset)
			refStr = ref();
	}
	ref(refStr);
	if (!refAsset)
		return false;
	if (!refAsset->load(usage))
		return false;
	refPtr = refAsset;

	return true;
}

bool Asset::fromJson(Usages, class Bytes* buf, rapidjson::Document &doc) const {
	buf->clear();

	if (!ref().empty()) {
		rapidjson::Value::MemberIterator exists = doc.FindMember(ASSET_REF_NAME);
		if (exists != doc.MemberEnd())
			doc.EraseMember(exists);

		rapidjson::Value jstrref, jvalref;
		jstrref.SetString(ASSET_REF_NAME, doc.GetAllocator());
		jvalref.SetString(ref().c_str(), doc.GetAllocator());
		doc.AddMember(jstrref, jvalref, doc.GetAllocator());
	}

	Json::Ptr json(Json::create());
	if (!json->fromJson(doc))
		return false;

	std::string str;
	if (!json->toString(str))
		return false;

	buf->writeString(str);

	if (_project->loader()) {
		if (!_project->loader()->encode(_project, this, buf))
			return false;
	}

	return true;
}

Object::Ptr Asset::fromBlank(Usages usage, const class Project* project, unsigned type, IDictionary::Ptr options) {
	Object::Ptr obj = nullptr;
	switch (type) {
	case Palette::TYPE(): {
			Palette::Ptr ptr(Palette::create(IMAGE_PALETTE_COLOR_COUNT));
			obj = ptr;
		}

		break;
	case Image::TYPE():
		if (options) {
			const Int width = (Int)options->get("width");
			const Int height = (Int)options->get("height");
			const std::string refStr = (std::string)options->get(ASSET_REF_NAME);
			Palette::Ptr palette = nullptr;
			do {
				LockGuard<RecursiveMutex>::UniquePtr acquired;
				Project* prj = project->acquire(acquired);
				if (!prj)
					break;

				Asset* refAsset = prj->get(refStr.c_str());
				if (!refAsset)
					break;
				if (!refAsset->load(usage))
					break;
				Object::Ptr refObj = refAsset->object(usage);
				if (!refObj || !Object::is<Palette::Ptr>(refObj))
					break;
				palette = Object::as<Palette::Ptr>(refObj);
			} while (false);

			Image::Ptr ptr = Image::Ptr(Image::create(palette));
			ptr->fromBlank(width, height, palette ? palette->count() : 0);
			obj = ptr;
		} else {
			Image::Ptr ptr(Image::create(nullptr));
			obj = ptr;
		}

		break;
	case Sprite::TYPE():
		if (options) {
			const Int width = (Int)options->get("width");
			const Int height = (Int)options->get("height");
			Sprite::Ptr ptr(Sprite::create(width, height));
			obj = ptr;
		} else {
			Sprite::Ptr ptr(Sprite::create(BITTY_SPRITE_DEFAULT_WIDTH, BITTY_SPRITE_DEFAULT_HEIGHT));
			obj = ptr;
		}

		break;
	case Map::TYPE():
		if (options) {
			const Int width = (Int)options->get("width");
			const Int height = (Int)options->get("height");
			Int tileWidth = BITTY_MAP_TILE_DEFAULT_SIZE;
			Int tileHeight = BITTY_MAP_TILE_DEFAULT_SIZE;
			do {
				if (!options->contains("tiles"))
					break;
				Object::Ptr objTiles = (Object::Ptr)options->get("tiles");
				if (!objTiles || !Object::is<IDictionary::Ptr>(objTiles))
					break;
				IDictionary::Ptr dictTiles = Object::as<IDictionary::Ptr>(objTiles);
				if (!dictTiles->contains("count"))
					break;
				Object::Ptr objCount = (Object::Ptr)dictTiles->get("count");
				if (!objCount || !Object::is<IList::Ptr>(objCount))
					break;
				IList::Ptr lstCount = Object::as<IList::Ptr>(objCount);
				if (lstCount->count() < 2)
					break;
				tileWidth = (Int)lstCount->at(0);
				tileHeight = (Int)lstCount->at(1);
			} while (false);
			const std::string refStr = (std::string)options->get(ASSET_REF_NAME);
			Texture::Ptr texPtr = nullptr;
			bool batch = false;
			do {
				LockGuard<RecursiveMutex>::UniquePtr acquired;
				Project* prj = project->acquire(acquired);
				if (!prj)
					break;

				batch = (prj->strategy() & Project::BATCH_MAP) != Project::NONE;

				Asset* refAsset = prj->get(refStr.c_str());
				if (!refAsset)
					break;
				if (!refAsset->load(usage))
					break;
				texPtr = refAsset->texture(usage);
			} while (false);

			Map::Tiles tiles{
				texPtr,
				Math::Vec2i(texPtr->width() / tileWidth, texPtr->height() / tileHeight)
			};
			static_assert(sizeof(int) == sizeof(Int32), "Wrong type size.");
			Bytes::Ptr cels(Bytes::create());
			for (Int i = 0; i < width * height; ++i)
				cels->writeInt32(0);
			Map::Ptr ptr(Map::create(&tiles, batch));
			ptr->load((int*)cels->pointer(), width, height);
			obj = ptr;
		} else {
			bool batch = false;
			do {
				LockGuard<RecursiveMutex>::UniquePtr acquired;
				Project* prj = project->acquire(acquired);
				if (!prj)
					break;

				batch = (prj->strategy() & Project::BATCH_MAP) != Project::NONE;
			} while (false);

			Map::Ptr ptr(Map::create(nullptr, batch));
			obj = ptr;
		}

		break;
	case Font::TYPE(): {
			Bytes::Ptr ptr(Bytes::create());
			obj = ptr;
		}

		break;
	case Code::TYPE(): {
			Code::Ptr ptr(Code::create());
			obj = ptr;
		}

		break;
	case Json::TYPE(): {
			Json::Ptr ptr(Json::create());
			obj = ptr;
		}

		break;
	case Text::TYPE(): {
			Text::Ptr ptr(Text::create());
			obj = ptr;
		}

		break;
	case Bytes::TYPE(): {
			Bytes::Ptr ptr(Bytes::create());
			obj = ptr;
		}

		break;
	default:
		assert(false && "Not implemented.");

		break;
	}

	return obj;
}

std::string Asset::extOf(const std::string &path) {
	const std::string &partial = path;
	const size_t pos = Text::lastIndexOf(partial, '.');
	if (pos == std::string::npos)
		return "";

	return partial.substr(pos + 1);
}

unsigned Asset::typeOf(const std::string &ext, bool alowBytes) {
	auto match = [] (const std::string &ext, const std::string &pattern) -> bool {
		if (pattern.empty())
			return false;

		return Text::endsWith(ext, pattern, true) &&
			(ext.length() == pattern.length() ||
				(ext.length() >= pattern.length() + 1 && ext[ext.length() - pattern.length() - 1] == '.')
			);
	};

	if (ext.empty())
		return alowBytes ? Bytes::TYPE() : 0;

	if (match(ext, BITTY_PALETTE_EXT))
		return Palette::TYPE();
	else if (match(ext, BITTY_IMAGE_EXT) || match(ext, "png") || match(ext, "jpg") || match(ext, "bmp") || match(ext, "tga"))
		return Image::TYPE();
	else if (match(ext, BITTY_SPRITE_EXT))
		return Sprite::TYPE();
	else if (match(ext, BITTY_MAP_EXT))
		return Map::TYPE();
	else if (match(ext, "mp3") || match(ext, "ogg") || match(ext, "wav") || match(ext, "mid"))
		return Sound::TYPE();
	else if (match(ext, BITTY_FONT_EXT))
		return Font::TYPE();
	else if (match(ext, BITTY_LUA_EXT))
		return Code::TYPE();
	else if (match(ext, BITTY_JSON_EXT))
		return Json::TYPE();
	else if (match(ext, BITTY_TEXT_EXT))
		return Text::TYPE();

	return alowBytes ? Bytes::TYPE() : 0;
}

unsigned Asset::inferencedTypeOf(const std::string &content) {
	if (content.empty())
		return 0;

	rapidjson::Document doc;
	if (!Json::fromString(doc, content.c_str()))
		return Text::TYPE();

	if (Jpath::has(doc, "tiles", "count") && Jpath::has(doc, "width") && Jpath::has(doc, "height") && Jpath::has(doc, ASSET_REF_NAME))
		return Map::TYPE();
	else if (Jpath::has(doc, "count") && Jpath::has(doc, "data") && Jpath::has(doc, ASSET_REF_NAME))
		return Sprite::TYPE();
	else if (Jpath::has(doc, "width") && Jpath::has(doc, "height"))
		return Image::TYPE();
	else if (Jpath::has(doc, "count") && Jpath::has(doc, "data"))
		return Palette::TYPE();

	return Json::TYPE();
}

int Asset::compare(const Asset* left, const Asset* right) {
	if (left->type() == Map::TYPE() && right->type() != Map::TYPE())
		return -1;
	else if (left->type() != Map::TYPE() && right->type() == Map::TYPE())
		return 1;

	if (left->type() == Sprite::TYPE() && right->type() != Sprite::TYPE())
		return -1;
	else if (left->type() != Sprite::TYPE() && right->type() == Sprite::TYPE())
		return 1;

	if (left->type() == Image::TYPE() && right->type() != Image::TYPE())
		return -1;
	else if (left->type() != Image::TYPE() && right->type() == Image::TYPE())
		return 1;

	if (left->type() == Palette::TYPE() && right->type() != Palette::TYPE())
		return -1;
	else if (left->type() != Palette::TYPE() && right->type() == Palette::TYPE())
		return 1;

	return Entry::compare(left->entry(), right->entry());
}

bool Asset::connect(const char* package, const char* ent) {
	if (!package || !ent)
		return false;

	if (Path::existsDirectory(package)) { // Is a directory.
		const std::string full = Path::combine(package, ent);

		directoryInfo(DirectoryInfo::create(package));
		fileInfo(FileInfo::create(full.c_str()));
	} else { // Is an archive.
		fileInfo(FileInfo::create(package));
	}

	return true;
}

void Asset::cleanup(void) {
	if (!directoryInfo())
		return;

	auto clean = [] (const char* root, const Text::Array &parts) -> void {
		std::string path = root;
		for (const std::string &part : parts)
			path = Path::combine(path.c_str(), part.c_str());
		DirectoryInfo::Ptr dirInfo(DirectoryInfo::make(path.c_str()));
		if (!dirInfo->exists())
			return;
		FileInfos::Ptr fileInfos = dirInfo->getFiles("*;*.*", true);
		DirectoryInfos::Ptr dirInfos = dirInfo->getDirectories(true);
		if (fileInfos->count() > 0 || dirInfos->count() > 0)
			return;

		dirInfo->remove(false);
	};
	Text::Array parts = entry().parts();
	parts.pop_back();
	while (!parts.empty()) {
		clean(directoryInfo()->fullPath().c_str(), parts);
		parts.pop_back();
	}
}

void Asset::editor(class Editable* editor) {
	_editor = editor;
}

void Asset::editor(std::nullptr_t) {
	_editor = nullptr;
}

void Asset::object(Usages usage, const Object::Ptr &obj) {
	if (!obj) {
		object(usage, nullptr);

		return;
	}

	switch (usage) {
	case RUNNING:
		_object = obj;

		break;
	case EDITING:
		_editing = obj;

		break;
	default: // Do nothing.
		break;
	}

#if defined BITTY_DEBUG
	const char* usg = usage == RUNNING ? "running" : "editing";
	const Object::Ptr &ptr = usage == RUNNING ? _object : _editing;
	const long refs = ptr.use_count();

	if (entry().empty())
		fprintf(stdout, "Asset object(%ld) loaded for %s.\n", refs, usg);
	else
		fprintf(stdout, "Asset object(%ld) loaded for %s: \"%s\".\n", refs, usg, entry().c_str());
#endif /* BITTY_DEBUG */
}

void Asset::object(Usages usage, std::nullptr_t) {
#if defined BITTY_DEBUG
	const char* usg = usage == RUNNING ? "running" : "editing";
	const Object::Ptr &ptr = usage == RUNNING ? _object : _editing;
	const long refs = ptr ? ptr.use_count() - 1 : -1;

	if (refs >= 0) {
		if (entry().empty())
			fprintf(stdout, "Asset object(%ld) unloaded for %s.\n", refs, usg);
		else
			fprintf(stdout, "Asset object(%ld) unloaded for %s: \"%s\".\n", refs, usg, entry().c_str());
	}
#endif /* BITTY_DEBUG */

	switch (usage) {
	case RUNNING:
		_object = nullptr;

		break;
	case EDITING:
		_editing = nullptr;

		break;
	default: // Do nothing.
		break;
	}
}

/* ===========================================================================} */
