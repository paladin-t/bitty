/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "scripting_lua.h"
#include "scripting_lua_api_physics.h"
#include "../lib/chipmunk2d/include/chipmunk/chipmunk.h"
#ifdef __cplusplus
extern "C" {
#endif
#	include "../lib/chipmunk2d/include/chipmunk/chipmunk_structs.h"
#ifdef __cplusplus
}
#endif
#include <map>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef LUA_WEAK_PTR
#	define LUA_WEAK_PTR(TYPE, PTR, DTOR, WEAK, SHARED) \
		const TYPE::Ptr SHARED( \
			PTR, \
			DTOR \
		); \
		const TYPE::WeakPtr WEAK = SHARED;
#endif /* LUA_WEAK_PTR */

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

namespace Lua { // Library.

/**< Math. */

LUA_CHECK_ALIAS(Math::Vec2f, Vec2)
LUA_READ_ALIAS(Math::Vec2f, Vec2)
LUA_WRITE_ALIAS(Math::Vec2f, Vec2)
LUA_WRITE_ALIAS_CONST(Math::Vec2f, Vec2)

LUA_CHECK_ALIAS(Math::Rectf, Rect)
LUA_READ_ALIAS(Math::Rectf, Rect)
LUA_WRITE_ALIAS(Math::Rectf, Rect)
LUA_WRITE_ALIAS_CONST(Math::Rectf, Rect)

LUA_CHECK_ALIAS(Math::Rotf, Rot)
// LUA_READ_ALIAS(Math::Rotf, Rot)
LUA_WRITE_ALIAS(Math::Rotf, Rot)
LUA_WRITE_ALIAS_CONST(Math::Rotf, Rot)

}

namespace Lua { // Engine.

/**< Vertex. */

struct Vertex {
	typedef std::vector<cpVect> Array;
};

/**< Transform. */

struct Transform {
	typedef std::shared_ptr<cpTransform> Ptr;

	static cpTransform* create(cpFloat a, cpFloat b, cpFloat c, cpFloat d, cpFloat tx, cpFloat ty) {
		return new cpTransform{ a, b, c, d, tx, ty };
	}
	static cpTransform* create(const cpTransform &val) {
		return new cpTransform{ val.a, val.b, val.c, val.d, val.tx, val.ty };
	}
	static void destroy(cpTransform* val) {
		delete val;
	}
};
LUA_CHECK_ALIAS(Transform::Ptr, Transform)
LUA_READ_ALIAS(Transform::Ptr, Transform)
LUA_WRITE_ALIAS(Transform::Ptr, Transform)
LUA_WRITE_ALIAS_CONST(Transform::Ptr, Transform)

/**< Arbiter. */

struct Arbiter {
	typedef std::shared_ptr<cpArbiter> Ptr;
	typedef std::weak_ptr<cpArbiter> WeakPtr;
	typedef std::vector<Ptr> Array;
};
LUA_CHECK_ALIAS(Arbiter::WeakPtr, Arbiter)
LUA_READ_ALIAS(Arbiter::WeakPtr, Arbiter)
LUA_WRITE_ALIAS(Arbiter::WeakPtr, Arbiter)
LUA_WRITE_ALIAS_CONST(Arbiter::WeakPtr, Arbiter)
#ifndef LUA_ARTIBER_WEAK_AND_SHARED_PTR
#	define LUA_ARTIBER_WEAK_AND_SHARED_PTR(PTR, WEAK, SHARED) LUA_WEAK_PTR(Arbiter, PTR, Arbiter_dtor, WEAK, SHARED)
#endif /* LUA_ARTIBER_WEAK_AND_SHARED_PTR */
#ifndef LUA_ARTIBER_WEAK_PTR
#	define LUA_ARTIBER_WEAK_PTR(PTR, WEAK) LUA_ARTIBER_WEAK_AND_SHARED_PTR(PTR, WEAK, WEAK##_)
#endif /* LUA_ARTIBER_WEAK_PTR */

/**< Shape. */

struct ShapeFilter {
	typedef std::shared_ptr<cpShapeFilter> Ptr;
};
LUA_CHECK_ALIAS(ShapeFilter::Ptr, ShapeFilter)
LUA_READ_ALIAS(ShapeFilter::Ptr, ShapeFilter)
LUA_WRITE_ALIAS(ShapeFilter::Ptr, ShapeFilter)
LUA_WRITE_ALIAS_CONST(ShapeFilter::Ptr, ShapeFilter)

struct Shape {
	typedef std::shared_ptr<cpShape> Ptr;
	typedef std::weak_ptr<cpShape> WeakPtr;
	typedef std::vector<Ptr> Array;
};
LUA_CHECK_ALIAS(Shape::Ptr, Shape)
LUA_READ_ALIAS(Shape::Ptr, Shape)
LUA_WRITE_ALIAS(Shape::Ptr, Shape)
LUA_WRITE_ALIAS_CONST(Shape::Ptr, Shape)

/**< Body. */

struct Body {
	typedef std::shared_ptr<cpBody> Ptr;
	typedef std::weak_ptr<cpBody> WeakPtr;
	typedef std::vector<Ptr> Array;
};
LUA_CHECK_ALIAS(Body::Ptr, Body)
LUA_READ_ALIAS(Body::Ptr, Body)
LUA_WRITE_ALIAS(Body::Ptr, Body)
LUA_WRITE_ALIAS_CONST(Body::Ptr, Body)

/**< Constraints. */

struct Constraint {
	typedef std::shared_ptr<cpConstraint> Ptr;
	typedef std::weak_ptr<cpConstraint> WeakPtr;
	typedef std::vector<Ptr> Array;
};
LUA_CHECK_ALIAS(Constraint::Ptr, Constraint)
LUA_READ_ALIAS(Constraint::Ptr, Constraint)
LUA_WRITE_ALIAS(Constraint::Ptr, Constraint)
LUA_WRITE_ALIAS_CONST(Constraint::Ptr, Constraint)

/**< Contact. */

struct Contact {
	typedef std::shared_ptr<Contact> Ptr;
	typedef std::vector<Contact> Array;

	cpVect pointA;
	cpVect pointB;
	cpFloat distance = 0;

	Contact() {
	}
	Contact(const Contact &other) :
		pointA(other.pointA), pointB(other.pointB), distance(other.distance)
	{
	}
	Contact(
		const cpVect &pointA_, const cpVect &pointB_, cpFloat distance_
	) :
		pointA(pointA_), pointB(pointB_), distance(distance_)
	{
	}
	~Contact() {
	}
};
LUA_CHECK_ALIAS(Contact::Ptr, Contact)
LUA_READ_ALIAS(Contact::Ptr, Contact)
LUA_WRITE_ALIAS(Contact::Ptr, Contact)
LUA_WRITE_ALIAS_CONST(Contact::Ptr, Contact)

static_assert(CP_MAX_CONTACTS_PER_ARBITER == 2, "Wrong size.");
struct Contacts {
	typedef std::shared_ptr<Contacts> Ptr;

	Shape::Ptr shape = nullptr;
	cpVect normal;
	int count = 0;
	Contact points[CP_MAX_CONTACTS_PER_ARBITER];

	Contacts() {
	}
	Contacts(const Contacts &other) :
		shape(other.shape), normal(other.normal), count(other.count)
	{
		points[0] = other.points[0];
		points[1] = other.points[1];
	}
	Contacts(
		Shape::Ptr shape_, const cpVect &normal_, int count_,
		const cpVect &point1A_, const cpVect &point1B_, cpFloat distance1_,
		const cpVect &point2A_, const cpVect &point2B_, cpFloat distance2_
	) :
		shape(shape_), normal(normal_), count(count_)
	{
		points[0].pointA = point1A_;
		points[0].pointB = point1B_;
		points[0].distance = distance1_;
		points[1].pointA = point2A_;
		points[1].pointB = point2B_;
		points[1].distance = distance2_;
	}
	~Contacts() {
	}
};
LUA_CHECK_ALIAS(Contacts::Ptr, Contacts)
LUA_READ_ALIAS(Contacts::Ptr, Contacts)
LUA_WRITE_ALIAS(Contacts::Ptr, Contacts)
LUA_WRITE_ALIAS_CONST(Contacts::Ptr, Contacts)

/**< Queries. */

struct PointQuery {
	typedef std::shared_ptr<PointQuery> Ptr;
	typedef std::vector<PointQuery> Array;

	Shape::Ptr shape = nullptr;
	cpVect point;
	cpFloat distance = 0;
	cpVect gradient;

	PointQuery() {
	}
	PointQuery(const PointQuery &other) :
		shape(other.shape), point(other.point), distance(other.distance), gradient(other.gradient)
	{
	}
	PointQuery(Shape::Ptr shape_, const cpVect &point_, cpFloat distance_, const cpVect &gradient_) :
		shape(shape_), point(point_), distance(distance_), gradient(gradient_)
	{
	}
	~PointQuery() {
	}
};
LUA_CHECK_ALIAS(PointQuery::Ptr, PointQuery)
LUA_READ_ALIAS(PointQuery::Ptr, PointQuery)
LUA_WRITE_ALIAS(PointQuery::Ptr, PointQuery)
LUA_WRITE_ALIAS_CONST(PointQuery::Ptr, PointQuery)

struct SegmentQuery {
	typedef std::shared_ptr<SegmentQuery> Ptr;
	typedef std::vector<SegmentQuery> Array;

	Shape::Ptr shape = nullptr;
	cpVect point;
	cpVect normal;
	cpFloat alpha = 0;

	SegmentQuery() {
	}
	SegmentQuery(const SegmentQuery &other) :
		shape(other.shape), point(other.point), normal(other.normal), alpha(other.alpha)
	{
	}
	SegmentQuery(Shape::Ptr shape_, const cpVect &point_, const cpVect &normal_, cpFloat alpha_) :
		shape(shape_), point(point_), normal(normal_), alpha(alpha_)
	{
	}
	~SegmentQuery() {
	}
};
LUA_CHECK_ALIAS(SegmentQuery::Ptr, SegmentQuery)
LUA_READ_ALIAS(SegmentQuery::Ptr, SegmentQuery)
LUA_WRITE_ALIAS(SegmentQuery::Ptr, SegmentQuery)
LUA_WRITE_ALIAS_CONST(SegmentQuery::Ptr, SegmentQuery)

struct BoundingBoxQuery {
	typedef std::shared_ptr<BoundingBoxQuery> Ptr;
	typedef std::vector<BoundingBoxQuery> Array;

	Shape::Ptr shape = nullptr;

	BoundingBoxQuery() {
	}
	BoundingBoxQuery(const BoundingBoxQuery &other) :
		shape(other.shape)
	{
	}
	BoundingBoxQuery(Shape::Ptr shape_) :
		shape(shape_)
	{
	}
	~BoundingBoxQuery() {
	}
};
LUA_CHECK_ALIAS(BoundingBoxQuery::Ptr, BoundingBoxQuery)
LUA_READ_ALIAS(BoundingBoxQuery::Ptr, BoundingBoxQuery)
LUA_WRITE_ALIAS(BoundingBoxQuery::Ptr, BoundingBoxQuery)
LUA_WRITE_ALIAS_CONST(BoundingBoxQuery::Ptr, BoundingBoxQuery)

struct ShapeQuery : public Contacts {
	typedef std::shared_ptr<ShapeQuery> Ptr;
	typedef std::vector<ShapeQuery> Array;

	using Contacts::Contacts;
};
LUA_CHECK_ALIAS(ShapeQuery::Ptr, ShapeQuery)
LUA_READ_ALIAS(ShapeQuery::Ptr, ShapeQuery)
LUA_WRITE_ALIAS(ShapeQuery::Ptr, ShapeQuery)
LUA_WRITE_ALIAS_CONST(ShapeQuery::Ptr, ShapeQuery)

/**< Space. */

struct Space {
	typedef std::shared_ptr<cpSpace> Ptr;
	typedef std::weak_ptr<cpSpace> WeakPtr;
};
LUA_CHECK_ALIAS(Space::Ptr, Space)
LUA_READ_ALIAS(Space::Ptr, Space)
LUA_WRITE_ALIAS(Space::Ptr, Space)
LUA_WRITE_ALIAS_CONST(Space::Ptr, Space)

}

namespace Lua { // Generic.

/**< Structures. */

static void check(lua_State* L, cpVect &ret, Index idx = Index(1)) {
	Math::Vec2f* val = nullptr;
	check(L, val, idx);

	if (val)
		ret = cpVect{ val->x, val->y };
	else
		ret = cpVect{ 0, 0 };
}

static void read(lua_State* L, cpVect &ret, Index idx = Index(1)) {
	Math::Vec2f* val = nullptr;
	read(L, val, idx);

	if (val)
		ret = cpVect{ val->x, val->y };
	else
		ret = cpVect{ 0, 0 };
}

template<> int write(lua_State* L, const cpVect &val) {
	const Math::Vec2f ret(val.x, val.y);

	return write(L, &ret);
}
template<> int write(lua_State* L, cpVect &val) {
	return write<>(L, (const cpVect &)val);
}
static int write(lua_State* L, const cpVect &val) {
	const Math::Vec2f ret(val.x, val.y);

	return write(L, &ret);
}

static void check(lua_State* L, cpBB &ret, Index idx = Index(1)) {
	Math::Rectf* val = nullptr;
	check(L, val, idx);

	if (val)
		ret = cpBB{ val->xMin(), val->yMin(), val->xMax(), val->yMax() };
	else
		ret = cpBB{ 0, 0, 0, 0 };
}

static void read(lua_State* L, cpBB &ret, Index idx = Index(1)) {
	Math::Rectf* val = nullptr;
	read(L, val, idx);

	if (val)
		ret = cpBB{ val->xMin(), val->yMin(), val->xMax(), val->yMax() };
	else
		ret = cpBB{ 0, 0, 0, 0 };
}

static int write(lua_State* L, const cpBB &val) {
	const Math::Rectf ret(val.l, val.b, val.r, val.t);

	return write(L, &ret);
}

static int write(lua_State* L, const Shape::Array &val) {
	newTable(L, (int)val.size());
	const int tbl = getTop(L);
	int index = 1; // 1-based.
	for (auto it = val.begin(); it != val.end(); ++it) {
		Shape::Ptr elem = *it;
		write(L, &elem);
		set(L, tbl, index);
		++index;
	}

	return 1;
}
static int write(lua_State* L, Shape::Array &val) {
	return write(L, (const Shape::Array &)val);
}

static int write(lua_State* L, const Body::Array &val) {
	newTable(L, (int)val.size());
	const int tbl = getTop(L);
	int index = 1; // 1-based.
	for (auto it = val.begin(); it != val.end(); ++it) {
		Body::Ptr elem = *it;
		write(L, &elem);
		set(L, tbl, index);
		++index;
	}

	return 1;
}
static int write(lua_State* L, Body::Array &val) {
	return write(L, (const Body::Array &)val);
}

static int write(lua_State* L, const Constraint::Array &val) {
	newTable(L, (int)val.size());
	const int tbl = getTop(L);
	int index = 1; // 1-based.
	for (auto it = val.begin(); it != val.end(); ++it) {
		Constraint::Ptr elem = *it;
		write(L, &elem);
		set(L, tbl, index);
		++index;
	}

	return 1;
}
static int write(lua_State* L, Constraint::Array &val) {
	return write(L, (const Constraint::Array &)val);
}

/**< Contact. */

/*static void read(lua_State* L, Contact::Array &ret, Index idx = Index(1)) {
	ret.clear();

	if (!isArray(L, idx))
		return;

	const lua_Unsigned size = len(L, idx);
	for (int i = 1; i <= (int)size; ++i) { // 1-based.
		get(L, idx, i);

		Contact::Ptr* val = nullptr;
		read<-1>(L, val);
		if (val && val->get()) {
			Contact query;
			query.pointA = val->get()->pointA;
			query.pointB = val->get()->pointB;
			query.distance = val->get()->distance;
			ret.push_back(query);
		}

		pop(L, 1);
	}
}*/

static int write(lua_State* L, const Contact::Array &val) {
	newTable(L, (int)val.size());
	const int tbl = getTop(L);
	int index = 1; // 1-based.
	for (auto it = val.begin(); it != val.end(); ++it) {
		Contact::Ptr elem(
			new Contact(*it),
			[] (const Contact* contact) -> void {
				delete contact;
			}
		);
		write(L, &elem);
		set(L, tbl, index);
		++index;
	}

	return 1;
}
static int write(lua_State* L, Contact::Array &val) {
	return write(L, (const Contact::Array &)val);
}

/**< Queries. */

static int write(lua_State* L, const PointQuery::Array &val) {
	newTable(L, (int)val.size());
	const int tbl = getTop(L);
	int index = 1; // 1-based.
	for (auto it = val.begin(); it != val.end(); ++it) {
		PointQuery::Ptr elem(
			new PointQuery(*it),
			[] (const PointQuery* query) -> void {
				delete query;
			}
		);
		write(L, &elem);
		set(L, tbl, index);
		++index;
	}

	return 1;
}
static int write(lua_State* L, PointQuery::Array &val) {
	return write(L, (const PointQuery::Array &)val);
}

static int write(lua_State* L, const SegmentQuery::Array &val) {
	newTable(L, (int)val.size());
	const int tbl = getTop(L);
	int index = 1; // 1-based.
	for (auto it = val.begin(); it != val.end(); ++it) {
		SegmentQuery::Ptr elem(
			new SegmentQuery(*it),
			[] (const SegmentQuery* query) -> void {
				delete query;
			}
		);
		write(L, &elem);
		set(L, tbl, index);
		++index;
	}

	return 1;
}
static int write(lua_State* L, SegmentQuery::Array &val) {
	return write(L, (const SegmentQuery::Array &)val);
}

static int write(lua_State* L, const BoundingBoxQuery::Array &val) {
	newTable(L, (int)val.size());
	const int tbl = getTop(L);
	int index = 1; // 1-based.
	for (auto it = val.begin(); it != val.end(); ++it) {
		BoundingBoxQuery::Ptr elem(
			new BoundingBoxQuery(*it),
			[] (const BoundingBoxQuery* query) -> void {
				delete query;
			}
		);
		write(L, &elem);
		set(L, tbl, index);
		++index;
	}

	return 1;
}
static int write(lua_State* L, BoundingBoxQuery::Array &val) {
	return write(L, (const BoundingBoxQuery::Array &)val);
}

static int write(lua_State* L, const ShapeQuery::Array &val) {
	newTable(L, (int)val.size());
	const int tbl = getTop(L);
	int index = 1; // 1-based.
	for (auto it = val.begin(); it != val.end(); ++it) {
		ShapeQuery::Ptr elem(
			new ShapeQuery(*it),
			[] (const ShapeQuery* query) -> void {
				delete query;
			}
		);
		write(L, &elem);
		set(L, tbl, index);
		++index;
	}

	return 1;
}
static int write(lua_State* L, ShapeQuery::Array &val) {
	return write(L, (const ShapeQuery::Array &)val);
}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Engine
*/

namespace Lua {

namespace Engine {

/**< Forward declarations. */

static void Arbiter_dtor(cpArbiter* arbiter);

static void Constraint_dtor(cpConstraint* constraint);

static cpBool Space_alwaysCollide(cpArbiter*, cpSpace*, cpDataPointer);
static void Space_doNothing(cpArbiter*, cpSpace*, cpDataPointer);
static cpBool Space_defaultBegin(cpArbiter* arbiter, cpSpace* space, cpDataPointer userData);
static cpBool Space_defaultPreSolve(cpArbiter* arbiter, cpSpace* space, cpDataPointer userData);
static void Space_defaultPostSolve(cpArbiter* arbiter, cpSpace* space, cpDataPointer userData);
static void Space_defaultSeparate(cpArbiter* arbiter, cpSpace* space, cpDataPointer userData);

/**< Extra data structures. */

template<typename Ptr, typename WeakPtr> struct ReferencableData {
	WeakPtr self;

	ReferencableData(Ptr &self_) : self(self_) {
	}
};

struct ManagableData {
	bool managed = false;
};

template<typename Key, typename Val> struct Cache {
	typedef std::map<const Key*, Val> Dictionary;
	typedef typename Dictionary::iterator Iterator;
	typedef std::function<bool(Key*, Val &)> Resetter;
	typedef std::vector<const Key*> Obsolete;

	Dictionary dictionary;
	Obsolete obsolete;

	Cache() {
	}
	~Cache() {
	}

	Val* find(const Key* key) {
		Iterator it = dictionary.find(key);
		if (it == dictionary.end())
			return nullptr;

		return &it->second;
	}
	bool add(const Key* key, const Val &val) {
		std::pair<Iterator, bool> ret = dictionary.insert(std::make_pair(key, val));

		return ret.second;
	}
	bool remove(const Key* key) {
		Iterator it = dictionary.find(key);
		if (it == dictionary.end())
			return false;

		dictionary.erase(it);

		return true;
	}
	void collect(Resetter resetter) {
		for (Iterator it = dictionary.begin(); it != dictionary.end(); ++it) {
			Key* key = (Key*)it->first;
			Val &val = it->second;
			if (resetter(key, val))
				obsolete.push_back(key);
		}
		if (!obsolete.empty()) {
			for (const Key* key : obsolete)
				remove(key);
			obsolete.clear();
		}
	}
};

struct ShapeData : public ReferencableData<Shape::Ptr, Shape::WeakPtr>, public ManagableData {
	enum Types {
		INVALID,
		CIRCLE,
		SEGMENT,
		POLY
	};

	Types type = INVALID;

	ShapeData(Shape::Ptr &self_, Types type_) : ReferencableData(self_), type(type_) {
	}
	~ShapeData() {
	}

	static Shape::Ptr ref(const cpShape* shape);
	static ShapeData* get(const cpShape* shape) {
		if (!shape)
			return nullptr;

		cpDataPointer data = cpShapeGetUserData(shape);

		return (ShapeData*)data;
	}
};

struct BodyData : public ReferencableData<Body::Ptr, Body::WeakPtr>, public ManagableData {
	lua_State* L = nullptr;
	Function::Ptr velocityHandler = nullptr;
	Function::Ptr positionHandler = nullptr;

	BodyData(Body::Ptr &self_, lua_State* L_) : ReferencableData(self_), L(L_) {
	}
	~BodyData() {
	}

	static Body::Ptr ref(const cpBody* body);
	static BodyData* get(const cpBody* body) {
		if (!body)
			return nullptr;

		cpDataPointer data = cpBodyGetUserData(body);

		return (BodyData*)data;
	}

	static void onVelocityUpdating(cpBody* body, cpVect gravity, cpFloat damping, cpFloat delta) {
		BodyData* data = (BodyData*)get(body);
		if (!data->velocityHandler)
			return;

		const Body::Ptr arg1 = BodyData::ref(body);
		const cpVect &arg2 = gravity;
		const cpFloat arg3 = damping;
		const cpFloat arg4 = delta;
		ScriptingLua::check(data->L, call(data->L, *data->velocityHandler, &arg1, arg2, arg3, arg4));
	}
	static void onPositionUpdating(cpBody* body, cpFloat delta) {
		BodyData* data = (BodyData*)get(body);
		if (!data->positionHandler)
			return;

		const Body::Ptr arg1 = BodyData::ref(body);
		const cpFloat arg2 = delta;
		ScriptingLua::check(data->L, call(data->L, *data->positionHandler, &arg1, arg2));
	}
};

struct SpaceData : public ReferencableData<Space::Ptr, Space::WeakPtr> {
	typedef Cache<cpShape, Shape::Ptr> ShapeCache;
	typedef Cache<cpBody, Body::Ptr> BodyCache;
	typedef Cache<cpConstraint, Constraint::Ptr> ConstraintCache;

	typedef std::map<void*, Function::Ptr> StepHandlerDictionary;
	struct CollisionKey {
		cpCollisionType first = 0;
		cpCollisionType second = 0;

		CollisionKey() {
		}
		CollisionKey(cpCollisionType _1st, cpCollisionType _2nd) : first(_1st), second(_2nd) {
		}

		bool operator < (const CollisionKey &other) const {
			return compare(other) < 0;
		}

		int compare(const CollisionKey &other) const {
			if (first < other.first)
				return -1;
			else if (first > other.first)
				return 1;

			if (second < other.second)
				return -1;
			else if (second > other.second)
				return 1;

			return 0;
		}
	};
	struct CollisionHandler {
		Function::Ptr beginHandler = nullptr;
		Function::Ptr preSolveHandler = nullptr;
		Function::Ptr postSolveHandler = nullptr;
		Function::Ptr separateHandler = nullptr;

		CollisionHandler() {
		}
		CollisionHandler(Function::Ptr began, Function::Ptr preSolved, Function::Ptr postSolved, Function::Ptr separated) :
			beginHandler(began),
			preSolveHandler(preSolved),
			postSolveHandler(postSolved),
			separateHandler(separated)
		{
		}

		void clear(void) {
			beginHandler = nullptr;
			preSolveHandler = nullptr;
			postSolveHandler = nullptr;
			separateHandler = nullptr;
		}
	};
	typedef std::map<CollisionKey, CollisionHandler> CollisionHandlerDictionary;

	lua_State* L = nullptr;
	bool quitting = false;
	bool calling = false;
	bool querying = false;

	ShapeCache shapeCache;
	BodyCache bodyCache;
	ConstraintCache constraintCache;
	bool obsoleteCollectEnabled = true;
	int obsoleteCollectThreshold = 1000;
	int obsoleteObjectCount = 0;

	StepHandlerDictionary postStepHandlers;
	CollisionHandler defaultHandler;
	CollisionHandlerDictionary handlers;
	CollisionHandlerDictionary wildcardHandlers;
	CollisionKey key;

	SpaceData(Space::Ptr &self_, lua_State* L_) : ReferencableData(self_), L(L_) {
	}
	~SpaceData() {
	}

