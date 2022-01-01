/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "renderer.h"
#include "sprite.h"
#include <deque>

/*
** {===========================================================================
** Sprite
*/

class SpriteImpl : public Sprite {
private:
	struct Frame {
		Texture::Ptr texture = nullptr;
		Math::Recti area;
		double interval = 0.0;
		std::string key;

		Frame() {
		}
		Frame(Texture::Ptr texture_, Math::Recti area_, double interval_) : texture(texture_), area(area_), interval(interval_) {
		}
		Frame(Texture::Ptr texture_, Math::Recti area_, double interval_, const char* key_) : texture(texture_), area(area_), interval(interval_) {
			if (key_)
				key = key_;
		}
	};
	typedef std::deque<Frame> Frames;

private:
	int _width = 0;
	int _height = 0;
	bool _hFlip = false;
	bool _vFlip = false;
	Frames _frames;

	bool _loop = true;
	int _playingBegin = -1;
	int _playingEnd = -1;
	bool _playing = false;
	unsigned _id = 0;
	int _cursor = 0;
	double _ticks = 0.0;

public:
	SpriteImpl() {
	}
	virtual ~SpriteImpl() override {
		unload();
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Sprite** ptr) const override {
		if (!ptr)
			return false;

		SpriteImpl* result = static_cast<SpriteImpl*>(Sprite::create(_width, _height));

		result->_width = _width;
		result->_height = _height;
		result->_hFlip = _hFlip;
		result->_vFlip = _vFlip;

		result->_frames = _frames;
		result->_loop = _loop;
		result->_playingBegin = _playingBegin;
		result->_playingEnd = _playingEnd;
		result->_playing = _playing;
		result->_cursor = _cursor;
		result->_ticks = _ticks;

		*ptr = result;

		return true;
	}
	virtual bool clone(Object** ptr) const override {
		Sprite* obj = nullptr;
		if (!clone(&obj))
			return false;

		*ptr = obj;

		return true;
	}

	virtual int width(void) const override {
		return _width >= 0 ? _width : 0;
	}
	virtual int height(void) const override {
		return _height >= 0 ? _height : 0;
	}

	virtual bool hFlip(void) const override {
		return _hFlip;
	}
	virtual void hFlip(bool f) override {
		_hFlip = f;
	}
	virtual bool vFlip(void) const override {
		return _vFlip;
	}
	virtual void vFlip(bool f) override {
		_vFlip = f;
	}

	virtual int count(void) const override {
		return (int)_frames.size();
	}
	virtual int indexOf(const std::string &key, int start) const override {
		if (key.empty())
			return -1;

		if (start < 0)
			start = 0;

		for (int i = start; i < (int)_frames.size(); ++i) {
			const Frame &f = _frames[i];
			if (!f.key.empty() && key == SPRITE_ANY_KEY) // Found any.
				return i;
			if (f.key == key)
				return i;
		}

		if (key == SPRITE_ANY_KEY)
			return (int)_frames.size();

		return -1;
	}
	virtual Range rangeOf(const std::string &key, int start) const override {
		const int beginIdx = indexOf(key, start);
		const int endIdx = indexOf(SPRITE_ANY_KEY, beginIdx + 1) - 1;

		return std::make_tuple(beginIdx, endIdx);
	}

