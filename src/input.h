/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __INPUT_H__
#define __INPUT_H__

#include "bitty.h"
#include "mathematics.h"
#include "plus.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef INPUT_GAMEPAD_COUNT
#	define INPUT_GAMEPAD_COUNT 2
#endif /* INPUT_GAMEPAD_COUNT */

#ifndef INPUT_GAMEPAD_MAX_SCALE
#	define INPUT_GAMEPAD_MAX_SCALE 10.0f
#endif /* INPUT_GAMEPAD_MAX_SCALE */
#ifndef INPUT_GAMEPAD_MAX_X_PADDING
#	define INPUT_GAMEPAD_MAX_X_PADDING 30.0f
#endif /* INPUT_GAMEPAD_MAX_X_PADDING */
#ifndef INPUT_GAMEPAD_MAX_Y_PADDING
#	define INPUT_GAMEPAD_MAX_Y_PADDING 100.0f
#endif /* INPUT_GAMEPAD_MAX_Y_PADDING */

/* ===========================================================================} */

/*
** {===========================================================================
** Input
**
** @note There are four kinds of input sources:
**   1. Gamepad. Can be binded to joystick, keyboard or onscreen gamepad.
**   2. Game controller. Corresponds to actual hardware.
**   3. Keyboard. Corresponds to actual hardware.
**   4. Mouse. Corresponds to actual mouse or touch screen hardware.
*/

/**
 * @brief Input handler.
 */
class Input {
public:
	enum Devices {
		INVALID,
		KEYBOARD,
		JOYSTICK
	};
	enum Buttons {
		LEFT,
		RIGHT,
		UP,
		DOWN,
		A,
		B,

		BUTTON_COUNT
	};
	enum Types : unsigned short {
		VALUE,
		HAT,
		AXIS
	};
	struct Hat {
		enum Types : unsigned short {
			LEFT,
			RIGHT,
			UP,
			DOWN
		};

		short index;
		Types value;
	};
	struct Axis {
		short index;
		short value;
	};
	struct Button {
		Devices device = INVALID; // Joystick, or keyboard.
		short index = 0;          // Joystick index, or always 0 for keyboard.
		Types type = VALUE;
		union {
			int value = 0;        // Joystick button, or key value for keyboard.
			Hat hat;              // Joystick hat.
			Axis axis;            // Joystick axis.
		};

		Button();
		Button(Devices dev, short idx, int val);
		Button(Devices dev, short idx, short hatIdx, Hat::Types hatVal);
		Button(Devices dev, short idx, short axisIdx, short axisVal);
		Button(const Button &other) = delete;

		Button &operator = (const Button &other);

		bool operator == (const Button &other) const;
		bool operator != (const Button &other) const;
	};
	struct Gamepad {
		Button buttons[BUTTON_COUNT];

		Gamepad();
		Gamepad(const Gamepad &other);

		Gamepad &operator = (const Gamepad &other);

		bool operator == (const Gamepad &other) const;
		bool operator != (const Gamepad &other) const;
	};

	enum Activities {
		INACTIVE = 0,
		GAMEPAD_ACTIVE = 1 << 0,
		CONTROLLER_ACTIVE = 1 << 1,
		KEYBOARD_ACTIVE = 1 << 2,
		MOUSE_ACTIVE = 1 << 3
	};

public:
	virtual bool open(void) = 0;
	virtual bool close(void) = 0;

	virtual void reset(void) = 0;

	virtual int joystickCount(void) = 0;
	/**
	 * @return `SDL_Joystick*`.
	 */
	virtual void* joystickAt(int index, const char** name /* nullable */) = 0;

	virtual int controllerCount(void) = 0;
	/**
	 * @return `SDL_GameController*`.
	 */
	virtual void* controllerAt(int index, const char** name /* nullable */, const char** type /* nullable */, bool* attached /* nullable */) = 0;

	virtual void config(const Gamepad* pads /* nullable */, int padCount) = 0;

	virtual std::string nameOf(const Button &btn) = 0;

	virtual bool pressed(Button &btn) = 0;

	virtual void update(
		class Window* wnd, class Renderer* rnd,
		const Math::Rectf* clientArea /* nullable */, const Math::Vec2i* canvasSize /* nullable */,
		int scale
	) = 0;

	virtual int updateOnscreenGamepad(
		class Window* wnd, class Renderer* rnd,
		struct ImFont* font,
		bool swapAB,
		float scale,
		float paddingX, float paddingY
	) = 0;

	/**
	 * @param[in] btn Button index, -1 to get any button.
	 */
	virtual int buttonDown(int btn, int idx) const = 0;
	/**
	 * @param[in] btn Button index, -1 to get any button.
	 */
	virtual int buttonUp(int btn, int idx) const = 0;
	virtual bool rumbleGamepad(int idx, int lowHz, int hiHz, unsigned ms) = 0;

	virtual int controllerDown(int btn, int idx) const = 0;
	virtual int controllerUp(int btn, int idx) const = 0;
	virtual bool rumbleController(int idx, int lowHz, int hiHz, unsigned ms) = 0;

	virtual bool keyDown(int key) const = 0;
	virtual bool keyUp(int key) const = 0;

	/**
	 * @param[out] x
	 * @param[out] y
	 * @param[out] b0
	 * @param[out] b1
	 * @param[out] b2
	 * @param[out] wheelX
	 * @param[out] wheelY
	 */
	virtual bool mouse(
		int idx,
		int* x /* nullable */, int* y /* nullable */,
		bool* b0 /* nullable */, bool* b1 /* nullable */, bool* b2 /* nullable */,
		int* wheelX /* nullable */, int* wheelY /* nullable */
	) const = 0;

	virtual void sync(void) = 0;

	virtual Activities active(void) = 0;

	static Input* create(void);
	static void destroy(Input* ptr);
};

/* ===========================================================================} */

#endif /* __INPUT_H__ */