	static Space::Ptr ref(const cpSpace* space);
	static SpaceData* get(const cpSpace* space) {
		if (!space)
			return nullptr;

		cpDataPointer data = cpSpaceGetUserData(space);

		return (SpaceData*)data;
	}

	static void onPostStepped(cpSpace* space, void* key, void* /* userData */) {
		SpaceData* data = get(space);
		if (data->quitting)
			return;

		VariableGuard<decltype(data->calling)> guardCalling(&data->calling, data->calling, true);

		StepHandlerDictionary::iterator it = data->postStepHandlers.find(key);
		if (it == data->postStepHandlers.end())
			return;

		const Space::Ptr arg1 = SpaceData::ref(space);
		ScriptingLua::check(data->L, call(data->L, *it->second, &arg1));

		data->postStepHandlers.erase(it);
	}
	static cpBool onDefaultCollisionBegan(cpArbiter* arbiter, cpSpace* space, cpDataPointer userData) {
		SpaceData* data = SpaceData::get(space);
		if (data->quitting)
			return cpFalse;

		VariableGuard<decltype(data->calling)> guardCalling(&data->calling, data->calling, true);

		if (!data->defaultHandler.beginHandler)
			return Space_alwaysCollide(arbiter, space, userData);

		bool ret = false;
		const Space::Ptr arg1 = SpaceData::ref(space);
		LUA_ARTIBER_WEAK_PTR(arbiter, arg2)
		ScriptingLua::check(data->L, call(ret, data->L, *data->defaultHandler.beginHandler, &arg1, &arg2));

		return ret ? cpTrue : cpFalse;
	}
	static cpBool onDefaultCollisionPreSolved(cpArbiter* arbiter, cpSpace* space, cpDataPointer userData) {
		SpaceData* data = SpaceData::get(space);
		if (data->quitting)
			return cpFalse;

		VariableGuard<decltype(data->calling)> guardCalling(&data->calling, data->calling, true);

		if (!data->defaultHandler.preSolveHandler)
			return Space_alwaysCollide(arbiter, space, userData);

		bool ret = false;
		const Space::Ptr arg1 = SpaceData::ref(space);
		LUA_ARTIBER_WEAK_PTR(arbiter, arg2)
		ScriptingLua::check(data->L, call(ret, data->L, *data->defaultHandler.preSolveHandler, &arg1, &arg2));

		return ret ? cpTrue : cpFalse;
	}
	static void onDefaultCollisionPostSolved(cpArbiter* arbiter, cpSpace* space, cpDataPointer userData) {
		SpaceData* data = SpaceData::get(space);
		if (data->quitting)
			return;

		VariableGuard<decltype(data->calling)> guardCalling(&data->calling, data->calling, true);

		if (!data->defaultHandler.postSolveHandler) {
			Space_doNothing(arbiter, space, userData);

			return;
		}

		const Space::Ptr arg1 = SpaceData::ref(space);
		LUA_ARTIBER_WEAK_PTR(arbiter, arg2)
		ScriptingLua::check(data->L, call(data->L, *data->defaultHandler.postSolveHandler, &arg1, &arg2));
	}
	static void onDefaultCollisionSeparated(cpArbiter* arbiter, cpSpace* space, cpDataPointer userData) {
		SpaceData* data = SpaceData::get(space);
		if (data->quitting)
			return;

		VariableGuard<decltype(data->calling)> guardCalling(&data->calling, data->calling, true);

		if (!data->defaultHandler.separateHandler) {
			Space_doNothing(arbiter, space, userData);

			return;
		}

		const Space::Ptr arg1 = SpaceData::ref(space);
		LUA_ARTIBER_WEAK_PTR(arbiter, arg2)
		ScriptingLua::check(data->L, call(data->L, *data->defaultHandler.separateHandler, &arg1, &arg2));
	}

	static cpBool onCollisionBegan(cpArbiter* arbiter, cpSpace* space, cpDataPointer userData) {
		SpaceData* data = SpaceData::get(space);
		if (data->quitting)
			return cpFalse;

		VariableGuard<decltype(data->calling)> guardCalling(&data->calling, data->calling, true);

		cpShape* shapeA = nullptr;
		cpShape* shapeB = nullptr;
		cpArbiterGetShapes(arbiter, &shapeA, &shapeB);
		const SpaceData::CollisionKey key(cpShapeGetCollisionType(shapeA), cpShapeGetCollisionType(shapeB));

		CollisionHandlerDictionary::const_iterator it = data->handlers.find(key);
		if (it == data->handlers.end())
			return Space_defaultBegin(arbiter, space, userData);
		const CollisionHandler &handler = it->second;
		if (!handler.beginHandler)
			return Space_defaultBegin(arbiter, space, userData);

		bool ret = false;
		const Space::Ptr arg1 = SpaceData::ref(space);
		LUA_ARTIBER_WEAK_PTR(arbiter, arg2)
		ScriptingLua::check(data->L, call(ret, data->L, *handler.beginHandler, &arg1, &arg2));

		return ret ? cpTrue : cpFalse;
	}
	static cpBool onCollisionPreSolved(cpArbiter* arbiter, cpSpace* space, cpDataPointer userData) {
		SpaceData* data = SpaceData::get(space);
		if (data->quitting)
			return cpFalse;

		VariableGuard<decltype(data->calling)> guardCalling(&data->calling, data->calling, true);

		cpShape* shapeA = nullptr;
		cpShape* shapeB = nullptr;
		cpArbiterGetShapes(arbiter, &shapeA, &shapeB);
		const SpaceData::CollisionKey key(cpShapeGetCollisionType(shapeA), cpShapeGetCollisionType(shapeB));

		CollisionHandlerDictionary::const_iterator it = data->handlers.find(key);
		if (it == data->handlers.end())
			return Space_defaultPreSolve(arbiter, space, userData);
		const CollisionHandler &handler = it->second;
		if (!handler.preSolveHandler)
			return Space_defaultPreSolve(arbiter, space, userData);

		bool ret = false;
		const Space::Ptr arg1 = SpaceData::ref(space);
		LUA_ARTIBER_WEAK_PTR(arbiter, arg2)
		ScriptingLua::check(data->L, call(ret, data->L, *handler.preSolveHandler, &arg1, &arg2));

		return ret ? cpTrue : cpFalse;
	}
	static void onCollisionPostSolved(cpArbiter* arbiter, cpSpace* space, cpDataPointer userData) {
		SpaceData* data = SpaceData::get(space);
		if (data->quitting)
			return;

		VariableGuard<decltype(data->calling)> guardCalling(&data->calling, data->calling, true);

		cpShape* shapeA = nullptr;
		cpShape* shapeB = nullptr;
		cpArbiterGetShapes(arbiter, &shapeA, &shapeB);
		const SpaceData::CollisionKey key(cpShapeGetCollisionType(shapeA), cpShapeGetCollisionType(shapeB));

		CollisionHandlerDictionary::const_iterator it = data->handlers.find(key);
		if (it == data->handlers.end()) {
			Space_defaultPostSolve(arbiter, space, userData);

			return;
		}
		const CollisionHandler &handler = it->second;
		if (!handler.postSolveHandler) {
			Space_defaultPostSolve(arbiter, space, userData);

			return;
		}

		const Space::Ptr arg1 = SpaceData::ref(space);
		LUA_ARTIBER_WEAK_PTR(arbiter, arg2)
		ScriptingLua::check(data->L, call(data->L, *handler.postSolveHandler, &arg1, &arg2));
	}
	static void onCollisionSeparated(cpArbiter* arbiter, cpSpace* space, cpDataPointer userData) {
		SpaceData* data = SpaceData::get(space);
		if (data->quitting)
			return;

		VariableGuard<decltype(data->calling)> guardCalling(&data->calling, data->calling, true);

		cpShape* shapeA = nullptr;
		cpShape* shapeB = nullptr;
		cpArbiterGetShapes(arbiter, &shapeA, &shapeB);
		const SpaceData::CollisionKey key(cpShapeGetCollisionType(shapeA), cpShapeGetCollisionType(shapeB));

		CollisionHandlerDictionary::const_iterator it = data->handlers.find(key);
		if (it == data->handlers.end()) {
			Space_defaultSeparate(arbiter, space, userData);

			return;
		}
		const CollisionHandler &handler = it->second;
		if (!handler.separateHandler) {
			Space_defaultSeparate(arbiter, space, userData);

			return;
		}

		const Space::Ptr arg1 = SpaceData::ref(space);
		LUA_ARTIBER_WEAK_PTR(arbiter, arg2)
		ScriptingLua::check(data->L, call(data->L, *handler.separateHandler, &arg1, &arg2));
	}

	static cpBool onWildcardBegan(cpArbiter* arbiter, cpSpace* space, cpDataPointer userData) {
		SpaceData* data = SpaceData::get(space);
		if (data->quitting)
			return cpFalse;

		VariableGuard<decltype(data->calling)> guardCalling(&data->calling, data->calling, true);

		cpShape* shapeA = nullptr;
		cpShape* shapeB = nullptr;
		cpArbiterGetShapes(arbiter, &shapeA, &shapeB);
		const SpaceData::CollisionKey key(cpShapeGetCollisionType(shapeA), CP_WILDCARD_COLLISION_TYPE);

		CollisionHandlerDictionary::const_iterator it = data->wildcardHandlers.find(key);
		if (it == data->wildcardHandlers.end())
			return Space_alwaysCollide(arbiter, space, userData);
		const CollisionHandler &handler = it->second;
		if (!handler.beginHandler)
			return Space_alwaysCollide(arbiter, space, userData);

		bool ret = false;
		const Space::Ptr arg1 = SpaceData::ref(space);
		LUA_ARTIBER_WEAK_PTR(arbiter, arg2)
		ScriptingLua::check(data->L, call(ret, data->L, *handler.beginHandler, &arg1, &arg2));

		return ret ? cpTrue : cpFalse;
	}
	static cpBool onWildcardPreSolved(cpArbiter* arbiter, cpSpace* space, cpDataPointer userData) {
		SpaceData* data = SpaceData::get(space);
		if (data->quitting)
			return cpFalse;

		VariableGuard<decltype(data->calling)> guardCalling(&data->calling, data->calling, true);

		cpShape* shapeA = nullptr;
		cpShape* shapeB = nullptr;
		cpArbiterGetShapes(arbiter, &shapeA, &shapeB);
		const SpaceData::CollisionKey key(cpShapeGetCollisionType(shapeA), CP_WILDCARD_COLLISION_TYPE);

		CollisionHandlerDictionary::const_iterator it = data->wildcardHandlers.find(key);
		if (it == data->wildcardHandlers.end())
			return Space_alwaysCollide(arbiter, space, userData);
		const CollisionHandler &handler = it->second;
		if (!handler.preSolveHandler)
			return Space_alwaysCollide(arbiter, space, userData);

		bool ret = false;
		const Space::Ptr arg1 = SpaceData::ref(space);
		LUA_ARTIBER_WEAK_PTR(arbiter, arg2)
		ScriptingLua::check(data->L, call(ret, data->L, *handler.preSolveHandler, &arg1, &arg2));

		return ret ? cpTrue : cpFalse;
	}
	static void onWildcardPostSolved(cpArbiter* arbiter, cpSpace* space, cpDataPointer userData) {
		SpaceData* data = SpaceData::get(space);
		if (data->quitting)
			return;

		VariableGuard<decltype(data->calling)> guardCalling(&data->calling, data->calling, true);

		cpShape* shapeA = nullptr;
		cpShape* shapeB = nullptr;
		cpArbiterGetShapes(arbiter, &shapeA, &shapeB);
		const SpaceData::CollisionKey key(cpShapeGetCollisionType(shapeA), CP_WILDCARD_COLLISION_TYPE);

		CollisionHandlerDictionary::const_iterator it = data->wildcardHandlers.find(key);
		if (it == data->wildcardHandlers.end()) {
			Space_doNothing(arbiter, space, userData);

			return;
		}
		const CollisionHandler &handler = it->second;
		if (!handler.postSolveHandler) {
			Space_doNothing(arbiter, space, userData);

			return;
		}

		const Space::Ptr arg1 = SpaceData::ref(space);
		LUA_ARTIBER_WEAK_PTR(arbiter, arg2)
		ScriptingLua::check(data->L, call(data->L, *handler.postSolveHandler, &arg1, &arg2));
	}
	static void onWildcardSeparated(cpArbiter* arbiter, cpSpace* space, cpDataPointer userData) {
		SpaceData* data = SpaceData::get(space);
		if (data->quitting)
			return;

		VariableGuard<decltype(data->calling)> guardCalling(&data->calling, data->calling, true);

		cpShape* shapeA = nullptr;
		cpShape* shapeB = nullptr;
		cpArbiterGetShapes(arbiter, &shapeA, &shapeB);
		const SpaceData::CollisionKey key(cpShapeGetCollisionType(shapeA), CP_WILDCARD_COLLISION_TYPE);

		CollisionHandlerDictionary::const_iterator it = data->wildcardHandlers.find(key);
		if (it == data->wildcardHandlers.end()) {
			Space_doNothing(arbiter, space, userData);

			return;
		}
		const CollisionHandler &handler = it->second;
		if (!handler.separateHandler) {
			Space_doNothing(arbiter, space, userData);

			return;
		}

		const Space::Ptr arg1 = SpaceData::ref(space);
		LUA_ARTIBER_WEAK_PTR(arbiter, arg2)
		ScriptingLua::check(data->L, call(data->L, *handler.separateHandler, &arg1, &arg2));
	}
};

struct ConstraintData : public ReferencableData<Constraint::Ptr, Constraint::WeakPtr>, public ManagableData {
	enum Types {
		INVALID,
		DAMPED_ROTARY_SPRING,
		DAMPED_SPRING,
		GEAR_JOINT,
		GROOVE_JOINT,
		PIN_JOINT,
		PIVOT_JOINT,
		RATCHET_JOINT,
		ROTARY_LIMIT_JOINT,
		SIMPLE_MOTOR,
		SLIDE_JOINT
	};

	lua_State* L = nullptr;
	Function::Ptr preSolveHandler = nullptr;
	Function::Ptr postSolveHandler = nullptr;

	ConstraintData(Constraint::Ptr &self_, lua_State* L_) : ReferencableData(self_), L(L_) {
	}
	~ConstraintData() {
	}

	static Constraint::Ptr ref(const cpConstraint* constraint);
	static ConstraintData* get(const cpConstraint* constraint) {
		if (!constraint)
			return nullptr;

		cpDataPointer data = cpConstraintGetUserData(constraint);

		return (ConstraintData*)data;
	}

	static void onPreSolved(cpConstraint* constraint, cpSpace* space) {
		SpaceData* data_ = SpaceData::get(space);
		if (data_->quitting)
			return;

		ConstraintData* data = get(constraint);
		if (!data->preSolveHandler)
			return;

		const Space::Ptr arg1 = SpaceData::ref(space);
		const Constraint::Ptr arg2(
			constraint,
			Constraint_dtor
		);
		ScriptingLua::check(data->L, call(data->L, *data->preSolveHandler, &arg1, &arg2));
	}
	static void onPostSolved(cpConstraint* constraint, cpSpace* space) {
		SpaceData* data_ = SpaceData::get(space);
		if (data_->quitting)
			return;

		ConstraintData* data = get(constraint);
		if (!data->postSolveHandler)
			return;

		const Space::Ptr arg1 = SpaceData::ref(space);
		const Constraint::Ptr arg2(
			constraint,
			Constraint_dtor
		);
		ScriptingLua::check(data->L, call(data->L, *data->postSolveHandler, &arg1, &arg2));
	}
};

struct QueryData {
	lua_State* L = nullptr;
	int* ret = nullptr;
	Function::Ptr* callback = nullptr;
	void* collection = nullptr;
	cpSpace* space = nullptr;

	QueryData(lua_State* L_, int* ret_, Function::Ptr* callback_, void* collection_, cpSpace* space_) : L(L_), ret(ret_), callback(callback_), collection(collection_), space(space_) {
	}
};

struct IterationData {
	lua_State* L = nullptr;
	int* ret = nullptr;
	Function::Ptr* callback = nullptr;
	void* collection = nullptr;

	IterationData(lua_State* L_, int* ret_, Function::Ptr* callback_, void* collection_) : L(L_), ret(ret_), callback(callback_), collection(collection_) {
	}
};

Shape::Ptr ShapeData::ref(const cpShape* shape) {
	if (!shape)
		return nullptr;

	ShapeData* shapeData = ShapeData::get(shape);
	if (shapeData) {
		if (!shapeData->self.expired())
			return shapeData->self.lock();
	}

	cpSpace* space = cpShapeGetSpace(shape);
	if (!space)
		return nullptr;

	SpaceData* spaceData = SpaceData::get(space);
	if (!spaceData)
		return nullptr;

	Shape::Ptr* shapePtr = spaceData->shapeCache.find(shape);
	if (!shapePtr)
		return nullptr;

	return *shapePtr;
}

Body::Ptr BodyData::ref(const cpBody* body) {
	if (!body)
		return nullptr;

	BodyData* bodyData = BodyData::get(body);
	if (bodyData) {
		if (!bodyData->self.expired())
			return bodyData->self.lock();
	}

	cpSpace* space = cpBodyGetSpace(body);
	if (!space)
		return nullptr;

	SpaceData* spaceData = SpaceData::get(space);
	if (!spaceData)
		return nullptr;

	Body::Ptr* bodyPtr = spaceData->bodyCache.find(body);
	if (!bodyPtr)
		return nullptr;

	return *bodyPtr;
}

Constraint::Ptr ConstraintData::ref(const cpConstraint* constraint) {
	if (!constraint)
		return nullptr;

	ConstraintData* constraintData = ConstraintData::get(constraint);
	if (constraintData) {
		if (!constraintData->self.expired())
			return constraintData->self.lock();
	}

	cpSpace* space = cpConstraintGetSpace(constraint);
	if (!space)
		return nullptr;

	SpaceData* spaceData = SpaceData::get(space);
	if (!spaceData)
		return nullptr;

	Constraint::Ptr* constraintPtr = spaceData->constraintCache.find(constraint);
	if (!constraintPtr)
		return nullptr;

	return *constraintPtr;
}

Space::Ptr SpaceData::ref(const cpSpace* space) {
	if (!space)
		return nullptr;

	SpaceData* spaceData = SpaceData::get(space);
	if (spaceData) {
		if (!spaceData->self.expired())
			return spaceData->self.lock();
	}

	return nullptr;
}

/**< Resetter. */

template<typename Raw, typename Ptr> bool collectOne(Raw* key, Ptr &val, std::function<bool(Raw*)> func) {
	if (!func(key))
		return false;

	const long n = val.use_count();
	if (n <= 1) {
		val = nullptr;

		return true;
	}

	return false;
}

static void collect(cpSpace* space) {
	SpaceData* spaceData = SpaceData::get(space);
	spaceData->constraintCache.collect(
		std::bind(
			collectOne<cpConstraint, Constraint::Ptr>,
			std::placeholders::_1, std::placeholders::_2,
			[] (cpConstraint* constraint) -> bool {
				cpSpace* space = cpConstraintGetSpace(constraint);
				if (space)
					return false;

				return true;
			}
		)
	);
	spaceData->shapeCache.collect(
		std::bind(
			collectOne<cpShape, Shape::Ptr>,
			std::placeholders::_1, std::placeholders::_2,
			[] (cpShape* shape) -> bool {
				cpSpace* space = cpShapeGetSpace(shape);
				if (space)
					return false;

				return true;
			}
		)
	);
	spaceData->bodyCache.collect(
		std::bind(
			collectOne<cpBody, Body::Ptr>,
			std::placeholders::_1, std::placeholders::_2,
			[] (cpBody* body) -> bool {
				cpSpace* space = cpBodyGetSpace(body);
				if (space)
					return false;

				return true;
			}
		)
	);
}

}

}

namespace Lua {

namespace Engine {

/**< Transform. */

static void Transform_dtor(cpTransform* transform) {
	Transform::destroy(transform);
}

static int Transform_ctor(lua_State* L) {
	cpFloat a = 0;
	cpFloat b = 0;
	cpFloat c = 0;
	cpFloat d = 0;
	cpFloat tx = 0;
	cpFloat ty = 0;
	check<>(L, a, b, c, d, tx, ty);

	Transform::Ptr obj(
		Transform::create(a, b, c, d, tx, ty),
		Transform_dtor
	);

	return write(L, &obj);
}

static int Transform_ctorTranspose(lua_State* L) {
	cpFloat a = 0;
	cpFloat c = 0;
	cpFloat tx = 0;
	cpFloat b = 0;
	cpFloat d = 0;
	cpFloat ty = 0;
	check<>(L, a, c, tx, b, d, ty);

	Transform::Ptr obj(
		Transform::create(a, b, c, d, tx, ty),
		Transform_dtor
	);

	return write(L, &obj);
}

static int Transform_ctorTranslate(lua_State* L) {
	cpVect translate;
	check<>(L, translate);

	const cpTransform val = cpTransformTranslate(translate);
	Transform::Ptr obj(
		Transform::create(val),
		Transform_dtor
	);

	return write(L, &obj);
}

static int Transform_ctorScale(lua_State* L) {
	const int n = getTop(L);
	cpFloat scaleX = 0;
	cpFloat scaleY = 0;
	if (n >= 2)
		check<>(L, scaleX, scaleY);
	else
		check<>(L, scaleX);

	if (n == 1)
		scaleY = scaleX;

	const cpTransform val = cpTransformScale(scaleX, scaleY);
	Transform::Ptr obj(
		Transform::create(val),
		Transform_dtor
	);

	return write(L, &obj);
}

static int Transform_ctorRotate(lua_State* L) {
	cpFloat radians = 0;
	Math::Rotf* rot = nullptr;
	if (isNumber(L)) {
		check<>(L, radians);
	} else {
		check<>(L, rot);

		if (rot)
			radians = rot->angle();
	}

	const cpTransform val = cpTransformRotate(radians);
	Transform::Ptr obj(
		Transform::create(val),
		Transform_dtor
	);

	return write(L, &obj);
}

static int Transform_ctorRigid(lua_State* L) {
	cpVect translate;
	cpFloat radians = 0;
	check<>(L, translate, radians);

	const cpTransform val = cpTransformRigid(translate, radians);
	Transform::Ptr obj(
		Transform::create(val),
		Transform_dtor
	);

	return write(L, &obj);
}

static int Transform_ctorRigidInverse(lua_State* L) {
	Transform::Ptr* trans = nullptr;
	check<>(L, trans);

	if (trans) {
		const cpTransform val = cpTransformRigidInverse(*trans->get());
		Transform::Ptr obj(
			Transform::create(val),
			Transform_dtor
		);

		return write(L, &obj);
	}

	return write(L, nullptr);
}

static int Transform_ctorWrap(lua_State* L) {
	Transform::Ptr* outer = nullptr;
	Transform::Ptr* inner = nullptr;
	check<>(L, outer, inner);

	if (outer && inner) {
		const cpTransform val = cpTransformWrap(*outer->get(), *inner->get());
		Transform::Ptr obj(
			Transform::create(val),
			Transform_dtor
		);

		return write(L, &obj);
	}

	return write(L, nullptr);
}

static int Transform_ctorWrapInverse(lua_State* L) {
	Transform::Ptr* outer = nullptr;
	Transform::Ptr* inner = nullptr;
	check<>(L, outer, inner);

	if (outer && inner) {
		const cpTransform val = cpTransformWrapInverse(*outer->get(), *inner->get());
		Transform::Ptr obj(
			Transform::create(val),
			Transform_dtor
		);

		return write(L, &obj);
	}

	return write(L, nullptr);
}

static int Transform_ctorOrtho(lua_State* L) {
	cpBB bb;
	check<>(L, bb);

	const cpTransform val = cpTransformOrtho(bb);
	Transform::Ptr obj(
		Transform::create(val),
		Transform_dtor
	);

	return write(L, &obj);
}

static int Transform_ctorBoneScale(lua_State* L) {
	cpVect v0;
	cpVect v1;
	check<>(L, v0, v1);

	const cpTransform val = cpTransformBoneScale(v0, v1);
	Transform::Ptr obj(
		Transform::create(val),
		Transform_dtor
	);

	return write(L, &obj);
}

static int Transform_ctorAxialScale(lua_State* L) {
	cpVect axis;
	cpVect pivot;
	cpFloat scale = 0;
	check<>(L, axis, pivot, scale);

	const cpTransform val = cpTransformAxialScale(axis, pivot, scale);
	Transform::Ptr obj(
		Transform::create(val),
		Transform_dtor
	);

	return write(L, &obj);
}

static int Transform___mul(lua_State* L) {
	Transform::Ptr* obj = nullptr;
	Transform::Ptr* other = nullptr;
	check<>(L, obj, other);

	if (obj && obj->get() && other && other->get()) {
		const cpTransform val = cpTransformMult(*obj->get(), *other->get());
		Transform::Ptr ret(
			Transform::create(val),
			Transform_dtor
		);

		return write(L, &ret);
	}

	return 0;
}

static int Transform_transformPoint(lua_State* L) {
	Transform::Ptr* obj = nullptr;
	cpVect p;
	read<>(L, obj, p);

	if (obj && obj->get()) {
		const cpVect ret = cpTransformPoint(*obj->get(), p);

		return write(L, ret);
	}

	return write(L, nullptr);
}

static int Transform_transformVector(lua_State* L) {
	Transform::Ptr* obj = nullptr;
	cpVect p;
	read<>(L, obj, p);

	if (obj && obj->get()) {
		const cpVect ret = cpTransformVect(*obj->get(), p);

		return write(L, ret);
	}

	return write(L, nullptr);
}

static int Transform_transformBoundingBox(lua_State* L) {
	Transform::Ptr* obj = nullptr;
	cpBB bb;
	read<>(L, obj, bb);

	if (obj && obj->get()) {
		const cpBB ret = cpTransformbBB(*obj->get(), bb);

		return write(L, ret);
	}

	return write(L, nullptr);
}

static int Transform___index(lua_State* L) {
	Transform::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "a") == 0) {
		const cpFloat ret = obj->get()->a;

		return write(L, ret);
	} else if (strcmp(field, "b") == 0) {
		const cpFloat ret = obj->get()->b;

		return write(L, ret);
	} else if (strcmp(field, "c") == 0) {
		const cpFloat ret = obj->get()->c;

		return write(L, ret);
	} else if (strcmp(field, "d") == 0) {
		const cpFloat ret = obj->get()->d;

		return write(L, ret);
	} else if (strcmp(field, "tx") == 0) {
		const cpFloat ret = obj->get()->tx;

		return write(L, ret);
	} else if (strcmp(field, "ty") == 0) {
		const cpFloat ret = obj->get()->ty;

		return write(L, ret);
	} else if (strcmp(field, "inversed") == 0) {
		const cpTransform val = cpTransformInverse(*obj->get());
		const Transform::Ptr ret(
			Transform::create(val),
			Transform_dtor
		);

		return write(L, &ret);
	} else {
		return __index(L, field);
	}
}

static int Transform___newindex(lua_State* L) {
	Transform::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "a") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		obj->get()->a = val;
	} else if (strcmp(field, "b") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		obj->get()->b = val;
	} else if (strcmp(field, "c") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		obj->get()->c = val;
	} else if (strcmp(field, "d") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		obj->get()->d = val;
	} else if (strcmp(field, "tx") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		obj->get()->tx = val;
	} else if (strcmp(field, "ty") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		obj->get()->ty = val;
	}

	return 0;
}

