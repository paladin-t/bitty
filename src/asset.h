/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __ASSET_H__
#define __ASSET_H__

#include "bitty.h"
#include "entry.h"
#include "generic.h"
#include "json.h"
#include "texture.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef ASSET_REF_NAME
#	define ASSET_REF_NAME "ref"
#endif /* ASSET_REF_NAME */

/* ===========================================================================} */

/*
** {===========================================================================
** Asset
*/

/**
 * @brief Asset entity.
 */
class Asset {
public:
	typedef Dual<Asset*> List;

	typedef std::function<Asset*(class Project*)> Creator;
	typedef std::function<void(Asset*)> Destroyer;

	enum Usages {
		NONE = 0,
		RUNNING = 1 << 0,
		EDITING = 1 << 1
	};

	struct States {
	public:
		enum {
			CLOSED,     // Closed, without editor tab or view.
			EDITABLE,   // Editable.
			INSPECTABLE // Activity for temporary inspecting.
		};
		typedef unsigned Activity;

	private:
		Activity _activity = CLOSED;
		bool _focusing = false;
		bool _selected = false;

	public:
		States();
		~States();

		/**
		 * @brief Gets the activity.
		 */
		Activity activity(void) const;
		/**
		 * @brief Sets a specific activity.
		 */
		void activate(Activity act);
		/**
		 * @brief Deactivates to `CLOSED`, and resets all other states.
		 */
		void deactivate(void);

		/**
		 * @brief Gets whether this asset (tab) is being focused.
		 */
		bool focusing(void);
		/**
		 * @brief Sets this asset (tab) as to be focused.
		 */
		void focus(void);

		/**
		 * @brief Gets whether this asset in a list view is selected.
		 */
		bool selected(void) const;
		/**
		 * @brief Sets this asset in a list view as selected.
		 */
		void select(void);
		/**
		 * @brief Sets this asset in a list view as not selected.
		 */
		void deselect(void);
	};

public:
	BITTY_PROPERTY_READONLY(unsigned, type)

	BITTY_PROPERTY_READONLY_PTR(class DirectoryInfo, directoryInfo)
	BITTY_PROPERTY_READONLY_PTR(class FileInfo, fileInfo)
	BITTY_PROPERTY_READONLY(Entry, entry)

	BITTY_PROPERTY(std::string, ref)

	BITTY_PROPERTY_READONLY_PTR(States, states)

	BITTY_PROPERTY(bool, custom)

private:
	bool _dirty = false;
	Usages _readyFor = NONE;

	class Project* _project = nullptr; // Foreign.

	Object::Ptr _object = nullptr;
	Object::Ptr _editing = nullptr;

	Texture::Ptr _texture = nullptr;
	Texture::Ptr _painting = nullptr;

	class Editable* _editor = nullptr;

public:
	Asset(class Project* project);
	~Asset();

	BITTY_CLASS_TYPE('A', 'S', 'T', 'A')

	/**
	 * @brief Links the running object to a buffer of a specific type.
	 */
	bool link(unsigned type, class Bytes* buf /* nullable */, const char* entry /* nullable */, Object::Ptr ref /* nullable */);
	/**
	 * @brief Links the running object to an `Object`.
	 */
	bool link(Object::Ptr obj /* nullable */, const char* entry /* nullable */);
	/**
	 * @brief Links to a file.
	 */
	bool link(const char* package /* nullable */, const char* entry /* nullable */);
	/**
	 * @brief Unlinks the `type`, `entry` and filesystem informations.
	 */
	bool unlink(void);

	/**
	 * @brief Gets the full path of the package.
	 *
	 * @return Directory name or archive file name.
	 */
	const char* package(void) const;

	/**
	 * @brief Gets whether the asset is revertible.
	 */
	bool revertible(void) const;

	/**
	 * @brief Gets whether the asset is referencing.
	 */
	unsigned referencing(void) const;

	/**
	 * @brief Gets a resident `Object` pointer reference for specific assets.
	 */
	Object::Ptr &object(Usages usage);
	/**
	 * @brief Gets a resident `Texture` pointer for an image asset, while the object
	 *   is holding the `Image`. The result is cached.
	 */
	Texture::Ptr texture(Usages usage);
	/**
	 * @brief Gets either a nomadic `Sfx` or `Music` pointer for a sound asset, while
	 *   the (running) object is holding the `Sound`. The result is not cached.
	 */
	Object::Ptr sound(unsigned type);
	/**
	 * @brief Gets an active editor.
	 */
	class Editable* editor(void) const;

