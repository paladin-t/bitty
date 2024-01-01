/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "palette.h"
#include <SDL.h>

/*
** {===========================================================================
** Macros and constants
*/

static const Color PALETTE_DEFAULT_COLORS_ARRAY[] = PALETTE_DEFAULT_COLORS;

/* ===========================================================================} */

/*
** {===========================================================================
** Palette
*/

class PaletteImpl : public Palette {
private:
	Color* _colors = nullptr;
	int _count = 0;
	bool _dirty = true;

	SDL_Palette* _palette = nullptr;

public:
	PaletteImpl(int count) {
		if (count > 0) {
			_count = count;
			_colors = new Color[_count];
			memset(_colors, 255, _count * sizeof(Color));
			memcpy(_colors, PALETTE_DEFAULT_COLORS_ARRAY, std::min(sizeof(PALETTE_DEFAULT_COLORS_ARRAY), _count * sizeof(Color)));
		} else {
			_count = 0;
		}
	}
	virtual ~PaletteImpl() override {
		if (_palette) {
			SDL_FreePalette(_palette);
			_palette = nullptr;
		}

		clear();
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Palette** ptr, bool /* graphical */) const override {
		if (!ptr)
			return false;

		PaletteImpl* result = static_cast<PaletteImpl*>(Palette::create(_count));
		if (_colors && _count > 0)
			memcpy(result->_colors, _colors, sizeof(Color) * _count);

		*ptr = result;

		return true;
	}
	virtual bool clone(Palette** ptr) const override {
		return clone(ptr, true);
	}
	virtual bool clone(Object** ptr) const override {
		Palette* obj = nullptr;
		if (!clone(&obj, true))
			return false;

		*ptr = obj;

		return true;
	}

	virtual void* pointer(void) override {
		return palette();
	}

	virtual bool validate(void) override {
		return !!palette();
	}

	virtual int count(void) const override {
		return _count;
	}

	virtual const Color* get(int index, Color &col) const override {
		col = Color();

		if (index < 0 || index >= _count)
			return nullptr;

		col = _colors[index];

		return &col;
	}
	virtual bool set(int index, const Color* col) override {
		if (index < 0 || index >= _count)
			return false;

		if (!col)
			return false;

		_colors[index] = *col;

		_dirty = true;

		return true;
	}

	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const override {
		val.SetObject();

		rapidjson::Value jstrcount;
		jstrcount.SetString("count", doc.GetAllocator());
		rapidjson::Value jnumcount;
		jnumcount.SetInt(_count);
		val.AddMember(jstrcount, jnumcount, doc.GetAllocator());

		rapidjson::Value jstrdata;
		jstrdata.SetString("data", doc.GetAllocator());
		rapidjson::Value jvaldata;
		jvaldata.SetArray();
		for (int i = 0; i < _count; ++i) {
			const Color &col = _colors[i];
			rapidjson::Value jcol;
			jcol.SetArray();
			jcol.PushBack(col.r, doc.GetAllocator());
			jcol.PushBack(col.g, doc.GetAllocator());
			jcol.PushBack(col.b, doc.GetAllocator());
			jcol.PushBack(col.a, doc.GetAllocator());
			jvaldata.PushBack(jcol, doc.GetAllocator());
		}
		val.AddMember(jstrdata, jvaldata, doc.GetAllocator());

		return true;
	}
	virtual bool toJson(rapidjson::Document &val) const override {
		return toJson(val, val);
	}
	virtual bool fromJson(const rapidjson::Value &val) override {
		clear();

		if (!val.IsObject())
			return false;

		rapidjson::Value::ConstMemberIterator jcount = val.FindMember("count");
		if (jcount == val.MemberEnd() || !jcount->value.IsNumber())
			return false;
		const int count = jcount->value.GetInt();
		rapidjson::Value::ConstMemberIterator jdata = val.FindMember("data");
		if (jdata == val.MemberEnd() || !jdata->value.IsArray())
			return false;

		const rapidjson::Value &jarr = jdata->value;
		if (count != (int)jarr.Size())
			return false;
		_count = jarr.Size();
		_colors = new Color[_count];
		for (int i = 0; i < _count; ++i)
			_colors[i] = Color();

		int i = 0;
		while (i < count && i < (int)jarr.Size()) {
			Color &col = _colors[i];
			const rapidjson::Value &jcol = jarr[i];
			if (jcol.IsArray() && jcol.Size() == 4) {
				const rapidjson::Value &c0 = jcol[0];
				const rapidjson::Value &c1 = jcol[1];
				const rapidjson::Value &c2 = jcol[2];
				const rapidjson::Value &c3 = jcol[3];
				if (c0.IsInt() && c1.IsInt() && c2.IsInt() && c3.IsInt()) {
					col = {
						(Byte)c0.GetInt(),
						(Byte)c1.GetInt(),
						(Byte)c2.GetInt(),
						(Byte)c3.GetInt()
					};
				}
			}

			++i;
		}
		while (i < count) {
			Color &col = _colors[i];
			col = Color(0, 0, 0, 0);

			++i;
		}

		return true;
	}
	virtual bool fromJson(const rapidjson::Document &val) override {
		const rapidjson::Value &jval = val;

		return fromJson(jval);
	}

private:
	SDL_Palette* palette(void) {
		if (_count <= 0) {
			if (_dirty)
				_dirty = false;

			return nullptr;
		}

		if (!_palette) {
			_palette = SDL_AllocPalette(_count);

			_dirty = true;
		}

		if (_dirty) {
			SDL_SetPaletteColors(_palette, (SDL_Color*)_colors, 0, _count);

			_dirty = false;
		}

		return _palette;
	}

	void clear(void) {
		if (_colors) {
			delete [] _colors;
			_colors = nullptr;
		}
		_count = 0;

		_dirty = true;
	}
};

Palette* Palette::create(int count) {
	PaletteImpl* result = new PaletteImpl(count);

	return result;
}

void Palette::destroy(Palette* ptr) {
	PaletteImpl* impl = static_cast<PaletteImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