static void open_Transform(lua_State* L) {
	def(
		L, "Transform",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Transform_ctor },
				luaL_Reg{ "newTranspose", Transform_ctorTranspose },
				luaL_Reg{ "newTranslate", Transform_ctorTranslate },
				luaL_Reg{ "newScale", Transform_ctorScale },
				luaL_Reg{ "newRotate", Transform_ctorRotate },
				luaL_Reg{ "newRigid", Transform_ctorRigid },
				luaL_Reg{ "newRigidInverse", Transform_ctorRigidInverse },
				luaL_Reg{ "newWrap", Transform_ctorWrap },
				luaL_Reg{ "newWrapInverse", Transform_ctorWrapInverse },
				luaL_Reg{ "newOrtho", Transform_ctorOrtho },
				luaL_Reg{ "newBoneScale", Transform_ctorBoneScale },
				luaL_Reg{ "newAxialScale", Transform_ctorAxialScale },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Transform::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Transform::Ptr> },
			luaL_Reg{ "__mul", Transform___mul },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "transformPoint", Transform_transformPoint },
			luaL_Reg{ "transformVector", Transform_transformVector },
			luaL_Reg{ "transformBoundingBox", Transform_transformBoundingBox },
			luaL_Reg{ nullptr, nullptr }
		),
		Transform___index, Transform___newindex
	);

	getGlobal(L, "Transform");
	{
		const Transform::Ptr identity(
			Transform::create(cpTransformIdentity),
			Transform_dtor
		);
		if (identity)
			setTable(L, "Identity", &identity);

		setTable(
			L,
			"__name", "Transform"
		);
	}
	pop(L);
}

/**< Arbiter. */

static void Arbiter_dtor(cpArbiter* arbiter) {
	(void)arbiter;
}

static int Arbiter___len(lua_State* L) {
	Arbiter::WeakPtr* obj = nullptr;
	check<>(L, obj);

	Arbiter::Ptr ptr;
	if (obj) {
		ptr = obj->lock();
	}
	if (!ptr.get()) {
		error(L, "The arbiter is expired.");

		return 0;
	}
	if (ptr.get()) {
		const int ret = cpArbiterGetCount(ptr.get());

		return write(L, ret);
	}

	return 0;
}

static int Arbiter_ignore(lua_State* L) {
	Arbiter::WeakPtr* obj = nullptr;
	read<>(L, obj);

	Arbiter::Ptr ptr;
	if (obj) {
		ptr = obj->lock();
	}
	if (!ptr.get()) {
		error(L, "The arbiter is expired.");

		return 0;
	}
	if (ptr.get()) {
		const bool ret = !!cpArbiterIgnore(ptr.get());

		return write(L, ret);
	}

	return write(L, false);
}

static int Arbiter_getShapes(lua_State* L) {
	Arbiter::WeakPtr* obj = nullptr;
	read<>(L, obj);

	Arbiter::Ptr ptr;
	if (obj) {
		ptr = obj->lock();
	}
	if (!ptr.get()) {
		error(L, "The arbiter is expired.");

		return 0;
	}
	if (ptr.get()) {
		cpShape* shapeA = nullptr;
		cpShape* shapeB = nullptr;
		cpArbiterGetShapes(ptr.get(), &shapeA, &shapeB);
		Shape::Ptr ret1 = nullptr;
		if (shapeA)
			ret1 = ShapeData::ref(shapeA);
		Shape::Ptr ret2 = nullptr;
		if (shapeB)
			ret2 = ShapeData::ref(shapeB);

		return write(L, &ret1, &ret2);
	}

	return write(L, nullptr, nullptr);
}

static int Arbiter_getBodies(lua_State* L) {
	Arbiter::WeakPtr* obj = nullptr;
	read<>(L, obj);

	Arbiter::Ptr ptr;
	if (obj) {
		ptr = obj->lock();
	}
	if (!ptr.get()) {
		error(L, "The arbiter is expired.");

		return 0;
	}
	if (ptr.get()) {
		cpBody* bodyA = nullptr;
		cpBody* bodyB = nullptr;
		cpArbiterGetBodies(ptr.get(), &bodyA, &bodyB);
		Body::Ptr ret1 = nullptr;
		if (bodyA)
			ret1 = BodyData::ref(bodyA);
		Body::Ptr ret2 = nullptr;
		if (bodyB)
			ret2 = BodyData::ref(bodyB);

		return write(L, &ret1, &ret2);
	}

	return write(L, nullptr, nullptr);
}

static int Arbiter_callWildcardBeginA(lua_State* L) {
	Arbiter::WeakPtr* obj = nullptr;
	Space::Ptr* space = nullptr;
	read<>(L, obj, space);

	Arbiter::Ptr ptr;
	if (obj) {
		ptr = obj->lock();
	}
	if (!ptr.get()) {
		error(L, "The arbiter is expired.");

		return 0;
	}
	if (ptr.get() && space && space->get()) {
		const bool ret = !!cpArbiterCallWildcardBeginA(ptr.get(), space->get());

		return write(L, ret);
	}

	return write(L, false);
}

static int Arbiter_callWildcardBeginB(lua_State* L) {
	Arbiter::WeakPtr* obj = nullptr;
	Space::Ptr* space = nullptr;
	read<>(L, obj, space);

	Arbiter::Ptr ptr;
	if (obj) {
		ptr = obj->lock();
	}
	if (!ptr.get()) {
		error(L, "The arbiter is expired.");

		return 0;
	}
	if (ptr.get() && space && space->get()) {
		const bool ret = !!cpArbiterCallWildcardBeginB(ptr.get(), space->get());

		return write(L, ret);
	}

	return write(L, false);
}

static int Arbiter_callWildcardPreSolveA(lua_State* L) {
	Arbiter::WeakPtr* obj = nullptr;
	Space::Ptr* space = nullptr;
	read<>(L, obj, space);

	Arbiter::Ptr ptr;
	if (obj) {
		ptr = obj->lock();
	}
	if (!ptr.get()) {
		error(L, "The arbiter is expired.");

		return 0;
	}
	if (ptr.get() && space && space->get()) {
		const bool ret = !!cpArbiterCallWildcardPreSolveA(ptr.get(), space->get());

		return write(L, ret);
	}

	return write(L, false);
}

static int Arbiter_callWildcardPreSolveB(lua_State* L) {
	Arbiter::WeakPtr* obj = nullptr;
	Space::Ptr* space = nullptr;
	read<>(L, obj, space);

	Arbiter::Ptr ptr;
	if (obj) {
		ptr = obj->lock();
	}
	if (!ptr.get()) {
		error(L, "The arbiter is expired.");

		return 0;
	}
	if (ptr.get() && space && space->get()) {
		const bool ret = !!cpArbiterCallWildcardPreSolveB(ptr.get(), space->get());

		return write(L, ret);
	}

	return write(L, false);
}

static int Arbiter_callWildcardPostSolveA(lua_State* L) {
	Arbiter::WeakPtr* obj = nullptr;
	Space::Ptr* space = nullptr;
	read<>(L, obj, space);

	Arbiter::Ptr ptr;
	if (obj) {
		ptr = obj->lock();
	}
	if (!ptr.get()) {
		error(L, "The arbiter is expired.");

		return 0;
	}
	if (ptr.get() && space && space->get())
		cpArbiterCallWildcardPostSolveA(ptr.get(), space->get());

	return 0;
}

static int Arbiter_callWildcardPostSolveB(lua_State* L) {
	Arbiter::WeakPtr* obj = nullptr;
	Space::Ptr* space = nullptr;
	read<>(L, obj, space);

	Arbiter::Ptr ptr;
	if (obj) {
		ptr = obj->lock();
	}
	if (!ptr.get()) {
		error(L, "The arbiter is expired.");

		return 0;
	}
	if (ptr.get() && space && space->get())
		cpArbiterCallWildcardPostSolveB(ptr.get(), space->get());

	return 0;
}

static int Arbiter_callWildcardSeparateA(lua_State* L) {
	Arbiter::WeakPtr* obj = nullptr;
	Space::Ptr* space = nullptr;
	read<>(L, obj, space);

	Arbiter::Ptr ptr;
	if (obj) {
		ptr = obj->lock();
	}
	if (!ptr.get()) {
		error(L, "The arbiter is expired.");

		return 0;
	}
	if (ptr.get() && space && space->get())
		cpArbiterCallWildcardSeparateA(ptr.get(), space->get());

	return 0;
}

static int Arbiter_callWildcardSeparateB(lua_State* L) {
	Arbiter::WeakPtr* obj = nullptr;
	Space::Ptr* space = nullptr;
	read<>(L, obj, space);

	Arbiter::Ptr ptr;
	if (obj) {
		ptr = obj->lock();
	}
	if (!ptr.get()) {
		error(L, "The arbiter is expired.");

		return 0;
	}
	if (ptr.get() && space && space->get())
		cpArbiterCallWildcardSeparateB(ptr.get(), space->get());

	return 0;
}

static int Arbiter___index(lua_State* L) {
	Arbiter::WeakPtr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	Arbiter::Ptr ptr;
	if (obj) {
		ptr = obj->lock();
	}
	if (!ptr.get()) {
		error(L, "The arbiter is expired.");

		return 0;
	}
	if (!ptr.get() || !field)
		return 0;

	if (strcmp(field, "id") == 0) {
		const uintptr_t ret = (uintptr_t)ptr.get();

		return write(L, ret);
	} else if (strcmp(field, "restitution") == 0) {
		const cpFloat ret = cpArbiterGetRestitution(ptr.get());

		return write(L, ret);
	} else if (strcmp(field, "friction") == 0) {
		const cpFloat ret = cpArbiterGetFriction(ptr.get());

		return write(L, ret);
	} else if (strcmp(field, "surfaceVelocity") == 0) {
		const cpVect ret = cpArbiterGetSurfaceVelocity(ptr.get());

		return write(L, ret);
	} else if (strcmp(field, "totalImpulse") == 0) {
		const cpVect ret = cpArbiterTotalImpulse(ptr.get());

		return write(L, ret);
	} else if (strcmp(field, "totalKineticEnergy") == 0) {
		const cpFloat ret = cpArbiterTotalKE(ptr.get());

		return write(L, ret);
	} else if (strcmp(field, "contacts") == 0) {
		const cpContactPointSet points = cpArbiterGetContactPointSet(ptr.get());
		const Shape::Ptr arg1 = nullptr;
		const cpVect &arg2 = points.normal;
		const int arg3 = points.count;
		const cpVect arg4 = points.count >= 1 ? points.points[0].pointA : cpVect{ 0, 0 };
		const cpVect arg5 = points.count >= 1 ? points.points[0].pointB : cpVect{ 0, 0 };
		const cpFloat arg6 = points.count >= 1 ? points.points[0].distance : 0;
		const cpVect arg7 = points.count >= 2 ? points.points[1].pointA : cpVect{ 0, 0 };
		const cpVect arg8 = points.count >= 2 ? points.points[1].pointB : cpVect{ 0, 0 };
		const cpFloat arg9 = points.count >= 2 ? points.points[1].distance : 0;
		const Contacts::Ptr ret = Contacts::Ptr(new Contacts(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9));

		return write(L, &ret);
	} else if (strcmp(field, "isFirstContact") == 0) {
		const bool ret = !!cpArbiterIsFirstContact(ptr.get());

		return write(L, ret);
	} else if (strcmp(field, "isRemoval") == 0) {
		const bool ret = !!cpArbiterIsRemoval(ptr.get());

		return write(L, ret);
	} else if (strcmp(field, "normal") == 0) {
		const cpVect ret = cpArbiterGetNormal(ptr.get());

		return write(L, ret);
	} else if (strcmp(field, "point1A") == 0) {
		const int n = cpArbiterGetCount(ptr.get());
		if (n >= 1) {
			const cpVect ret = cpArbiterGetPointA(ptr.get(), 0);

			return write(L, ret);
		}

		return write(L, nullptr);
	} else if (strcmp(field, "point1B") == 0) {
		const int n = cpArbiterGetCount(ptr.get());
		if (n >= 1) {
			const cpVect ret = cpArbiterGetPointB(ptr.get(), 0);

			return write(L, ret);
		}

		return write(L, nullptr);
	} else if (strcmp(field, "depth1") == 0) {
		const int n = cpArbiterGetCount(ptr.get());
		if (n >= 1) {
			const cpFloat ret = cpArbiterGetDepth(ptr.get(), 0);

			return write(L, ret);
		}

		return write(L, nullptr);
	} else if (strcmp(field, "point2A") == 0) {
		const int n = cpArbiterGetCount(ptr.get());
		if (n >= 2) {
			const cpVect ret = cpArbiterGetPointA(ptr.get(), 1);

			return write(L, ret);
		}

		return write(L, nullptr);
	} else if (strcmp(field, "point2B") == 0) {
		const int n = cpArbiterGetCount(ptr.get());
		if (n >= 2) {
			const cpVect ret = cpArbiterGetPointB(ptr.get(), 1);

			return write(L, ret);
		}

		return write(L, nullptr);
	} else if (strcmp(field, "depth2") == 0) {
		const int n = cpArbiterGetCount(ptr.get());
		if (n >= 2) {
			const cpFloat ret = cpArbiterGetDepth(ptr.get(), 1);

			return write(L, ret);
		}

		return write(L, nullptr);
	} else {
		return __index(L, field);
	}
}

static int Arbiter___newindex(lua_State* L) {
	Arbiter::WeakPtr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	Arbiter::Ptr ptr;
	if (obj) {
		ptr = obj->lock();
	}
	if (!ptr.get()) {
		error(L, "The arbiter is expired.");

		return 0;
	}
	if (!ptr.get() || !field)
		return 0;

	if (strcmp(field, "restitution") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpArbiterSetRestitution(ptr.get(), val);
	} else if (strcmp(field, "friction") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpArbiterSetFriction(ptr.get(), val);
	} else if (strcmp(field, "surfaceVelocity") == 0) {
		cpVect val;
		read<3>(L, val);

		cpArbiterSetSurfaceVelocity(ptr.get(), val);
	} else if (strcmp(field, "contacts") == 0) {
		Contacts::Ptr* val = nullptr;
		read<3>(L, val);

		if (val && val->get()) {
			cpContactPointSet points;
			points.normal = val->get()->normal;
			points.count = val->get()->count;
			points.points[0].pointA = val->get()->points[0].pointA;
			points.points[0].pointB = val->get()->points[0].pointB;
			points.points[0].distance = val->get()->points[0].distance;
			points.points[1].pointA = val->get()->points[1].pointA;
			points.points[1].pointB = val->get()->points[1].pointB;
			points.points[1].distance = val->get()->points[1].distance;
			cpArbiterSetContactPointSet(ptr.get(), &points);
		}
	}

	return 0;
}

static void open_Arbiter(lua_State* L) {
	def(
		L, "Arbiter",
		LUA_LIB(
			array<luaL_Reg>()
		),
		array(
			luaL_Reg{ "__gc", __gc<Arbiter::WeakPtr> },
			luaL_Reg{ "__tostring", __tostring<Arbiter::WeakPtr> },
			luaL_Reg{ "__len", Arbiter___len },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "ignore", Arbiter_ignore },
			luaL_Reg{ "getShapes", Arbiter_getShapes },
			luaL_Reg{ "getBodies", Arbiter_getBodies },
			luaL_Reg{ "callWildcardBeginA", Arbiter_callWildcardBeginA },
			luaL_Reg{ "callWildcardBeginB", Arbiter_callWildcardBeginB },
			luaL_Reg{ "callWildcardPreSolveA", Arbiter_callWildcardPreSolveA },
			luaL_Reg{ "callWildcardPreSolveB", Arbiter_callWildcardPreSolveB },
			luaL_Reg{ "callWildcardPostSolveA", Arbiter_callWildcardPostSolveA },
			luaL_Reg{ "callWildcardPostSolveB", Arbiter_callWildcardPostSolveB },
			luaL_Reg{ "callWildcardSeparateA", Arbiter_callWildcardSeparateA },
			luaL_Reg{ "callWildcardSeparateB", Arbiter_callWildcardSeparateB },
			luaL_Reg{ nullptr, nullptr }
		),
		Arbiter___index, Arbiter___newindex
	);

	getGlobal(L, "Arbiter");
	setTable(
		L,
		"__name", "Arbiter"
	);
	pop(L);
}

/**< Shape. */

static void ShapeFilter_dtor(cpShapeFilter* shapeFilter) {
	delete shapeFilter;
}

static int ShapeFilter_ctor(lua_State* L) {
	cpGroup group = 0;
	cpBitmask categories = 0;
	cpBitmask mask = 0;
	check<>(L, group, categories, mask);

	ShapeFilter::Ptr obj(
		new cpShapeFilter{ group, categories, mask },
		ShapeFilter_dtor
	);

	return write(L, &obj);
}

static int ShapeFilter___index(lua_State* L) {
	ShapeFilter::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "group") == 0) {
		const cpGroup ret = obj->get()->group;

		return write(L, ret);
	} else if (strcmp(field, "categories") == 0) {
		const cpBitmask ret = obj->get()->categories;

		return write(L, ret);
	} else if (strcmp(field, "mask") == 0) {
		const cpBitmask ret = obj->get()->mask;

		return write(L, ret);
	} else {
		return __index(L, field);
	}
}

static int ShapeFilter___newindex(lua_State* L) {
	ShapeFilter::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "group") == 0) {
		cpGroup val = 0;
		read<3>(L, val);

		obj->get()->group = val;
	} else if (strcmp(field, "categories") == 0) {
		cpBitmask val = 0;
		read<3>(L, val);

		obj->get()->categories = val;
	} else if (strcmp(field, "mask") == 0) {
		cpBitmask val = 0;
		read<3>(L, val);

		obj->get()->mask = val;
	}

	return 0;
}

static void open_ShapeFilter(lua_State* L) {
	def(
		L, "ShapeFilter",
		LUA_LIB(
			array(
				luaL_Reg{ "new", ShapeFilter_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<ShapeFilter::Ptr> },
			luaL_Reg{ "__tostring", __tostring<ShapeFilter::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ nullptr, nullptr }
		),
		ShapeFilter___index, ShapeFilter___newindex
	);

	getGlobal(L, "ShapeFilter");
	setTable(
		L,
		"__name", "ShapeFilter"
	);
	pop(L);
}

static void Shape_dtor(cpShape* shape) {
	ShapeData* data = ShapeData::get(shape);
	if (data && data->managed) {
		cpShapeDestroy(shape);
	} else if (data && !data->managed) {
		delete data;
		cpShapeSetUserData(shape, nullptr);

		cpShapeFree(shape);
	}
}

template<int Idx> static int Shape_ctorCircle(lua_State* L) {
	Body::Ptr* body = nullptr;
	cpFloat radius = 0;
	cpVect offset;
	check<Idx>(L, body, radius, offset);

	if (body && body->get()) {
		Shape::Ptr obj(
			cpCircleShapeNew(body->get(), radius, offset),
			Shape_dtor
		);
		ShapeData* data = new ShapeData(obj, ShapeData::CIRCLE);
		cpShapeSetUserData(obj.get(), data);

		return write(L, &obj);
	}

	return write(L, nullptr);
}

template<int Idx> static int Shape_ctorSegment(lua_State* L) {
	Body::Ptr* body = nullptr;
	cpVect a, b;
	cpFloat radius = 0;
	check<Idx>(L, body, a, b, radius);

	if (body && body->get()) {
		Shape::Ptr obj(
			cpSegmentShapeNew(body->get(), a, b, radius),
			Shape_dtor
		);
		ShapeData* data = new ShapeData(obj, ShapeData::SEGMENT);
		cpShapeSetUserData(obj.get(), data);

		return write(L, &obj);
	}

	return write(L, nullptr);
}

template<int Idx> static int Shape_ctorPoly(lua_State* L) {
	const int n = getTop(L);
	Body::Ptr* body = nullptr;
	Vertex::Array verts;
	Transform::Ptr* t = nullptr;
	cpFloat radius = 0;
	if (n >= 4 + Idx - 1)
		check<Idx>(L, body, verts, t, radius);
	else
		check<Idx>(L, body, verts, radius);

	if (body && body->get()) {
		if (verts.empty())
			return write(L, nullptr);

		const cpTransform trans = (t && t->get()) ? *t->get() : cpTransformIdentity;
		Shape::Ptr obj = nullptr;
		if (n >= 4 + Idx - 1) {
			obj = Shape::Ptr(
				cpPolyShapeNew(body->get(), (int)verts.size(), &verts.front(), trans, radius),
				Shape_dtor
			);
		} else {
			obj = Shape::Ptr(
				cpPolyShapeNewRaw(body->get(), (int)verts.size(), &verts.front(), radius),
				Shape_dtor
			);
		}
		ShapeData* data = new ShapeData(obj, ShapeData::POLY);
		cpShapeSetUserData(obj.get(), data);

		return write(L, &obj);
	}

	return write(L, nullptr);
}

template<int Idx> static int Shape_ctorBox(lua_State* L) {
	const int n = getTop(L);
	Body::Ptr* body = nullptr;
	cpFloat width = 0;
	cpFloat height = 0;
	cpFloat radius = 0;
	cpBB box{ 0, 0, 0, 0 };
	if (n >= 4 + Idx - 1)
		check<Idx>(L, body, width, height, radius);
	else
		check<Idx>(L, body, box, radius);

	if (body && body->get()) {
		Shape::Ptr obj = nullptr;
		if (n >= 4 + Idx - 1) {
			obj = Shape::Ptr(
				cpBoxShapeNew(body->get(), width, height, radius),
				Shape_dtor
			);
		} else {
			obj = Shape::Ptr(
				cpBoxShapeNew2(body->get(), box, radius),
				Shape_dtor
			);
		}
		ShapeData* data = new ShapeData(obj, ShapeData::POLY);
		cpShapeSetUserData(obj.get(), data);

		return write(L, &obj);
	}

	return write(L, nullptr);
}

static int Shape_ctor(lua_State* L) {
	Enum y = (Enum)ShapeData::INVALID;
	check<>(L, y);

	switch ((ShapeData::Types)y) {
	case ShapeData::INVALID:
		return write(L, nullptr);
	case ShapeData::CIRCLE:
		return Shape_ctorCircle<2>(L);
	case ShapeData::SEGMENT:
		return Shape_ctorSegment<2>(L);
	case ShapeData::POLY: {
			Math::Rectf* bb = nullptr;
			read<3>(L, bb);
			if (bb || isNumber(L, 3))
				return Shape_ctorBox<2>(L);

			return Shape_ctorPoly<2>(L);
		}
	default:
		return write(L, nullptr);
	}
}

static int Shape_cacheBoundingBox(lua_State* L) {
	Shape::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj && obj->get()) {
		const cpBB ret = cpShapeCacheBB(obj->get());

		return write(L, ret);
	}

	return write(L, nullptr);
}

static int Shape_update(lua_State* L) {
	Shape::Ptr* obj = nullptr;
	Transform::Ptr* t = nullptr;
	read<>(L, obj, t);

	if (obj && obj->get()) {
		const cpTransform trans = (t && t->get()) ? *t->get() : cpTransformIdentity;
		const cpBB ret = cpShapeUpdate(obj->get(), trans);

		return write(L, ret);
	}

	return write(L, nullptr);
}

static int Shape_pointQuery(lua_State* L) {
	Shape::Ptr* obj = nullptr;
	cpVect p;
	read<>(L, obj, p);

	if (obj && obj->get()) {
		cpPointQueryInfo info;
		const cpFloat ret2 = cpShapePointQuery(obj->get(), p, &info);
		if (info.shape) {
			const Shape::Ptr arg1 = ShapeData::ref(info.shape);
			const cpVect &arg2 = info.point;
			const cpFloat arg3 = info.distance;
			const cpVect &arg4 = info.gradient;
			PointQuery::Ptr ret1(new PointQuery(arg1, arg2, arg3, arg4));

			return write(L, &ret1, ret2);
		}
	}

	return write(L, nullptr, nullptr);
}

