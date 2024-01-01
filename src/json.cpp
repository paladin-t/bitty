/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "json.h"
#include "text.h"

/*
** {===========================================================================
** JSON
*/

class JsonImpl : public Json {
private:
	rapidjson::Document _document;

public:
	virtual ~JsonImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Object** ptr) const override { // Non-clonable.
		if (ptr)
			*ptr = nullptr;

		return false;
	}

	virtual bool toAny(Variant &val) const override {
		return get(_document, val);
	}
	virtual bool fromAny(const Variant &val) override {
		return set(_document, val, _document.GetAllocator());
	}

	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const override {
		val.CopyFrom(_document, doc.GetAllocator(), true);

		return true;
	}
	virtual bool toJson(rapidjson::Document &val) const override {
		val.CopyFrom(_document, val.GetAllocator(), true);

		return true;
	}
	virtual bool fromJson(const rapidjson::Value &val) override {
		_document.CopyFrom(val, _document.GetAllocator(), true);

		return true;
	}
	virtual bool fromJson(const rapidjson::Document &val) override {
		_document.CopyFrom(val, _document.GetAllocator(), true);

		return true;
	}

	virtual bool toString(std::string &val, bool pretty) const override {
		return Json::toString(_document, val, pretty);
	}
	virtual bool fromString(const std::string &val, Error* error) override {
		return Json::fromString(_document, val.c_str(), nullptr, error);
	}

private:
	bool get(const rapidjson::Value &jval, Variant &val) const {
		switch (jval.GetType()) {
		case rapidjson::kNullType:
			val = Variant(nullptr);

			break;
		case rapidjson::kFalseType:
			val = Variant(false);

			break;
		case rapidjson::kTrueType:
			val = Variant(true);

			break;
		case rapidjson::kObjectType: {
				IDictionary::Ptr ptr(Dictionary::create());

				rapidjson::Value::ConstObject jobj = jval.GetObject();
				for (rapidjson::Value::ConstMemberIterator it = jobj.MemberBegin(); it != jobj.MemberEnd(); ++it) {
					const rapidjson::Value &jk = it->name;
					const rapidjson::Value &jv = it->value;

					Variant k = nullptr, v = nullptr;
					get(jk, k);
					get(jv, v);

					ptr->set((std::string)k, v);
				}

				val = Variant(ptr);
			}

			break;
		case rapidjson::kArrayType: {
				IList::Ptr ptr(List::create());

				rapidjson::Value::ConstArray jarr = jval.GetArray();
				for (rapidjson::SizeType i = 0; i < jarr.Size(); ++i) {
					const rapidjson::Value &ji = jarr[i];

					Variant::Int idx = (Variant::Int)i;
					(void)idx;
					Variant v = nullptr;
					get(ji, v);

					ptr->add(v);
				}

				val = Variant(ptr);
			}

			break;
		case rapidjson::kStringType:
			val = Variant(jval.GetString());

			break;
		case rapidjson::kNumberType: {
				if (jval.IsInt())
					val = Variant((Variant::Int)jval.GetInt());
				else if (jval.IsInt64())
					val = Variant((Variant::Int)jval.GetInt64());
				else if (jval.IsUint())
					val = Variant((Variant::Int)jval.GetUint());
				else if (jval.IsUint64())
					val = Variant((Variant::Int)jval.GetUint64());
				else if (jval.IsFloat())
					val = Variant((Variant::Real)jval.GetFloat());
				else
					val = Variant((Variant::Real)jval.GetDouble());
			}

			break;
		}

		return true;
	}
	bool set(rapidjson::Value &jval, const Variant &val, rapidjson::MemoryPoolAllocator<> &allocator) {
		switch (val.type()) {
		case Variant::INTEGER:
			jval.SetInt((Variant::Int)val);

			break;
		case Variant::REAL:
			jval.SetDouble((Variant::Real)val);

			break;
		case Variant::BOOLEAN:
			jval.SetBool((bool)val);

			break;
		case Variant::NIL:
			jval.SetNull();

			break;
		case Variant::STRING:
			jval.SetString((const char*)val, allocator);

			break;
		case Variant::POINTER: // Do nothing.
			break;
		case Variant::OBJECT: {
				Object::Ptr ptr = (Object::Ptr)val;
				if (Object::is<IList::Ptr>(ptr)) {
					jval.SetArray();

					IList::Ptr lst = Object::as<IList::Ptr>(ptr);
					for (int i = 0; i < lst->count(); ++i) {
						rapidjson::Value jv;
						set(jv, lst->at(i), allocator);

						jval.PushBack(jv, allocator);
					}
				} else if (Object::is<IDictionary::Ptr>(ptr)) {
					jval.SetObject();

					IDictionary::Ptr dict = Object::as<IDictionary::Ptr>(ptr);
					const IDictionary::Keys keys = dict->keys();
					for (const std::string &k : keys) {
						rapidjson::Value jk;
						rapidjson::Value jv;
						jk.SetString(k.c_str(), allocator);
						set(jv, dict->get(k), allocator);

						jval.AddMember(jk, jv, allocator);
					}
				} else {
					jval.SetNull();
				}
			}

			break;
		}

		return true;
	}
};

Json::Error::Error() {
}

bool Json::toString(const rapidjson::Document &doc, std::string &val, bool pretty) {
	rapidjson::StringBuffer buffer;
	if (pretty) {
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
		writer.SetIndent(' ', 2);
		doc.Accept(writer);
	} else {
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		doc.Accept(writer);
	}
	val = buffer.GetString();

	return true;
}

bool Json::fromString(rapidjson::Document &doc, const char* json, const char* file, Error* error) {
	rapidjson::ParseResult ret = doc.Parse(json);

	return Json::processParsingResult(ret, doc, json, file, error);
}

bool Json::processParsingResult(const rapidjson::ParseResult &ret, const rapidjson::Document &/* doc */, const char* json, const char* file, Error* error) {
	if (!ret) {
		const char* what = rapidjson::GetParseError_En(ret.Code());
		if (error) {
			error->message = what;
			error->position = (unsigned)ret.Offset();
		}
		if (file)
			fprintf(stderr, "JSON parse error: \"%s\", %s (%u).\n", file, what, (unsigned)ret.Offset());
		else
			fprintf(stderr, "JSON parse error: %s (%u).\n", what, (unsigned)ret.Offset());
		const char* off = json + ret.Offset();
		(void)off;

		return false;
	}

	return true;
}

Json* Json::create(void) {
	JsonImpl* result = new JsonImpl();

	return result;
}

void Json::destroy(Json* ptr) {
	JsonImpl* impl = static_cast<JsonImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
