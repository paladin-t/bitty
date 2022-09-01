![](imgs/logo.png)

## Physics Manual

[Manual](manual) | [**Physics**](https://paladin-t.github.io/bitty/physics.html) | [Operations](operations) | [Others](others)

## Table of Content

* [Fundamental](#fundamental)
	* [Why Physics](#why-physics)
	* [Glossary](#glossary)
* [Physics](#physics)
	* [Space](#space)
	* [Shape](#shape)
	* [Body](#body)
	* [Constraints](#constraints)
		* [Constraint](#constraint)
		* [DampedRotarySpring](#dampedrotaryspring)
		* [DampedSpring](#dampedspring)
		* [GearJoint](#gearjoint)
		* [GrooveJoint](#groovejoint)
		* [PinJoint](#pinjoint)
		* [PivotJoint](#pivotjoint)
		* [RatchetJoint](#ratchetjoint)
		* [RotaryLimitJoint](#rotarylimitjoint)
		* [SimpleMotor](#simplemotor)
		* [SlideJoint](#slidejoint)
	* [Arbiter](#arbiter)
	* [Transform](#transform)
	* [ShapeFilter](#shapefilter)
	* [Contact](#contact)
	* [Contacts](#contacts)
	* [Queries](#queries)
		* [PointQuery](#pointquery)
		* [SegmentQuery](#segmentquery)
		* [BoundingBoxQuery](#boundingboxquery)
		* [ShapeQuery](#shapequery)

<!-- End Table of Content -->

[TOP](#physics)

# Fundamental

This module is a **Beta** feature. There may be unrevealed bugs, and the final API may be different from the current version.

Bitty Engine is integrated with the [Chipmunk2D](https://chipmunk-physics.net/) physics library to offer 2D dynamics simulation in Lua. The knowledges of the concepts and programming interfaces in Chipmunk2D are widely applicable to programming it in Bitty Engine.

Reading the [official documentations](https://chipmunk-physics.net/documentation.php) would be helpful to understand and master this module, although the original Chipmunk2D is written in C. You'll find that almost every interface has its counterpart, i.e. the Lua operations in Bitty Engine `Physics.Shape.new(Physics.Shape.Circle, ...)`, `Physics.Body.new(...)`, `body.position = ...`, `space:addBody(...)` are equivalent to the C version `cpCircleShapeNew(...)`, `cpBodyNew(...)`, `cpBodySetPosition(...)`, `cpSpaceAddBody(...)` in Chipmunk2D.

## Why Physics

First of all, this module is just a physics engine. It has nothing to do with graphics, it is only responsible for receiving input data, and outputing calculation result.

If your program doesn't contain complicated behaviours, then the generic algorithms and solutions would be capable. However, it is worth the time and effort required to learn it, because this module helps you simulate how objects interact with each other in a 2D world.

The physics module encapsuled complex calculations and procedures, exposed necessary interfaces, and runs at a decent speed. It uses some of the generic structures, i.e. `Vec2`, `Rect` to represent point, bounding box, etc.

## Glossary

**Space**

Spaces are containers for simulating objects in the physics module. You add bodies, shapes and joints to a space and then update the space as a whole. They control how all the rigid bodies, shapes, and constraints interact together.

**Shape**

By attaching shapes to bodies, you can define the a body's shape. You can attach as many shapes to a single body as you need to in order to define a complex shape. Shapes contain the surface properties of an object such as how much friction or elasticity it has.

**Body**

A rigid body holds the physical properties of an object (mass, position, rotation, velocity, etc.). It does not have a shape until you attach one or more collision shapes to it. Rigid bodies generally tend to have a 1:1 correlation to sprites in a program. You should structure your program so that you use the position and rotation of the rigid body for drawing your sprite.

**Constraint**

Constraints and joints describe how bodies are attached to each other.

**Arbiter**

The physics module's arbiter struct encapsulates a pair of colliding shapes and all of the data about their collision. Arbiters are created when a collision starts, and persist until those shapes are no longer colliding, thus you only get weak references to arbiters in code.

[TOP](#physics)

# Physics

All the programming interfaces of the physics module are packed in a named table `Physics`.

<!-- Begin Physics -->

**Constants**

* `Physics.NoGroup`: used to indicate "no group" for `ShapeFilter`
* `Physics.AllCategories`: used to indicate "all categories" for `ShapeFilter`
* `Physics.WildcardCollisionType`: used to indicate "wildcard collision type" for collision handlers

**Static Functions**

* `Physics.momentForCircle(m, r1, r2, offset)`: calculates the moment of inertia for a hollow circle
	* `r1`, `r2`: the inner and outer diameters in no particular order; a solid circle has an inner diameter of 0
* `Physics.areaForCircle(r1, r2)`: calculates the area of a hollow circle
* `Physics.momentForSegment(m, a, b, radius)`: calculates the moment of inertia for a line segment; the endpoints `a` and `b` are relative to the body
* `Physics.areaForSegment(a, b, radius)`: calculates the area of a beveled segment; will always be 0 if radius is 0
* `Physics.momentForPolygon(m, verts, offset, radius)`: calculates the moment of inertia for a solid polygon shape assuming its center of gravity is at its centroid; the `offset` is added to each vertex
* `Physics.areaForPolygon(verts, radius)`: calculates the signed area of a polygon shape
	* returns a negative number for polygons with a clockwise winding
* `Physics.centroidForPolygon(verts)`: calculates the centroid for a polygon
* `Physics.momentForBox(m, width, height)`: calculates the moment of inertia for a solid box centered on the body
* `Physics.momentForBox(m, box)`: calculates the moment of inertia for a solid box centered on the body
	* `box`: the specific box `Rect`
* `Physics.convexHull(verts, tol = 0)`: calculates the convex hull of a given set of points
	* `tol`: the allowed amount to shrink the hull when simplifying it; a tolerance of 0.0 creates an exact hull
	* returns the points in the hull, in a list of `Vec2`, and a secondary value for where the first vertex in the hull came from
* `Physics.closetPointOnSegment(p, a, b)`: calculates the closest point on the line segment `a`-`b` to the point `p`

<!-- End Physics -->

## Space

<!-- Begin Space -->

**Constructors**

* `Physics.Space.new()`: constructs a space object

**Object Fields**

* `space.id`: readonly, gets the ID of the `Space`; you can use it to identify a `Space` object, i.e. in a Lua table
* `space.iterations`: gets or sets the iterations of the `Space`, defaults to 10; iterations allow you to control the accuracy of the solver
* `space.gravity`: gets or sets the global gravity applied to the `Space` as `Vec2`, defaults to `Vec2.new(0, 0)`, can be overridden on a per `Body` basis by writing custom integration functions; changing the gravity will activate all sleeping bodies in the `Space`
* `space.damping`: gets or sets the amount of simple damping to apply to the `Space`, defaults to 1; a value of 0.9 means that each `Body` will lose 10% of its velocity per second; like gravity, it can be overridden on a per `Body` basis
* `space.idleSpeedThreshold`: gets or sets the speed threshold for a `Body` to be considered idle; the default value of 0 means the `Space` estimates a good threshold based on gravity
* `space.sleepTimeThreshold`: gets or sets the sleep time threshold; the default value of infinity disables the sleeping feature
* `space.collisionSlop`: gets or sets the amount of overlap between shapes that is allowed, defaults to 0.1; to improve stability, set this as high as you can without noticable overlapping
* `space.collisionBias`: gets or sets the bias value controls what percentage of overlap remains unfixed after a second; the physics module allows fast moving objects to overlap, then fixes the overlap over time; overlapping objects are unavoidable even if swept collisions are supported, and this is an efficient and stable way to deal with overlapping objects; defaults to ~0.2%, with range of values from 0.0 to 1.0, but using 0 is not recommended for stability reasons; the default value is calculated as (1.0 - 0.1) ^ 60.0 meaning that it attempts to correct 10% of error ever 1/60th of a second; note that very very few programs will need to change this value
* `space.collisionPersistence`: gets or sets the number of frames the `Space` keeps collision solutions around for, defaults to 3, and very very very few programs will need to change this value; helps prevent jittering contacts from getting worse
* `space.staticBody`: readonly, gets the dedicated static `Body` for the `Space`
* `space.currentTimeStep`: readonly, gets the current or most recent timestep
* `space.isLocked`: readonly, gets the locked status of the `Space`; results in `true` when you cannot add/remove objects from the `Space`; in particular, spaces are locked when in a collision callback; instead, run your code in a post-step callback instead
* `space.bodies`: readonly, gets all the bodies in the `Space`
* `space.shapes`: readonly, gets all the shapes in the `Space`
* `space.constraints`: readonly, gets all the constraints in the `Space`

**Methods**

* `space:cacheBoundingBox(shape)`: synchronizes `Shape` with the `Body` it has been attached to

* `space:setPostStepHandler(handler, key)`: sets the callback to be called before `space:step(...)` returns; only the first callback registered for any unique value of key will be recorded; it is only allowed to add this type of callback from inside of a collision handler or query callback; note that post-step callbacks are not run in any particular order, if you need to sequence a number of events, you'll need to put them in a single callback
	* `handler`: in form of `function (space) end`, an invokable object which accepts `Space`
	* `key`: can be one in `Shape`, `Body`, `Constraint` or integer
	* returns `true` if the callback is scheduled, otherwise `false` when the key has already been used
* `space:setDefaultCollisionHandler(onBegan, onPreSolved = nil, onPostSolved = nil, onSeparated = nil)`: sets the default collision handler which is used to process all collisions that don't have a more specific handler
	* `onBegan`: in form of `function (space, arbiter) return boolean end`, an invokable object which accepts `Space`, `Arbiter`, returns `true` if the collision is accepted, otherwise `false` for ignored until separation; this function is called when two shapes with types that match `Physics.WildcardCollisionType` begin colliding
	* `onPreSolved`: in form of `function (space, arbiter) return boolean end`, an invokable object which accepts `Space`, `Arbiter`, returns `true` if the collision is accepted, otherwise `false` for ignored until separation; this function is called each step when two shapes with types that match `Physics.WildcardCollisionType` are colliding; it is called before the collision solver runs so that you can affect a collision's outcome
	* `onPostSolved`: in form of `function (space, arbiter) end`, an invokable object which accepts `Space`, `Arbiter`; this function is called each step when two shapes with types that match `Physics.WildcardCollisionType` are colliding, it is called after the collision solver runs so that you can read back information about the collision to trigger events in your program
	* `onSeparated`: in form of `function (space, arbiter) end`, an invokable object which accepts `Space`, `Arbiter`; this function is called when two shapes with types that match `Physics.WildcardCollisionType` stop colliding
* `space:setCollisionHandler(typeA, typeB, onBegan, onPreSolved = nil, onPostSolved = nil, onSeparated = nil)`: sets a collision handler for the specific collision type pair; whenever shapes with collision types `typeA` and `typeB` collide, this handler will be used to process the collision events
	* `typeA`: the first collision type
	* `typeB`: the second collision type
	* `onBegan`: in form of `function (space, arbiter) return boolean end`, an invokable object which accepts `Space`, `Arbiter`, returns `true` if the collision is accepted, otherwise `false` for ignored until separation; this function is called when two shapes with types that match this collision handler begin colliding
	* `onPreSolved`: in form of `function (space, arbiter) return boolean end`, an invokable object which accepts `Space`, `Arbiter`, returns `true` if the collision is accepted, otherwise `false` for ignored until separation; this function is called each step when two shapes with types that match this collision handler are colliding, it is called before the collision solver runs so that you can affect a collision's outcome
	* `onPostSolved`: in form of `function (space, arbiter) end`, an invokable object which accepts `Space`, `Arbiter`; this function is called each step when two shapes with types that match this collision handler are colliding, it is called after the collision solver runs so that you can read back information about the collision to trigger events in your program
	* `onSeparated`: in form of `function (space, arbiter) end`, an invokable object which accepts `Space`, `Arbiter`; this function is called when two shapes with types that match this collision handler stop colliding
* `space:setWildcardHandler(type, onBegan, onPreSolved = nil, onPostSolved = nil, onSeparated = nil)`: sets a wildcard collision handler for given collision type; this handler will be used any time an object with this type collides with another object, regardless of its type
	* `type`: the specific collision type
	* `onBegan`: in form of `function (space, arbiter) return boolean end`, an invokable object which accepts `Space`, `Arbiter`, returns `true` if the collision is accepted, otherwise `false` for ignored until separation; this function is called when two shapes with types that match this collision handler begin colliding
	* `onPreSolved`: in form of `function (space, arbiter) return boolean end`, an invokable object which accepts `Space`, `Arbiter`, returns `true` if the collision is accepted, otherwise `false` for ignored until separation; this function is called each step when two shapes with types that match this collision handler are colliding, it is called before the collision solver runs so that you can affect a collision's outcome
	* `onPostSolved`: in form of `function (space, arbiter) end`, an invokable object which accepts `Space`, `Arbiter`; this function is called each step when two shapes with types that match this collision handler are colliding, it is called after the collision solver runs so that you can read back information about the collision to trigger events in your program
	* `onSeparated`: in form of `function (space, arbiter) end`, an invokable object which accepts `Space`, `Arbiter`; this function is called when two shapes with types that match this collision handler stop colliding

* `space:addShape(shape)`: adds a `Shape` to the `Space`
	* `shape`: the specific `Shape` to be added
	* returns the added `Shape` or `nil`
* `space:removeShape(shape)`: removes a `Shape` from the `Space`
	* `shape`: the specific `Shape` to be removed
	* returns `true` for success, otherwise `false`
* `space:hasShape(shape)`: gets whether the specific `Shape` exists in the `Space`
	* `shape`: the specific `Shape` to check
	* returns `true` for exists, otherwise `false`
* `space:addBody(body)`: adds a `Body` to the `Space`
	* `body`: the specific `Body` to be added
	* returns the added `Body` or `nil`
* `space:removeBody(body)`: removes a `Body` from the `Space`
	* `body`: the specific `Body` to be removed
	* returns `true` for success, otherwise `false`
* `space:hasBody(body)`: gets whether the specific `Body` exists in the `Space`
	* `body`: the specific `Body` to check
	* returns `true` for exists, otherwise `false`
* `space:addConstraint(constraint)`: adds a `Constraint` to the `Space`
	* `constraint`: the specific `Constraint` to be added
	* returns the added `Constraint` or `nil`
* `space:removeConstraint(constraint)`: removes a `Constraint` from the `Space`
	* `constraint`: the specific `Constraint` to be removed
	* returns `true` for success, otherwise `false`
* `space:hasConstraint(constraint)`: gets whether the specific `Constraint` exists in the `Space`
	* `constraint`: the specific `Constraint` to check
	* returns `true` for exists, otherwise `false`

<!-- * `space:pointQuery()`:
* `space:pointQueryAll()`:
* `space:pointQueryNearest()`:
* `space:segmentQuery()`:
* `space:segmentQueryAll()`:
* `space:segmentQueryFirst()`:
* `space:boundingBoxQuery()`:
* `space:boundingBoxQueryAll()`:
* `space:shapeQuery()`:
* `space:shapeQueryAll()`: -->
* `space:query(point, maxDistance, queryAll = false[, filter])`: performs a point query
	* `point`: the specific point `Vec2` to query at
	* `maxDistance`: the max query distance
	* `queryAll`: `true` to query all matched, `false` to query nearest
	* `filter`: the query filter
	* returns a list of `PointQuery` if queries all; otherwise one `PointQuery` or `nil` if queries nearest
* `space:query(start, end, radius, queryAll = false[, filter])`: performs a segment query
	* `start`: the start point `Vec2` of the specific segment
	* `end`: the end point `Vec2` of the specific segment
	* `radius`: the query radius
	* `queryAll`: `true` to query all matched, `false` to query nearest
	* `filter`: the query filter
	* returns a list of `SegmentQuery` if queries all; otherwise one `SegmentQuery` or `nil` if queries nearest
* `space:query(bb[, filter])`: performs a bounding box query
	* `bb`: the specific bounding box `Rect` to query
	* `filter`: the query filter
	* returns a list of `BoundingBoxQuery`
* `space:query(shape)`: performs a shape query
	* `shape`: the specific `Shape` to query with
	* returns a list of `ShapeQuery`

* `space:foreach(y, handler)`: iterates all shapes, bodies or constraints in the `Space`
	* `y`: the specific target type to iterate, can be one in `Physics.Shape`, `Physics.Body` or `Physics.Constraint`
	* `handler`: in form of `function (obj) end`, an invokable object which accepts `Shape`, `Body` or `Constraint` object
	* returns the itereted object count

* `space:reindexStatic()`: reindexes all static shapes
* `space:reindexShape(shape)`: reindexes the specific `Shape`
* `space:reindexShapesForBody(body)`: reindexes all the shapes for a certain `Body`

* `space:useSpatialHash(dim, count)`: switches the `Space` to use a spatial hash instead of the bounding box tree

* `space:step(delta)`: updates the `Space` for the given time step

* `space:collect([opt[, threshold]])`: collects all unused objects; the C version Chipmunk2D doesn't offer any automatic memory management, to adapt it to Lua, the `Shape`, `Body` and `Constraint` objects are cached when it is added to a `Space`, this cache is either manually or automatically collectable; generally you don't need to call this method manually, the default behaviour is that it will perform an automatic collecting when a specific count of objects (defaults to 1000) went obsolete
	* `opt`: can be one in "collect", "stop", "restart", "isrunning", "threshold", "limit", omit to perform a manual collect

Available options:

| Option | Behaviour | Return value |
|---|---|---|
| "collect" | Performs a manual collecting | - |
| "stop" | Stops automatic collecting | - |
| "restart" | Restarts automatic collecting | - |
| "isrunning" | Gets whether automatic collecting is running | `true` for running, otherwise `false` |
| "threshold" | Gets the automatic collecting threshold | The threshold |
| "limit" | Sets the automatic collecting threshold | The threshold |

<!-- End Space -->

## Shape

<!-- Begin Shape -->

**Constants**

<!-- * `Physics.Shape.Invalid` -->
* `Physics.Shape.Circle`
* `Physics.Shape.Segment`
* `Physics.Shape.Polygon`

**Constructors**

* `Physics.Shape.new(y, ...)`: constructs a shape object; `y` can be specialized as the following versions
* `Physics.Shape.new(y = Physics.Shape.Circle, body, radius, offset)`: constructs a circle shape object
	* `body`: the `Body` to attach the circle `Shape` to
	* `radius`: the circle radius
	* `offset`: the offset from the `Body`'s center of gravity in `Body` local coordinates
* `Physics.Shape.new(y = Physics.Shape.Segment, body, a, b, radius)`: constructs a segment shape object
	* `body`: the `Body` to attach the segment `Shape` to
	* `a`: the first endpoint
	* `b`: the second endpoint
	* `radius`: the thickness of the segment
* `Physics.Shape.new(y = Physics.Shape.Polygon, body, verts, t, radius)`: constructs a polygon shape object
	* `body`: the `Body` to attach the polygon `Shape` to
	* `verts`: a list of `Vec2` vertex, `Transform` will be applied to every one, a convex hull will be calculated from the vertexes automatically
	* `t`: the specific `Transform`
	* `radius`: the polygon `Shape` will be created with this radius, increasing the size of the `Shape`
* `Physics.Shape.new(y = Physics.Shape.Polygon, body, width, height, radius)`: shortcut to construct a box (polygon) shape object
	* `body`: the `Body` to attach the polygon `Shape` to; the box will always be centered at the center of gravity of the `Body`
	* `width`: the box width
	* `height`: the box height
	* `radius`: the `Shape`'s radius; adding a small radius will bevel the corners and can significantly reduce problems where the box gets stuck on seams in your geometry
* `Physics.Shape.new(y = Physics.Shape.Polygon, body, box, radius)`: shortcut to construct a box (polygon) shape object
	* `body`: the `Body` to attach the polygon `Shape` to; the box will always be centered at the center of gravity of the `Body`
	* `box`: the box `Rect`
	* `radius`: the `Shape`'s radius; adding a small radius will bevel the corners and can significantly reduce problems where the box gets stuck on seams in your geometry

**Object Fields**

* `shape.id`: readonly, gets the ID of the `Shape`; you can use it to identify a `Shape` object, i.e. in a Lua table
* `shape.type`: readonly, gets the geometry type of the `Shape`
* `shape.space`: readonly, gets the `Space` where the `Shape` has been attached to
* `shape.body`: gets or sets the `Body` where the `Shape` has been attached to
* `shape.mass`: gets or sets the mass of the `Shape`
* `shape.density`: gets or sets the density of the `Shape`
* `shape.moment`: readonly, gets the moment of the `Shape`
* `shape.area`: readonly, gets the area of the `Shape`
* `shape.centerOfGravity`: readonly, gets the center of gravity of the `Shape`
* `shape.boundingBox`: readonly, gets the bounding box of the `Shape`
* `shape.sensor`: gets or sets the sensor of the `Shape`, a boolean value indicates whether this `Shape` is a sensor, sensors only call collision callbacks, and never generate real collisions
* `shape.elasticity`: gets or sets the elasticity of the `Shape`; a value of 0.0 gives no bounce, while a value of 1.0 will give a "perfect" bounce; however due to inaccuracies in the simulation using 1.0 or greater is not recommended
* `shape.friction`: gets or sets the friction coefficient
* `shape.surfaceVelocity`: gets or sets the surface velocity of the `Shape`; useful for creating conveyor belts or players that move around; this value is only used when calculating friction, not resolving the collision
* `shape.collisionType`: gets or sets the collision type of the `Shape`; you can assign types to collision shapes that trigger callbacks when objects of certain types touch
* `shape.filter`: gets or sets the collision filter for this `Shape`
* `shape.offset`: readonly, gets the offset `Vec2` if this is a circle `Shape`
* `shape.radius`: readonly, gets the radius of the `Shape`
* `shape.pointA`: readonly, gets the first point `Vec2` if this is a segment `Shape`
* `shape.pointB`: readonly, gets the second point `Vec2` if this is a segment `Shape`
* `shape.normal`: readonly, gets the normal `Vec2` if this is a segment `Shape`
* `shape.vertexes`: readonly, gets the vertex points if this is a polygon `Shape`, in a list of `Vec2`

**Methods**

* `shape:cacheBoundingBox()`: synchronizes `Shape` with the `Body` it has been attached to
	* returns the cached bounding box `Rect`

* `shape:update(t)`: updates the `Shape`'s `Transform`
	* `t`: the specific `Transform`
	* returns the updated bounding box `Rect`

<!-- * `shape:pointQuery()`
* `shape:segmentQuery()` -->
* `shape:query(p)`: performs a nearest point query, it finds the closest point on the surface of `Shape` to a specific point
	* `p`: the specific point `Vec2`
	* returns queried `PointQuery` or `nil`, and a secondary number value for the distance between the points, a negative distance means the point is inside the `Shape`
* `shape:query(a, b, radius)`: performs a segment query against a `Shape`
	* `a`: the start point `Vec2` of the specific segment
	* `b`: the end point `Vec2` of the specific segment
	* `radius`: the segment radius
	* returns queried `SegmentQuery` or `nil`

* `shape:collides(other)`: checks whether the two `Shape`s collide
	* `other`: the other `Shape`
	* returns checked `Contacts`

* `shape:setSegmentNeighbors(prev, next)`: when you have a number of segment shapes that are all joined together, things can still collide with the "cracks" between the segments; by setting the neighbor segment endpoints you can specify to avoid colliding with the inner parts of the crack

<!-- * `shape:getPolygonVertexCount()`: gets the vertex count in the polygon `Shape`
* `shape:getPolygonVertex(index)`: gets the vertex at the specific index
	* `index`: the specific index to retrieve
	* returns the vertex `Vec2` -->

<!-- End Shape -->

## Body

<!-- Begin Body -->

**Constants**

* `Physics.Body.Dynamic`
* `Physics.Body.Kinematic`
* `Physics.Body.Static`

**Constructors**

* `Physics.Body.new(y, ...)`: constructs a body object; `y` can be specialized as the following versions
* `Physics.Body.new(y = Physics.Body.Dynamic, mass, moment)`: constructs a body object
* `Physics.Body.new(y = Physics.Body.Kinematic)`: constructs a body object
* `Physics.Body.new(y = Physics.Body.Static)`: constructs a body object

**Object Fields**

* `body.id`: readonly, gets the ID of the `Body`; you can use it to identify a `Body` object, i.e. in a Lua table
* `body.isSleeping`: readonly, gets the whether the `Body` is sleeping
* `body.type`: gets or sets the `Body`'s type
* `body.space`: readonly, gets the `Space` where the `Body` has been attached to
* `body.mass`: gets or sets the mass of the `Body`
* `body.moment`: gets or sets the moment of the `Body`
* `body.position`: gets or sets the position of the `Body`
* `body.centerOfGravity`: gets or sets the center of gravity of the `Body`
* `body.velocity`: gets or sets the velocity of the `Body`
* `body.force`: gets or sets the force of the `Body`
* `body.angle`: gets or sets the angle of the `Body`
* `body.angularVelocity`: gets or sets the angular velocity of the `Body`
* `body.torque`: gets or sets the torque of the `Body`
* `body.rotation`: readonly, gets the rotation of the `Body`
* `body.shapes`: readonly, gets the `Shape`s of the `Body`
* `body.constraints`: readonly, gets the `Constraint`s of the `Body`

**Methods**

When objects in the physics module sleep, they sleep as a group of all objects that are touching or jointed together. When an object is woken up, all of the objects in its group are woken up.

* `body:activate()`: resets the idle timer on this dynamic `Body`; if it was sleeping, wake it and any other bodies it was touching
* `body:activate(filter)`: activates all bodies touching this static `Body`; if filter is not `nil`, then only bodies touching through filter will be awoken
	* `filter`: the specific filter `Shape`
* `body:sleep()`: forces a `Body` to fall asleep immediately even if it's in midair; cannot be called from a callback
* `body:sleepWithGroup(group)`: sleeps a group of objects together; you can use this to initialize levels and start stacks of objects in a pre-sleeping state
	* `group`: it acts identically to `body:sleep()` if you pass `nil` as group by starting a new group; if you pass a sleeping `Body` for group, `Body` will be awoken when `group` is awoken

* `body:setVelocityHandler(handler)`: sets the callback used to update a `Body`'s velocity
* `body:setPositionHandler(handler)`: sets the callback used to update a `Body`'s position; note that it's not generally recommended to override this unless you call the default position update function
* `body:updateVelocity(gravity, damping, delta)`: default velocity integration function
* `body:updatePosition(delta)`: default position integration function

* `body:localToWorld(point)`: converts from `Body` local coordinates to world space coordinates
	* `point`: the specific point to convert
	* returns the converted point `Vec2`
* `body:worldToLocal(point)`: converts from world space coordinates to `Body` local coordinates
	* `point`: the specific point to convert
	* returns the converted point `Vec2`

* `body:applyForceAtWorldPoint(force, point)`: adds the force to `Body` as if applied from the world point
	* `force`: the specific force `Vec2` to apply
	* `point`: the specific point `Vec2` to apply to
* `body:applyForceAtLocalPoint(force, point)`: adds the local force to `Body` as if applied from the `Body` local point
	* `force`: the specific force `Vec2` to apply
	* `point`: the specific point `Vec2` to apply to
* `body:applyImpulseAtWorldPoint(impulse, point)`: adds the impuse to `Body` as if applied from the world point
	* `impulse`: the specific impulse `Vec2` to apply
	* `point`: the specific point `Vec2` to apply to
* `body:applyImpulseAtLocalPoint(impulse, point)`: adds the local impuse to `Body` as if applied from the `Body` local point
	* `impulse`: the specific impulse `Vec2` to apply
	* `point`: the specific point `Vec2` to apply to
* `body:getVelocityAtWorldPoint(point)`: gets the velocity on a `Body` (in world units) at a point on the `Body` in world coordinates
	* `point`: the specific point `Vec2` to get to
	* returns the velocity `Vec2`
* `body:getVelocityAtLocalPoint(point)`: gets the velocity on a `Body` (in world units) at a point on the `Body` in local coordinates
	* `point`: the specific point `Vec2` to get to
	* returns the velocity `Vec2`

* `body:kineticEnergy()`: gets the amount of kinetic energy contained by the `Body`
	* returns the `Body`'s kinetic energy

* `body:foreach(y, iterator)`: 

* `body:foreach(y, ...)`: iterates all `Shape`s, `Constraint`s or `Arbiter`s of this `Body`; `y` can be specialized as the following versions
* `body:foreach(y = Physics.Shape, iterator)`: iterates all `Shape`s of this `Body`
	* `iterator`: in form of `function (shape) end`, an invokable object which accepts `Shape`
	* returns the iterated object count
* `body:foreach(y = Physics.Constraint, iterator)`: iterates all `Constraint`s of this `Body`
	* `iterator`: in form of `function (constraint) end`, an invokable object which accepts `Constraint`
	* returns the iterated object count
* `body:foreach(y = Physics.Arbiter, iterator)`: iterates all `Arbiter`s of this `Body`
	* `iterator`: in form of `function (arbiter) end`, an invokable object which accepts `Arbiter`
	* returns the iterated object count

<!-- End Body -->

## Constraints

### Constraint

<!-- Begin Constraint -->

**Constants**

<!-- * `Physics.Constraint.Invalid` -->
* `Physics.Constraint.DampedRotarySpring`
* `Physics.Constraint.DampedSpring`
* `Physics.Constraint.GearJoint`
* `Physics.Constraint.GrooveJoint`
* `Physics.Constraint.PinJoint`
* `Physics.Constraint.PivotJoint`
* `Physics.Constraint.RatchetJoint`
* `Physics.Constraint.RotaryLimitJoint`
* `Physics.Constraint.SimpleMotor`
* `Physics.Constraint.SlideJoint`

**Object Fields**

* `constraint.id`: readonly, gets the ID of the `Constraint`; you can use it to identify a `Constraint` object, i.e. in a Lua table
* `constraint.space`: readonly, gets the `Space` where the `Constraint` belongs to
* `constraint.bodyA`: readonly, gets the first `Body` the `Constraint` is attached to
* `constraint.bodyB`: readonly, gets the second `Body` the `Constraint` is attached to
* `constraint.maxForce`: gets or sets the maximum force that this `Constraint` is allowed to use
* `constraint.errorBias`: gets or sets the maximum force that this `Constraint` is allowed to use, defaults to infinity
* `constraint.maxBias`: gets or sets the maximum rate at which joint error is corrected, defaults to infinity
* `constraint.collideBodies`: gets or sets whether the two bodies connected by the `Constraint` are allowed to collide, defaults to `false`
* `constraint.impulse`: readonly, gets the last impulse applied by this `Constraint`
* `constraint.isDampedRotarySpring`: readonly, gets whether the `Constraint` is a `DampedRotarySpring`
* `constraint.isDampedSpring`: readonly, gets whether the `Constraint` is a `DampedSpring`
* `constraint.isGearJoint`: readonly, gets whether the `Constraint` is a `GearJoint`
* `constraint.isGrooveJoint`: readonly, gets whether the `Constraint` is a `GrooveJoint`
* `constraint.isPinJoint`: readonly, gets whether the `Constraint` is a `PinJoint`
* `constraint.isPivotJoint`: readonly, gets whether the `Constraint` is a `PivotJoint`
* `constraint.isRatchetJoint`: readonly, gets whether the `Constraint` is a `RatchetJoint`
* `constraint.isRotaryLimitJoint`: readonly, gets whether the `Constraint` is a `RotaryLimitJoint`
* `constraint.isSimpleMotor`: readonly, gets whether the `Constraint` is a `SimpleMotor`
* `constraint.isSlideJoint`: readonly, gets whether the `Constraint` is a `SlideJoint`

**Methods**

* `constraint:setPreSolveHandler(handler)`: the post-solve handler that is called before the solver runs
	* `handler`: in form of `function (space, constraint) end`, an invokable object which accepts `Space` and `Constraint`
* `constraint:setPostSolveHandler(handler)`: the post-solve handler that is called before the solver runs
	* `handler`: in form of `function (space, constraint) end`, an invokable object which accepts `Space` and `Constraint`

<!-- End Constraint -->

### DampedRotarySpring

<!-- Begin DampedRotarySpring -->

**Constructors**

* `Physics.DampedRotarySpring.new()`: constructs a damped rotary spring constraint

**Object Fields**

* `constraint.restAngle`: gets or sets the rest length of the spring
* `constraint.stiffness`: gets or sets the stiffness of the spring in force/distance
* `constraint.damping`: gets or sets the damping of the spring

<!-- End DampedRotarySpring -->

### DampedSpring

<!-- Begin DampedSpring -->

**Constructors**

* `Physics.DampedSpring.new()`: constructs a damped spring constraint

**Object Fields**

* `constraint.anchorA`: gets or sets the location of the first anchor relative to the first `Body`
* `constraint.anchorB`: gets or sets the location of the second anchor relative to the second `Body`
* `constraint.restLength`: gets or sets the rest length of the spring
* `constraint.stiffness`: gets or sets the stiffness of the spring in force/distance
* `constraint.damping`: gets or sets the damping of the spring

<!-- End DampedSpring -->

### GearJoint

<!-- Begin GearJoint -->

**Constructors**

* `Physics.GearJoint.new()`: constructs a gear joint constraint

**Object Fields**

* `constraint.phase`: gets or sets the phase offset of the gears
* `constraint.ratio`: gets or sets the ratio of a gear joint

<!-- End GearJoint -->

### GrooveJoint

<!-- Begin GrooveJoint -->

**Constructors**

* `Physics.GrooveJoint.new()`: constructs a groove joint constraint

**Object Fields**

* `constraint.grooveA`: gets or sets the first endpoint of the groove relative to the first `Body`
* `constraint.grooveB`: gets or sets the second endpoint of the groove relative to the second `Body`
* `constraint.anchorB`: gets or sets the location of the second anchor relative to the second `Body`

<!-- End GrooveJoint -->

### PinJoint

<!-- Begin PinJoint -->

**Constructors**

* `Physics.PinJoint.new()`: constructs a pin joint constraint

**Object Fields**

* `constraint.anchorA`: gets or sets the location of the first anchor relative to the first `Body`
* `constraint.anchorB`: gets or sets the location of the second anchor relative to the second `Body`
* `constraint.distance`: gets or sets the distance the joint will maintain between the two anchors

<!-- End PinJoint -->

### PivotJoint

<!-- Begin PivotJoint -->

**Constructors**

* `Physics.PivotJoint.new()`: constructs a pivot joint constraint

**Object Fields**

* `constraint.anchorA`: gets or sets the location of the first anchor relative to the first `Body`
* `constraint.anchorB`: gets or sets the location of the second anchor relative to the second `Body`

<!-- End PivotJoint -->

### RatchetJoint

<!-- Begin RatchetJoint -->

**Constructors**

* `Physics.RatchetJoint.new()`: constructs a ratchet joint constraint

**Object Fields**

* `constraint.angle`: gets or sets the angle of the current ratchet tooth
* `constraint.phase`: gets or sets the phase offset of the ratchet
* `constraint.ratchet`: gets or sets the angular distance of each ratchet

<!-- End RatchetJoint -->

### RotaryLimitJoint

<!-- Begin RotaryLimitJoint -->

**Constructors**

* `Physics.RotaryLimitJoint.new()`: constructs a rotary limit joint constraint

**Object Fields**

* `constraint.min`: gets or sets the minimum distance the joint will maintain between the two anchors
* `constraint.max`: gets or sets the maximum distance the joint will maintain between the two anchors

<!-- End RotaryLimitJoint -->

### SimpleMotor

<!-- Begin SimpleMotor -->

**Constructors**

* `Physics.SimpleMotor.new()`: constructs a simple motor constraint

**Object Fields**

* `constraint.rate`: gets or sets the rate of the motor

<!-- End SimpleMotor -->

### SlideJoint

<!-- Begin SlideJoint -->

**Constructors**

* `Physics.SlideJoint.new()`: constructs a slide joint constraint

**Object Fields**

* `constraint.anchorA`: gets or sets the location of the first anchor relative to the first `Body`
* `constraint.anchorB`: gets or sets the location of the second anchor relative to the second `Body`
* `constraint.min`: gets or sets the minimum distance the joint will maintain between the two anchors
* `constraint.max`: gets or sets the maximum distance the joint will maintain between the two anchors

<!-- End SlideJoint -->

## Arbiter

<!-- Begin Arbiter -->

**Operators**

* `arbiter:__len()`: readonly, gets the number of contact points for this `Arbiter`

**Object Fields**

* `arbiter.id`: readonly, gets the ID of the `Arbiter`; you can use it to identify a `Arbiter` object, i.e. in a Lua table
* `arbiter.restitution`: gets or sets the restitution (elasticity) that will be applied to the pair of colliding objects
* `arbiter.friction`: gets or sets the friction coefficient that will be applied to the pair of colliding objects
* `arbiter.surfaceVelocity`: gets or sets the relative surface velocity of the two `Shape`s in contact
* `arbiter.totalImpulse`: readonly, gets the total impulse including the friction that was applied by this `Arbiter`; this property should only be used from a post-solve or post-step callback
* `arbiter.totalKineticEnergy`: readonly, gets the amount of energy lost in a collision including static, but not dynamic friction; this property should only be used from a post-solve or post-step callback
* `arbiter.contacts`: gets or sets the `Contact` for the `Arbiter`
* `arbiter.isFirstContact`: readonly, gets whether the `Arbiter` is the first contact, `true` if this is the first step a pair of objects started colliding, otherwise `false`
* `arbiter.isRemoval`: readonly, gets the whether the `Arbiter` is removal, `true` if the separate callback is due to a `Shape` being removed from the `Space`, otherwise `false`
* `arbiter.normal`: readonly, gets the normal `Vec2` of the collision
* `arbiter.point1A`: readonly, gets the position of the first contact point on the surface of the first `Shape`
* `arbiter.point1B`: readonly, gets the position of the first contact point on the surface of the second `Shape`
* `arbiter.depth1`: readonly, gets the depth of the first contact point
* `arbiter.point2A`: readonly, gets the position of the second contact point on the surface of the first `Shape`
* `arbiter.point2B`: readonly, gets the position of the second contact point on the surface of the second `Shape`
* `arbiter.depth2`: readonly, gets the depth of the second contact point

**Methods**

* `arbiter:ignore()`: marks a collision pair to be ignored until the two objects separate; pre-solve and post-solve callbacks will not be called, but the separate callback will be called
* `arbiter:getShapes()`: gets the `Shape`s in the order that they were defined in the collision handler associated with this `Arbiter`; i.e. if you defined the handler as `space:setCollisionHandler(1, 2, ...)`, you you will find that `shapeA.collisionType` equals to 1 and `shapeB.collisionType` equals to 2
	* returns two values for the `Shape`s
* `arbiter:getBodies()`: gets the bodies in the order that they were defined in the collision handler associated with this `Arbiter`; i.e. if you defined the handler as `space:setCollisionHandler(1, 2, ...)`, you you will find that `bodyA.collisionType` equals to 1 and `bodyB.collisionType` equals to 2
	* returns two values for the bodies
* `arbiter:callWildcardBeginA(space)`: if you want a custom callback to invoke the wildcard callback for the first collision type, you must call this function explicitly; you must decide how to handle the wildcard's return value since it may disagree with the other wildcard handler's return value or your own
	* returns the returned value from the callback handler
* `arbiter:callWildcardBeginB(space)`: if you want a custom callback to invoke the wildcard callback for the second collision type, you must call this function explicitly; you must decide how to handle the wildcard's return value since it may disagree with the other wildcard handler's return value or your own
	* returns the returned value from the callback handler
* `arbiter:callWildcardPreSolveA(space)`: if you want a custom callback to invoke the wildcard callback for the first collision type, you must call this function explicitly; you must decide how to handle the wildcard's return value since it may disagree with the other wildcard handler's return value or your own
	* returns the returned value from the callback handler
* `arbiter:callWildcardPreSolveB(space)`: if you want a custom callback to invoke the wildcard callback for the second collision type, you must call this function explicitly; you must decide how to handle the wildcard's return value since it may disagree with the other wildcard handler's return value or your own
	* returns the returned value from the callback handler
* `arbiter:callWildcardPostSolveA(space)`: if you want a custom callback to invoke the wildcard callback for the first collision type, you must call this function explicitly
* `arbiter:callWildcardPostSolveB(space)`: if you want a custom callback to invoke the wildcard callback for the second collision type, you must call this function explicitly
* `arbiter:callWildcardSeparateA(space)`: if you want a custom callback to invoke the wildcard callback for the first collision type, you must call this function explicitly
* `arbiter:callWildcardSeparateB(space)`: if you want a custom callback to invoke the wildcard callback for the second collision type, you must call this function explicitly

<!-- End Arbiter -->

## Transform

<!-- Begin Transform -->

**Constants**

* `Physics.Transform.Identity`

**Constructors**

* `Physics.Transform.new(a, b, c, d, tx, ty)`: constructs a transform object
* `Physics.Transform.newTranspose(a, c, tx, b, d, ty)`: constructs a transform object
* `Physics.Transform.newTranslate(translate)`: constructs a transform object
	* `translate`: `Vec2`
* `Physics.Transform.newScale(scaleX, scaleY)`: constructs a transform object
* `Physics.Transform.newScale(scale)`: constructs a transform object
* `Physics.Transform.newRotate(radians)`: constructs a transform object
* `Physics.Transform.newRigid(translate, radians)`: constructs a transform object
	* `translate`: `Vec2`
* `Physics.Transform.newRigidInverse(trans)`: constructs a transform object
	* `trans`: `Transform`
* `Physics.Transform.newWrap(outer, inner)`: constructs a transform object
	* `outer`: `Transform`
	* `inner`: `Transform`
* `Physics.Transform.newWrapInverse(outer, inner)`: constructs a transform object
	* `outer`: `Transform`
	* `inner`: `Transform`
* `Physics.Transform.newOrtho(bb)`: constructs a transform object
	* `bb`: `Rect`
* `Physics.Transform.newBoneScale(v1, v2)`: constructs a transform object
	* `v1`: `Vec2`
	* `v2`: `Vec2`
* `Physics.Transform.newAxialScale(axis, pivot, scale)`: constructs a transform object
	* `axis`: `Vec2`
	* `pivot`: `Vec2`

**Operators**

* `transform:__mul(other)`: multiplies with another `Transform`
	* `other`: the other `Transform`
	* returns the multiplied `Transform`

**Object Fields**

* `transform.a`: gets or sets the `a` component of the `Transform`
* `transform.b`: gets or sets the `b` component of the `Transform`
* `transform.c`: gets or sets the `c` component of the `Transform`
* `transform.d`: gets or sets the `d` component of the `Transform`
* `transform.tx`: gets or sets the `tx` component of the `Transform`
* `transform.ty`: gets or sets the `ty` component of the `Transform`
* `transform.inversed`: readonly, gets the inverse of a transform matrix

**Methods**

* `transform:transformPoint(point)`: transform the specific point `Vec2` with this `Transform`
	* returns the transformed `Vec2`
* `transform:transformVector(point)`: transform the specific vector `Vec2` with this `Transform`
	* returns the transformed `Vec2`
* `transform:transformBoundingBox(bb)`: transform the specific bounding box `Rect` with this `Transform`
	* returns the transformed `Rect`

<!-- End Transform -->

## ShapeFilter

<!-- Begin ShapeFilter -->

**Constructors**

* `Physics.ShapeFilter.new(group, categories, mask)`: constructs a shape filter object

**Object Fields**

* `shapeFilter.group`: gets or sets the group of the shape filter
* `shapeFilter.categories`: gets or sets the categories of the shape filter
* `shapeFilter.mask`: gets or sets the mask of the shape filter

<!-- End ShapeFilter -->

## Contact

<!-- Begin Contact -->

**Object Fields**

* `contact.pointA`: readonly, gets the `Contact` point on the first `Shape`
* `contact.pointB`: readonly, gets the `Contact` point on the second `Shape`
* `contact.distance`: readonly, gets the penetration distance of the two `Shape`s; overlapping means it will be negative

<!-- End Contact -->

## Contacts

<!-- Begin Contacts -->

**Operators**

* `contacts:__len()`: readonly, gets the number of contact points

**Object Fields**

* `contacts.shape`: readonly, gets the `Shape`
* `contacts.normal`: readonly, gets the normal of the collision
* `contacts.points`: readonly, gets the contact points, in a list of `Contact`

<!-- End Contacts -->

## Queries

### PointQuery

<!-- Begin PointQuery -->

**Object Fields**

* `pointQuery.shape`: readonly, gets the nearest `Shape`
* `pointQuery.point`: readonly, gets the closest point on the `Shape`'s surface (in world space coordinates)
* `pointQuery.distance`: readonly, gets the distance to the point; the distance is negative if the point is inside the `shape`
* `pointQuery.gradient`: readonly, gets the gradient of the signed distance function

<!-- End PointQuery -->

### SegmentQuery

<!-- Begin SegmentQuery -->

**Object Fields**

* `segmentQuery.shape`: readonly, gets the `Shape` that was hit
* `segmentQuery.point`: readonly, gets the point of impact
* `segmentQuery.normal`: readonly, gets the normal of the surface hit
* `segmentQuery.alpha`: readonly, gets the normalized distance along the query segment, with range of values from 0.0 to 1.0

<!-- End SegmentQuery -->

### BoundingBoxQuery

<!-- Begin BoundingBoxQuery -->

**Object Fields**

* `boundingBoxQuery.shape`: readonly, gets the `Shape` that was hit

<!-- End BoundingBoxQuery -->

### ShapeQuery

<!-- Begin ShapeQuery -->

**Operators**

* `shapeQuery:__len()`: readonly, gets the number of contact points

**Object Fields**

* `shapeQuery.shape`: readonly, gets the `Shape`
* `shapeQuery.normal`: readonly, gets the normal of the collision
* `shapeQuery.points`: readonly, gets the contact points, in a list of `Contact`

<!-- End ShapeQuery -->

[TOP](#physics)