static int Shape_segmentQuery(lua_State* L) {
	Shape::Ptr* obj = nullptr;
	cpVect a, b;
	cpFloat radius = 0;
	read<>(L, obj, a, b, radius);

	if (obj && obj->get()) {
		cpSegmentQueryInfo info;
		const bool ret2 = !!cpShapeSegmentQuery(obj->get(), a, b, radius, &info);
		if (info.shape) {
			const Shape::Ptr arg1 = ShapeData::ref(info.shape);
			const cpVect &arg2 = info.point;
			const cpVect &arg3 = info.normal;
			const cpFloat arg4 = info.alpha;
			SegmentQuery::Ptr ret1(new SegmentQuery(arg1, arg2, arg3, arg4));

			return write(L, &ret1, ret2);
		}
	}

	return write(L, nullptr, nullptr);
}

static int Shape_query(lua_State* L) {
	const int n = getTop(L);
	if (n >= 4)
		return Shape_segmentQuery(L);
	else
		return Shape_pointQuery(L);
}

static int Shape_collides(lua_State* L) {
	Shape::Ptr* obj = nullptr;
	Shape::Ptr* other = nullptr;
	read<>(L, obj, other);

	if (obj && obj->get() && other && other->get()) {
		const cpContactPointSet points = cpShapesCollide(obj->get(), other->get());
		const Shape::Ptr arg1 = nullptr;
		const cpVect &arg2 = points.normal;
		const int arg3 = points.count;
		const cpVect arg4 = points.count >= 1 ? points.points[0].pointA : cpVect{ 0, 0 };
		const cpVect arg5 = points.count >= 1 ? points.points[0].pointB : cpVect{ 0, 0 };
		const cpFloat arg6 = points.count >= 1 ? points.points[0].distance : 0;
		const cpVect arg7 = points.count >= 2 ? points.points[1].pointA : cpVect{ 0, 0 };
		const cpVect arg8 = points.count >= 2 ? points.points[1].pointB : cpVect{ 0, 0 };
		const cpFloat arg9 = points.count >= 2 ? points.points[1].distance : 0;
		const Contacts::Ptr ret = Contacts::Ptr(new Contacts(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9));

		return write(L, &ret);
	}

	return write(L, nullptr);
}

static int Shape_setSegmentNeighbors(lua_State* L) {
	Shape::Ptr* obj = nullptr;
	cpVect prev, next;
	read<>(L, obj, prev, next);

	if (obj && obj->get()) {
		const ShapeData* data = ShapeData::get(obj->get());
		if (data->type == ShapeData::SEGMENT) {
			cpSegmentShapeSetNeighbors(obj->get(), prev, next);

			return write(L, true);
		}
	}

	return write(L, false);
}

static int Shape_getPolygonVertexCount(lua_State* L) {
	Shape::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj && obj->get()) {
		const ShapeData* data = ShapeData::get(obj->get());
		if (data->type == ShapeData::POLY) {
			const int ret = cpPolyShapeGetCount(obj->get());

			return write(L, ret);
		}
	}

	return write(L, nullptr);
}

static int Shape_getPolygonVertex(lua_State* L) {
	Shape::Ptr* obj = nullptr;
	int index = 0;
	read<>(L, obj, index);

	if (obj && obj->get()) {
		const ShapeData* data = ShapeData::get(obj->get());
		if (data->type == ShapeData::POLY) {
			const int m = cpPolyShapeGetCount(obj->get());
			--index; // 1-based.
			if (index >= 0 && index < m) {
				const cpVect ret = cpPolyShapeGetVert(obj->get(), index);

				return write(L, ret);
			}
		}
	}

	return write(L, nullptr);
}

static int Shape___index(lua_State* L) {
	Shape::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "id") == 0) {
		const uintptr_t ret = (uintptr_t)obj->get();

		return write(L, ret);
	} else if (strcmp(field, "type") == 0) {
		const ShapeData* data = ShapeData::get(obj->get());

		return write(L, (Enum)data->type);
	} else if (strcmp(field, "space") == 0) {
		cpSpace* ptr = cpShapeGetSpace(obj->get());
		Space::Ptr ret = nullptr;
		if (ptr)
			ret = SpaceData::ref(ptr);

		return write(L, &ret);
	} else if (strcmp(field, "body") == 0) {
		cpBody* ptr = cpShapeGetBody(obj->get());
		Body::Ptr ret = nullptr;
		if (ptr)
			ret = BodyData::ref(ptr);

		return write(L, &ret);
	} else if (strcmp(field, "mass") == 0) {
		const cpFloat ret = cpShapeGetMass(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "density") == 0) {
		const cpFloat ret = cpShapeGetDensity(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "moment") == 0) {
		const cpFloat ret = cpShapeGetMoment(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "area") == 0) {
		const cpFloat ret = cpShapeGetArea(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "centerOfGravity") == 0) {
		const cpVect ret = cpShapeGetCenterOfGravity(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "boundingBox") == 0) {
		const cpBB ret = cpShapeGetBB(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "sensor") == 0) {
		const bool ret = !!cpShapeGetSensor(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "elasticity") == 0) {
		const cpFloat ret = cpShapeGetElasticity(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "friction") == 0) {
		const cpFloat ret = cpShapeGetFriction(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "surfaceVelocity") == 0) {
		const cpVect ret = cpShapeGetSurfaceVelocity(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "collisionType") == 0) {
		const cpCollisionType ret = cpShapeGetCollisionType(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "filter") == 0) {
		cpShapeFilter val = cpShapeGetFilter(obj->get());
		ShapeFilter::Ptr ret(
			new cpShapeFilter{ val.group, val.categories, val.mask },
			ShapeFilter_dtor
		);

		return write(L, &ret);
	} else if (strcmp(field, "offset") == 0) {
		const ShapeData* data = ShapeData::get(obj->get());
		if (data->type == ShapeData::CIRCLE) {
			const cpVect ret = cpCircleShapeGetOffset(obj->get());

			return write(L, ret);
		}

		return write(L, nullptr);
	} else if (strcmp(field, "radius") == 0) {
		const ShapeData* data = ShapeData::get(obj->get());
		switch (data->type) {
		case ShapeData::CIRCLE: {
				const cpFloat ret = cpCircleShapeGetRadius(obj->get());

				return write(L, ret);
			}
		case ShapeData::SEGMENT: {
				const cpFloat ret = cpSegmentShapeGetRadius(obj->get());

				return write(L, ret);
			}
		case ShapeData::POLY: {
				const cpFloat ret = cpPolyShapeGetRadius(obj->get());

				return write(L, ret);
			}
		default:
			return write(L, nullptr);
		}
	} else if (strcmp(field, "pointA") == 0) {
		const ShapeData* data = ShapeData::get(obj->get());
		if (data->type == ShapeData::SEGMENT) {
			const cpVect ret = cpSegmentShapeGetA(obj->get());

			return write(L, ret);
		}

		return write(L, nullptr);
	} else if (strcmp(field, "pointB") == 0) {
		const ShapeData* data = ShapeData::get(obj->get());
		if (data->type == ShapeData::SEGMENT) {
			const cpVect ret = cpSegmentShapeGetB(obj->get());

			return write(L, ret);
		}

		return write(L, nullptr);
	} else if (strcmp(field, "normal") == 0) {
		const ShapeData* data = ShapeData::get(obj->get());
		if (data->type == ShapeData::SEGMENT) {
			const cpVect ret = cpSegmentShapeGetNormal(obj->get());

			return write(L, ret);
		}

		return write(L, nullptr);
	} else if (strcmp(field, "vertexes") == 0) {
		const ShapeData* data = ShapeData::get(obj->get());
		if (data->type == ShapeData::POLY) {
			Vertex::Array ret;
			const int m = cpPolyShapeGetCount(obj->get());
			for (int i = 0; i < m; ++i) {
				const cpVect vet = cpPolyShapeGetVert(obj->get(), i);
				ret.push_back(vet);
			}

			return write(L, ret);
		}

		return write(L, nullptr);
	} else {
		return __index(L, field);
	}
}

static int Shape___newindex(lua_State* L) {
	Shape::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "body") == 0) {
		Body::Ptr* val = 0;
		read<3>(L, val);

		if (val && val->get())
			cpShapeSetBody(obj->get(), val->get());
	} else if (strcmp(field, "mass") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpShapeSetMass(obj->get(), val);
	} else if (strcmp(field, "density") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpShapeSetDensity(obj->get(), val);
	} else if (strcmp(field, "sensor") == 0) {
		bool val = 0;
		read<3>(L, val);

		cpShapeSetSensor(obj->get(), val ? cpTrue : cpFalse);
	} else if (strcmp(field, "elasticity") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpShapeSetElasticity(obj->get(), val);
	} else if (strcmp(field, "friction") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpShapeSetFriction(obj->get(), val);
	} else if (strcmp(field, "surfaceVelocity") == 0) {
		cpVect val;
		read<3>(L, val);

		cpShapeSetSurfaceVelocity(obj->get(), val);
	} else if (strcmp(field, "collisionType") == 0) {
		cpCollisionType val = 0;
		read<3>(L, val);

		cpShapeSetCollisionType(obj->get(), val);
	} else if (strcmp(field, "filter") == 0) {
		ShapeFilter::Ptr* val = 0;
		read<3>(L, val);

		if (val && val->get())
			cpShapeSetFilter(obj->get(), *val->get());
	}

	return 0;
}

static void open_Shape(lua_State* L) {
	def(
		L, "Shape",
		LUA_LIB(
			array(
				luaL_Reg{ "newCircle", Shape_ctorCircle<1> },
				luaL_Reg{ "newSegment", Shape_ctorSegment<1> },
				luaL_Reg{ "newPoly", Shape_ctorPoly<1> },
				luaL_Reg{ "newBox", Shape_ctorBox<1> },
				luaL_Reg{ "new", Shape_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Shape::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Shape::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "cacheBoundingBox", Shape_cacheBoundingBox },
			luaL_Reg{ "update", Shape_update },
			luaL_Reg{ "pointQuery", Shape_pointQuery },
			luaL_Reg{ "segmentQuery", Shape_segmentQuery },
			luaL_Reg{ "query", Shape_query },
			luaL_Reg{ "collides", Shape_collides },
			luaL_Reg{ "setSegmentNeighbors", Shape_setSegmentNeighbors },
			luaL_Reg{ "getPolygonVertexCount", Shape_getPolygonVertexCount },
			luaL_Reg{ "getPolygonVertex", Shape_getPolygonVertex },
			luaL_Reg{ nullptr, nullptr }
		),
		Shape___index, Shape___newindex
	);

	getGlobal(L, "Shape");
	setTable(
		L,
		"Invalid", (Enum)ShapeData::INVALID,
		"Circle", (Enum)ShapeData::CIRCLE,
		"Segment", (Enum)ShapeData::SEGMENT,
		"Polygon", (Enum)ShapeData::POLY,

		"__name", "Shape"
	);
	pop(L);
}

/**< Body. */

static void Body_dtor(cpBody* body) {
	BodyData* data = BodyData::get(body);
	if (data && data->managed) {
		cpBodyDestroy(body);
	} else if (data && !data->managed) {
		delete data;
		cpBodySetUserData(body, nullptr);

		cpBodyFree(body);
	}
}

template<int Idx> static int Body_ctorDynamic(lua_State* L) {
	cpFloat mass = 0;
	cpFloat moment = 0;
	check<Idx>(L, mass, moment);

	Body::Ptr obj(
		cpBodyNew(mass, moment),
		Body_dtor
	);
	BodyData* data = new BodyData(obj, L);
	cpBodySetUserData(obj.get(), data);

	return write(L, &obj);
}

static int Body_ctorKinematic(lua_State* L) {
	Body::Ptr obj(
		cpBodyNewKinematic(),
		Body_dtor
	);
	BodyData* data = new BodyData(obj, L);
	cpBodySetUserData(obj.get(), data);

	return write(L, &obj);
}

static int Body_ctorStatic(lua_State* L) {
	Body::Ptr obj(
		cpBodyNewStatic(),
		Body_dtor
	);
	BodyData* data = new BodyData(obj, L);
	cpBodySetUserData(obj.get(), data);

	return write(L, &obj);
}

static int Body_ctor(lua_State* L) {
	Enum y = (Enum)CP_BODY_TYPE_DYNAMIC;
	check<>(L, y);

	switch ((cpBodyType)y) {
	case CP_BODY_TYPE_DYNAMIC:
		return Body_ctorDynamic<2>(L);
	case CP_BODY_TYPE_KINEMATIC:
		return Body_ctorKinematic(L);
	case CP_BODY_TYPE_STATIC:
		return Body_ctorStatic(L);
	default:
		return Body_ctorDynamic<2>(L);
	}
}

static int Body_activate(lua_State* L) {
	const int n = getTop(L);
	Body::Ptr* obj = nullptr;
	Shape::Ptr* filter = nullptr;
	if (n >= 2)
		read<>(L, obj, filter);
	else
		read<>(L, obj);

	if (obj && obj->get()) {
		if (n >= 2) {
			if (cpBodyGetType(obj->get()) == CP_BODY_TYPE_STATIC) {
				if (filter && filter->get())
					cpBodyActivateStatic(obj->get(), filter->get());
				else
					cpBodyActivateStatic(obj->get(), nullptr);
			}
		} else {
			if (cpBodyGetType(obj->get()) == CP_BODY_TYPE_DYNAMIC)
				cpBodyActivate(obj->get());
		}
	}

	return 0;
}

static int Body_sleep(lua_State* L) {
	Body::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj && obj->get())
		cpBodySleep(obj->get());

	return 0;
}

static int Body_sleepWithGroup(lua_State* L) {
	Body::Ptr* obj = nullptr;
	Body::Ptr* group = nullptr;
	read<>(L, obj, group);

	if (obj && obj->get() && group && group->get())
		cpBodySleepWithGroup(obj->get(), group->get());

	return 0;
}

static int Body_setVelocityHandler(lua_State* L) {
	Body::Ptr* obj = nullptr;
	Function::Ptr handler = nullptr;
	read<>(L, obj, handler);

	if (obj && obj->get()) {
		BodyData* data = BodyData::get(obj->get());
		if (handler) {
			data->velocityHandler = handler;
			cpBodySetVelocityUpdateFunc(obj->get(), BodyData::onVelocityUpdating);
		} else {
			data->velocityHandler = nullptr;
			cpBodySetVelocityUpdateFunc(obj->get(), nullptr);
		}
	}

	return 0;
}

static int Body_setPositionHandler(lua_State* L) {
	Body::Ptr* obj = nullptr;
	Function::Ptr handler = nullptr;
	read<>(L, obj, handler);

	if (obj && obj->get()) {
		BodyData* data = BodyData::get(obj->get());
		if (handler) {
			data->positionHandler = handler;
			cpBodySetPositionUpdateFunc(obj->get(), BodyData::onPositionUpdating);
		} else {
			data->positionHandler = nullptr;
			cpBodySetPositionUpdateFunc(obj->get(), nullptr);
		}
	}

	return 0;
}

static int Body_updateVelocity(lua_State* L) {
	Body::Ptr* obj = nullptr;
	cpVect gravity;
	cpFloat damping = 0;
	cpFloat delta = 0;
	read<>(L, obj, gravity, damping, delta);

	if (obj && obj->get())
		cpBodyUpdateVelocity(obj->get(), gravity, damping, delta);

	return 0;
}

static int Body_updatePosition(lua_State* L) {
	Body::Ptr* obj = nullptr;
	cpFloat delta = 0;
	read<>(L, obj, delta);

	if (obj && obj->get())
		cpBodyUpdatePosition(obj->get(), delta);

	return 0;
}

static int Body_localToWorld(lua_State* L) {
	Body::Ptr* obj = nullptr;
	cpVect point;
	read<>(L, obj, point);

	if (obj && obj->get()) {
		const cpVect ret = cpBodyLocalToWorld(obj->get(), point);

		return write(L, ret);
	}

	return write(L, nullptr);
}

static int Body_worldToLocal(lua_State* L) {
	Body::Ptr* obj = nullptr;
	cpVect point;
	read<>(L, obj, point);

	if (obj && obj->get()) {
		const cpVect ret = cpBodyWorldToLocal(obj->get(), point);

		return write(L, ret);
	}

	return write(L, nullptr);
}

static int Body_applyForceAtWorldPoint(lua_State* L) {
	Body::Ptr* obj = nullptr;
	cpVect force;
	cpVect point;
	read<>(L, obj, force, point);

	if (obj && obj->get())
		cpBodyApplyForceAtWorldPoint(obj->get(), force, point);

	return 0;
}

static int Body_applyForceAtLocalPoint(lua_State* L) {
	Body::Ptr* obj = nullptr;
	cpVect force;
	cpVect point;
	read<>(L, obj, force, point);

	if (obj && obj->get())
		cpBodyApplyForceAtLocalPoint(obj->get(), force, point);

	return 0;
}

static int Body_applyImpulseAtWorldPoint(lua_State* L) {
	Body::Ptr* obj = nullptr;
	cpVect impulse;
	cpVect point;
	read<>(L, obj, impulse, point);

	if (obj && obj->get())
		cpBodyApplyImpulseAtWorldPoint(obj->get(), impulse, point);

	return 0;
}

static int Body_applyImpulseAtLocalPoint(lua_State* L) {
	Body::Ptr* obj = nullptr;
	cpVect impulse;
	cpVect point;
	read<>(L, obj, impulse, point);

	if (obj && obj->get())
		cpBodyApplyImpulseAtLocalPoint(obj->get(), impulse, point);

	return 0;
}

static int Body_getVelocityAtWorldPoint(lua_State* L) {
	Body::Ptr* obj = nullptr;
	cpVect point;
	read<>(L, obj, point);

	if (obj && obj->get()) {
		const cpVect ret = cpBodyGetVelocityAtWorldPoint(obj->get(), point);

		return write(L, ret);
	}

	return write(L, nullptr);
}

static int Body_getVelocityAtLocalPoint(lua_State* L) {
	Body::Ptr* obj = nullptr;
	cpVect point;
	read<>(L, obj, point);

	if (obj && obj->get()) {
		const cpVect ret = cpBodyGetVelocityAtLocalPoint(obj->get(), point);

		return write(L, ret);
	}

	return write(L, nullptr);
}

static int Body_kineticEnergy(lua_State* L) {
	Body::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj && obj->get()) {
		const cpFloat ret = cpBodyKineticEnergy(obj->get());

		return write(L, ret);
	}

	return write(L, nullptr);
}

template<int Idx> int Body_foreachShape(lua_State* L) {
	Body::Ptr* obj = nullptr;
	Function::Ptr callback = nullptr;
	read<>(L, obj);
	read<Idx>(L, callback);

	if (obj && obj->get() && callback) {
		int ret = 0;
		IterationData data(L, &ret, &callback, nullptr);
		auto callback_ = [] (cpBody* /* body */, cpShape* shape, void* data) -> void {
			IterationData* data_ = (IterationData*)data;
			const Shape::Ptr arg1 = ShapeData::ref(shape);
			if (arg1) {
				ScriptingLua::check(data_->L, call(data_->L, **data_->callback, &arg1));
				++*data_->ret;
			}
		};
		cpBodyEachShape(obj->get(), callback_, &data);

		return write(L, ret);
	}

	return write(L, nullptr);
}

template<int Idx> int Body_foreachConstraint(lua_State* L) {
	Body::Ptr* obj = nullptr;
	Function::Ptr callback = nullptr;
	read<>(L, obj);
	read<Idx>(L, callback);

	if (obj && obj->get() && callback) {
		int ret = 0;
		IterationData data(L, &ret, &callback, nullptr);
		auto callback_ = [] (cpBody* /* body */, cpConstraint* constraint, void* data) -> void {
			IterationData* data_ = (IterationData*)data;
			const Constraint::Ptr arg1 = ConstraintData::ref(constraint);
			if (arg1) {
				ScriptingLua::check(data_->L, call(data_->L, **data_->callback, &arg1));
				++*data_->ret;
			}
		};
		cpBodyEachConstraint(obj->get(), callback_, &data);

		return write(L, ret);
	}

	return write(L, nullptr);
}

template<int Idx> int Body_foreachArbiter(lua_State* L) {
	Body::Ptr* obj = nullptr;
	Function::Ptr callback = nullptr;
	read<>(L, obj);
	read<Idx>(L, callback);

	if (obj && obj->get() && callback) {
		int ret = 0;
		Arbiter::Array arbiters;
		IterationData data(L, &ret, &callback, &arbiters);
		auto callback_ = [] (cpBody* /* body */, cpArbiter* arbiter, void* data) -> void {
			IterationData* data_ = (IterationData*)data;
			Arbiter::Array* arbiters = (Arbiter::Array*)data_->collection;
			LUA_ARTIBER_WEAK_AND_SHARED_PTR(arbiter, arg1, arg1_)
			if (arg1_) {
				arbiters->push_back(arg1_);
				ScriptingLua::check(data_->L, call(data_->L, **data_->callback, &arg1));
				++*data_->ret;
			}
		};
		cpBodyEachArbiter(obj->get(), callback_, &data);

		arbiters.clear();

		return write(L, ret);
	}

	return write(L, nullptr);
}

static int Body_foreach(lua_State* L) {
	std::string y;
	if (isTable(L, 2)) {
		push(L, 2);
		getTable(L, "__name", y);
		pop(L);
	} else if (isString(L, 2)) {
		Placeholder _1;
		read<>(L, _1, y);
	}

	if (y == "Shape")
		return Body_foreachShape<3>(L);
	else if (y == "Constraint")
		return Body_foreachConstraint<3>(L);
	else if (y == "Arbiter")
		return Body_foreachArbiter<3>(L);

	return write(L, nullptr);
}

static int Body___index(lua_State* L) {
	Body::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "id") == 0) {
		const uintptr_t ret = (uintptr_t)obj->get();

		return write(L, ret);
	} else if (strcmp(field, "isSleeping") == 0) {
		const bool ret = !!cpBodyIsSleeping(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "type") == 0) {
		const cpBodyType ret = cpBodyGetType(obj->get());

		return write(L, (Enum)ret);
	} else if (strcmp(field, "space") == 0) {
		cpSpace* ptr = cpBodyGetSpace(obj->get());
		Space::Ptr ret = nullptr;
		if (ptr)
			ret = SpaceData::ref(ptr);

		return write(L, &ret);
	} else if (strcmp(field, "mass") == 0) {
		const cpFloat ret = cpBodyGetMass(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "moment") == 0) {
		const cpFloat ret = cpBodyGetMoment(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "position") == 0) {
		const cpVect ret = cpBodyGetPosition(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "centerOfGravity") == 0) {
		const cpVect ret = cpBodyGetCenterOfGravity(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "velocity") == 0) {
		const cpVect ret = cpBodyGetVelocity(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "force") == 0) {
		const cpVect ret = cpBodyGetForce(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "angle") == 0) {
		const cpFloat ret = cpBodyGetAngle(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "angularVelocity") == 0) {
		const cpFloat ret = cpBodyGetAngularVelocity(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "torque") == 0) {
		const cpFloat ret = cpBodyGetTorque(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "rotation") == 0) {
		const cpVect ret = cpBodyGetRotation(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "shapes") == 0) {
		Shape::Array ret;
		IterationData data(L, nullptr, nullptr, &ret);
		auto callback_ = [] (cpBody* /* body */, cpShape* shape, void* data) -> void {
			IterationData* data_ = (IterationData*)data;
			Shape::Array* coll = (Shape::Array*)data_->collection;
			const Shape::Ptr arg1 = ShapeData::ref(shape);
			if (arg1)
				coll->push_back(arg1);
		};
		cpBodyEachShape(obj->get(), callback_, &data);

		return write(L, ret);
	} else if (strcmp(field, "constraints") == 0) {
		Constraint::Array ret;
		IterationData data(L, nullptr, nullptr, &ret);
		auto callback_ = [] (cpBody* /* body */, cpConstraint* constraint, void* data) -> void {
			IterationData* data_ = (IterationData*)data;
			Constraint::Array* coll = (Constraint::Array*)data_->collection;
			const Constraint::Ptr arg1 = ConstraintData::ref(constraint);
			if (arg1)
				coll->push_back(arg1);
		};
		cpBodyEachConstraint(obj->get(), callback_, &data);

		return write(L, ret);
	} else {
		return __index(L, field);
	}
}

static int Body___newindex(lua_State* L) {
	Body::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "type") == 0) {
		Enum val = 0;
		read<3>(L, val);

		cpSpace* space = cpBodyGetSpace(obj->get());
		if (space) {
			if (cpSpaceIsLocked(space)) {
				error(L, "Cannot set the Body's type when the Space is locked.");

				return 0;
			}
		}

		cpBodySetType(obj->get(), (cpBodyType)val);
	} else if (strcmp(field, "mass") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		if (cpBodyGetType(obj->get()) != CP_BODY_TYPE_DYNAMIC) {
			error(L, "You cannot set the mass of kinematic or static bodies.");

			return 0;
		}
		if (val < 0 && val >= INFINITY) {
			error(L, "`mass` must be positive and finite.");

			return 0;
		}
		cpBodySetMass(obj->get(), val);
	} else if (strcmp(field, "moment") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		if (val < 0) {
			error(L, "`moment` of inertia must be positive or zero.");

			return 0;
		}
		cpBodySetMoment(obj->get(), val);
	} else if (strcmp(field, "position") == 0) {
		cpVect val;
		read<3>(L, val);

		cpBodySetPosition(obj->get(), val);
	} else if (strcmp(field, "centerOfGravity") == 0) {
		cpVect val;
		read<3>(L, val);

		cpBodySetCenterOfGravity(obj->get(), val);
	} else if (strcmp(field, "velocity") == 0) {
		cpVect val;
		read<3>(L, val);

		cpBodySetVelocity(obj->get(), val);
	} else if (strcmp(field, "force") == 0) {
		cpVect val;
		read<3>(L, val);

		cpBodySetForce(obj->get(), val);
	} else if (strcmp(field, "angle") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpBodySetAngle(obj->get(), val);
	} else if (strcmp(field, "angularVelocity") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpBodySetAngularVelocity(obj->get(), val);
	} else if (strcmp(field, "torque") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpBodySetTorque(obj->get(), val);
	}

	return 0;
}

