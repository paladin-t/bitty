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
#include "bytes.h"
#include "datetime.h"
#include "editable.h"
#include "filesystem.h"
#include "image.h"
#include "map.h"
#include "platform.h"
#include "project.h"
#include "sprite.h"
#include "../lib/jpath/jpath.hpp"

/*
** {===========================================================================
** Project
*/

Project::Factory::Factory() {
}

Project::Factory::Factory(Asset::Creator create_, Asset::Destroyer destroy_) : create(create_), destroy(destroy_) {
}

Project::Factory::Factory(const Factory &other) : create(other.create), destroy(other.destroy) {
}

Project::Factory &Project::Factory::operator = (const Factory &other) {
	create = other.create;
	destroy = other.destroy;

	return *this;
}

Project::Project() {
	language(BITTY_LUA_EXT);
	preference(Archive::TXT);
	ignoreDotFiles(true);
	strategy(NONE);
	readonly(false);

	id(0);
	title("...");
	order(0);

	const std::string ent = entry();
	_assets = Asset::List(
		[ent] (const Asset* left, const Asset* right) -> int {
			return Entry::compare(left->entry().parts(), right->entry().parts(), &ent);
		},
		nullptr
	);
}

Project::~Project() {
	close();
}

Project* Project::acquire(LockGuard<RecursiveMutex>::UniquePtr &guard) const {
	guard = LockGuard<RecursiveMutex>::UniquePtr(new LockGuard<RecursiveMutex>(_lock));

	return const_cast<Project*>(this);
}

bool Project::open(class Renderer* rnd) {
	if (_opened)
		return false;
	_opened = true;

	renderer(rnd);

	fprintf(stdout, "Project opened.\n");

	return true;
}

bool Project::close(void) {
	if (!_opened)
		return false;
	_opened = false;

	Asset::List::Collection all = _assets.all();
	for (Asset* asset : all) {
		_assets.remove(asset);

		Editable* editor = asset->editor();
		if (editor)
			editor->post(Editable::CLEAR_BREAKPOINTS);

		asset->finish((Asset::Usages)(Asset::RUNNING | Asset::EDITING), false);

		factory().destroy(asset);
	}
	_assets.clear();
	archive(nullptr);

	_dirty = false;

	renderer(nullptr);

	if (title().empty())
		fprintf(stdout, "Project closed.\n");
	else
		fprintf(stdout, "Project closed: \"%s\".\n", title().c_str());

	return true;
}

void Project::language(const std::string &lang) {
	_language = lang;
	entry(PROJECT_ENTRY_NAME "." + _language);
}

Text::Array Project::strategies(void) {
	Text::Array result;
	if (strategy() != NONE) {
		if ((strategy() & BATCH_MAP) != NONE)
			result.push_back("batch_map");
	}

	return result;
}

int Project::cleanup(Asset::Usages usage) {
	int result = 0;

	std::list<Asset*> all;
	_assets.foreach(
		[&all] (Asset* &asset, int) -> void {
			all.insert(
				std::upper_bound(
					all.begin(), all.end(),
					asset,
					[] (const Asset* left, const Asset* right) -> bool {
						return Asset::compare(left, right) < 0;
					}
				),
				asset
			);
		}
	);
	for (Asset* asset : all) {
		if (asset->finish(usage, true))
			++result;
	}

	return result;
}

bool Project::load(const char* strpath) {
	archive(nullptr);

	Asset::List::Collection all = _assets.all();
	for (Asset* asset : all) {
		_assets.remove(asset);

		asset->unload();

		asset->finish((Asset::Usages)(Asset::RUNNING | Asset::EDITING), false);

		factory().destroy(asset);
	}
	_assets.clear();

	path(strpath);

	if (Path::existsDirectory(path().c_str())) {
		DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(path().c_str());
		if (!dirInfo->exists())
			return false;

		FileInfos::Ptr files = dirInfo->getFiles("*;*.*", true, ignoreDotFiles());
		for (int i = 0; i < files->count(); ++i) {
			FileInfo::Ptr fileInfo = files->get(i);

			const std::string package = dirInfo->fullPath();
			std::string entry = fileInfo->fullPath();
			entry = entry.substr(package.length() + 1);

			Asset* asset = factory().create(this);
			asset->link(package.c_str(), entry.c_str());
			add(asset);
		}
	} else {
		Archive* arch = archive(Stream::READ);
		if (!arch)
			return false;

		Text::Array entries;
		if (!arch->all(entries))
			return false;
		for (const std::string &entry : entries) {
			const std::string package = path();

			Asset* asset = factory().create(this);
			asset->link(package.c_str(), entry.c_str());
			add(asset);
		}
	}

	parse();

	return true;
}

