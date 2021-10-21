/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __PROJECT_H__
#define __PROJECT_H__

#include "bitty.h"
#include "asset.h"
#include "plus.h"
#include "stream.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef PROJECT_INFO_NAME
#	define PROJECT_INFO_NAME "info"
#endif /* PROJECT_INFO_NAME */
#ifndef PROJECT_ENTRY_NAME
#	define PROJECT_ENTRY_NAME "main"
#endif /* PROJECT_ENTRY_NAME */

/* ===========================================================================} */

/*
** {===========================================================================
** Project
*/

/**
 * @brief Project entity.
 */
class Project : public NonCopyable {
public:
	struct Factory {
		Asset::Creator create = nullptr;
		Asset::Destroyer destroy = nullptr;

		Factory();
		Factory(Asset::Creator create, Asset::Destroyer destroy);
		Factory(const Factory &other);

		Factory &operator = (const Factory &other);
	};

	enum Strategies {
		NONE = 0,
		BATCH_MAP = 1 << 0
	};

	typedef std::function<void(const char*)> ErrorHandler;

public:
	BITTY_PROPERTY_PTR(class Renderer, renderer) // Foreign.
	BITTY_PROPERTY_PTR(class Loader, loader) // Foreign.
	BITTY_PROPERTY(Factory, factory)

	BITTY_FIELD(std::string, language)
	BITTY_PROPERTY(unsigned, preference)
	BITTY_PROPERTY(bool, ignoreDotFiles)
	BITTY_PROPERTY(Strategies, strategy)
	BITTY_PROPERTY(bool, readonly)
	BITTY_PROPERTY(std::string, path)
	BITTY_PROPERTY(std::string, entry)

	BITTY_PROPERTY(UInt64, id)
	BITTY_PROPERTY(std::string, title)
	BITTY_PROPERTY(std::string, description)
	BITTY_PROPERTY(std::string, author)
	BITTY_PROPERTY(std::string, version)
	BITTY_PROPERTY(std::string, genre)
	BITTY_PROPERTY(std::string, url)
	BITTY_PROPERTY(unsigned, order)

private:
	bool _opened = false;
	bool _dirty = false;

	class Archive* _archive = nullptr;
	Asset::List _assets; // Dual lists for ordered by asset name and editing orders respectively.
	int _iterating = 0;

	mutable RecursiveMutex _lock;

public:
	Project();
	~Project();

	/**
	 * @brief Acquires write access to the project.
	 *
	 * @param[out] guard The guard which holds the access right, gives back on disposing.
	 * @return The project pointer with write access.
	 */
	Project* acquire(LockGuard<RecursiveMutex>::UniquePtr &guard) const;

	/**
	 * @brief Opens the project for further operation.
	 */
	bool open(class Renderer* rnd);
	/**
	 * @brief Closes the project after all operations.
	 */
	bool close(void);

	/**
	 * @brief Sets the programming language of the project.
	 */
	void language(const std::string &lang);
	/**
	 * @brief Gets the effective strategies.
	 */
	Text::Array strategies(void);

	/**
	 * @brief Cleans up the project for a specific usage.
	 */
	int cleanup(Asset::Usages usage);

	/**
	 * @brief Loads project data from a specific path.
	 */
	bool load(const char* path);
	/**
	 * @brief Saves project data to a specific path.
	 */
	bool save(const char* path, bool redirect, ErrorHandler error = nullptr);
	/**
	 * @brief Unloads project data.
	 */
	int unload(void);

	/**
	 * @brief Parses the meta information of the project.
	 */
	bool parse(void);
	/**
	 * @brief Serializes the meta information of the project.
	 */
	bool serialize(void);

	/**
	 * @brief Gets whether the project contains unsaved modifications.
	 */
	bool dirty(void);
	/**
	 * @brief Sets whether the project contains unsaved modifications.
	 */
	void dirty(bool val);

	/**
	 * @brief Gets whether the project is archived (or directory-based).
	 */
	bool archived(void);
	/**
	 * @brief Gets the archive for a specific access.
	 *
	 * @return The archive pointer, or `nullptr` if it's directory-based.
	 */
	class Archive* archive(Stream::Accesses access);
	/**
	 * @brief Disposes the archive.
	 */
	void archive(std::nullptr_t);

	/**
	 * @brief Gets the meta information asset.
	 */
	Asset* info(void);
	/**
	 * @brief Gets the main entry asset.
	 */
	Asset* main(void);

	/**
	 * @brief Brings a specific asset to front for editing.
	 */
	Asset* bringToFront(Asset* asset);

	/**
	 * @brief Gets the asset count.
	 */
	int count(void);
	/**
	 * @brief Gets whether the project is empty.
	 */
	bool empty(void);
	/**
	 * @brief Gets an asset with a specific entry name.
	 */
	Asset* get(const char* entry);
	/**
	 * @brief Gets an asset with a specific index.
	 */
	Asset* get(Asset::List::Index index);
	/**
	 * @brief Adds an asset to the project.
	 */
	bool add(Asset* asset);
	/**
	 * @brief Removes an asset from the project.
	 */
	bool remove(Asset* asset);
	/**
	 * @brief Removes an asset from the project by index.
	 */
	bool remove(Asset::List::Index index);
	/**
	 * @brief Gets the index of a specific asset.
	 */
	Asset::List::Index indexOf(Asset* asset, bool second = false);
	/**
	 * @brief Gets whether is during asset iteration.
	 */
	bool iterating(void);
	/**
	 * @brief This function gives you both read and write access, but you should
	 *   never modify the `entry` from here.
	 */
	int foreach(Asset::List::Enumerator enumerator, bool second = false);
	/**
	 * @brief Sorts all assets.
	 */
	void sort(void);
};

/* ===========================================================================} */

#endif /* __PROJECT_H__ */