static void open_Body(lua_State* L) {
	def(
		L, "Body",
		LUA_LIB(
			array(
				luaL_Reg{ "newDynamic", Body_ctorDynamic<1> },
				luaL_Reg{ "newKinematic", Body_ctorKinematic },
				luaL_Reg{ "newStatic", Body_ctorStatic },
				luaL_Reg{ "new", Body_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Body::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Body::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "activate", Body_activate },
			luaL_Reg{ "sleep", Body_sleep },
			luaL_Reg{ "sleepWithGroup", Body_sleepWithGroup },
			luaL_Reg{ "setVelocityHandler", Body_setVelocityHandler },
			luaL_Reg{ "setPositionHandler", Body_setPositionHandler },
			luaL_Reg{ "updateVelocity", Body_updateVelocity },
			luaL_Reg{ "updatePosition", Body_updatePosition },
			luaL_Reg{ "localToWorld", Body_localToWorld },
			luaL_Reg{ "worldToLocal", Body_worldToLocal },
			luaL_Reg{ "applyForceAtWorldPoint", Body_applyForceAtWorldPoint },
			luaL_Reg{ "applyForceAtLocalPoint", Body_applyForceAtLocalPoint },
			luaL_Reg{ "applyImpulseAtWorldPoint", Body_applyImpulseAtWorldPoint },
			luaL_Reg{ "applyImpulseAtLocalPoint", Body_applyImpulseAtLocalPoint },
			luaL_Reg{ "getVelocityAtWorldPoint", Body_getVelocityAtWorldPoint },
			luaL_Reg{ "getVelocityAtLocalPoint", Body_getVelocityAtLocalPoint },
			luaL_Reg{ "kineticEnergy", Body_kineticEnergy },
			//luaL_Reg{ "foreachShape", Body_foreachShape<2> },
			//luaL_Reg{ "foreachConstraint", Body_foreachConstraint<2> },
			//luaL_Reg{ "foreachArbiter", Body_foreachArbiter<2> },
			luaL_Reg{ "foreach", Body_foreach },
			luaL_Reg{ nullptr, nullptr }
		),
		Body___index, Body___newindex
	);

	getGlobal(L, "Body");
	setTable(
		L,
		"Dynamic", (Enum)CP_BODY_TYPE_DYNAMIC,
		"Kinematic", (Enum)CP_BODY_TYPE_KINEMATIC,
		"Static", (Enum)CP_BODY_TYPE_STATIC,

		"__name", "Body"
	);
	pop(L);
}

/**< Constraints. */

static void Constraint_dtor(cpConstraint* constraint) {
	ConstraintData* data = ConstraintData::get(constraint);
	if (data && data->managed) {
		cpConstraintDestroy(constraint);
	} else if (data && !data->managed) {
		delete data;
		cpConstraintSetUserData(constraint, nullptr);

		cpConstraintFree(constraint);
	}
}

static int Constraint___indexBase(lua_State* L, Constraint::Ptr* obj, const char* field, bool* result) {
	if (strcmp(field, "id") == 0) {
		const uintptr_t ret = (uintptr_t)obj->get();

		return write(L, ret);
	} else if (strcmp(field, "space") == 0) {
		cpSpace* ptr = cpConstraintGetSpace(obj->get());
		Space::Ptr ret = nullptr;
		if (ptr)
			ret = SpaceData::ref(ptr);

		if (result)
			*result = true;

		return write(L, &ret);
	} else if (strcmp(field, "bodyA") == 0) {
		cpBody* ptr = cpConstraintGetBodyA(obj->get());
		Body::Ptr ret = nullptr;
		if (ptr)
			ret = BodyData::ref(ptr);

		if (result)
			*result = true;

		return write(L, &ret);
	} else if (strcmp(field, "bodyB") == 0) {
		cpBody* ptr = cpConstraintGetBodyB(obj->get());
		Body::Ptr ret = nullptr;
		if (ptr)
			ret = BodyData::ref(ptr);

		if (result)
			*result = true;

		return write(L, &ret);
	} else if (strcmp(field, "maxForce") == 0) {
		const cpFloat ret = cpConstraintGetMaxForce(obj->get());

		if (result)
			*result = true;

		return write(L, ret);
	} else if (strcmp(field, "errorBias") == 0) {
		const cpFloat ret = cpConstraintGetErrorBias(obj->get());

		if (result)
			*result = true;

		return write(L, ret);
	} else if (strcmp(field, "maxBias") == 0) {
		const cpFloat ret = cpConstraintGetMaxBias(obj->get());

		if (result)
			*result = true;

		return write(L, ret);
	} else if (strcmp(field, "collideBodies") == 0) {
		const bool ret = !!cpConstraintGetCollideBodies(obj->get());

		if (result)
			*result = true;

		return write(L, ret);
	} else if (strcmp(field, "impulse") == 0) {
		const cpFloat ret = cpConstraintGetImpulse(obj->get());

		if (result)
			*result = true;

		return write(L, ret);
	} else if (strcmp(field, "isDampedRotarySpring") == 0) {
		const bool ret = !!cpConstraintIsDampedRotarySpring(obj->get());

		if (result)
			*result = true;

		return write(L, ret);
	} else if (strcmp(field, "isDampedSpring") == 0) {
		const bool ret = !!cpConstraintIsDampedSpring(obj->get());

		if (result)
			*result = true;

		return write(L, ret);
	} else if (strcmp(field, "isGearJoint") == 0) {
		const bool ret = !!cpConstraintIsGearJoint(obj->get());

		if (result)
			*result = true;

		return write(L, ret);
	} else if (strcmp(field, "isGrooveJoint") == 0) {
		const bool ret = !!cpConstraintIsGrooveJoint(obj->get());

		if (result)
			*result = true;

		return write(L, ret);
	} else if (strcmp(field, "isPinJoint") == 0) {
		const bool ret = !!cpConstraintIsPinJoint(obj->get());

		if (result)
			*result = true;

		return write(L, ret);
	} else if (strcmp(field, "isPivotJoint") == 0) {
		const bool ret = !!cpConstraintIsPivotJoint(obj->get());

		if (result)
			*result = true;

		return write(L, ret);
	} else if (strcmp(field, "isRatchetJoint") == 0) {
		const bool ret = !!cpConstraintIsRatchetJoint(obj->get());

		if (result)
			*result = true;

		return write(L, ret);
	} else if (strcmp(field, "isRotaryLimitJoint") == 0) {
		const bool ret = !!cpConstraintIsRotaryLimitJoint(obj->get());

		if (result)
			*result = true;

		return write(L, ret);
	} else if (strcmp(field, "isSimpleMotor") == 0) {
		const bool ret = !!cpConstraintIsSimpleMotor(obj->get());

		if (result)
			*result = true;

		return write(L, ret);
	} else if (strcmp(field, "isSlideJoint") == 0) {
		const bool ret = !!cpConstraintIsSlideJoint(obj->get());

		if (result)
			*result = true;

		return write(L, ret);
	}

	if (result)
		*result = false;

	return 0;
}

static int Constraint___newindexBase(lua_State* L, Constraint::Ptr* obj, const char* field, bool* result) {
	if (strcmp(field, "maxForce") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		if (val < 0) {
			error(L, "`maxForce` must be positive or zero.");

			return 0;
		}
		cpConstraintSetMaxForce(obj->get(), val);

		if (result)
			*result = true;
	} else if (strcmp(field, "errorBias") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		if (val < 0) {
			error(L, "`errorBias` must be positive or zero.");

			return 0;
		}
		cpConstraintSetErrorBias(obj->get(), val);

		if (result)
			*result = true;
	} else if (strcmp(field, "maxBias") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		if (val < 0) {
			error(L, "`maxBias` must be positive or zero.");

			return 0;
		}
		cpConstraintSetMaxBias(obj->get(), val);

		if (result)
			*result = true;
	} else if (strcmp(field, "collideBodies") == 0) {
		bool val = 0;
		read<3>(L, val);

		cpConstraintSetCollideBodies(obj->get(), val ? cpTrue : cpFalse);

		if (result)
			*result = true;
	}

	if (result)
		*result = false;

	return 0;
}

static int DampedRotarySpring___index(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "restAngle") == 0) {
		const cpFloat ret = cpDampedRotarySpringGetRestAngle(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "stiffness") == 0) {
		const cpFloat ret = cpDampedRotarySpringGetStiffness(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "damping") == 0) {
		const cpFloat ret = cpDampedRotarySpringGetDamping(obj->get());

		return write(L, ret);
	} else {
		bool proceeded = false;
		const int result = Constraint___indexBase(L, obj, field, &proceeded);
		if (proceeded)
			return result;
		else
			return __index(L, field);
	}
}

static int DampedRotarySpring___newindex(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "restAngle") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpDampedRotarySpringSetRestAngle(obj->get(), val);

		return 0;
	} else if (strcmp(field, "stiffness") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpDampedRotarySpringSetStiffness(obj->get(), val);

		return 0;
	} else if (strcmp(field, "damping") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpDampedRotarySpringSetDamping(obj->get(), val);

		return 0;
	} else {
		bool proceeded = false;
		const int result = Constraint___newindexBase(L, obj, field, &proceeded);
		if (proceeded)
			return result;
		else
			return 0;
	}
}

static int DampedSpring___index(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "anchorA") == 0) {
		cpVect ret = cpDampedSpringGetAnchorA(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "anchorB") == 0) {
		cpVect ret = cpDampedSpringGetAnchorB(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "restLength") == 0) {
		const cpFloat ret = cpDampedSpringGetRestLength(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "stiffness") == 0) {
		const cpFloat ret = cpDampedSpringGetStiffness(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "damping") == 0) {
		const cpFloat ret = cpDampedSpringGetDamping(obj->get());

		return write(L, ret);
	} else {
		bool proceeded = false;
		const int result = Constraint___indexBase(L, obj, field, &proceeded);
		if (proceeded)
			return result;
		else
			return __index(L, field);
	}
}

static int DampedSpring___newindex(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "anchorA") == 0) {
		cpVect val;
		read<3>(L, val);

		cpDampedSpringSetAnchorA(obj->get(), val);

		return 0;
	} else if (strcmp(field, "anchorB") == 0) {
		cpVect val;
		read<3>(L, val);

		cpDampedSpringSetAnchorB(obj->get(), val);

		return 0;
	} else if (strcmp(field, "restLength") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpDampedSpringSetRestLength(obj->get(), val);

		return 0;
	} else if (strcmp(field, "stiffness") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpDampedSpringSetStiffness(obj->get(), val);

		return 0;
	} else if (strcmp(field, "damping") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpDampedSpringSetDamping(obj->get(), val);

		return 0;
	} else {
		bool proceeded = false;
		const int result = Constraint___newindexBase(L, obj, field, &proceeded);
		if (proceeded)
			return result;
		else
			return 0;
	}
}

static int GearJoint___index(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "phase") == 0) {
		const cpFloat ret = cpGearJointGetPhase(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "ratio") == 0) {
		const cpFloat ret = cpGearJointGetRatio(obj->get());

		return write(L, ret);
	} else {
		bool proceeded = false;
		const int result = Constraint___indexBase(L, obj, field, &proceeded);
		if (proceeded)
			return result;
		else
			return __index(L, field);
	}
}

static int GearJoint___newindex(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "phase") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpGearJointSetPhase(obj->get(), val);

		return 0;
	} else if (strcmp(field, "ratio") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpGearJointSetRatio(obj->get(), val);

		return 0;
	} else {
		bool proceeded = false;
		const int result = Constraint___newindexBase(L, obj, field, &proceeded);
		if (proceeded)
			return result;
		else
			return 0;
	}
}

static int GrooveJoint___index(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "grooveA") == 0) {
		const cpVect ret = cpGrooveJointGetGrooveA(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "grooveB") == 0) {
		const cpVect ret = cpGrooveJointGetGrooveB(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "anchorB") == 0) {
		const cpVect ret = cpGrooveJointGetAnchorB(obj->get());

		return write(L, ret);
	} else {
		bool proceeded = false;
		const int result = Constraint___indexBase(L, obj, field, &proceeded);
		if (proceeded)
			return result;
		else
			return __index(L, field);
	}
}

static int GrooveJoint___newindex(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "grooveA") == 0) {
		cpVect val;
		read<3>(L, val);

		cpGrooveJointSetGrooveA(obj->get(), val);

		return 0;
	} else if (strcmp(field, "grooveB") == 0) {
		cpVect val;
		read<3>(L, val);

		cpGrooveJointSetGrooveB(obj->get(), val);

		return 0;
	} else if (strcmp(field, "anchorB") == 0) {
		cpVect val;
		read<3>(L, val);

		cpGrooveJointSetAnchorB(obj->get(), val);

		return 0;
	} else {
		bool proceeded = false;
		const int result = Constraint___newindexBase(L, obj, field, &proceeded);
		if (proceeded)
			return result;
		else
			return 0;
	}
}

static int PinJoint___index(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "anchorA") == 0) {
		const cpVect ret = cpPinJointGetAnchorA(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "anchorB") == 0) {
		const cpVect ret = cpPinJointGetAnchorB(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "distance") == 0) {
		const cpFloat ret = cpPinJointGetDist(obj->get());

		return write(L, ret);
	} else {
		bool proceeded = false;
		const int result = Constraint___indexBase(L, obj, field, &proceeded);
		if (proceeded)
			return result;
		else
			return __index(L, field);
	}
}

static int PinJoint___newindex(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "anchorA") == 0) {
		cpVect val;
		read<3>(L, val);

		cpPinJointSetAnchorA(obj->get(), val);

		return 0;
	} else if (strcmp(field, "anchorB") == 0) {
		cpVect val;
		read<3>(L, val);

		cpPinJointSetAnchorB(obj->get(), val);

		return 0;
	} else if (strcmp(field, "distance") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpPinJointSetDist(obj->get(), val);

		return 0;
	} else {
		bool proceeded = false;
		const int result = Constraint___newindexBase(L, obj, field, &proceeded);
		if (proceeded)
			return result;
		else
			return 0;
	}
}

static int PivotJoint___index(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "anchorA") == 0) {
		const cpVect ret = cpPivotJointGetAnchorA(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "anchorB") == 0) {
		const cpVect ret = cpPivotJointGetAnchorB(obj->get());

		return write(L, ret);
	} else {
		bool proceeded = false;
		const int result = Constraint___indexBase(L, obj, field, &proceeded);
		if (proceeded)
			return result;
		else
			return __index(L, field);
	}
}

static int PivotJoint___newindex(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "anchorA") == 0) {
		cpVect val;
		read<3>(L, val);

		cpPivotJointSetAnchorA(obj->get(), val);

		return 0;
	} else if (strcmp(field, "anchorB") == 0) {
		cpVect val;
		read<3>(L, val);

		cpPivotJointSetAnchorB(obj->get(), val);

		return 0;
	} else {
		bool proceeded = false;
		const int result = Constraint___newindexBase(L, obj, field, &proceeded);
		if (proceeded)
			return result;
		else
			return 0;
	}
}

static int RatchetJoint___index(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "angle") == 0) {
		const cpFloat ret = cpRatchetJointGetAngle(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "phase") == 0) {
		const cpFloat ret = cpRatchetJointGetPhase(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "ratchet") == 0) {
		const cpFloat ret = cpRatchetJointGetRatchet(obj->get());

		return write(L, ret);
	} else {
		bool proceeded = false;
		const int result = Constraint___indexBase(L, obj, field, &proceeded);
		if (proceeded)
			return result;
		else
			return __index(L, field);
	}
}

static int RatchetJoint___newindex(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "angle") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpRatchetJointSetAngle(obj->get(), val);

		return 0;
	} else if (strcmp(field, "phase") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpRatchetJointSetPhase(obj->get(), val);

		return 0;
	} else if (strcmp(field, "ratchet") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpRatchetJointSetRatchet(obj->get(), val);

		return 0;
	} else {
		bool proceeded = false;
		const int result = Constraint___newindexBase(L, obj, field, &proceeded);
		if (proceeded)
			return result;
		else
			return 0;
	}
}

static int RotaryLimitJoint___index(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "min") == 0) {
		const cpFloat ret = cpRotaryLimitJointGetMin(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "max") == 0) {
		const cpFloat ret = cpRotaryLimitJointGetMax(obj->get());

		return write(L, ret);
	} else {
		bool proceeded = false;
		const int result = Constraint___indexBase(L, obj, field, &proceeded);
		if (proceeded)
			return result;
		else
			return __index(L, field);
	}
}

static int RotaryLimitJoint___newindex(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "min") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpRotaryLimitJointSetMin(obj->get(), val);

		return 0;
	} else if (strcmp(field, "max") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpRotaryLimitJointSetMax(obj->get(), val);

		return 0;
	} else {
		bool proceeded = false;
		const int result = Constraint___newindexBase(L, obj, field, &proceeded);
		if (proceeded)
			return result;
		else
			return 0;
	}
}

static int SimpleMotor___index(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "rate") == 0) {
		const cpFloat ret = cpSimpleMotorGetRate(obj->get());

		return write(L, ret);
	} else {
		bool proceeded = false;
		const int result = Constraint___indexBase(L, obj, field, &proceeded);
		if (proceeded)
			return result;
		else
			return __index(L, field);
	}
}

static int SimpleMotor___newindex(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "rate") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpSimpleMotorSetRate(obj->get(), val);

		return 0;
	} else {
		bool proceeded = false;
		const int result = Constraint___newindexBase(L, obj, field, &proceeded);
		if (proceeded)
			return result;
		else
			return 0;
	}
}

static int SlideJoint___index(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "anchorA") == 0) {
		const cpVect ret = cpSlideJointGetAnchorA(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "anchorB") == 0) {
		const cpVect ret = cpSlideJointGetAnchorB(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "min") == 0) {
		const cpFloat ret = cpSlideJointGetMin(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "max") == 0) {
		const cpFloat ret = cpSlideJointGetMax(obj->get());

		return write(L, ret);
	} else {
		bool proceeded = false;
		const int result = Constraint___indexBase(L, obj, field, &proceeded);
		if (proceeded)
			return result;
		else
			return __index(L, field);
	}
}

static int SlideJoint___newindex(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "anchorA") == 0) {
		cpVect val;
		read<3>(L, val);

		cpSlideJointSetAnchorA(obj->get(), val);

		return 0;
	} else if (strcmp(field, "anchorB") == 0) {
		cpVect val;
		read<3>(L, val);

		cpSlideJointSetAnchorB(obj->get(), val);

		return 0;
	} else if (strcmp(field, "min") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpSlideJointSetMin(obj->get(), val);

		return 0;
	} else if (strcmp(field, "max") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpSlideJointSetMax(obj->get(), val);

		return 0;
	} else {
		bool proceeded = false;
		const int result = Constraint___newindexBase(L, obj, field, &proceeded);
		if (proceeded)
			return result;
		else
			return 0;
	}
}

static int Constraint_setPreSolveHandler(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	Function::Ptr handler = nullptr;
	read<>(L, obj, handler);

	if (obj && obj->get()) {
		ConstraintData* data = ConstraintData::get(obj->get());
		if (handler) {
			data->preSolveHandler = handler;
			cpConstraintSetPostSolveFunc(obj->get(), ConstraintData::onPreSolved);
		} else {
			data->preSolveHandler = nullptr;
			cpConstraintSetPostSolveFunc(obj->get(), nullptr);
		}
	}

	return 0;
}

static int Constraint_setPostSolveHandler(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	Function::Ptr handler = nullptr;
	read<>(L, obj, handler);

	if (obj && obj->get()) {
		ConstraintData* data = ConstraintData::get(obj->get());
		if (handler) {
			data->postSolveHandler = handler;
			cpConstraintSetPostSolveFunc(obj->get(), ConstraintData::onPostSolved);
		} else {
			data->postSolveHandler = nullptr;
			cpConstraintSetPostSolveFunc(obj->get(), nullptr);
		}
	}

	return 0;
}

static int Constraint___index(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (cpConstraintIsDampedRotarySpring(obj->get()))
		return DampedRotarySpring___index(L);
	else if (cpConstraintIsDampedSpring(obj->get()))
		return DampedSpring___index(L);
	else if (cpConstraintIsGearJoint(obj->get()))
		return GearJoint___index(L);
	else if (cpConstraintIsGrooveJoint(obj->get()))
		return GrooveJoint___index(L);
	else if (cpConstraintIsPinJoint(obj->get()))
		return PinJoint___index(L);
	else if (cpConstraintIsPivotJoint(obj->get()))
		return PivotJoint___index(L);
	else if (cpConstraintIsRatchetJoint(obj->get()))
		return RatchetJoint___index(L);
	else if (cpConstraintIsRotaryLimitJoint(obj->get()))
		return RotaryLimitJoint___index(L);
	else if (cpConstraintIsSimpleMotor(obj->get()))
		return SimpleMotor___index(L);
	else if (cpConstraintIsSlideJoint(obj->get()))
		return SlideJoint___index(L);

	bool proceeded = false;
	const int result = Constraint___indexBase(L, obj, field, &proceeded);
	if (proceeded)
		return result;
	else
		return __index(L, field);
}

static int Constraint___newindex(lua_State* L) {
	Constraint::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (cpConstraintIsDampedRotarySpring(obj->get()))
		return DampedRotarySpring___newindex(L);
	else if (cpConstraintIsDampedSpring(obj->get()))
		return DampedSpring___newindex(L);
	else if (cpConstraintIsGearJoint(obj->get()))
		return GearJoint___newindex(L);
	else if (cpConstraintIsGrooveJoint(obj->get()))
		return GrooveJoint___newindex(L);
	else if (cpConstraintIsPinJoint(obj->get()))
		return PinJoint___newindex(L);
	else if (cpConstraintIsPivotJoint(obj->get()))
		return PivotJoint___newindex(L);
	else if (cpConstraintIsRatchetJoint(obj->get()))
		return RatchetJoint___newindex(L);
	else if (cpConstraintIsRotaryLimitJoint(obj->get()))
		return RotaryLimitJoint___newindex(L);
	else if (cpConstraintIsSimpleMotor(obj->get()))
		return SimpleMotor___newindex(L);
	else if (cpConstraintIsSlideJoint(obj->get()))
		return SlideJoint___newindex(L);

	bool proceeded = false;
	const int result = Constraint___newindexBase(L, obj, field, &proceeded);
	if (proceeded)
		return result;
	else
		return 0;
}

static void open_Constraint(lua_State* L) {
	def(
		L, "Constraint",
		LUA_LIB(
			array<luaL_Reg>()
		),
		array(
			luaL_Reg{ "__gc", __gc<Constraint::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Constraint::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "setPreSolveHandler", Constraint_setPreSolveHandler },
			luaL_Reg{ "setPostSolveHandler", Constraint_setPostSolveHandler },
			luaL_Reg{ nullptr, nullptr }
		),
		Constraint___index, Constraint___newindex
	);

	getGlobal(L, "Constraint");
	setTable(
		L,
		"Invalid", (Enum)ConstraintData::INVALID,
		"DampedRotarySpring", (Enum)ConstraintData::DAMPED_ROTARY_SPRING,
		"DampedSpring", (Enum)ConstraintData::DAMPED_SPRING,
		"GearJoint", (Enum)ConstraintData::GEAR_JOINT,
		"GrooveJoint", (Enum)ConstraintData::GROOVE_JOINT,
		"PinJoint", (Enum)ConstraintData::PIN_JOINT,
		"PivotJoint", (Enum)ConstraintData::PIVOT_JOINT,
		"RatchetJoint", (Enum)ConstraintData::RATCHET_JOINT,
		"RotaryLimitJoint", (Enum)ConstraintData::ROTARY_LIMIT_JOINT,
		"SimpleMotor", (Enum)ConstraintData::SIMPLE_MOTOR,
		"SlideJoint", (Enum)ConstraintData::SLIDE_JOINT,

		"__name", "Constraint"
	);
	pop(L);
}

static int DampedRotarySpring_ctor(lua_State* L) {
	Body::Ptr* bodyA = nullptr;
	Body::Ptr* bodyB = nullptr;
	cpFloat restAngle = 0;
	cpFloat stiffness = 0;
	cpFloat damping = 0;
	check<>(L, bodyA, bodyB, restAngle, stiffness, damping);

	if (bodyA && bodyA->get() && bodyB && bodyB->get()) {
		cpConstraint* ptr = cpDampedRotarySpringNew(bodyA->get(), bodyB->get(), restAngle, stiffness, damping);
		Constraint::Ptr obj(
			ptr,
			Constraint_dtor
		);
		ConstraintData* data = new ConstraintData(obj, L);
		cpConstraintSetUserData(ptr, data);

		return write(L, &obj);
	}

	return write(L, nullptr);
}