bool Project::save(const char* path_, bool redirect, ErrorHandler error) {
	parse();
	serialize();

	Asset* asset = info();
	if (asset)
		asset->prepare(Asset::EDITING, true);

	std::map<std::string, Bytes::Ptr> cache;
	_assets.foreach(
		[&] (Asset* &asset_, int) -> void {
			if (asset_->object(Asset::EDITING))
				return;

			const std::string entry = asset_->entry().name();
			Bytes::Ptr buf(Bytes::create());
			bool saved = asset_->toBytes(buf.get());
			if (!saved) // Try to get from asset object, this happens when an asset has not been yet persisted.
				saved = asset_->object(Asset::EDITING) && asset_->save(Asset::EDITING, buf.get());
			if (!saved)
				saved = asset_->object(Asset::RUNNING) && asset_->save(Asset::RUNNING, buf.get());
			if (!saved) {
				if (error) {
					std::string msg = "Cannot save to: ";
					msg += entry;
					msg += ", due to unsolved ref or corrupt file.";
					error(msg.c_str());
				} else {
					fprintf(stderr, "Cannot save to: %s, due to unsolved ref or corrupt file.\n", entry.c_str());
				}
			}
			cache.insert(std::make_pair(entry, buf));

			Platform::idle();
		}
	);

	const bool changed = path() != path_;

	if (path().empty() || redirect)
		path(path_);

	archive(nullptr);

	Archive* arch = archive(Stream::WRITE); // Overwrite for a new archive.
	(void)arch;
	_assets.foreach(
		[&] (Asset* &asset_, int) -> void {
			Editable* editor = asset_->editor();
			if (editor)
				editor->flush();

			const std::string entry = asset_->entry().name();
			auto kv = cache.find(entry);
			if (kv != cache.end()) {
				Bytes::Ptr buf = kv->second;
				if (changed)
					asset_->link(path_, entry.c_str());
				asset_->fromBytes(buf.get());

				Platform::idle();

				return;
			}

			if (changed)
				asset_->link(path_, entry.c_str());
			asset_->save(Asset::EDITING);

			Platform::idle();
		}
	);
	cache.clear();

	if (asset)
		asset->finish(Asset::EDITING, true);

	if (changed && redirect)
		return load(path_);

	return true;
}

int Project::unload(void) {
	strategy(NONE);
	path().clear();

	id(0);
	description().clear();
	author().clear();
	version().clear();
	genre().clear();
	url().clear();
	order(0);

	archive(nullptr);

	_dirty = false;

	int result = 0;
	Asset::List::Collection all = _assets.all();
	for (Asset* asset : all) {
		_assets.remove(asset);

		if (asset->unload())
			++result;

		asset->finish((Asset::Usages)(Asset::RUNNING | Asset::EDITING), false);

		factory().destroy(asset);
	}
	_assets.clear();

	return result;
}

bool Project::parse(void) {
	strategy(NONE);

	id(0);
	title("...");
	description().clear();
	author().clear();
	version().clear();
	genre().clear();
	url().clear();
	order(0);

	Asset* asset = info();
	if (!asset)
		return false;
	if (!asset->prepare(Asset::RUNNING, true))
		return false;
	if (!asset->object(Asset::RUNNING))
		return false;
	Json::Ptr json = Object::as<Json::Ptr>(asset->object(Asset::RUNNING));
	if (!json)
		return false;

	rapidjson::Document doc;
	if (!json->toJson(doc))
		return false;
	Jpath::get(doc, id(), "id");
	Jpath::get(doc, title(), "title");
	Jpath::get(doc, description(), "description");
	Jpath::get(doc, author(), "author");
	Jpath::get(doc, version(), "version");
	Jpath::get(doc, genre(), "genre");
	Jpath::get(doc, url(), "url");
	Jpath::get(doc, order(), "order");
	Text::Array strategies;
	if (Jpath::get(doc, strategies, "strategies")) { // String constants.
		for (const std::string &s : strategies) {
			if (s == "batch_map")
				strategy((Strategies)(strategy() | BATCH_MAP));
		}
	}

	return true;
}

bool Project::serialize(void) {
	Asset* asset = info();
	if (!asset) {
		asset = factory().create(this);
		asset->link(Json::Ptr(Json::create()), PROJECT_INFO_NAME "." BITTY_JSON_EXT);
		add(asset);
	}
	if (!asset->object(Asset::RUNNING))
		return false;
	Json::Ptr json = Object::as<Json::Ptr>(asset->object(Asset::RUNNING));
	if (!json)
		return false;

	rapidjson::Document doc;
	Jpath::set(doc, doc, id(), "id");
	Jpath::set(doc, doc, title(), "title");
	Jpath::set(doc, doc, description(), "description");
	Jpath::set(doc, doc, author(), "author");
	Jpath::set(doc, doc, version(), "version");
	Jpath::set(doc, doc, genre(), "genre");
	Jpath::set(doc, doc, url(), "url");
	if (order() != 0)
		Jpath::set(doc, doc, order(), "order");
	if (strategy() != NONE) {
		Text::Array strategies;
		if ((strategy() & BATCH_MAP) != NONE)
			strategies.push_back("batch_map");
		if (!strategies.empty())
			Jpath::set(doc, doc, strategies, "strategies");
	}
	if (!json->fromJson(doc))
		return false;

	return true;
}