	virtual bool get(int index, Texture::Ptr* tex, Math::Recti* area, double* interval, const char** key) const override {
		if (tex)
			*tex = nullptr;
		if (area)
			*area = Math::Recti();
		if (interval)
			*interval = std::numeric_limits<double>::quiet_NaN();
		if (key)
			*key = nullptr;
		if (index < 0 || index >= (int)_frames.size())
			return false;

		const Frame &f = _frames[index];
		if (tex)
			*tex = f.texture;
		if (area)
			*area = f.area;
		if (interval)
			*interval = f.interval;
		if (key)
			*key = f.key.c_str();

		return true;
	}
	virtual bool set(int index, Texture::Ptr tex, const Math::Recti* area, const double* interval, const char* key) override {
		if (index < 0 || index >= (int)_frames.size())
			return false;

		Frame &f = _frames[index];
		f.texture = tex;
		if (area)
			f.area = *area;
		if (interval)
			f.interval = std::max(*interval, 0.0);
		if (key)
			f.key = key;

		return true;
	}
	virtual bool set(int index, const Math::Recti* area, const double* interval, const char* key) override {
		if (index < 0 || index >= (int)_frames.size())
			return false;

		Frame &f = _frames[index];
		if (area)
			f.area = *area;
		if (interval)
			f.interval = std::max(*interval, 0.0);
		if (key)
			f.key = key;

		return true;
	}
	virtual void add(Texture::Ptr tex, const Math::Recti* area, const double* interval, const char* key) override {
		const Math::Recti frameArea = area ? *area : Math::Recti::byXYWH(0, 0, tex->width(), tex->height());
		_frames.push_back(
			Frame(
				tex,
				frameArea,
				interval ? *interval : SPRITE_DEFAULT_INTERVAL,
				key
			)
		);
	}
	virtual bool insert(int index, Texture::Ptr tex, const Math::Recti* area, const double* interval, const char* key) override {
		if (index < 0 || index > (int)_frames.size())
			return false;

		const Math::Recti frameArea = area ? *area : Math::Recti::byXYWH(0, 0, tex->width(), tex->height());
		_frames.insert(
			_frames.begin() + index,
			Frame(
				tex,
				frameArea,
				interval ? *interval : SPRITE_DEFAULT_INTERVAL,
				key
			)
		);

		return true;
	}
	virtual bool remove(int index, Texture::Ptr* tex, Math::Recti* area, double* interval, std::string* key) override {
		if (key)
			key->clear();

		const char* str = nullptr;
		if (!get(index, tex, area, interval, &str))
			return false;

		if (key && str)
			*key = str;

		_frames.erase(_frames.begin() + index);

		return true;
	}

	virtual bool play(int begin, int end, bool reset, bool loop, double* duration) override {
		if (begin < 0 || begin >= (int)_frames.size())
			begin = 0;
		if (end < 0 || end >= (int)_frames.size())
			end = (int)_frames.size() - 1;

		_playingBegin = begin;
		_playingEnd = end;
		_loop = loop;
		if (_cursor < _playingBegin || _cursor > _playingEnd)
			reset = true;
		if (reset)
			_cursor = _playingBegin;

		_playing = true;

		if (duration) {
			*duration = 0;
			for (int i = begin; i <= end; ++i) {
				const Frame &frame = _frames[i];
				*duration += frame.interval;
			}
		}

		return true;
	}
	virtual bool play(const std::string &key, bool reset, bool loop, double* duration) override {
		const Range range = rangeOf(key, 0);

		return play(std::get<0>(range), std::get<1>(range), reset, loop, duration);
	}
	virtual void pause(void) override {
		_playing = false;
	}
	virtual void resume(void) override {
		_playing = true;
	}
	virtual void stop(void) override {
		_playing = false;
		_cursor = 0;
		_ticks = 0.0;
	}
	virtual bool current(int* index, Texture::Ptr* tex, Math::Recti* area, double* interval, const char** key) const override {
		if (index)
			*index = _cursor;

		return get(_cursor, tex, area, interval, key);
	}

	virtual bool update(double delta, unsigned* id) override {
		if (id != nullptr) {
			if (_id == *id)
				return true;
			_id = *id;
		}

		if (!_playing)
			return false;

		if (_cursor < 0 || _cursor >= (int)_frames.size())
			_cursor = 0;
		if (_frames.empty())
			return false;

		_ticks += delta;
		const Frame &f = _frames[_cursor];
		if (_ticks >= f.interval) {
			_ticks -= f.interval;
			++_cursor;
			if (_loop) {
				if (_cursor > _playingEnd)
					_cursor = _playingBegin;
				else if (_cursor >= (int)_frames.size())
					_cursor = _playingBegin;
			} else {
				if (_cursor > _playingEnd)
					_cursor = _playingEnd;
				else if (_cursor >= (int)_frames.size())
					_cursor = (int)_frames.size() - 1;
			}
		}

		return true;
	}