	/**
	 * @brief Gets whether the asset is ready for a specific usage.
	 */
	bool readyFor(Usages usage) const;
	/**
	 * @brief Prepares the asset for a specific usage.
	 *
	 * @param[in] shallow `true` to ignore editor, otherwise to initialize it.
	 */
	bool prepare(Usages usage, bool shallow);
	/**
	 * @brief Finishes the asset for a specific usage.
	 *
	 * @param[in] shallow `true` to keep editor, otherwise to dispose it.
	 */
	bool finish(Usages usage, bool shallow);

	/**
	 * @brief Loads the asset from bytes for a specific usage.
	 */
	bool load(Usages usage, class Bytes* buf, Object::Ptr ref /* nullable */, bool implicit);
	/**
	 * @brief Loads the asset from filesystem for a specific usage.
	 */
	bool load(Usages usage);
	/**
	 * @brief Reloads the asset from bytes for a specific usage.
	 */
	bool reload(Usages usage, class Bytes* buf, Object::Ptr ref /* nullable */, bool implicit);
	/**
	 * @brief Reloads the asset from filesystem for a specific usage.
	 */
	bool reload(Usages usage);
	/**
	 * @brief Saves the asset to bytes for a specific usage.
	 *
	 * @param[out] buf
	 */
	bool save(Usages usage, class Bytes* buf);
	/**
	 * @brief Saves the asset to filesystem for a specific usage.
	 */
	bool save(Usages usage);
	/**
	 * @brief Unloads the objects and the reference information.
	 */
	bool unload(void);

	/**
	 * @brief Gets whether the asset contains unsaved modifications.
	 */
	bool dirty(void) const;
	/**
	 * @brief Sets whether the asset contains unsaved modifications.
	 */
	void dirty(bool val);

	/**
	 * @brief Gets the full path of the asset.
	 *
	 * @return 'dir_path/asset_entry.xxx' for directory-based package, otherwise
	 *   'arc_path.bit/asset_entry.xxx' for archive-based package.
	 */
	std::string fullPath(void) const;
	/**
	 * @brief Gets the extension name of the asset.
	 */
	std::string extName(void) const;

	/**
	 * @brief Gets whether the asset exists on filesystem.
	 */
	bool exists(void) const;
	/**
	 * @brief Makes the asset onto filesystem.
	 */
	bool make(void);
	/**
	 * @brief Removes the asset from filesystem.
	 */
	bool remove(void);
	/**
	 * @brief Renames the asset to a specific name on filesystem.
	 */
	bool rename(const char* newNameExt);

	/**
	 * @brief Fills from project to bytes.
	 *
	 * @param[out] buf
	 */
	bool toBytes(class Bytes* buf) const;
	/**
	 * @brief Fills from bytes to project.
	 *
	 * @param[in] buf
	 */
	bool fromBytes(class Bytes* buf);

	/**
	 * @brief Fills from bytes to JSON.
	 *
	 * @param[in] buf
	 * @param[out] doc
	 * @param[out] ref
	 */
	bool toJson(Usages usage, class Bytes* buf, rapidjson::Document &doc, Asset* &ref);
	/**
	 * @brief Fills from JSON to bytes.
	 *
	 * @param[out] buf
	 * @param[in, out] doc
	 */
	bool fromJson(Usages usage, class Bytes* buf, rapidjson::Document &doc) const;

	/**
	 * @brief Fills from a specific arguments to a blank `Object`.
	 */
	static Object::Ptr fromBlank(Usages usage, const class Project* project, unsigned type, IDictionary::Ptr options);

	/**
	 * @brief Gets the extension name of a specific path.
	 */
	static std::string extOf(const std::string &path);
	/**
	 * @brief Gets the type of a specific extension name.
	 */
	static unsigned typeOf(const std::string &ext, bool alowBytes);
	/**
	 * @brief Inferences the type of a specific asset content.
	 */
	static unsigned inferencedTypeOf(const std::string &content);

	/**
	 * @brief Compares two assets by referencing orders.
	 *
	 * @return -1 if `left` is possibly referencing `right`, 1 if `right is
	 *   possibly referencing `left`, otherwise compares by entry names.
	 */
	static int compare(const Asset* left, const Asset* right);

private:
	bool connect(const char* package, const char* ent);
	void cleanup(void);

	void editor(class Editable* editor);
	void editor(std::nullptr_t);

	void object(Usages usage, const Object::Ptr &obj);
	void object(Usages usage, std::nullptr_t);
};

/* ===========================================================================} */

#endif /* __ASSET_H__ */