bool Project::dirty(void) {
	if (_dirty)
		return true;

	for (Asset::List::ConstIterator it = _assets.begin(); it != _assets.end(); ++it) {
		const Asset* asset = *it;
		if (asset->dirty())
			return true;
	}

	return false;
}

void Project::dirty(bool val) {
	_dirty = val;

	if (!val) {
		for (Asset::List::ConstIterator it = _assets.begin(); it != _assets.end(); ++it) {
			Asset* asset = *it;
			asset->dirty(false);
		}
	}
}

bool Project::archived(void) {
	if (path().empty())
		return true;

	if (Path::existsDirectory(path().c_str())) // Is not an archive, but a directory.
		return false;

	return true;
}

class Archive* Project::archive(Stream::Accesses access) {
	if (_archive) {
		if (_archive->accessibility() != access) {
			_archive->close();
			_archive->open(path().c_str(), access);
		}

		return _archive;
	}

	if (Path::existsDirectory(path().c_str())) // Is not an archive, but a directory.
		return nullptr;

	const bool forWriting = access == Stream::WRITE || access == Stream::APPEND || access == Stream::READ_WRITE;
	Archive::Formats type = (Archive::Formats)preference();
	if (!forWriting) {
		if (Path::existsFile(path().c_str()))
			type = Archive::formatOf(path().c_str()); // Infer archive type for reading an existing file.
	}

	_archive = Archive::create(type);
	_archive->open(path().c_str(), access);

	return _archive;
}

void Project::archive(std::nullptr_t) {
	if (_archive) {
		_archive->close();

		Archive::destroy(_archive);
		_archive = nullptr;
	}
}

Asset* Project::info(void) {
	return get(PROJECT_INFO_NAME "." BITTY_JSON_EXT);
}

Asset* Project::main(void) {
	if (_assets.empty())
		return nullptr;

	return _assets.front();
}

Asset* Project::bringToFront(Asset* asset) {
	if (!asset)
		return nullptr;

	if (_assets.second.empty())
		return nullptr;

	if (asset != _assets.second.front()) {
		Asset::List::SecondCollection::iterator it = std::find(_assets.second.begin(), _assets.second.end(), asset);
		if (it == _assets.second.end())
			return nullptr;

		_assets.second.erase(it);
		_assets.second.push_front(asset);
	}

	return asset;
}

int Project::count(void) {
	return _assets.count();
}

bool Project::empty(void) {
	return _assets.empty();
}

Asset* Project::get(const char* entry_) {
	if (!entry_)
		return nullptr;

	const std::string ent = entry();
	const Entry key = Entry(entry_);
	Asset::List::ConstIterator it = _assets.get<Text::Array>(
		key.parts(),
		[ent] (const Asset* const &asset, const Text::Array &entry) -> int {
			return Entry::compare(asset->entry().parts(), entry, &ent);
		}
	);
	if (it != _assets.end()) {
		Asset* asset = *it;

		return asset;
	}

	return nullptr;
}

Asset* Project::get(Asset::List::Index index) {
	if (index < 0 || index >= _assets.count())
		return nullptr;

	if (index.second())
		return _assets.second[index];
	else
		return _assets.first[index];
}

bool Project::add(Asset* asset) {
	return _assets.add(asset);
}

bool Project::remove(Asset* asset) {
	if (!_assets.remove(asset))
		return false;

	Editable* editor = asset->editor();
	if (editor)
		editor->post(Editable::CLEAR_BREAKPOINTS);

	asset->finish((Asset::Usages)(Asset::RUNNING | Asset::EDITING), false);

	factory().destroy(asset);

	return true;
}

bool Project::remove(Asset::List::Index index) {
	Asset* asset = get(index);
	if (!asset)
		return false;

	return remove(asset);
}

Asset::List::Index Project::indexOf(Asset* asset, bool second) {
	return _assets.indexOf(asset, second);
}

bool Project::iterating(void) {
	return _iterating > 0;
}

int Project::foreach(Asset::List::Enumerator enumerator, bool second) {
	if (second) {
		++_iterating;
		int result = 0;
		Asset::List::Index i(0, true);
		for (Asset::List::Collection::iterator it = _assets.second.begin(); it != _assets.second.end(); ++it) {
			Asset* asset = *it;
			enumerator(asset, i++);
			++result;
		}
		--_iterating;

		return result;
	}

	++_iterating;
	const int result = _assets.foreach(enumerator);
	--_iterating;

	return result;
}

void Project::sort(void) {
	_assets.sort();
}

/* ===========================================================================} */