static void open_DampedRotarySpring(lua_State* L) {
	def(
		L, "DampedRotarySpring",
		LUA_LIB(
			array(
				luaL_Reg{ "new", DampedRotarySpring_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Constraint::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Constraint::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ nullptr, nullptr }
		),
		DampedRotarySpring___index, DampedRotarySpring___newindex
	);

	getGlobal(L, "DampedRotarySpring");
	setTable(
		L,
		"__name", "DampedRotarySpring"
	);
	pop(L);
}

static int DampedSpring_ctor(lua_State* L) {
	Body::Ptr* bodyA = nullptr;
	Body::Ptr* bodyB = nullptr;
	cpVect anchorA;
	cpVect anchorB;
	cpFloat restLength = 0;
	cpFloat stiffness = 0;
	cpFloat damping = 0;
	check<>(L, bodyA, bodyB, anchorA, anchorB, restLength, stiffness, damping);

	if (bodyA && bodyA->get() && bodyB && bodyB->get()) {
		cpConstraint* ptr = cpDampedSpringNew(bodyA->get(), bodyB->get(), anchorA, anchorB, restLength, stiffness, damping);
		Constraint::Ptr obj(
			ptr,
			Constraint_dtor
		);
		ConstraintData* data = new ConstraintData(obj, L);
		cpConstraintSetUserData(ptr, data);

		return write(L, &obj);
	}

	return write(L, nullptr);
}

static void open_DampedSpring(lua_State* L) {
	def(
		L, "DampedSpring",
		LUA_LIB(
			array(
				luaL_Reg{ "new", DampedSpring_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Constraint::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Constraint::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ nullptr, nullptr }
		),
		DampedSpring___index, DampedSpring___newindex
	);

	getGlobal(L, "DampedSpring");
	setTable(
		L,
		"__name", "DampedSpring"
	);
	pop(L);
}

static int GearJoint_ctor(lua_State* L) {
	Body::Ptr* bodyA = nullptr;
	Body::Ptr* bodyB = nullptr;
	cpFloat phase = 0;
	cpFloat ratio = 0;
	check<>(L, bodyA, bodyB, phase, ratio);

	if (bodyA && bodyA->get() && bodyB && bodyB->get()) {
		cpConstraint* ptr = cpGearJointNew(bodyA->get(), bodyB->get(), phase, ratio);
		Constraint::Ptr obj(
			ptr,
			Constraint_dtor
		);
		ConstraintData* data = new ConstraintData(obj, L);
		cpConstraintSetUserData(ptr, data);

		return write(L, &obj);
	}

	return write(L, nullptr);
}

static void open_GearJoint(lua_State* L) {
	def(
		L, "GearJoint",
		LUA_LIB(
			array(
				luaL_Reg{ "new", GearJoint_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Constraint::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Constraint::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ nullptr, nullptr }
		),
		GearJoint___index, GearJoint___newindex
	);

	getGlobal(L, "GearJoint");
	setTable(
		L,
		"__name", "GearJoint"
	);
	pop(L);
}

static int GrooveJoint_ctor(lua_State* L) {
	Body::Ptr* bodyA = nullptr;
	Body::Ptr* bodyB = nullptr;
	cpVect grooveA;
	cpVect grooveB;
	cpVect anchorB;
	check<>(L, bodyA, bodyB, grooveA, grooveB, anchorB);

	if (bodyA && bodyA->get() && bodyB && bodyB->get()) {
		cpConstraint* ptr = cpGrooveJointNew(bodyA->get(), bodyB->get(), grooveA, grooveB, anchorB);
		Constraint::Ptr obj(
			ptr,
			Constraint_dtor
		);
		ConstraintData* data = new ConstraintData(obj, L);
		cpConstraintSetUserData(ptr, data);

		return write(L, &obj);
	}

	return write(L, nullptr);
}

static void open_GrooveJoint(lua_State* L) {
	def(
		L, "GrooveJoint",
		LUA_LIB(
			array(
				luaL_Reg{ "new", GrooveJoint_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Constraint::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Constraint::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ nullptr, nullptr }
		),
		GrooveJoint___index, GrooveJoint___newindex
	);

	getGlobal(L, "GrooveJoint");
	setTable(
		L,
		"__name", "GrooveJoint"
	);
	pop(L);
}

static int PinJoint_ctor(lua_State* L) {
	Body::Ptr* bodyA = nullptr;
	Body::Ptr* bodyB = nullptr;
	cpVect anchorA;
	cpVect anchorB;
	check<>(L, bodyA, bodyB, anchorA, anchorB);

	if (bodyA && bodyA->get() && bodyB && bodyB->get()) {
		cpConstraint* ptr = cpPinJointNew(bodyA->get(), bodyB->get(), anchorA, anchorB);
		Constraint::Ptr obj(
			ptr,
			Constraint_dtor
		);
		ConstraintData* data = new ConstraintData(obj, L);
		cpConstraintSetUserData(ptr, data);

		return write(L, &obj);
	}

	return write(L, nullptr);
}

static void open_PinJoint(lua_State* L) {
	def(
		L, "PinJoint",
		LUA_LIB(
			array(
				luaL_Reg{ "new", PinJoint_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Constraint::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Constraint::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ nullptr, nullptr }
		),
		PinJoint___index, PinJoint___newindex
	);

	getGlobal(L, "PinJoint");
	setTable(
		L,
		"__name", "PinJoint"
	);
	pop(L);
}

static int PivotJoint_ctor(lua_State* L) {
	const int n = getTop(L);
	Body::Ptr* bodyA = nullptr;
	Body::Ptr* bodyB = nullptr;
	cpVect anchorA = { 0, 0 };
	cpVect anchorB = { 0, 0 };
	cpVect pivot = { 0, 0 };
	if (n >= 4)
		check<>(L, bodyA, bodyB, anchorA, anchorB);
	else
		check<>(L, bodyA, bodyB, pivot);

	if (bodyA && bodyA->get() && bodyB && bodyB->get()) {
		cpConstraint* ptr = nullptr;
		if (n >= 4)
			ptr = cpPivotJointNew2(bodyA->get(), bodyB->get(), anchorA, anchorB);
		else
			ptr = cpPivotJointNew(bodyA->get(), bodyB->get(), pivot);
		Constraint::Ptr obj(
			ptr,
			Constraint_dtor
		);
		ConstraintData* data = new ConstraintData(obj, L);
		cpConstraintSetUserData(ptr, data);

		return write(L, &obj);
	}

	return write(L, nullptr);
}

static void open_PivotJoint(lua_State* L) {
	def(
		L, "PivotJoint",
		LUA_LIB(
			array(
				luaL_Reg{ "new", PivotJoint_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Constraint::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Constraint::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ nullptr, nullptr }
		),
		PivotJoint___index, PivotJoint___newindex
	);

	getGlobal(L, "PivotJoint");
	setTable(
		L,
		"__name", "PivotJoint"
	);
	pop(L);
}

static int RatchetJoint_ctor(lua_State* L) {
	Body::Ptr* bodyA = nullptr;
	Body::Ptr* bodyB = nullptr;
	cpFloat phase = 0;
	cpFloat ratchet = 0;
	check<>(L, bodyA, bodyB, phase, ratchet);

	if (bodyA && bodyA->get() && bodyB && bodyB->get()) {
		cpConstraint* ptr = cpRatchetJointNew(bodyA->get(), bodyB->get(), phase, ratchet);
		Constraint::Ptr obj(
			ptr,
			Constraint_dtor
		);
		ConstraintData* data = new ConstraintData(obj, L);
		cpConstraintSetUserData(ptr, data);

		return write(L, &obj);
	}

	return write(L, nullptr);
}

static void open_RatchetJoint(lua_State* L) {
	def(
		L, "RatchetJoint",
		LUA_LIB(
			array(
				luaL_Reg{ "new", RatchetJoint_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Constraint::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Constraint::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ nullptr, nullptr }
		),
		RatchetJoint___index, RatchetJoint___newindex
	);

	getGlobal(L, "RatchetJoint");
	setTable(
		L,
		"__name", "RatchetJoint"
	);
	pop(L);
}

static int RotaryLimitJoint_ctor(lua_State* L) {
	Body::Ptr* bodyA = nullptr;
	Body::Ptr* bodyB = nullptr;
	cpFloat min = 0;
	cpFloat max = 0;
	check<>(L, bodyA, bodyB, min, max);

	if (bodyA && bodyA->get() && bodyB && bodyB->get()) {
		cpConstraint* ptr = cpRotaryLimitJointNew(bodyA->get(), bodyB->get(), min, max);
		Constraint::Ptr obj(
			ptr,
			Constraint_dtor
		);
		ConstraintData* data = new ConstraintData(obj, L);
		cpConstraintSetUserData(ptr, data);

		return write(L, &obj);
	}

	return write(L, nullptr);
}

static void open_RotaryLimitJoint(lua_State* L) {
	def(
		L, "RotaryLimitJoint",
		LUA_LIB(
			array(
				luaL_Reg{ "new", RotaryLimitJoint_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Constraint::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Constraint::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ nullptr, nullptr }
		),
		RotaryLimitJoint___index, RotaryLimitJoint___newindex
	);

	getGlobal(L, "RotaryLimitJoint");
	setTable(
		L,
		"__name", "RotaryLimitJoint"
	);
	pop(L);
}

static int SimpleMotor_ctor(lua_State* L) {
	Body::Ptr* bodyA = nullptr;
	Body::Ptr* bodyB = nullptr;
	cpFloat rate = 0;
	check<>(L, bodyA, bodyB, rate);

	if (bodyA && bodyA->get() && bodyB && bodyB->get()) {
		cpConstraint* ptr = cpSimpleMotorNew(bodyA->get(), bodyB->get(), rate);
		Constraint::Ptr obj(
			ptr,
			Constraint_dtor
		);
		ConstraintData* data = new ConstraintData(obj, L);
		cpConstraintSetUserData(ptr, data);

		return write(L, &obj);
	}

	return write(L, nullptr);
}

static void open_SimpleMotor(lua_State* L) {
	def(
		L, "SimpleMotor",
		LUA_LIB(
			array(
				luaL_Reg{ "new", SimpleMotor_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Constraint::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Constraint::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ nullptr, nullptr }
		),
		SimpleMotor___index, SimpleMotor___newindex
	);

	getGlobal(L, "SimpleMotor");
	setTable(
		L,
		"__name", "SimpleMotor"
	);
	pop(L);
}

static int SlideJoint_ctor(lua_State* L) {
	Body::Ptr* bodyA = nullptr;
	Body::Ptr* bodyB = nullptr;
	cpVect anchorA;
	cpVect anchorB;
	cpFloat min = 0;
	cpFloat max = 0;
	check<>(L, bodyA, bodyB, anchorA, anchorB, min, max);

	if (bodyA && bodyA->get() && bodyB && bodyB->get()) {
		cpConstraint* ptr = cpSlideJointNew(bodyA->get(), bodyB->get(), anchorA, anchorB, min, max);
		Constraint::Ptr obj(
			ptr,
			Constraint_dtor
		);
		ConstraintData* data = new ConstraintData(obj, L);
		cpConstraintSetUserData(ptr, data);

		return write(L, &obj);
	}

	return write(L, nullptr);
}

static void open_SlideJoint(lua_State* L) {
	def(
		L, "SlideJoint",
		LUA_LIB(
			array(
				luaL_Reg{ "new", SlideJoint_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Constraint::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Constraint::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ nullptr, nullptr }
		),
		SlideJoint___index, SlideJoint___newindex
	);

	getGlobal(L, "SlideJoint");
	setTable(
		L,
		"__name", "SlideJoint"
	);
	pop(L);
}

/**< Contact. */

static int Contact___index(lua_State* L) {
	Contact::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "pointA") == 0) {
		const cpVect &ret = obj->get()->pointA;

		return write(L, ret);
	} else if (strcmp(field, "pointB") == 0) {
		const cpVect &ret = obj->get()->pointB;

		return write(L, ret);
	} else if (strcmp(field, "distance") == 0) {
		const cpFloat ret = obj->get()->distance;

		return write(L, ret);
	} else {
		return __index(L, field);
	}
}

static int Contact___newindex(lua_State* L) {
	Contact::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "pointA") == 0) {
		cpVect val;
		read<3>(L, val);

		obj->get()->pointA = val;
	} else if (strcmp(field, "pointB") == 0) {
		cpVect val;
		read<3>(L, val);

		obj->get()->pointB = val;
	} else if (strcmp(field, "distance") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		obj->get()->distance = val;
	}

	return 0;
}

static void open_Contact(lua_State* L) {
	def(
		L, "Contact",
		LUA_LIB(
			array<luaL_Reg>()
		),
		array(
			luaL_Reg{ "__gc", __gc<Contact::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Contact::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ nullptr, nullptr }
		),
		Contact___index, Contact___newindex
	);

	getGlobal(L, "Contact");
	setTable(
		L,
		"__name", "Contact"
	);
	pop(L);
}

static int Contacts___len(lua_State* L) {
	Contacts::Ptr* obj = nullptr;
	check<>(L, obj);

	if (obj && obj->get()) {
		const int ret = obj->get()->count;

		return write(L, ret);
	}

	return 0;
}

static int Contacts___index(lua_State* L) {
	Contacts::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "shape") == 0) {
		const Shape::Ptr &ret = obj->get()->shape;

		return write(L, &ret);
	} else if (strcmp(field, "normal") == 0) {
		const cpVect &ret = obj->get()->normal;

		return write(L, ret);
	} else if (strcmp(field, "points") == 0) {
		Contact::Array ret;
		for (int i = 0; i < obj->get()->count; ++i)
			ret.push_back(obj->get()->points[i]);

		return write(L, ret);
	} else {
		return __index(L, field);
	}
}

static int Contacts___newindex(lua_State* L) {
	Contacts::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	return 0;
}

static void open_Contacts(lua_State* L) {
	def(
		L, "Contacts",
		LUA_LIB(
			array<luaL_Reg>()
		),
		array(
			luaL_Reg{ "__gc", __gc<Contacts::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Contacts::Ptr> },
			luaL_Reg{ "__len", Contacts___len },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ nullptr, nullptr }
		),
		Contacts___index, Contacts___newindex
	);

	getGlobal(L, "Contacts");
	setTable(
		L,
		"__name", "Contacts"
	);
	pop(L);
}

/**< Queries. */

static int PointQuery___index(lua_State* L) {
	PointQuery::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "shape") == 0) {
		const Shape::Ptr &ret = obj->get()->shape;

		return write(L, &ret);
	} else if (strcmp(field, "point") == 0) {
		const cpVect &ret = obj->get()->point;

		return write(L, ret);
	} else if (strcmp(field, "distance") == 0) {
		const cpFloat ret = obj->get()->distance;

		return write(L, ret);
	} else if (strcmp(field, "gradient") == 0) {
		const cpVect &ret = obj->get()->gradient;

		return write(L, ret);
	} else {
		return __index(L, field);
	}
}

static int PointQuery___newindex(lua_State* L) {
	PointQuery::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	return 0;
}

static void open_PointQuery(lua_State* L) {
	def(
		L, "PointQuery",
		LUA_LIB(
			array<luaL_Reg>()
		),
		array(
			luaL_Reg{ "__gc", __gc<PointQuery::Ptr> },
			luaL_Reg{ "__tostring", __tostring<PointQuery::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ nullptr, nullptr }
		),
		PointQuery___index, PointQuery___newindex
	);

	getGlobal(L, "PointQuery");
	setTable(
		L,
		"__name", "PointQuery"
	);
	pop(L);
}

static int SegmentQuery___index(lua_State* L) {
	SegmentQuery::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "shape") == 0) {
		const Shape::Ptr &ret = obj->get()->shape;

		return write(L, &ret);
	} else if (strcmp(field, "point") == 0) {
		const cpVect &ret = obj->get()->point;

		return write(L, ret);
	} else if (strcmp(field, "normal") == 0) {
		const cpVect &ret = obj->get()->normal;

		return write(L, ret);
	} else if (strcmp(field, "alpha") == 0) {
		const cpFloat ret = obj->get()->alpha;

		return write(L, ret);
	} else {
		return __index(L, field);
	}
}

static int SegmentQuery___newindex(lua_State* L) {
	SegmentQuery::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	return 0;
}

static void open_SegmentQuery(lua_State* L) {
	def(
		L, "SegmentQuery",
		LUA_LIB(
			array<luaL_Reg>()
		),
		array(
			luaL_Reg{ "__gc", __gc<SegmentQuery::Ptr> },
			luaL_Reg{ "__tostring", __tostring<SegmentQuery::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ nullptr, nullptr }
		),
		SegmentQuery___index, SegmentQuery___newindex
	);

	getGlobal(L, "SegmentQuery");
	setTable(
		L,
		"__name", "SegmentQuery"
	);
	pop(L);
}

static int BoundingBoxQuery___index(lua_State* L) {
	BoundingBoxQuery::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "shape") == 0) {
		const Shape::Ptr &ret = obj->get()->shape;

		return write(L, &ret);
	} else {
		return __index(L, field);
	}
}

static int BoundingBoxQuery___newindex(lua_State* L) {
	BoundingBoxQuery::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	return 0;
}

static void open_BoundingBoxQuery(lua_State* L) {
	def(
		L, "BoundingBoxQuery",
		LUA_LIB(
			array<luaL_Reg>()
		),
		array(
			luaL_Reg{ "__gc", __gc<BoundingBoxQuery::Ptr> },
			luaL_Reg{ "__tostring", __tostring<BoundingBoxQuery::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ nullptr, nullptr }
		),
		BoundingBoxQuery___index, BoundingBoxQuery___newindex
	);

	getGlobal(L, "BoundingBoxQuery");
	setTable(
		L,
		"__name", "BoundingBoxQuery"
	);
	pop(L);
}

static int ShapeQuery___len(lua_State* L) {
	ShapeQuery::Ptr* obj = nullptr;
	check<>(L, obj);

	if (obj && obj->get()) {
		const int ret = obj->get()->count;

		return write(L, ret);
	}

	return 0;
}

static int ShapeQuery___index(lua_State* L) {
	ShapeQuery::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "shape") == 0) {
		const Shape::Ptr &ret = obj->get()->shape;

		return write(L, &ret);
	} else if (strcmp(field, "normal") == 0) {
		const cpVect &ret = obj->get()->normal;

		return write(L, ret);
	} else if (strcmp(field, "points") == 0) {
		Contact::Array ret;
		for (int i = 0; i < obj->get()->count; ++i)
			ret.push_back(obj->get()->points[i]);

		return write(L, ret);
	} else {
		return __index(L, field);
	}
}

static int ShapeQuery___newindex(lua_State* L) {
	ShapeQuery::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	return 0;
}

static void open_ShapeQuery(lua_State* L) {
	def(
		L, "ShapeQuery",
		LUA_LIB(
			array<luaL_Reg>()
		),
		array(
			luaL_Reg{ "__gc", __gc<ShapeQuery::Ptr> },
			luaL_Reg{ "__tostring", __tostring<ShapeQuery::Ptr> },
			luaL_Reg{ "__len", ShapeQuery___len },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ nullptr, nullptr }
		),
		ShapeQuery___index, ShapeQuery___newindex
	);

	getGlobal(L, "ShapeQuery");
	setTable(
		L,
		"__name", "ShapeQuery"
	);
	pop(L);
}

/**< Space. */

static cpBool Space_alwaysCollide(cpArbiter*, cpSpace*, cpDataPointer) {
	return cpTrue;
}

static void Space_doNothing(cpArbiter*, cpSpace*, cpDataPointer) {
}

static cpBool Space_defaultBegin(cpArbiter* arbiter, cpSpace* space, cpDataPointer /* data */) {
	cpBool retA = cpArbiterCallWildcardBeginA(arbiter, space);
	cpBool retB = cpArbiterCallWildcardBeginB(arbiter, space);

	return retA && retB;
}

static cpBool Space_defaultPreSolve(cpArbiter* arbiter, cpSpace* space, cpDataPointer /* data */) {
	cpBool retA = cpArbiterCallWildcardPreSolveA(arbiter, space);
	cpBool retB = cpArbiterCallWildcardPreSolveB(arbiter, space);

	return retA && retB;
}

static void Space_defaultPostSolve(cpArbiter* arbiter, cpSpace* space, cpDataPointer /* data */) {
	cpArbiterCallWildcardPostSolveA(arbiter, space);
	cpArbiterCallWildcardPostSolveB(arbiter, space);
}

static void Space_defaultSeparate(cpArbiter* arbiter, cpSpace* space, cpDataPointer /* data */) {
	cpArbiterCallWildcardSeparateA(arbiter, space);
	cpArbiterCallWildcardSeparateB(arbiter, space);
}

static bool Space_removeConstraint(cpSpace* space, cpConstraint* constraint) {
	ConstraintData* data = ConstraintData::get(constraint);
	if (!data->managed)
		return false;
	data->managed = false;

	SpaceData* spaceData = SpaceData::get(space);
	//spaceData->constraintCache.remove(constraint->get());

	cpSpaceRemoveConstraint(space, constraint);
	++spaceData->obsoleteObjectCount;

	return true;
}

static void Space_dtor(cpSpace* space) {
	// Prepare.
	SpaceData* spaceData = SpaceData::get(space);
	spaceData->quitting = true;

	// Dispose the static body.
	cpBody* staticBody = cpSpaceGetStaticBody(space);
	if (staticBody) {
		spaceData->bodyCache.remove(staticBody);

		BodyData* data = BodyData::get(staticBody);
		delete data;
		cpBodySetUserData(staticBody, nullptr);
	}

	// Collect the objects.
	spaceData->constraintCache.collect(
		std::bind(
			collectOne<cpConstraint, Constraint::Ptr>,
			std::placeholders::_1, std::placeholders::_2,
			[] (cpConstraint* constraint) -> bool {
				cpSpace* space = cpConstraintGetSpace(constraint);
				if (space) {
					ConstraintData* data = ConstraintData::get(constraint);
					if (data->managed)
						data->managed = false;

					cpSpaceRemoveConstraint(space, constraint);
				}

				return true;
			}
		)
	);
	spaceData->shapeCache.collect(
		std::bind(
			collectOne<cpShape, Shape::Ptr>,
			std::placeholders::_1, std::placeholders::_2,
			[] (cpShape* shape) -> bool {
				cpSpace* space = cpShapeGetSpace(shape);
				if (space) {
					ShapeData* data = ShapeData::get(shape);
					if (data->managed)
						data->managed = false;

					cpSpaceRemoveShape(space, shape);
				}

				return true;
			}
		)
	);
	spaceData->bodyCache.collect(
		std::bind(
			collectOne<cpBody, Body::Ptr>,
			std::placeholders::_1, std::placeholders::_2,
			[] (cpBody* body) -> bool {
				cpSpace* space = cpBodyGetSpace(body);
				if (space) {
					BodyData* data = BodyData::get(body);
					if (data->managed)
						data->managed = false;

					cpSpaceRemoveBody(space, body);
				}

				return true;
			}
		)
	);

	// Dispose the objects.
	cpSpaceEachConstraint(
		space,
		[] (cpConstraint* constraint, void* data) -> void {
			cpSpace* space = (cpSpace*)data;
			cpSpaceAddPostStepCallback(
				space,
				[] (cpSpace* space, void* data, void*) -> void {
					cpConstraint* constraint = (cpConstraint*)data;

					ConstraintData* data_ = ConstraintData::get(constraint);
					if (data_) {
						delete data_;
						cpConstraintSetUserData(constraint, nullptr);
					}

					cpSpaceRemoveConstraint(space, constraint);
					cpConstraintFree(constraint);
				},
				constraint,
				nullptr
			);
		},
		space
	);
	cpSpaceEachShape(
		space,
		[] (cpShape* shape, void* data) -> void {
			cpSpace* space = (cpSpace*)data;
			cpSpaceAddPostStepCallback(
				space,
				[] (cpSpace* space, void* data, void*) -> void {
					cpShape* shape = (cpShape*)data;

					ShapeData* data_ = ShapeData::get(shape);
					if (data_) {
						delete data_;
						cpShapeSetUserData(shape, nullptr);
					}

					cpSpaceRemoveShape(space, shape);
					cpShapeFree(shape);
				},
				shape,
				nullptr
			);
		},
		space
	);
	cpSpaceEachBody(
		space,
		[] (cpBody* body, void* data) -> void {
			cpSpace* space = (cpSpace*)data;
			cpSpaceAddPostStepCallback(
				space,
				[] (cpSpace* space, void* data, void*) -> void {
					cpBody* body = (cpBody*)data;

					BodyData* data_ = BodyData::get(body);
					if (data_) {
						delete data_;
						cpBodySetUserData(body, nullptr);
					}

					cpSpaceRemoveBody(space, body);
					cpBodyFree(body);
				},
				body,
				nullptr
			);
		},
		space
	);

	// Dispose the space.
	SpaceData* data = SpaceData::get(space);
	delete data;

	cpSpaceFree(space);
}

static int Space_ctor(lua_State* L) {
	Space::Ptr obj(
		cpSpaceNew(),
		Space_dtor
	);
	SpaceData* data = new SpaceData(obj, L);
	cpSpaceSetUserData(obj.get(), data);

	return write(L, &obj);
}

static int Space_setPostStepHandler(lua_State* L) {
	Space::Ptr* obj = nullptr;
	Function::Ptr handler = nullptr;
	Shape::Ptr* shapeKey = nullptr; Body::Ptr* bodyKey = nullptr; Constraint::Ptr* constraintKey = nullptr; uintptr_t uintKey = 0;
	read<>(L, obj, handler);
	do {
		read<3>(L, shapeKey);
		if (shapeKey)
			break;
		read<3>(L, bodyKey);
		if (bodyKey)
			break;
		read<3>(L, constraintKey);
		if (constraintKey)
			break;
		read<3>(L, uintKey);
		if (uintKey)
			break;
	} while (false);

	if (obj && obj->get()) {
		SpaceData* data = SpaceData::get(obj->get());
		if (!data->calling && !data->querying) {
			// Do nothing.

			//error(L, "Cannot set post step handler now.");

			//return 0;
		}
		if (handler) {
			void* key = shapeKey ? (void*)shapeKey :
				bodyKey ? (void*)bodyKey :
				constraintKey ? (void*)constraintKey :
				uintKey ? (void*)(uintptr_t)uintKey :
				(void*)obj;
			if (!key)
				return 0;

			data->postStepHandlers.insert(std::make_pair(key, handler));
			const bool ret = !!cpSpaceAddPostStepCallback(obj->get(), SpaceData::onPostStepped, key, nullptr);

			return write(L, ret);
		} else {
			return 0;
		}
	}

	return 0;
}

