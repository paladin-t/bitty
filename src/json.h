/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __JSON_H__
#define __JSON_H__

#include "bitty.h"
#include "object.h"
#include "../lib/rapidjson/include/rapidjson/document.h"
#include "../lib/rapidjson/include/rapidjson/error/en.h"
#include "../lib/rapidjson/include/rapidjson/prettywriter.h"
#include "../lib/rapidjson/include/rapidjson/writer.h"

/*
** {===========================================================================
** JSON
*/

/**
 * @brief JSON object.
 */
class Json : public virtual Object {
public:
	typedef std::shared_ptr<Json> Ptr;

	struct Error {
		std::string message;
		unsigned position = 0;

		Error();
	};

public:
	BITTY_CLASS_TYPE('J', 'S', 'O', 'N')

	/**
	 * @param[out] val
	 */
	virtual bool toAny(Variant &val) const = 0;
	virtual bool fromAny(const Variant &val) = 0;

	/**
	 * @param[out] val
	 * @param[in, out] doc
	 */
	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const = 0;
	/**
	 * @param[in, out] val
	 */
	virtual bool toJson(rapidjson::Document &val) const = 0;
	virtual bool fromJson(const rapidjson::Value &val) = 0;
	virtual bool fromJson(const rapidjson::Document &val) = 0;

	/**
	 * @param[out] val
	 */
	virtual bool toString(std::string &val, bool pretty = true) const = 0;
	virtual bool fromString(const std::string &val, Error* error = nullptr) = 0;

	/**
	 * @param[out] val
	 */
	static bool toString(const rapidjson::Document &doc, std::string &val, bool pretty = true);
	static bool fromString(rapidjson::Document &doc, const char* json, const char* file = nullptr, Error* error = nullptr);

	static bool processParsingResult(const rapidjson::ParseResult &ret, const rapidjson::Document &doc, const char* json, const char* file = nullptr, Error* error = nullptr);

	static Json* create(void);
	static void destroy(Json* ptr);
};

/* ===========================================================================} */

#endif /* __JSON_H__ */