	virtual void render(
		class Renderer* rnd,
		int x, int y, int width, int height,
		const double* rotAngle, const Math::Vec2f* rotCenter,
		const Color* color, bool colorChanged, bool alphaChanged
	) const override {
		Math::Recti area;
		Texture::Ptr tex = nullptr;
		if (!get(_cursor, &tex, &area, nullptr, nullptr))
			return;
		if (!tex)
			return;

		Math::Recti dst;
		if (!(width > 0 && height > 0)) {
			width = area.width();
			height = area.height();
		}
		dst = Math::Recti::byXYWH(x, y, width, height);

		const Math::Recti viewport = Math::Recti::byXYWH(0, 0, rnd->width(), rnd->height());
		if (!Math::intersects(viewport, dst))
			return;

		rnd->render(tex.get(), &area, &dst, rotAngle, rotCenter, hFlip(), vFlip(), color, colorChanged, alphaChanged);
	}

	virtual bool load(Texture::Ptr tex, const Math::Recti* fullArea, const Math::Vec2i* frameSize, double interval, bool columnMajorOrder) override {
		if (!tex)
			return false;

		interval = std::max(interval, 0.0);

		const Math::Recti fullAreaRect = fullArea ? *fullArea : Math::Recti::byXYWH(0, 0, tex->width(), tex->height());
		if (frameSize) {
			if (columnMajorOrder) {
				for (int i = fullAreaRect.xMin(); i + frameSize->x < fullAreaRect.xMax(); i += frameSize->x) {
					for (int j = fullAreaRect.yMin(); j + frameSize->y < fullAreaRect.yMax(); j += frameSize->y) {
						_frames.push_back(
							Frame(
								tex,
								Math::Recti::byXYWH(i, j, frameSize->x, frameSize->y),
								interval
							)
						);
					}
				}
			} else {
				for (int j = fullAreaRect.yMin(); j + frameSize->y < fullAreaRect.yMax(); j += frameSize->y) {
					for (int i = fullAreaRect.xMin(); i + frameSize->x < fullAreaRect.xMax(); i += frameSize->x) {
						_frames.push_back(
							Frame(
								tex,
								Math::Recti::byXYWH(i, j, frameSize->x, frameSize->y),
								interval
							)
						);
					}
				}
			}

			_width = frameSize->x;
			_height = frameSize->y;
		} else {
			_frames.push_back(Frame{ tex, fullAreaRect, interval });

			_width = fullAreaRect.width();
			_height = fullAreaRect.height();
		}

		return true;
	}
	virtual bool load(int width, int height) override {
		_width = width;
		_height = height;

		return true;
	}
	virtual void unload(void) override {
		_frames.clear();

		stop();
	}

	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const override {
		val.SetObject();

		if (_width > 0 && _height > 0) {
			rapidjson::Value jstrwidth, jstrheight;
			jstrwidth.SetString("width", doc.GetAllocator());
			jstrheight.SetString("height", doc.GetAllocator());
			rapidjson::Value jvalwidth, jvalheight;
			jvalwidth.SetInt(_width);
			jvalheight.SetInt(_height);
			val.AddMember(jstrwidth, jvalwidth, doc.GetAllocator());
			val.AddMember(jstrheight, jvalheight, doc.GetAllocator());
		}

		rapidjson::Value jstrcount;
		jstrcount.SetString("count", doc.GetAllocator());
		rapidjson::Value jvalcount;
		jvalcount.SetInt(count());
		val.AddMember(jstrcount, jvalcount, doc.GetAllocator());

		rapidjson::Value jstrdata;
		jstrdata.SetString("data", doc.GetAllocator());
		rapidjson::Value jvaldata;
		jvaldata.SetArray();
		for (int i = 0; i < count(); ++i) {
			rapidjson::Value jframe;
			jframe.SetObject();

			Math::Recti area;
			double interval = 0;
			const char* key = nullptr;
			get(i, nullptr, &area, &interval, &key);
			rapidjson::Value jstrx, jstry, jstrw, jstrh, jstrinterval, jstrkey;
			jstrx.SetString("x", doc.GetAllocator());
			jstry.SetString("y", doc.GetAllocator());
			jstrw.SetString("width", doc.GetAllocator());
			jstrh.SetString("height", doc.GetAllocator());
			jstrinterval.SetString("interval", doc.GetAllocator());
			jstrkey.SetString("key", doc.GetAllocator());
			rapidjson::Value jvalx, jvaly, jvalw, jvalh, jvalinterval, jvalkey;
			jvalx.SetInt(area.xMin());
			jvaly.SetInt(area.yMin());
			jvalw.SetInt(area.width());
			jvalh.SetInt(area.height());
			jvalinterval.SetDouble(interval);
			if (key)
				jvalkey.SetString(key, doc.GetAllocator());
			else
				jvalkey.SetString("", doc.GetAllocator());
			jframe.AddMember(jstrx, jvalx, doc.GetAllocator());
			jframe.AddMember(jstry, jvaly, doc.GetAllocator());
			jframe.AddMember(jstrw, jvalw, doc.GetAllocator());
			jframe.AddMember(jstrh, jvalh, doc.GetAllocator());
			jframe.AddMember(jstrinterval, jvalinterval, doc.GetAllocator());
			jframe.AddMember(jstrkey, jvalkey, doc.GetAllocator());
			jvaldata.PushBack(jframe, doc.GetAllocator());
		}
		val.AddMember(jstrdata, jvaldata, doc.GetAllocator());

		return true;
	}
	virtual bool toJson(rapidjson::Document &val) const override {
		return toJson(val, val);
	}
	virtual bool fromJson(Texture::Ptr tex, const rapidjson::Value &val) override {
		unload();

		if (!val.IsObject())
			return false;

		rapidjson::Value::ConstMemberIterator jwidth = val.FindMember("width");
		rapidjson::Value::ConstMemberIterator jheight = val.FindMember("height");
		if ((jwidth != val.MemberEnd() && jwidth->value.IsInt()) && (jheight != val.MemberEnd() && jheight->value.IsInt())) {
			_width = jwidth->value.GetInt();
			_height = jheight->value.GetInt();
		}

		rapidjson::Value::ConstMemberIterator jcount = val.FindMember("count");
		if (jcount == val.MemberEnd() || !jcount->value.IsInt())
			return false;
		const int count = jcount->value.GetInt();

		rapidjson::Value::ConstMemberIterator jdata = val.FindMember("data");
		if (jdata == val.MemberEnd() || !jdata->value.IsArray())
			return false;

		rapidjson::Value::ConstArray jframes = jdata->value.GetArray();
		for (rapidjson::SizeType i = 0; i < jframes.Size() && (int)i < count; ++i) {
			const rapidjson::Value &jframe = jframes[i];
			rapidjson::Value::ConstMemberIterator jx = jframe.FindMember("x");
			rapidjson::Value::ConstMemberIterator jy = jframe.FindMember("y");
			rapidjson::Value::ConstMemberIterator jw = jframe.FindMember("width");
			rapidjson::Value::ConstMemberIterator jh = jframe.FindMember("height");
			if (jx == jframe.MemberEnd() || jy == jframe.MemberEnd() || jw == jframe.MemberEnd() || jh == jframe.MemberEnd())
				continue;
			if (!jx->value.IsInt() || !jy->value.IsInt() || !jw->value.IsInt() || !jh->value.IsInt())
				continue;
			const int x = jx->value.GetInt();
			const int y = jy->value.GetInt();
			const int w = jw->value.GetInt();
			const int h = jh->value.GetInt();
			rapidjson::Value::ConstMemberIterator jinterval = jframe.FindMember("interval");
			rapidjson::Value::ConstMemberIterator jkey = jframe.FindMember("key");
			if (jinterval == jframe.MemberEnd() || jkey == jframe.MemberEnd())
				continue;
			if (!jinterval->value.IsNumber() || !jkey->value.IsString())
				continue;
			double interval = jinterval->value.GetDouble();
			std::string key = jkey->value.GetString();
			const Math::Recti area = Math::Recti::byXYWH(x, y, w, h);
			add(tex, &area, &interval, key.c_str());
		}

		return true;
	}
	virtual bool fromJson(Texture::Ptr tex, const rapidjson::Document &val) override {
		const rapidjson::Value &jval = val;

		return fromJson(tex, jval);
	}
};

Sprite* Sprite::create(int width, int height) {
	SpriteImpl* result = new SpriteImpl();
	result->load(width, height);

	return result;
}

void Sprite::destroy(Sprite* ptr) {
	SpriteImpl* impl = static_cast<SpriteImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