static int Space_setDefaultCollisionHandler(lua_State* L) {
	const int n = getTop(L);
	Space::Ptr* obj = nullptr;
	Function::Ptr onBegan = nullptr;
	Function::Ptr onPreSolved = nullptr;
	Function::Ptr onPostSolved = nullptr;
	Function::Ptr onSeparated = nullptr;
	if (n >= 5)
		read<>(L, obj, onBegan, onPreSolved, onPostSolved, onSeparated);
	else if (n == 4)
		read<>(L, obj, onBegan, onPreSolved, onPostSolved);
	else if (n == 3)
		read<>(L, obj, onBegan, onPreSolved);
	else if (n == 2)
		read<>(L, obj, onBegan);

	if (obj && obj->get()) {
		SpaceData* data = SpaceData::get(obj->get());
		if (onBegan || onPreSolved || onPostSolved || onSeparated)
			data->defaultHandler = SpaceData::CollisionHandler(onBegan, onPreSolved, onPostSolved, onSeparated);
		else
			data->defaultHandler.clear();

		cpCollisionHandler* handler = cpSpaceAddDefaultCollisionHandler(obj->get());
		handler->beginFunc = onBegan ? SpaceData::onDefaultCollisionBegan : Space_alwaysCollide;
		handler->preSolveFunc = onPreSolved ? SpaceData::onDefaultCollisionPreSolved : Space_alwaysCollide;
		handler->postSolveFunc = onPostSolved ? SpaceData::onDefaultCollisionPostSolved : Space_doNothing;
		handler->separateFunc = onSeparated ? SpaceData::onDefaultCollisionSeparated : Space_doNothing;
	}

	return 0;
}

static int Space_setCollisionHandler(lua_State* L) {
	const int n = getTop(L);
	Space::Ptr* obj = nullptr;
	cpCollisionType typeA = 0, typeB = 0;
	Function::Ptr onBegan = nullptr;
	Function::Ptr onPreSolved = nullptr;
	Function::Ptr onPostSolved = nullptr;
	Function::Ptr onSeparated = nullptr;
	if (n >= 7)
		read<>(L, obj, typeA, typeB, onBegan, onPreSolved, onPostSolved, onSeparated);
	else if (n == 6)
		read<>(L, obj, typeA, typeB, onBegan, onPreSolved, onPostSolved);
	else if (n == 5)
		read<>(L, obj, typeA, typeB, onBegan, onPreSolved);
	else if (n == 4)
		read<>(L, obj, typeA, typeB, onBegan);

	if (obj && obj->get()) {
		SpaceData* data = SpaceData::get(obj->get());
		const SpaceData::CollisionKey key(typeA, typeB);
		SpaceData::CollisionHandlerDictionary::iterator it = data->handlers.find(key);
		if (it != data->handlers.end())
			data->handlers.erase(it);
		if (onBegan || onPreSolved || onPostSolved || onSeparated) {
			data->handlers.insert(
				std::make_pair(
					key,
					SpaceData::CollisionHandler(onBegan, onPreSolved, onPostSolved, onSeparated)
				)
			);
		}

		cpCollisionHandler* handler = cpSpaceAddCollisionHandler(obj->get(), typeA, typeB);
		handler->beginFunc = onBegan ? SpaceData::onCollisionBegan : Space_defaultBegin;
		handler->preSolveFunc = onPreSolved ? SpaceData::onCollisionPreSolved : Space_defaultPreSolve;
		handler->postSolveFunc = onPostSolved ? SpaceData::onCollisionPostSolved : Space_defaultPostSolve;
		handler->separateFunc = onSeparated ? SpaceData::onCollisionSeparated : Space_defaultSeparate;
	}

	return 0;
}

static int Space_setWildcardHandler(lua_State* L) {
	const int n = getTop(L);
	Space::Ptr* obj = nullptr;
	cpCollisionType type = 0;
	Function::Ptr onBegan = nullptr;
	Function::Ptr onPreSolved = nullptr;
	Function::Ptr onPostSolved = nullptr;
	Function::Ptr onSeparated = nullptr;
	if (n >= 6)
		read<>(L, obj, type, onBegan, onPreSolved, onPostSolved, onSeparated);
	else if (n == 5)
		read<>(L, obj, type, onBegan, onPreSolved, onPostSolved);
	else if (n == 4)
		read<>(L, obj, type, onBegan, onPreSolved);
	else if (n == 3)
		read<>(L, obj, type, onBegan);

	if (obj && obj->get()) {
		SpaceData* data = SpaceData::get(obj->get());
		const SpaceData::CollisionKey key(type, CP_WILDCARD_COLLISION_TYPE);
		SpaceData::CollisionHandlerDictionary::iterator it = data->wildcardHandlers.find(key);
		if (it != data->wildcardHandlers.end())
			data->wildcardHandlers.erase(it);
		if (onBegan || onPreSolved || onPostSolved || onSeparated) {
			data->wildcardHandlers.insert(
				std::make_pair(
					key,
					SpaceData::CollisionHandler(onBegan, onPreSolved, onPostSolved, onSeparated)
				)
			);
		}

		cpCollisionHandler* handler = cpSpaceAddWildcardHandler(obj->get(), type);
		handler->beginFunc = onBegan ? SpaceData::onWildcardBegan : Space_alwaysCollide;
		handler->preSolveFunc = onPreSolved ? SpaceData::onWildcardPreSolved : Space_alwaysCollide;
		handler->postSolveFunc = onPostSolved ? SpaceData::onWildcardPostSolved : Space_doNothing;
		handler->separateFunc = onSeparated ? SpaceData::onWildcardSeparated : Space_doNothing;
	}

	return 0;
}

static int Space_addShape(lua_State* L) {
	Space::Ptr* obj = nullptr;
	Shape::Ptr* shape = nullptr;
	read<>(L, obj, shape);

	if (obj && obj->get() && shape && shape->get()) {
		if (cpShapeGetSpace(shape->get()) == obj->get()) {
			error(L, "You have already added this shape to this space. You must not add it a second time.");

			return write(L, nullptr);
		}
		if (!!cpShapeGetSpace(shape->get())) {
			error(L, "You have already added this shape to another space. You cannot add it to a second.");

			return write(L, nullptr);
		}
		if (!cpShapeGetBody(shape->get())) {
			error(L, "The shape's body is not defined.");

			return write(L, nullptr);
		}
		if (cpBodyGetSpace(cpShapeGetBody(shape->get())) != obj->get()) {
			error(L, "The shape's body must be added to the space before the shape.");

			return write(L, nullptr);
		}
		if (cpSpaceIsLocked(obj->get())) {
			error(L, "Cannot add Shape when the Space is locked.");

			return write(L, nullptr);
		}

		ShapeData* data = ShapeData::get(shape->get());
		if (data->managed)
			return write(L, nullptr);
		data->managed = true;

		cpShape* ptr = cpSpaceAddShape(obj->get(), shape->get());
		if (ptr) {
			assert(ptr == shape->get());

			SpaceData* spaceData = SpaceData::get(obj->get());
			spaceData->shapeCache.add(shape->get(), *shape);

			Shape::Ptr ret = ShapeData::ref(ptr);

			return write(L, &ret);
		}
	}

	return write(L, nullptr);
}

static int Space_removeShape(lua_State* L) {
	Space::Ptr* obj = nullptr;
	Shape::Ptr* shape = nullptr;
	read<>(L, obj, shape);

	if (obj && obj->get() && shape && shape->get()) {
		if (!cpSpaceContainsShape(obj->get(), shape->get())) {
			error(L, "Cannot remove a shape that was not added to the space. (Removed twice maybe?)");

			return write(L, false);
		}
		if (cpSpaceIsLocked(obj->get())) {
			error(L, "Cannot remove Shape when the Space is locked.");

			return write(L, false);
		}

		ShapeData* data = ShapeData::get(shape->get());
		if (!data->managed)
			return write(L, false);
		data->managed = false;

		SpaceData* spaceData = SpaceData::get(obj->get());
		//spaceData->shapeCache.remove(shape->get());

		cpSpaceRemoveShape(obj->get(), shape->get());
		++spaceData->obsoleteObjectCount;

		return write(L, true);
	}

	return write(L, false);
}

static int Space_hasShape(lua_State* L) {
	Space::Ptr* obj = nullptr;
	Shape::Ptr* shape = nullptr;
	read<>(L, obj, shape);

	if (obj && obj->get() && shape && shape->get()) {
		const bool ret = !!cpSpaceContainsShape(obj->get(), shape->get());

		return write(L, ret);
	}

	return write(L, false);
}

static int Space_addBody(lua_State* L) {
	Space::Ptr* obj = nullptr;
	Body::Ptr* body = nullptr;
	read<>(L, obj, body);

	if (obj && obj->get() && body && body->get()) {
		if (cpBodyGetSpace(body->get()) == obj->get()) {
			error(L, "You have already added this body to this space. You must not add it a second time.");

			return write(L, nullptr);
		}
		if (!!cpBodyGetSpace(body->get())) {
			error(L, "You have already added this body to another space. You cannot add it to a second.");

			return write(L, nullptr);
		}
		if (cpSpaceIsLocked(obj->get())) {
			error(L, "Cannot add Body when the Space is locked.");

			return write(L, nullptr);
		}

		BodyData* data = BodyData::get(body->get());
		if (data->managed)
			return write(L, nullptr);
		data->managed = true;

		cpBody* ptr = cpSpaceAddBody(obj->get(), body->get());
		if (ptr) {
			assert(ptr == body->get());

			SpaceData* spaceData = SpaceData::get(obj->get());
			spaceData->bodyCache.add(body->get(), *body);

			Body::Ptr ret = BodyData::ref(ptr);

			return write(L, &ret);
		}
	}

	return write(L, nullptr);
}

static int Space_removeBody(lua_State* L) {
	Space::Ptr* obj = nullptr;
	Body::Ptr* body = nullptr;
	read<>(L, obj, body);

	if (obj && obj->get() && body && body->get()) {
		if (cpSpaceGetStaticBody(obj->get()) == body->get()) {
			error(L, "Cannot remove the designated static body for the space.");

			return write(L, false);
		}
		if (!cpSpaceContainsBody(obj->get(), body->get())) {
			error(L, "Cannot remove a body that was not added to the space. (Removed twice maybe?)");

			return write(L, false);
		}
		if (cpSpaceIsLocked(obj->get())) {
			error(L, "Cannot remove Body when the Space is locked.");

			return write(L, false);
		}

		BodyData* data = BodyData::get(body->get());
		if (!data->managed)
			return write(L, false);
		data->managed = false;

		SpaceData* spaceData = SpaceData::get(obj->get());
		//spaceData->bodyCache.remove(body->get());

		cpBodyEachConstraint(
			body->get(),
			[] (cpBody* body, cpConstraint* constraint, void* /* data */) -> void {
				cpSpace* space = cpBodyGetSpace(body);
				if (cpSpaceContainsConstraint(space, constraint)) {
					Space_removeConstraint(space, constraint);
				} else {
					if (constraint->a == body)
						constraint->a = nullptr;
					if (constraint->b == body)
						constraint->b = nullptr;
				}
			},
			nullptr
		);

		cpSpaceRemoveBody(obj->get(), body->get());
		++spaceData->obsoleteObjectCount;

		return write(L, true);
	}

	return write(L, false);
}

static int Space_hasBody(lua_State* L) {
	Space::Ptr* obj = nullptr;
	Body::Ptr* body = nullptr;
	read<>(L, obj, body);

	if (obj && obj->get() && body && body->get()) {
		const bool ret = !!cpSpaceContainsBody(obj->get(), body->get());

		return write(L, ret);
	}

	return write(L, false);
}

static int Space_addConstraint(lua_State* L) {
	Space::Ptr* obj = nullptr;
	Constraint::Ptr* constraint = nullptr;
	read<>(L, obj, constraint);

	if (obj && obj->get() && constraint && constraint->get()) {
		if (cpConstraintGetSpace(constraint->get()) == obj->get()) {
			error(L, "You have already added this constraint to this space. You must not add it a second time.");

			return write(L, nullptr);
		}
		if (!!cpConstraintGetSpace(constraint->get())) {
			error(L, "You have already added this constraint to another space. You cannot add it to a second.");

			return write(L, nullptr);
		}
		if (!cpConstraintGetBodyA(constraint->get()) || !cpConstraintGetBodyB(constraint->get())) {
			error(L, "Constraint is attached to a nil body.");

			return write(L, nullptr);
		}
		if (cpSpaceIsLocked(obj->get())) {
			error(L, "Cannot add Constraint when the Space is locked.");

			return write(L, nullptr);
		}

		ConstraintData* data = ConstraintData::get(constraint->get());
		if (data->managed)
			return write(L, nullptr);
		data->managed = true;

		cpConstraint* ptr = cpSpaceAddConstraint(obj->get(), constraint->get());
		if (ptr) {
			assert(ptr == constraint->get());

			SpaceData* spaceData = SpaceData::get(obj->get());
			spaceData->constraintCache.add(constraint->get(), *constraint);

			Constraint::Ptr ret = ConstraintData::ref(ptr);

			return write(L, &ret);
		}
	}

	return write(L, nullptr);
}

static int Space_removeConstraint(lua_State* L) {
	Space::Ptr* obj = nullptr;
	Constraint::Ptr* constraint = nullptr;
	read<>(L, obj, constraint);

	if (obj && obj->get() && constraint && constraint->get()) {
		if (!cpSpaceContainsConstraint(obj->get(), constraint->get())) {
			error(L, "Cannot remove a constraint that was not added to the space. (Removed twice maybe?)");

			return write(L, false);
		}
		if (cpSpaceIsLocked(obj->get())) {
			error(L, "Cannot remove Constraint when the Space is locked.");

			return write(L, false);
		}

		const bool ret = Space_removeConstraint(obj->get(), constraint->get());

		return write(L, ret);
	}

	return write(L, false);
}

static int Space_hasConstraint(lua_State* L) {
	Space::Ptr* obj = nullptr;
	Constraint::Ptr* constraint = nullptr;
	read<>(L, obj, constraint);

	if (obj && obj->get() && constraint && constraint->get()) {
		const bool ret = !!cpSpaceContainsConstraint(obj->get(), constraint->get());

		return write(L, ret);
	}

	return write(L, false);
}

static int Space_pointQuery(lua_State* L) {
	const int n = getTop(L);
	Space::Ptr* obj = nullptr;
	cpVect point;
	cpFloat maxDistance = 0;
	bool queryAll = false;
	ShapeFilter::Ptr* filter = nullptr;
	if (n >= 5)
		read<>(L, obj, point, maxDistance, queryAll, filter);
	else if (n >= 4)
		read<>(L, obj, point, maxDistance, queryAll);
	else
		read<>(L, obj, point, maxDistance);

	if (obj && obj->get()) {
		if (queryAll) {
			PointQuery::Array ret;
			QueryData data(L, nullptr, nullptr, &ret, obj->get());
			const cpShapeFilter filter_ = (filter && filter->get()) ? *filter->get() : CP_SHAPE_FILTER_ALL;
			auto callback_ = [] (cpShape* shape, cpVect point, cpFloat distance, cpVect gradient, void* data) -> void {
				QueryData* data_ = (QueryData*)data;
				SpaceData* spaceData = SpaceData::get(data_->space);

				bool querying = false;
				VariableGuard<decltype(spaceData->calling)> guardQuerying(spaceData ? &spaceData->querying : &querying, spaceData ? spaceData->querying : querying, true);

				PointQuery::Array* coll = (PointQuery::Array*)data_->collection;
				const Shape::Ptr arg1 = ShapeData::ref(shape);
				const cpVect &arg2 = point;
				const cpFloat arg3 = distance;
				const cpVect &arg4 = gradient;
				coll->push_back(PointQuery(arg1, arg2, arg3, arg4));
			};
			cpSpacePointQuery(obj->get(), point, maxDistance, filter_, callback_, &data);

			return write(L, ret);
		} else {
			const cpShapeFilter filter_ = (filter && filter->get()) ? *filter->get() : CP_SHAPE_FILTER_ALL;
			cpPointQueryInfo info;
			cpShape* shape = cpSpacePointQueryNearest(obj->get(), point, maxDistance, filter_, &info);
			if (shape) {
				const Shape::Ptr arg1 = ShapeData::ref(shape);
				const cpVect &arg2 = info.point;
				const cpFloat arg3 = info.distance;
				const cpVect &arg4 = info.gradient;
				PointQuery::Ptr ret(new PointQuery(arg1, arg2, arg3, arg4));

				return write(L, &ret);
			}
		}
	}

	return write(L, nullptr);
}

static int Space_pointQueryAll(lua_State* L) {
	const int n = getTop(L);
	Space::Ptr* obj = nullptr;
	cpVect point;
	cpFloat maxDistance = 0;
	Function::Ptr callback = nullptr;
	ShapeFilter::Ptr* filter = nullptr;
	if (n >= 5)
		read<>(L, obj, point, maxDistance, callback, filter);
	else
		read<>(L, obj, point, maxDistance, callback);

	if (obj && obj->get() && callback) {
		int ret = 0;
		QueryData data(L, &ret, &callback, nullptr, obj->get());
		const cpShapeFilter filter_ = (filter && filter->get()) ? *filter->get() : CP_SHAPE_FILTER_ALL;
		auto callback_ = [] (cpShape* shape, cpVect point, cpFloat distance, cpVect gradient, void* data) -> void {
			QueryData* data_ = (QueryData*)data;
			SpaceData* spaceData = SpaceData::get(data_->space);

			bool querying = false;
			VariableGuard<decltype(spaceData->calling)> guardQuerying(spaceData ? &spaceData->querying : &querying, spaceData ? spaceData->querying : querying, true);

			const Shape::Ptr arg1 = ShapeData::ref(shape);
			const cpVect &arg2 = point;
			const cpFloat arg3 = distance;
			const cpVect &arg4 = gradient;
			ScriptingLua::check(data_->L, call(data_->L, **data_->callback, &arg1, arg2, arg3, arg4));
			++*data_->ret;
		};
		cpSpacePointQuery(obj->get(), point, maxDistance, filter_, callback_, &data);

		return write(L, ret);
	}

	return write(L, nullptr);
}

static int Space_pointQueryNearest(lua_State* L) {
	const int n = getTop(L);
	Space::Ptr* obj = nullptr;
	cpVect point;
	cpFloat maxDistance = 0;
	ShapeFilter::Ptr* filter = nullptr;
	if (n >= 4)
		read<>(L, obj, point, maxDistance, filter);
	else
		read<>(L, obj, point, maxDistance);

	if (obj && obj->get()) {
		const cpShapeFilter filter_ = (filter && filter->get()) ? *filter->get() : CP_SHAPE_FILTER_ALL;
		cpPointQueryInfo info;
		cpShape* shape = cpSpacePointQueryNearest(obj->get(), point, maxDistance, filter_, &info);
		if (shape) {
			const Shape::Ptr arg1 = ShapeData::ref(shape);
			const cpVect &arg2 = info.point;
			const cpFloat arg3 = info.distance;
			const cpVect &arg4 = info.gradient;
			PointQuery::Ptr ret(new PointQuery(arg1, arg2, arg3, arg4));

			return write(L, &ret);
		}
	}

	return write(L, nullptr);
}

static int Space_segmentQuery(lua_State* L) {
	const int n = getTop(L);
	Space::Ptr* obj = nullptr;
	cpVect start;
	cpVect end;
	cpFloat radius = 0;
	bool queryAll = false;
	ShapeFilter::Ptr* filter = nullptr;
	if (n >= 6)
		read<>(L, obj, start, end, radius, queryAll, filter);
	else if (n >= 5)
		read<>(L, obj, start, end, radius, queryAll);
	else
		read<>(L, obj, start, end, radius);

	if (obj && obj->get()) {
		if (queryAll) {
			SegmentQuery::Array ret;
			QueryData data(L, nullptr, nullptr, &ret, obj->get());
			const cpShapeFilter filter_ = (filter && filter->get()) ? *filter->get() : CP_SHAPE_FILTER_ALL;
			auto callback_ = [] (cpShape* shape, cpVect point, cpVect normal, cpFloat alpha, void* data) -> void {
				QueryData* data_ = (QueryData*)data;
				SpaceData* spaceData = SpaceData::get(data_->space);

				bool querying = false;
				VariableGuard<decltype(spaceData->calling)> guardQuerying(spaceData ? &spaceData->querying : &querying, spaceData ? spaceData->querying : querying, true);

				SegmentQuery::Array* coll = (SegmentQuery::Array*)data_->collection;
				const Shape::Ptr arg1 = ShapeData::ref(shape);
				const cpVect &arg2 = point;
				const cpVect &arg3 = normal;
				const cpFloat arg4 = alpha;
				coll->push_back(SegmentQuery(arg1, arg2, arg3, arg4));
			};
			cpSpaceSegmentQuery(obj->get(), start, end, radius, filter_, callback_, &data);

			return write(L, ret);
		} else {
			const cpShapeFilter filter_ = (filter && filter->get()) ? *filter->get() : CP_SHAPE_FILTER_ALL;
			cpSegmentQueryInfo info;
			cpShape* shape = cpSpaceSegmentQueryFirst(obj->get(), start, end, radius, filter_, &info);
			if (shape) {
				const Shape::Ptr arg1 = ShapeData::ref(shape);
				const cpVect &arg2 = info.point;
				const cpVect &arg3 = info.normal;
				const cpFloat arg4 = info.alpha;
				SegmentQuery::Ptr ret(new SegmentQuery(arg1, arg2, arg3, arg4));

				return write(L, &ret);
			}
		}
	}

	return write(L, nullptr);
}

static int Space_segmentQueryAll(lua_State* L) {
	const int n = getTop(L);
	Space::Ptr* obj = nullptr;
	cpVect start;
	cpVect end;
	cpFloat radius = 0;
	Function::Ptr callback = nullptr;
	ShapeFilter::Ptr* filter = nullptr;
	if (n >= 6)
		read<>(L, obj, start, end, radius, callback, filter);
	else
		read<>(L, obj, start, end, radius, callback);

	if (obj && obj->get() && callback) {
		int ret = 0;
		QueryData data(L, &ret, &callback, nullptr, obj->get());
		const cpShapeFilter filter_ = (filter && filter->get()) ? *filter->get() : CP_SHAPE_FILTER_ALL;
		auto callback_ = [] (cpShape* shape, cpVect point, cpVect normal, cpFloat alpha, void* data) -> void {
			QueryData* data_ = (QueryData*)data;
			SpaceData* spaceData = SpaceData::get(data_->space);

			bool querying = false;
			VariableGuard<decltype(spaceData->calling)> guardQuerying(spaceData ? &spaceData->querying : &querying, spaceData ? spaceData->querying : querying, true);

			const Shape::Ptr arg1 = ShapeData::ref(shape);
			const cpVect &arg2 = point;
			const cpVect &arg3 = normal;
			const cpFloat arg4 = alpha;
			ScriptingLua::check(data_->L, call(data_->L, **data_->callback, &arg1, arg2, arg3, arg4));
			++*data_->ret;
		};
		cpSpaceSegmentQuery(obj->get(), start, end, radius, filter_, callback_, &data);

		return write(L, ret);
	}

	return write(L, nullptr);
}

static int Space_segmentQueryFirst(lua_State* L) {
	const int n = getTop(L);
	Space::Ptr* obj = nullptr;
	cpVect start;
	cpVect end;
	cpFloat radius = 0;
	ShapeFilter::Ptr* filter = nullptr;
	if (n >= 5)
		read<>(L, obj, start, end, radius, filter);
	else
		read<>(L, obj, start, end, radius);

	if (obj && obj->get()) {
		const cpShapeFilter filter_ = (filter && filter->get()) ? *filter->get() : CP_SHAPE_FILTER_ALL;
		cpSegmentQueryInfo info;
		cpShape* shape = cpSpaceSegmentQueryFirst(obj->get(), start, end, radius, filter_, &info);
		if (shape) {
			const Shape::Ptr arg1 = ShapeData::ref(shape);
			const cpVect &arg2 = info.point;
			const cpVect &arg3 = info.normal;
			const cpFloat arg4 = info.alpha;
			SegmentQuery::Ptr ret(new SegmentQuery(arg1, arg2, arg3, arg4));

			return write(L, &ret);
		}
	}

	return write(L, nullptr);
}

static int Space_boundingBoxQuery(lua_State* L) {
	const int n = getTop(L);
	Space::Ptr* obj = nullptr;
	cpBB bb;
	ShapeFilter::Ptr* filter = nullptr;
	if (n >= 3)
		read<>(L, obj, bb, filter);
	else
		read<>(L, obj, bb);

	if (obj && obj->get()) {
		BoundingBoxQuery::Array ret;
		QueryData data(L, nullptr, nullptr, &ret, obj->get());
		const cpShapeFilter filter_ = (filter && filter->get()) ? *filter->get() : CP_SHAPE_FILTER_ALL;
		auto callback_ = [] (cpShape* shape, void* data) -> void {
			QueryData* data_ = (QueryData*)data;
			SpaceData* spaceData = SpaceData::get(data_->space);

			bool querying = false;
			VariableGuard<decltype(spaceData->calling)> guardQuerying(spaceData ? &spaceData->querying : &querying, spaceData ? spaceData->querying : querying, true);

			BoundingBoxQuery::Array* coll = (BoundingBoxQuery::Array*)data_->collection;
			const Shape::Ptr arg1 = ShapeData::ref(shape);
			coll->push_back(BoundingBoxQuery(arg1));
		};
		cpSpaceBBQuery(obj->get(), bb, filter_, callback_, &data);

		return write(L, ret);
	}

	return write(L, nullptr);
}

static int Space_boundingBoxQueryAll(lua_State* L) {
	const int n = getTop(L);
	Space::Ptr* obj = nullptr;
	cpBB bb;
	Function::Ptr callback = nullptr;
	ShapeFilter::Ptr* filter = nullptr;
	if (n >= 4)
		read<>(L, obj, bb, callback, filter);
	else
		read<>(L, obj, bb, callback);

	if (obj && obj->get() && callback) {
		int ret = 0;
		QueryData data(L, &ret, &callback, nullptr, obj->get());
		const cpShapeFilter filter_ = (filter && filter->get()) ? *filter->get() : CP_SHAPE_FILTER_ALL;
		auto callback_ = [] (cpShape* shape, void* data) -> void {
			QueryData* data_ = (QueryData*)data;
			SpaceData* spaceData = SpaceData::get(data_->space);

			bool querying = false;
			VariableGuard<decltype(spaceData->calling)> guardQuerying(spaceData ? &spaceData->querying : &querying, spaceData ? spaceData->querying : querying, true);

			const Shape::Ptr arg1 = ShapeData::ref(shape);
			ScriptingLua::check(data_->L, call(data_->L, **data_->callback, &arg1));
			++*data_->ret;
		};
		cpSpaceBBQuery(obj->get(), bb, filter_, callback_, &data);

		return write(L, ret);
	}

	return write(L, nullptr);
}

static int Space_shapeQuery(lua_State* L) {
	Space::Ptr* obj = nullptr;
	Shape::Ptr* shape = nullptr;
	read<>(L, obj, shape);

	if (obj && obj->get() && shape && shape->get()) {
		ShapeQuery::Array ret;
		QueryData data(L, nullptr, nullptr, &ret, obj->get());
		auto callback_ = [] (cpShape* shape, cpContactPointSet* points, void* data) -> void {
			QueryData* data_ = (QueryData*)data;
			SpaceData* spaceData = SpaceData::get(data_->space);

			bool querying = false;
			VariableGuard<decltype(spaceData->calling)> guardQuerying(spaceData ? &spaceData->querying : &querying, spaceData ? spaceData->querying : querying, true);

			ShapeQuery::Array* coll = (ShapeQuery::Array*)data_->collection;
			const Shape::Ptr arg1 = ShapeData::ref(shape);
			const cpVect &arg2 = points->normal;
			const int arg3 = points->count;
			const cpVect arg4 = points->count >= 1 ? points->points[0].pointA : cpVect{ 0, 0 };
			const cpVect arg5 = points->count >= 1 ? points->points[0].pointB : cpVect{ 0, 0 };
			const cpFloat arg6 = points->count >= 1 ? points->points[0].distance : 0;
			const cpVect arg7 = points->count >= 2 ? points->points[1].pointA : cpVect{ 0, 0 };
			const cpVect arg8 = points->count >= 2 ? points->points[1].pointB : cpVect{ 0, 0 };
			const cpFloat arg9 = points->count >= 2 ? points->points[1].distance : 0;
			coll->push_back(ShapeQuery(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9));
		};
		cpSpaceShapeQuery(obj->get(), shape->get(), callback_, &data);

		return write(L, ret);
	}

	return write(L, nullptr);
}

static int Space_shapeQueryAll(lua_State* L) {
	Space::Ptr* obj = nullptr;
	Shape::Ptr* shape = nullptr;
	Function::Ptr callback = nullptr;
	read<>(L, obj, shape, callback);

	if (obj && shape && callback) {
		int ret = 0;
		QueryData data(L, &ret, &callback, nullptr, obj->get());
		auto callback_ = [] (cpShape* shape, cpContactPointSet* points, void* data) -> void {
			QueryData* data_ = (QueryData*)data;
			SpaceData* spaceData = SpaceData::get(data_->space);

			bool querying = false;
			VariableGuard<decltype(spaceData->calling)> guardQuerying(spaceData ? &spaceData->querying : &querying, spaceData ? spaceData->querying : querying, true);

			const Shape::Ptr arg1 = ShapeData::ref(shape);
			const cpVect &arg2 = points->normal;
			const cpVect arg3 = points->count >= 1 ? points->points[0].pointA : cpVect{ 0, 0 };
			const cpVect arg4 = points->count >= 1 ? points->points[0].pointB : cpVect{ 0, 0 };
			const cpFloat arg5 = points->count >= 1 ? points->points[0].distance : 0;
			const cpVect arg6 = points->count >= 2 ? points->points[1].pointA : cpVect{ 0, 0 };
			const cpVect arg7 = points->count >= 2 ? points->points[1].pointB : cpVect{ 0, 0 };
			const cpFloat arg8 = points->count >= 2 ? points->points[1].distance : 0;
			ScriptingLua::check(data_->L, call(data_->L, **data_->callback, &arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8));
			++*data_->ret;
		};
		cpSpaceShapeQuery(obj->get(), shape->get(), callback_, &data);

		return write(L, ret);
	}

	return write(L, nullptr);
}

static int Space_query(lua_State* L) {
	Math::Vec2f* arg2_Vect = nullptr;
	Math::Vec2f* arg3_Vect = nullptr;
	Math::Rectf* arg2_BB = nullptr;
	Shape::Ptr* arg2_Shape = nullptr;

	read<3>(L, arg3_Vect);
	if (arg3_Vect)
		return Space_segmentQuery(L);

	read<2>(L, arg2_Vect);
	if (arg2_Vect)
		return Space_pointQuery(L);

	read<2>(L, arg2_Shape);
	if (arg2_Shape && arg2_Shape->get())
		return Space_shapeQuery(L);

	read<2>(L, arg2_BB);
	if (arg2_BB)
		return Space_boundingBoxQuery(L);

	return write(L, nullptr);
}

template<int Idx> int Space_foreachBody(lua_State* L) {
	Space::Ptr* obj = nullptr;
	Function::Ptr callback = nullptr;
	read<>(L, obj);
	read<Idx>(L, callback);

	if (obj && obj->get() && callback) {
		int ret = 0;
		IterationData data(L, &ret, &callback, nullptr);
		auto callback_ = [] (cpBody* body, void* data) -> void {
			IterationData* data_ = (IterationData*)data;
			const Body::Ptr arg1 = BodyData::ref(body);
			if (arg1) {
				ScriptingLua::check(data_->L, call(data_->L, **data_->callback, &arg1));
				++*data_->ret;
			}
		};
		cpSpaceEachBody(obj->get(), callback_, &data);

		return write(L, ret);
	}

	return write(L, nullptr);
}

template<int Idx> int Space_foreachShape(lua_State* L) {
	Space::Ptr* obj = nullptr;
	Function::Ptr callback = nullptr;
	read<>(L, obj);
	read<Idx>(L, callback);

	if (obj && obj->get() && callback) {
		int ret = 0;
		IterationData data(L, &ret, &callback, nullptr);
		auto callback_ = [] (cpShape* shape, void* data) -> void {
			IterationData* data_ = (IterationData*)data;
			const Shape::Ptr arg1 = ShapeData::ref(shape);
			if (arg1) {
				ScriptingLua::check(data_->L, call(data_->L, **data_->callback, &arg1));
				++*data_->ret;
			}
		};
		cpSpaceEachShape(obj->get(), callback_, &data);

		return write(L, ret);
	}

	return write(L, nullptr);
}

template<int Idx> int Space_foreachConstraint(lua_State* L) {
	Space::Ptr* obj = nullptr;
	Function::Ptr callback = nullptr;
	read<>(L, obj);
	read<Idx>(L, callback);

	if (obj && obj->get() && callback) {
		int ret = 0;
		IterationData data(L, &ret, &callback, nullptr);
		auto callback_ = [] (cpConstraint* constraint, void* data) -> void {
			IterationData* data_ = (IterationData*)data;
			const Constraint::Ptr arg1 = ConstraintData::ref(constraint);
			if (arg1) {
				ScriptingLua::check(data_->L, call(data_->L, **data_->callback, &arg1));
				++*data_->ret;
			}
		};
		cpSpaceEachConstraint(obj->get(), callback_, &data);

		return write(L, ret);
	}

	return write(L, nullptr);
}

static int Space_foreach(lua_State* L) {
	std::string y;
	if (isTable(L, 2)) {
		push(L, 2);
		getTable(L, "__name", y);
		pop(L);
	} else if (isString(L, 2)) {
		Placeholder _1;
		read<>(L, _1, y);
	}

	if (y == "Body")
		return Space_foreachBody<3>(L);
	else if (y == "Shape")
		return Space_foreachShape<3>(L);
	else if (y == "Constraint")
		return Space_foreachConstraint<3>(L);

	return write(L, nullptr);
}

static int Space_reindexStatic(lua_State* L) {
	Space::Ptr* obj = nullptr;
	read<>(L, obj);

	if (obj && obj->get())
		cpSpaceReindexStatic(obj->get());

	return 0;
}

static int Space_reindexShape(lua_State* L) {
	Space::Ptr* obj = nullptr;
	Shape::Ptr* shape = nullptr;
	read<>(L, obj, shape);

	if (obj && obj->get() && shape && shape->get())
		cpSpaceReindexShape(obj->get(), shape->get());

	return 0;
}

static int Space_reindexShapesForBody(lua_State* L) {
	Space::Ptr* obj = nullptr;
	Body::Ptr* body = nullptr;
	read<>(L, obj, body);

	if (obj && obj->get() && body && body->get())
		cpSpaceReindexShapesForBody(obj->get(), body->get());

	return 0;
}

static int Space_useSpatialHash(lua_State* L) {
	Space::Ptr* obj = nullptr;
	cpFloat dim = 0;
	int count = 0;
	read<>(L, obj, dim, count);

	if (obj && obj->get())
		cpSpaceUseSpatialHash(obj->get(), dim, count);

	return 0;
}

static int Space_step(lua_State* L) {
	Space::Ptr* obj = nullptr;
	cpFloat delta = 0;
	read<>(L, obj, delta);

	if (obj && obj->get()) {
		cpSpaceStep(obj->get(), delta);

		SpaceData* spaceData = SpaceData::get(obj->get());
		if (spaceData->obsoleteCollectEnabled) {
			if (spaceData->obsoleteObjectCount >= spaceData->obsoleteCollectThreshold) {
				collect(obj->get());
				spaceData->obsoleteObjectCount = 0;
			}
		}
	}

	return 0;
}

static int Space_collect(lua_State* L) {
	const int n = getTop(L);
	Space::Ptr* obj = nullptr;
	std::string option;
	int threshold = 1000;
	if (n >= 3)
		read<>(L, obj, option, threshold);
	else if (n == 2)
		read<>(L, obj, option);
	else
		read<>(L, obj);

	if (obj && obj->get()) {
		if (n == 1) {
			collect(obj->get());

			return 0;
		}

		SpaceData* spaceData = SpaceData::get(obj->get());
		if (option.empty() || option == "collect") {
			collect(obj->get());
		} else if (option == "stop") {
			spaceData->obsoleteCollectEnabled = false;
		} else if (option == "restart") {
			spaceData->obsoleteCollectEnabled = true;
		} else if (option == "isrunning") {
			return write(L, spaceData->obsoleteCollectEnabled);
		} else if (option == "threshold") {
			return write(L, spaceData->obsoleteCollectThreshold);
		} else if (option == "limit") {
			threshold = std::max(threshold, 1);
			spaceData->obsoleteCollectThreshold = threshold;

			return write(L, spaceData->obsoleteCollectThreshold);
		} else {
			collect(obj->get());
		}
	}

	return 0;
}

static int Space___index(lua_State* L) {
	Space::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "id") == 0) {
		const uintptr_t ret = (uintptr_t)obj->get();

		return write(L, ret);
	} else if (strcmp(field, "iterations") == 0) {
		const int ret = cpSpaceGetIterations(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "gravity") == 0) {
		const cpVect ret = cpSpaceGetGravity(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "damping") == 0) {
		const cpFloat ret = cpSpaceGetDamping(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "idleSpeedThreshold") == 0) {
		const cpFloat ret = cpSpaceGetIdleSpeedThreshold(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "sleepTimeThreshold") == 0) {
		const cpFloat ret = cpSpaceGetSleepTimeThreshold(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "collisionSlop") == 0) {
		const cpFloat ret = cpSpaceGetCollisionSlop(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "collisionBias") == 0) {
		const cpFloat ret = cpSpaceGetCollisionBias(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "collisionPersistence") == 0) {
		const cpTimestamp ret = cpSpaceGetCollisionPersistence(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "staticBody") == 0) {
		cpBody* ptr = cpSpaceGetStaticBody(obj->get());
		Body::Ptr ret = nullptr;
		if (ptr) {
			ret = Body::Ptr(
				ptr,
				[] (cpBody* body) -> void {
					cpBodyDestroy(body);
				}
			);

			BodyData* data = new BodyData(ret, L);
			cpBodySetUserData(ret.get(), data);

			SpaceData* spaceData = SpaceData::get(obj->get());
			spaceData->bodyCache.add(ret.get(), ret);
		}

		return write(L, &ret);
	} else if (strcmp(field, "currentTimeStep") == 0) {
		const cpFloat ret = cpSpaceGetCurrentTimeStep(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "isLocked") == 0) {
		const bool ret = !!cpSpaceIsLocked(obj->get());

		return write(L, ret);
	} else if (strcmp(field, "bodies") == 0) {
		Body::Array ret;
		IterationData data(L, nullptr, nullptr, &ret);
		auto callback_ = [] (cpBody* body, void* data) -> void {
			IterationData* data_ = (IterationData*)data;
			Body::Array* coll = (Body::Array*)data_->collection;
			const Body::Ptr arg1 = BodyData::ref(body);
			if (arg1)
				coll->push_back(arg1);
		};
		cpSpaceEachBody(obj->get(), callback_, &data);

		return write(L, ret);
	} else if (strcmp(field, "shapes") == 0) {
		Shape::Array ret;
		IterationData data(L, nullptr, nullptr, &ret);
		auto callback_ = [] (cpShape* shape, void* data) -> void {
			IterationData* data_ = (IterationData*)data;
			Shape::Array* coll = (Shape::Array*)data_->collection;
			const Shape::Ptr arg1 = ShapeData::ref(shape);
			if (arg1)
				coll->push_back(arg1);
		};
		cpSpaceEachShape(obj->get(), callback_, &data);

		return write(L, ret);
	} else if (strcmp(field, "constraints") == 0) {
		Constraint::Array ret;
		IterationData data(L, nullptr, nullptr, &ret);
		auto callback_ = [] (cpConstraint* constraint, void* data) -> void {
			IterationData* data_ = (IterationData*)data;
			Constraint::Array* coll = (Constraint::Array*)data_->collection;
			const Constraint::Ptr arg1 = ConstraintData::ref(constraint);
			if (arg1)
				coll->push_back(arg1);
		};
		cpSpaceEachConstraint(obj->get(), callback_, &data);

		return write(L, ret);
	} else {
		return __index(L, field);
	}
}

static int Space___newindex(lua_State* L) {
	Space::Ptr* obj = nullptr;
	const char* field = nullptr;
	read<>(L, obj, field);

	if (!obj || !obj->get() || !field)
		return 0;

	if (strcmp(field, "iterations") == 0) {
		int val = 0;
		read<3>(L, val);

		if (val <= 0) {
			error(L, "Iterations must be positive and non-zero");

			return 0;
		}
		cpSpaceSetIterations(obj->get(), val);
	} else if (strcmp(field, "gravity") == 0) {
		Math::Vec2f* val = nullptr;
		read<3>(L, val);

		if (val)
			cpSpaceSetGravity(obj->get(), cpVect{ val->x, val->y });
		else
			cpSpaceSetGravity(obj->get(), cpVect{ 0, 0 });
	} else if (strcmp(field, "damping") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		if (val < 0) {
			error(L, "Damping must be positive.");

			return 0;
		}
		cpSpaceSetDamping(obj->get(), val);
	} else if (strcmp(field, "idleSpeedThreshold") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpSpaceSetIdleSpeedThreshold(obj->get(), val);
	} else if (strcmp(field, "sleepTimeThreshold") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpSpaceSetSleepTimeThreshold(obj->get(), val);
	} else if (strcmp(field, "collisionSlop") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpSpaceSetCollisionSlop(obj->get(), val);
	} else if (strcmp(field, "collisionBias") == 0) {
		cpFloat val = 0;
		read<3>(L, val);

		cpSpaceSetCollisionBias(obj->get(), val);
	} else if (strcmp(field, "collisionPersistence") == 0) {
		cpTimestamp val = 0;
		read<3>(L, val);

		cpSpaceSetCollisionPersistence(obj->get(), val);
	}

	return 0;
}

static void open_Space(lua_State* L) {
	def(
		L, "Space",
		LUA_LIB(
			array(
				luaL_Reg{ "new", Space_ctor },
				luaL_Reg{ nullptr, nullptr }
			)
		),
		array(
			luaL_Reg{ "__gc", __gc<Space::Ptr> },
			luaL_Reg{ "__tostring", __tostring<Space::Ptr> },
			luaL_Reg{ nullptr, nullptr }
		),
		array(
			luaL_Reg{ "setPostStepHandler", Space_setPostStepHandler },
			luaL_Reg{ "setDefaultCollisionHandler", Space_setDefaultCollisionHandler },
			luaL_Reg{ "setCollisionHandler", Space_setCollisionHandler },
			luaL_Reg{ "setWildcardHandler", Space_setWildcardHandler },
			luaL_Reg{ "addShape", Space_addShape },
			luaL_Reg{ "removeShape", Space_removeShape },
			luaL_Reg{ "hasShape", Space_hasShape },
			luaL_Reg{ "addBody", Space_addBody },
			luaL_Reg{ "removeBody", Space_removeBody },
			luaL_Reg{ "hasBody", Space_hasBody },
			luaL_Reg{ "addConstraint", Space_addConstraint },
			luaL_Reg{ "removeConstraint", Space_removeConstraint },
			luaL_Reg{ "hasConstraint", Space_hasConstraint },
			luaL_Reg{ "pointQuery", Space_pointQuery },
			luaL_Reg{ "pointQueryAll", Space_pointQueryAll },
			luaL_Reg{ "pointQueryNearest", Space_pointQueryNearest },
			luaL_Reg{ "segmentQuery", Space_segmentQuery },
			luaL_Reg{ "segmentQueryAll", Space_segmentQueryAll },
			luaL_Reg{ "segmentQueryFirst", Space_segmentQueryFirst },
			luaL_Reg{ "boundingBoxQuery", Space_boundingBoxQuery },
			luaL_Reg{ "boundingBoxQueryAll", Space_boundingBoxQueryAll },
			luaL_Reg{ "shapeQuery", Space_shapeQuery },
			luaL_Reg{ "shapeQueryAll", Space_shapeQueryAll },
			luaL_Reg{ "query", Space_query },
			//luaL_Reg{ "foreachBody", Space_foreachBody<2> },
			//luaL_Reg{ "foreachShape", Space_foreachShape<2> },
			//luaL_Reg{ "foreachConstraint", Space_foreachConstraint<2> },
			luaL_Reg{ "foreach", Space_foreach },
			luaL_Reg{ "reindexStatic", Space_reindexStatic },
			luaL_Reg{ "reindexShape", Space_reindexShape },
			luaL_Reg{ "reindexShapesForBody", Space_reindexShapesForBody },
			luaL_Reg{ "useSpatialHash", Space_useSpatialHash },
			luaL_Reg{ "step", Space_step },
			luaL_Reg{ "collect", Space_collect },
			luaL_Reg{ nullptr, nullptr }
		),
		Space___index, Space___newindex
	);

	getGlobal(L, "Space");
	setTable(
		L,
		"__name", "Space"
	);
	pop(L);
}

/**< Physics. */

static int Physics_momentForCircle(lua_State* L) {
	cpFloat m = 0;
	cpFloat r1 = 0;
	cpFloat r2 = 0;
	cpVect offset;
	read<>(L, m, r1, r2, offset);

	const cpFloat ret = cpMomentForCircle(m, r1, r2, offset);

	return write(L, ret);
}

static int Physics_areaForCircle(lua_State* L) {
	cpFloat r1 = 0;
	cpFloat r2 = 0;
	read<>(L, r1, r2);

	const cpFloat ret = cpAreaForCircle(r1, r2);

	return write(L, ret);
}

static int Physics_momentForSegment(lua_State* L) {
	cpFloat m = 0;
	cpVect a;
	cpVect b;
	cpFloat radius = 0;
	read<>(L, m, a, b, radius);

	const cpFloat ret = cpMomentForSegment(m, a, b, radius);

	return write(L, ret);
}

static int Physics_areaForSegment(lua_State* L) {
	cpVect a;
	cpVect b;
	cpFloat radius = 0;
	read<>(L, a, b, radius);

	const cpFloat ret = cpAreaForSegment(a, b, radius);

	return write(L, ret);
}

static int Physics_momentForPolygon(lua_State* L) {
	cpFloat m = 0;
	Vertex::Array verts;
	cpVect offset;
	cpFloat radius = 0;
	read<>(L, m, verts, offset, radius);

	if (verts.empty())
		return write(L, nullptr);

	const cpFloat ret = cpMomentForPoly(m, (int)verts.size(), &verts.front(), offset, radius);

	return write(L, ret);
}

static int Physics_areaForPolygon(lua_State* L) {
	Vertex::Array verts;
	cpFloat radius = 0;
	read<>(L, verts, radius);

	if (verts.empty())
		return write(L, nullptr);

	const cpFloat ret = cpAreaForPoly((int)verts.size(), &verts.front(), radius);

	return write(L, ret);
}

static int Physics_centroidForPolygon(lua_State* L) {
	Vertex::Array verts;
	read<>(L, verts);

	if (verts.empty())
		return write(L, nullptr);

	const cpVect ret = cpCentroidForPoly((int)verts.size(), &verts.front());

	return write(L, ret);
}

static int Physics_momentForBox(lua_State* L) {
	const int n = getTop(L);
	cpFloat m = 0;
	cpFloat width = 0;
	cpFloat height = 0;
	cpBB box{ 0, 0, 0, 0 };
	if (n >= 3)
		read<>(L, m, width, height);
	else
		read<>(L, m, box);

	cpFloat ret = 0;
	if (n >= 3)
		ret = cpMomentForBox(m, width, height);
	else
		ret = cpMomentForBox2(m, box);

	return write(L, ret);
}

static int Physics_convexHull(lua_State* L) {
	const int n = getTop(L);
	Vertex::Array verts;
	cpFloat tol = 0;
	if (n >= 2)
		read<>(L, verts, tol);
	else
		read<>(L, verts);

	if (verts.empty())
		return write(L, nullptr, nullptr);

	int ret2 = 0;
	Vertex::Array ret1 = verts;
	const int count = cpConvexHull((int)verts.size(), &verts.front(), &ret1.front(), &ret2, tol);
	++ret2; // 1-based.
	ret1.resize(count);

	return write(L, ret1, ret2);
}

static int Physics_closetPointOnSegment(lua_State* L) {
	cpVect p;
	cpVect a;
	cpVect b;
	read<>(L, p, a, b);

	const cpVect ret = cpClosetPointOnSegment(p, a, b);

	return write(L, ret);
}

static void open_Physics(lua_State* L) {
	getGlobal(L, "Physics");
	setTable(
		L,
		"NoGroup", CP_NO_GROUP,
		"AllCategories", CP_ALL_CATEGORIES,
		"WildcardCollisionType", CP_WILDCARD_COLLISION_TYPE,

		"momentForCircle", Physics_momentForCircle,
		"areaForCircle", Physics_areaForCircle,
		"momentForSegment", Physics_momentForSegment,
		"areaForSegment", Physics_areaForSegment,
		"momentForPolygon", Physics_momentForPolygon,
		"areaForPolygon", Physics_areaForPolygon,
		"centroidForPolygon", Physics_centroidForPolygon,
		"momentForBox", Physics_momentForBox,
		"convexHull", Physics_convexHull,
		"closetPointOnSegment", Physics_closetPointOnSegment
	);
	pop(L);
}

/**< Categories. */

void physics(class Executable* exec) {
	// Prepare.
	lua_State* L = (lua_State*)exec->pointer();

	// Transform.
	open_Transform(L);

	// Arbiter.
	open_Arbiter(L);

	// Shape.
	open_ShapeFilter(L);
	open_Shape(L);

	// Body.
	open_Body(L);

	// Constraints.
	open_Constraint(L);
	open_DampedRotarySpring(L);
	open_DampedSpring(L);
	open_GearJoint(L);
	open_GrooveJoint(L);
	open_PinJoint(L);
	open_PivotJoint(L);
	open_RatchetJoint(L);
	open_RotaryLimitJoint(L);
	open_SimpleMotor(L);
	open_SlideJoint(L);

	// Contact.
	open_Contact(L);
	open_Contacts(L);

	// Queries.
	open_PointQuery(L);
	open_SegmentQuery(L);
	open_BoundingBoxQuery(L);
	open_ShapeQuery(L);

	// Space.
	open_Space(L);

	// Pack the modules.
	pack(L, "Physics", "Transform");

	pack(L, "Physics", "Arbiter");

	pack(L, "Physics", "ShapeFilter");
	pack(L, "Physics", "Shape");

	pack(L, "Physics", "Body");

	pack(L, "Physics", "Constraint");
	pack(L, "Physics", "DampedRotarySpring");
	pack(L, "Physics", "DampedSpring");
	pack(L, "Physics", "GearJoint");
	pack(L, "Physics", "GrooveJoint");
	pack(L, "Physics", "PinJoint");
	pack(L, "Physics", "PivotJoint");
	pack(L, "Physics", "RatchetJoint");
	pack(L, "Physics", "RotaryLimitJoint");
	pack(L, "Physics", "SimpleMotor");
	pack(L, "Physics", "SlideJoint");

	pack(L, "Physics", "Contact");
	pack(L, "Physics", "Contacts");

	pack(L, "Physics", "PointQuery");
	pack(L, "Physics", "SegmentQuery");
	pack(L, "Physics", "BoundingBoxQuery");
	pack(L, "Physics", "ShapeQuery");

	pack(L, "Physics", "Space");

	open_Physics(L);
}

}

}

/* ===========================================================================} */
