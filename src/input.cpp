/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "input.h"
#include "renderer.h"
#include "text.h"
#include "window.h"
#include "../lib/imgui/imgui.h"
#include "../lib/imgui/imgui_internal.h"
#include <SDL.h>
#include <array>

/*
** {===========================================================================
** Input
*/

class InputImpl : public Input {
private:
	struct Joystick {
		SDL_Joystick* joystick = nullptr;
		std::string name;

		Joystick(SDL_Joystick* j, const char* n) : joystick(j), name(n) {
		}
	};
	typedef std::vector<Joystick> Joysticks;

	struct Controller {
		typedef std::array<Sint16, SDL_CONTROLLER_AXIS_MAX> AxisInitialValues;

		SDL_GameController* controller = nullptr;
		std::string name;
#if SDL_VERSION_ATLEAST(2, 0, 12)
		SDL_GameControllerType type = SDL_CONTROLLER_TYPE_UNKNOWN;
#endif /* SDL_VERSION_ATLEAST(2, 0, 12) */
		bool attached = false;
		AxisInitialValues axisInitialValues;

		Controller(SDL_GameController* c, const char* n, bool attached_) : controller(c), attached(attached_) {
			if (n)
				name = n;
			else
				name = "Unknown";
			axisInitialValues.fill(0);
		}
	};
	typedef std::vector<Controller> Controllers;

	typedef std::vector<Gamepad> Gamepads;

	typedef std::array<bool, BUTTON_COUNT> GamepadState;
	typedef std::vector<GamepadState> GamepadStates;

	struct ControllerState {
		typedef std::array<int, SDL_CONTROLLER_BUTTON_MAX> Buttons;
		typedef std::array<int, SDL_CONTROLLER_AXIS_MAX> Axises;

		Buttons buttons;
		Axises axises;

		ControllerState() {
			buttons.fill(0);
			axises.fill(0);
		}

		void clear(void) {
			buttons.fill(0);
			axises.fill(0);
		}
	};
	typedef std::vector<ControllerState> ControllerStates;

	typedef std::vector<Uint8> KeyStates;
	typedef SDL_Keymod KeymodStates;

	struct Mouse {
		typedef std::array<bool, 3> Buttons;

		int x = -1;
		int y = -1;
		Buttons buttons;
		int wheelX = 0;
		int wheelY = 0;
		bool valid = false;

		Mouse() {
			buttons.fill(0);
		}
		Mouse(int x_, int y_, bool b0, bool b1, bool b2, int wheelX_, int wheelY_) : x(x_), y(y_), wheelX(wheelX_), wheelY(wheelY_) {
			buttons[0] = b0;
			buttons[1] = b1;
			buttons[2] = b2;
		}
	};
	typedef std::vector<Mouse> MouseStates;

private:
	bool _opened = false;

	Joysticks _joysticks;
	Controllers _controllers;
	Gamepads _gamepads;

	GamepadStates _gamepadStatesNative;
	ControllerStates _controllerStatesNative;
	KeyStates _keyStatesNative;
	KeymodStates _keymodStatesNative = KMOD_NONE;
	MouseStates _mouseStatesNative;

	bool _onscreenGamepadStates1[BUTTON_COUNT];
	int _onscreenGamepadPressed = 0;
	GamepadStates _gamepadStates0;
	GamepadStates _gamepadStates1;
	ControllerStates _controllerStates0;
	ControllerStates _controllerStates1;
	KeyStates _keyStates0;
	KeyStates _keyStates1;
	KeymodStates _keymodStates0 = KMOD_NONE;
	KeymodStates _keymodStates1 = KMOD_NONE;
	MouseStates _mouseStates1;

	Activities _activeRequested = INACTIVE;
	mutable Activities _activeRequests = INACTIVE;

	mutable RecursiveMutex _lock;

public:
	virtual ~InputImpl() {
		clear(true);
	}

	virtual bool open(void) override {
		LockGuard<decltype(_lock)> guard(_lock);

		if (_opened)
			return false;
		_opened = true;

		clear(true);

		const int jsn = SDL_NumJoysticks();
		for (int i = 0; i < jsn; ++i) {
			SDL_Joystick* js = SDL_JoystickOpen(i);
			if (!js)
				continue;

			if (!SDL_JoystickGetAttached(js))
				continue;

			_joysticks.push_back(Joystick(js, SDL_JoystickName(js)));

			bool isController = false;
			if (SDL_IsGameController(i)) {
				SDL_GameController* controller = SDL_GameControllerOpen(i);
				if (controller) {
					const char* name = SDL_GameControllerNameForIndex(i);
					const bool attached = !!SDL_GameControllerGetAttached(controller);
					Controller ctrl(controller, name, attached);
#if SDL_VERSION_ATLEAST(2, 0, 12)
					const SDL_GameControllerType y = SDL_GameControllerTypeForIndex(i);
					ctrl.type = y;
#endif /* SDL_VERSION_ATLEAST(2, 0, 12) */
					_controllers.push_back(ctrl);
					for (int k = 0; k < SDL_CONTROLLER_AXIS_MAX; ++k) {
						Sint16 state = 0;
						if (SDL_JoystickGetAxisInitialState(js, k, &state))
							ctrl.axisInitialValues[k] = state;
					}
					isController = true;
				}
			}

			const char* name = SDL_JoystickName(js);
			if (name) {
				if (isController)
					fprintf(stdout, "Joystick \"%s\" connected as game controller.\n", name);
				else
					fprintf(stdout, "Joystick \"%s\" connected.\n", name);
			}
		}

		fprintf(stdout, "Input opened.\n");

		return true;
	}
	virtual bool close(void) override {
		LockGuard<decltype(_lock)> guard(_lock);

		if (!_opened)
			return false;
		_opened = false;

		clear(true);

		fprintf(stdout, "Input closed.\n");

		return true;
	}

	virtual void reset(void) override {
		clear(false);

		fprintf(stdout, "Input reset.\n");
	}

	virtual int joystickCount(void) override {
		return (int)_joysticks.size();
	}
	virtual void* joystickAt(int index, const char** name) override {
		if (name)
			*name = nullptr;

		if (index < 0 || index >= (int)_joysticks.size())
			return nullptr;

		if (name)
			*name = _joysticks[index].name.c_str();

		return _joysticks[index].joystick;
	}

	virtual int controllerCount(void) override {
		return (int)_controllers.size();
	}
	virtual void* controllerAt(int index, const char** name, const char** type, bool* attached) override {
		if (name)
			*name = nullptr;
		if (type)
			*type = nullptr;
		if (attached)
			*attached = false;

		if (index < 0 || index >= (int)_controllers.size())
			return nullptr;

		if (name)
			*name = _controllers[index].name.c_str();
		if (type) {
#if SDL_VERSION_ATLEAST(2, 0, 12)
			switch (_controllers[index].type) {
			case SDL_CONTROLLER_TYPE_XBOX360:
				*type = "Xbox 360";

				break;
			case SDL_CONTROLLER_TYPE_XBOXONE:
				*type = "Xbox One";

				break;
			case SDL_CONTROLLER_TYPE_PS3:
				*type = "PlayStation 3";

				break;
			case SDL_CONTROLLER_TYPE_PS4:
				*type = "PlayStation 4";

				break;
			case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
				*type = "Nintendo Switch Pro";

				break;
#if SDL_VERSION_ATLEAST(2, 0, 14)
			case SDL_CONTROLLER_TYPE_VIRTUAL:
				*type = "Virtual";

				break;
			case SDL_CONTROLLER_TYPE_PS5:
				*type = "PlayStation 5";

				break;
#endif /* SDL_VERSION_ATLEAST(2, 0, 14) */
#if SDL_VERSION_ATLEAST(2, 0, 16)
			case SDL_CONTROLLER_TYPE_AMAZON_LUNA:
				*type = "Luna";

				break;
			case SDL_CONTROLLER_TYPE_GOOGLE_STADIA:
				*type = "Stadia";

				break;
#endif /* SDL_VERSION_ATLEAST(2, 0, 16) */
			default:
				*type = "Unknown";

				break;
			}
#else /* SDL_VERSION_ATLEAST(2, 0, 12) */
			*type = "Unknown";
#endif /* SDL_VERSION_ATLEAST(2, 0, 12) */
		}
		if (attached)
			*attached = _controllers[index].attached;

		return _controllers[index].controller;
	}

	virtual void config(const Gamepad* pads, int padCount) override {
		LockGuard<decltype(_lock)> guard(_lock);

		_gamepads.clear();

		if (pads && padCount > 0) {
			for (int i = 0; i < padCount; ++i) {
				const Gamepad &pad = pads[i];
				_gamepads.push_back(pad);
			}
		}
	}

	virtual std::string nameOf(const Button &btn) override {
		switch (btn.device) {
		case INVALID:
			return "None";
		case KEYBOARD:
			if (btn.value < 0) {
				switch (-btn.value) {
				case KMOD_LCTRL:
					return "LCtrl";
				case KMOD_RCTRL:
					return "RCtrl";
				case KMOD_LSHIFT:
					return "LShift";
				case KMOD_RSHIFT:
					return "RShift";
				case KMOD_LALT:
					return "LAlt";
				case KMOD_RALT:
					return "RAlt";
				case KMOD_LGUI:
					return "LGUI";
				case KMOD_RGUI:
					return "RGUI";
				}
			}

			return SDL_GetScancodeName((SDL_Scancode)btn.value);
		case JOYSTICK: {
				std::string ret;

				if (btn.type == VALUE) {
					ret = "[" + Text::toString(btn.value) + "]";
				} else if (btn.type == HAT) {
					ret = "Hat" + Text::toString(btn.hat.index);
					switch (btn.hat.value) {
					case Hat::LEFT:
						ret += "[Left]";
					case Hat::RIGHT:
						ret += "[Right]";
					case Hat::UP:
						ret += "[Up]";
					case Hat::DOWN:
						ret += "[Down]";
					default:
						ret += "[Unknown]";
					}
				} else if (btn.type == AXIS) {
					ret = "Axis" + Text::toString(btn.axis.index);
					if (btn.axis.value < 0)
						ret += "[-]";
					else if (btn.axis.value > 0)
						ret += "[+]";
					else
						ret += "[?]";
				} else {
					ret = "Unknown";
				}

				const char* name = nullptr;
				joystickAt(btn.index, &name);
				if (name) {
					ret += "/";
					ret += name;

					return ret;
				} else {
					ret += "/Joystick";
					ret += Text::toString(btn.index);

					return ret;
				}
			}
		}

		return "None";
	}

	virtual bool pressed(Button &btn) override {
		const SDL_Keymod mod = SDL_GetModState();
		if (mod & KMOD_LCTRL) {
			btn = Button(KEYBOARD, 0, -KMOD_LCTRL);

			return true;
		} else if (mod & KMOD_RCTRL) {
			btn = Button(KEYBOARD, 0, -KMOD_RCTRL);

			return true;
		} else if (mod & KMOD_LSHIFT) {
			btn = Button(KEYBOARD, 0, -KMOD_LSHIFT);

			return true;
		} else if (mod & KMOD_RSHIFT) {
			btn = Button(KEYBOARD, 0, -KMOD_RSHIFT);

			return true;
		} else if (mod & KMOD_LALT) {
			btn = Button(KEYBOARD, 0, -KMOD_LALT);

			return true;
		} else if (mod & KMOD_RALT) {
			btn = Button(KEYBOARD, 0, -KMOD_RALT);

			return true;
		} else if (mod & KMOD_LGUI) {
			btn = Button(KEYBOARD, 0, -KMOD_LGUI);

			return true;
		} else if (mod & KMOD_RGUI) {
			btn = Button(KEYBOARD, 0, -KMOD_RGUI);

			return true;
		}
		int n = 0;
		const Uint8* state = SDL_GetKeyboardState(&n);
		for (int i = 0; i < n; ++i) {
			if (state[i]) {
				btn = Button(KEYBOARD, 0, i);

				return true;
			}
		}

		n = joystickCount();
		for (int i = 0; i < n; ++i) {
			const char* name = nullptr;
			SDL_Joystick* js = (SDL_Joystick*)joystickAt(i, &name);
			if (!js)
				continue;

			int m = SDL_JoystickNumButtons(js);
			for (int j = 0; j < m; ++j) {
				if (SDL_JoystickGetButton(js, j)) {
					btn = Button(JOYSTICK, (short)i, j);

					return true;
				}
			}

			m = SDL_JoystickNumHats(js);
			for (int j = 0; j < m; ++j) {
				const Uint8 hat = SDL_JoystickGetHat(js, j);
				if (hat & SDL_HAT_LEFT) {
					btn = Button(JOYSTICK, (short)i, (short)j, Hat::LEFT);

					return true;
				} else if (hat & SDL_HAT_RIGHT) {
					btn = Button(JOYSTICK, (short)i, (short)j, Hat::RIGHT);

					return true;
				} else if (hat & SDL_HAT_UP) {
					btn = Button(JOYSTICK, (short)i, (short)j, Hat::UP);

					return true;
				} else if (hat & SDL_HAT_DOWN) {
					btn = Button(JOYSTICK, (short)i, (short)j, Hat::DOWN);

					return true;
				}
			}

			m = SDL_JoystickNumAxes(js);
			for (int j = 0; j < m; ++j) {
				Sint16 init = 0;
				const SDL_bool hasInit = SDL_JoystickGetAxisInitialState(js, j, &init);
				const Sint16 axis = SDL_JoystickGetAxis(js, j);
				if (hasInit && init == 0 && axis != init) {
					btn = Button(JOYSTICK, (short)i, (short)j, (short)Math::sign(axis - init));

					return true;
				}
			}
		}

		return false;
	}

	virtual void update(
		class Window* wnd, class Renderer* /* rnd */,
		const Math::Rectf* clientArea, const Math::Vec2i* canvasSize,
		int scale
	) override {
		// Prepare.
		LockGuard<decltype(_lock)> guard(_lock);

		// Update the touch states.
		do {
			_mouseStatesNative.clear();

			const int wndw = wnd->width();
			const int wndh = wnd->height();
			int areax = 0, areay = 0, areaw = 0, areah = 0;
			if (clientArea) {
				areax = (int)clientArea->xMin();
				areay = (int)clientArea->yMin();
				areaw = (int)clientArea->width();
				areah = (int)clientArea->height();
			} else {
				areaw = wndw;
				areah = wndh;
				if (scale != 0 && scale != 1) {
					areaw /= scale;
					areah /= scale;
				}
			}
			int canvasw = 0, canvash = 0;
			if (canvasSize) {
				canvasw = (int)canvasSize->x;
				canvash = (int)canvasSize->y;
			} else {
				canvasw = wndw;
				canvash = wndh;
				if (scale != 0 && scale != 1) {
					canvasw /= scale;
					canvash /= scale;
				}
			}

			bool hasTouch = false;
			const int n = SDL_GetNumTouchDevices();
			for (int m = 0; m < n; ++m) {
				SDL_TouchID tid = SDL_GetTouchDevice(m);
				if (tid == 0)
					continue;
				const int f = SDL_GetNumTouchFingers(tid);
				while ((int)_mouseStatesNative.size() < f)
					_mouseStatesNative.push_back(Mouse(-1, -1, false, false, false, 0, 0));
				for (int i = 0; i < f; ++i) {
					SDL_Finger* finger = SDL_GetTouchFinger(tid, i);
					if (!finger)
						continue;
					Mouse &touch = _mouseStatesNative[i];
					if (touch.buttons[0])
						continue;
					touch = Mouse(
						(int)(finger->x * ((float)wndw - Math::EPSILON<float>())),
						(int)(finger->y * ((float)wndh - Math::EPSILON<float>())),
						true, false, false,
						0, 0
					);
					touch.valid = validatePoint(touch.x, touch.y, areax, areay, areaw, areah, canvasw, canvash, scale);
					if (touch.valid) {
						if (i == 0)
							hasTouch = true;
					} else {
						touch.buttons[0] = touch.buttons[1] = touch.buttons[2] = false;
					}
				}
			}
			if (!hasTouch) {
				ImGuiIO &io = ImGui::GetIO();

				if (_mouseStatesNative.empty())
					_mouseStatesNative.push_back(Mouse(-1, -1, false, false, false, 0, 0));
				Mouse &touch = _mouseStatesNative[0];
				Uint32 btns = SDL_GetMouseState(&touch.x, &touch.y);
				touch.buttons[0] = !!(btns & SDL_BUTTON(SDL_BUTTON_LEFT));
				touch.buttons[1] = !!(btns & SDL_BUTTON(SDL_BUTTON_RIGHT));
				touch.buttons[2] = !!(btns & SDL_BUTTON(SDL_BUTTON_MIDDLE));
				touch.wheelX = (int)io.MouseWheelH;
				touch.wheelY = (int)io.MouseWheel;
				touch.valid = validatePoint(touch.x, touch.y, areax, areay, areaw, areah, canvasw, canvash, scale);
				if (touch.valid) {
					hasTouch = true;
				} else {
					touch.buttons[0] = touch.buttons[1] = touch.buttons[2] = false;
				}
			}
		} while (false);

		// Update the keyboard states.
		do {
			_keyStatesNative.clear();

			int kc = 0;
			const Uint8* kbdState = SDL_GetKeyboardState(&kc);
			if ((int)_keyStatesNative.size() < kc)
				_keyStatesNative.resize(kc, 0);
			memcpy(&_keyStatesNative.front(), kbdState, kc);

			_keymodStatesNative = SDL_GetModState();
		} while (false);

		// Update the controller states.
		do {
			const Activities activity = active();
			if ((activity & CONTROLLER_ACTIVE) == INACTIVE) // Skip if no controller state required.
				break;

			if (_controllerStatesNative.size() < _controllers.size())
				_controllerStatesNative.resize(_controllers.size());

			for (int i = 0; i < (int)_controllers.size(); ++i) {
				ControllerState &state = _controllerStatesNative[i];
				state.clear();

				const Controller &c = _controllers[i];
				SDL_GameController* controller = c.controller;
				if (!controller)
					continue;

				for (int k = 0; k < SDL_CONTROLLER_BUTTON_MAX; ++k)
					state.buttons[k] = SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)k);
				for (int k = 0; k < SDL_CONTROLLER_AXIS_MAX; ++k)
					state.axises[k] = SDL_GameControllerGetAxis(controller, (SDL_GameControllerAxis)k);
			}
		} while (false);

		// Update the gamepad states.
		do {
			if (_gamepadStatesNative.size() < _gamepads.size())
				_gamepadStatesNative.resize(_gamepads.size());

			for (int i = 0; i < (int)_gamepads.size(); ++i) {
				GamepadState &state = _gamepadStatesNative[i];
				state.fill(false);

				const Gamepad &pad = _gamepads[i];
				for (int b = 0; b < BUTTON_COUNT; ++b) {
					if (i == 0) {
						if (_onscreenGamepadStates1[b]) {
							state[b] = true;

							continue;
						}
					}

					const Button &button = pad.buttons[b];
					switch (button.device) {
					case INVALID:
						break;
					case KEYBOARD: {
							const int val = button.value;
							if (val >= 0 && val < (int)_keyStatesNative.size()) {
								if (_keyStatesNative[val])
									state[b] = true;
							} else if (val < 0) {
								const SDL_Keymod mod = (SDL_Keymod)(-val);
								if (_keymodStatesNative & mod)
									state[b] = true;
							}
						}

						break;
					case JOYSTICK: {
							SDL_Joystick* js = nullptr;
							if (button.index < 0 || button.index >= (short)_joysticks.size())
								break;

							js = _joysticks[button.index].joystick;

							if (button.type == VALUE) {
								const int m = SDL_JoystickNumButtons(js);
								for (int j = 0; j < m; ++j) {
									if (button.value == j && SDL_JoystickGetButton(js, j))
										state[b] = true;
								}
							} else if (button.type == HAT) {
								const Uint8 hat = SDL_JoystickGetHat(js, button.hat.index);
								if (button.hat.value == Hat::LEFT && (hat & SDL_HAT_LEFT))
									state[b] = true;
								if (button.hat.value == Hat::RIGHT && (hat & SDL_HAT_RIGHT))
									state[b] = true;
								if (button.hat.value == Hat::UP && (hat & SDL_HAT_UP))
									state[b] = true;
								if (button.hat.value == Hat::DOWN && (hat & SDL_HAT_DOWN))
									state[b] = true;
							} else if (button.type == AXIS) {
								Sint16 init = 0;
								SDL_JoystickGetAxisInitialState(js, button.axis.index, &init);
								const Sint16 axis = SDL_JoystickGetAxis(js, button.axis.index);
								const short val = (short)Math::sign(axis - init);
								if (button.axis.value == val)
									state[b] = true;
							}
						}

						break;
					}
				}
			}

			if (_onscreenGamepadPressed > 0) {
				memset(_onscreenGamepadStates1, 0, sizeof(_onscreenGamepadStates1));
				_onscreenGamepadPressed = 0;
			}
		} while (false);
	}

	virtual int updateOnscreenGamepad(
		class Window* wnd_, class Renderer* rnd,
		struct ImFont* font,
		bool swapAB,
		float scale,
		float paddingX_, float paddingY_
	) override {
		// Prepare.
		const Activities activity = active();
		if ((activity & GAMEPAD_ACTIVE) == INACTIVE)
			return 0;

		const bool NOBLOCK = true;
		const ImColor BLACK(45, 39, 41, 128);
		const ImColor WHITE(255, 255, 255);
		const ImColor GRAY(128, 128, 128, 128);
		ImGuiWindow* wnd = ImGui::GetCurrentWindow();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 wndSize(
			wnd->ContentRegionRect.GetWidth(),
			wnd->ContentRegionRect.GetHeight() + wnd->TitleBarHeight()
		);
		const int scale_ = rnd->scale() / wnd_->scale();;

		const float dpadRadius = 60.0f * scale;
		const float btnRadius = 30.0f * scale;
		const float dpadCheckRadius = dpadRadius * 0.7f;
		const float btnCheckRadius = btnRadius * 1.25f;
		const float paddingX = (wndSize.x - dpadRadius * 2.0f) * (paddingX_ / 100.0f);
		const float paddingY = (wndSize.y - dpadRadius * 2.0f - wnd->TitleBarHeight() - 1.0f) * (paddingY_ / 100.0f);
		const float abSize = font ? (float)((int)(btnRadius * 1.5f / font->FontSize) * font->FontSize) : 0;

		const float top = (wndSize.y - (paddingY + dpadRadius)) - dpadRadius;
		const float left = (paddingX + dpadRadius) + dpadRadius;
		const float right = (wndSize.x - (paddingX + btnRadius) - (dpadRadius - btnRadius) * 2.0f) - btnRadius;
		if (top < wnd->TitleBarHeight() || left >= right)
			return 0;
#if defined BITTY_DEBUG
		drawList->AddLine(
			ImVec2(wnd->Pos.x + wndSize.x * 0.2f, wnd->Pos.y + top),
			ImVec2(wnd->Pos.x + wndSize.x * 0.8f, wnd->Pos.y + top),
			ImColor(0, 255, 0)
		);
		drawList->AddLine(
			ImVec2(wnd->Pos.x + left, wnd->Pos.y + wndSize.y * 0.7f),
			ImVec2(wnd->Pos.x + left, wnd->Pos.y + wndSize.y * 0.9f),
			ImColor(0, 255, 0)
		);
		drawList->AddLine(
			ImVec2(wnd->Pos.x + right, wnd->Pos.y + wndSize.y * 0.7f),
			ImVec2(wnd->Pos.x + right, wnd->Pos.y + wndSize.y * 0.9f),
			ImColor(0, 255, 0)
		);
#endif /* BITTY_DEBUG */

		const ImVec2 posDpad( // D-pad.
			wnd->Pos.x + (paddingX + dpadRadius),
			wnd->Pos.y + wndSize.y - (paddingY + dpadRadius)
		);
		ImVec2 posA( // A.
			wnd->Pos.x + wndSize.x - (paddingX + btnRadius),
			wnd->Pos.y + wndSize.y - (paddingY + btnRadius) - (dpadRadius - btnRadius) * 2.0f
		);
		ImVec2 posB( // B.
			wnd->Pos.x + wndSize.x - (paddingX + btnRadius) - (dpadRadius - btnRadius) * 2.0f,
			wnd->Pos.y + wndSize.y - (paddingY + btnRadius)
		);
		if (swapAB)
			std::swap(posA, posB);

		bool pad[BUTTON_COUNT] = {
			false, false, false, false,
			false, false
		};
		int pressed = 0;

		// Get input.
		if (NOBLOCK) {
			auto collides = [] (const ImVec4 &circ, float x, float y, int scale) -> bool {
				if (scale != 1) {
					x /= scale;
					y /= scale;
				}
				const float dx = x - circ.x;
				const float dy = y - circ.y;
				const float dist = std::sqrt(dx * dx + dy * dy);

				return dist <= circ.z;
			};
			const ImVec4 ranges[BUTTON_COUNT] = {
				ImVec4(posDpad.x - dpadCheckRadius * 1.0f, posDpad.y, dpadCheckRadius, 0.0f),
				ImVec4(posDpad.x + dpadCheckRadius * 1.0f, posDpad.y, dpadCheckRadius, 0.0f),
				ImVec4(posDpad.x, posDpad.y - dpadCheckRadius * 1.0f, dpadCheckRadius, 0.0f),
				ImVec4(posDpad.x, posDpad.y + dpadCheckRadius * 1.0f, dpadCheckRadius, 0.0f),
				ImVec4(posA.x, posA.y, btnCheckRadius, 0.0f),
				ImVec4(posB.x, posB.y, btnCheckRadius, 0.0f)
			};
			const int touchDev = SDL_GetNumTouchDevices();
			for (int i = 0; i < BUTTON_COUNT; ++i) {
				const ImVec4 &range = ranges[i];
#if defined BITTY_DEBUG
				drawList->AddCircle(ImVec2(range.x, range.y), range.z, ImColor(255, 0, 0), 15);
#endif /* BITTY_DEBUG */
				bool hasTouch = false;
				for (int m = 0; m < touchDev; ++m) {
					SDL_TouchID tid = SDL_GetTouchDevice(m);
					if (tid == 0)
						continue;

					const int f = SDL_GetNumTouchFingers(tid);
					for (int j = 0; j < f; ++j) {
						SDL_Finger* finger = SDL_GetTouchFinger(tid, j);
						if (!finger)
							continue;

						const float x = finger->x * (wndSize.x - Math::EPSILON<float>());
						const float y = finger->y * (wndSize.y - Math::EPSILON<float>());
						if (collides(range, x, y, scale_)) {
							pad[i] = true;
							++pressed;

							break;
						}
					}
					if (pad[i]) {
						hasTouch = true;

						break;
					}
				}

				if (!hasTouch) {
					int x = 0, y = 0;
					Uint32 btns = SDL_GetMouseState(&x, &y);
					if (!!(btns & SDL_BUTTON(SDL_BUTTON_LEFT))) {
						if (collides(range, (float)x, (float)y, scale_)) {
							pad[i] = true;
							++pressed;

							hasTouch = true;
						}
					}
				}
			}
		}

		// Render.
		drawList->AddCircle(posDpad, dpadRadius, WHITE, 19); // D-pad.
		drawList->AddCircle(posDpad, dpadRadius - 2.0f, BLACK, 19);
		if (pad[LEFT]) { // Left.
			drawList->AddTriangleFilled(
				ImVec2(posDpad.x - dpadRadius * 0.8f, posDpad.y),
				ImVec2(posDpad.x - dpadRadius * 0.4f, posDpad.y - dpadRadius * 0.2f),
				ImVec2(posDpad.x - dpadRadius * 0.4f, posDpad.y + dpadRadius * 0.2f),
				GRAY
			);
		}
		drawList->AddTriangle(
			ImVec2(posDpad.x - dpadRadius * 0.8f, posDpad.y),
			ImVec2(posDpad.x - dpadRadius * 0.4f, posDpad.y - dpadRadius * 0.2f),
			ImVec2(posDpad.x - dpadRadius * 0.4f, posDpad.y + dpadRadius * 0.2f),
			WHITE
		);
		if (pad[RIGHT]) { // Right.
			drawList->AddTriangleFilled(
				ImVec2(posDpad.x + dpadRadius * 0.8f, posDpad.y),
				ImVec2(posDpad.x + dpadRadius * 0.4f, posDpad.y + dpadRadius * 0.2f),
				ImVec2(posDpad.x + dpadRadius * 0.4f, posDpad.y - dpadRadius * 0.2f),
				GRAY
			);
		}
		drawList->AddTriangle(
			ImVec2(posDpad.x + dpadRadius * 0.8f, posDpad.y),
			ImVec2(posDpad.x + dpadRadius * 0.4f, posDpad.y - dpadRadius * 0.2f),
			ImVec2(posDpad.x + dpadRadius * 0.4f, posDpad.y + dpadRadius * 0.2f),
			WHITE
		);
		if (pad[UP]) { // Up.
			drawList->AddTriangleFilled(
				ImVec2(posDpad.x, posDpad.y - dpadRadius * 0.8f),
				ImVec2(posDpad.x + dpadRadius * 0.2f, posDpad.y - dpadRadius * 0.4f),
				ImVec2(posDpad.x - dpadRadius * 0.2f, posDpad.y - dpadRadius * 0.4f),
				GRAY
			);
		}
		drawList->AddTriangle(
			ImVec2(posDpad.x, posDpad.y - dpadRadius * 0.8f),
			ImVec2(posDpad.x - dpadRadius * 0.2f, posDpad.y - dpadRadius * 0.4f),
			ImVec2(posDpad.x + dpadRadius * 0.2f, posDpad.y - dpadRadius * 0.4f),
			WHITE
		);
		if (pad[DOWN]) { // Down.
			drawList->AddTriangleFilled(
				ImVec2(posDpad.x, posDpad.y + dpadRadius * 0.8f),
				ImVec2(posDpad.x - dpadRadius * 0.2f, posDpad.y + dpadRadius * 0.4f),
				ImVec2(posDpad.x + dpadRadius * 0.2f, posDpad.y + dpadRadius * 0.4f),
				GRAY
			);
		}
		drawList->AddTriangle(
			ImVec2(posDpad.x, posDpad.y + dpadRadius * 0.8f),
			ImVec2(posDpad.x - dpadRadius * 0.2f, posDpad.y + dpadRadius * 0.4f),
			ImVec2(posDpad.x + dpadRadius * 0.2f, posDpad.y + dpadRadius * 0.4f),
			WHITE
		);
		if (pad[A]) { // A.
			drawList->AddCircleFilled(posA, btnRadius, GRAY, 15);
		}
		drawList->AddCircle(posA, btnRadius, WHITE, 19);
		drawList->AddCircle(posA, btnRadius - 2.0f, BLACK, 19);
		if (font) {
			drawList->AddText(font, abSize, ImVec2(posA.x - abSize / 4.5f, posA.y - abSize / 2.0f + 2.0f), BLACK, "A");
			drawList->AddText(font, abSize, ImVec2(posA.x - abSize / 4.5f, posA.y - abSize / 2.0f), WHITE, "A");
		}
		if (pad[B]) { // B.
			drawList->AddCircleFilled(posB, btnRadius, GRAY, 15);
		}
		drawList->AddCircle(posB, btnRadius, WHITE, 19);
		drawList->AddCircle(posB, btnRadius - 2.0f, BLACK, 19);
		if (font) {
			drawList->AddText(font, abSize, ImVec2(posB.x - abSize / 4.5f, posB.y - abSize / 2.0f + 2.0f), BLACK, "B");
			drawList->AddText(font, abSize, ImVec2(posB.x - abSize / 4.5f, posB.y - abSize / 2.0f), WHITE, "B");
		}

		// Fill data.
		LockGuard<decltype(_lock)> guard(_lock);

		_onscreenGamepadPressed = pressed;
		memcpy(_onscreenGamepadStates1, pad, sizeof(_onscreenGamepadStates1));

		return pressed;
	}

	virtual int buttonDown(int btn, int idx) const override {
		_activeRequests = (Activities)(_activeRequests | GAMEPAD_ACTIVE);

		if (idx < 0 || idx >= (int)_gamepadStates1.size())
			return 0;

		const GamepadState &s1 = _gamepadStates1[idx];

		if (btn < 0) {
			for (int i = 0; i < (int)s1.size(); ++i) {
				if (s1[i])
					return 1;
			}

			return 0;
		}

		if (btn < 0 || btn >= (int)s1.size())
			return 0;

		return s1[btn] ? 1 : 0;
	}
	virtual int buttonUp(int btn, int idx) const override {
		_activeRequests = (Activities)(_activeRequests | GAMEPAD_ACTIVE);

		if (idx < 0 || idx >= (int)_gamepadStates1.size() || idx >= (int)_gamepadStates0.size())
			return 0;

		const GamepadState &s0 = _gamepadStates0[idx];
		const GamepadState &s1 = _gamepadStates1[idx];

		if (btn < 0) {
			for (int i = 0; i < (int)s1.size(); ++i) {
				if (s0[i] && !s1[i])
					return 1;
			}

			return 0;
		}

		if (btn < 0 || btn >= (int)s1.size() || btn >= (int)s0.size())
			return 0;

		return s0[btn] && !s1[btn] ? 1 : 0;
	}
	virtual bool rumbleGamepad(int idx, int lowHz, int hiHz, unsigned ms) override {
		LockGuard<decltype(_lock)> guard(_lock);

		if (idx < 0 || idx >= (int)_gamepads.size() || idx >= (int)_gamepads.size())
			return false;

		int joystickIndex = -1;
		const Gamepad &pad = _gamepads[idx];
		for (int i = 0; i < BUTTON_COUNT; ++i) {
			if (pad.buttons[i].device == JOYSTICK) {
				joystickIndex = pad.buttons[i].index;

				break;
			}
		}

		SDL_Joystick* js = (SDL_Joystick*)joystickAt(joystickIndex, nullptr);
		if (!js)
			return false;

		return SDL_JoystickRumble(js, (Uint16)lowHz, (Uint16)hiHz, ms) == 0;
	}

	virtual int controllerDown(int btn, int idx) const override {
		// Prepare.
		_activeRequests = (Activities)(_activeRequests | CONTROLLER_ACTIVE);

		// Get button state.
		if (idx < 0 || idx >= (int)_controllerStates1.size())
			return 0;

		const ControllerState &s1 = _controllerStates1[idx];

		if (btn >= 0 && btn < (int)s1.buttons.size())
			return s1.buttons[btn];

		// Get axis state.
		if (btn < 0) {
			btn = -btn - 1;

			int initial = 0;
			do {
				LockGuard<decltype(_lock)> guard(_lock);

				if (idx < 0 || idx >= (int)_controllers.size())
					break;
				const Controller &c = _controllers[idx];
				initial = c.axisInitialValues[btn];
			} while (false);

			if (btn >= 0 && btn <= (int)s1.axises.size()) {
				if (s1.axises[btn] != initial)
					return s1.axises[btn];
			}
		}

		// Finish.
		return 0;
	}
	virtual int controllerUp(int btn, int idx) const override {
		// Prepare.
		_activeRequests = (Activities)(_activeRequests | CONTROLLER_ACTIVE);

		// Get button state.
		if (idx < 0 || idx >= (int)_controllerStates1.size() || idx >= (int)_controllerStates0.size())
			return 0;

		const ControllerState &s0 = _controllerStates0[idx];
		const ControllerState &s1 = _controllerStates1[idx];

		if (btn >= 0 && btn < (int)s1.buttons.size()) {
			if (s0.buttons[btn] && !s1.buttons[btn])
				return s0.buttons[btn];
			else
				return 0;
		}

		// Get axis state.
		if (btn < 0) {
			btn = -btn - 1;

			int initial = 0;
			do {
				LockGuard<decltype(_lock)> guard(_lock);

				if (idx < 0 || idx >= (int)_controllers.size())
					break;
				const Controller &c = _controllers[idx];
				initial = c.axisInitialValues[btn];
			} while (false);

			if (btn >= 0 && btn <= (int)s1.axises.size()) {
				if (s0.axises[btn] != initial && s1.axises[btn] == initial)
					return s0.axises[btn];
				else
					return 0;
			}
		}

		// Finish.
		return 0;
	}
	virtual bool rumbleController(int idx, int lowHz, int hiHz, unsigned ms) override {
		LockGuard<decltype(_lock)> guard(_lock);

		if (idx < 0 || idx >= (int)_controllers.size() || idx >= (int)_controllers.size())
			return false;

		const Controller &c = _controllers[idx];
		SDL_GameController* controller = c.controller;
		if (!controller)
			return false;

		return SDL_GameControllerRumble(controller, (Uint16)lowHz, (Uint16)hiHz, ms) == 0;
	}

	virtual bool keyDown(int key) const override {
		_activeRequests = (Activities)(_activeRequests | KEYBOARD_ACTIVE);

		if (key < 0 && !_keyStates1.empty()) {
			for (int i = 0; i < (int)_keyStates1.size(); ++i) {
				if (!!_keyStates1[i])
					return true;
			}

			return false;
		}

		SDL_Scancode scancode = SDL_GetScancodeFromKey((SDL_Keycode)key);
		key = (int)scancode;
		if (key < 0 || key >= (int)_keyStates1.size())
			return false;

		return !!_keyStates1[key];
	}
	virtual bool keyUp(int key) const override {
		_activeRequests = (Activities)(_activeRequests | KEYBOARD_ACTIVE);

		if (key < 0 && !_keyStates1.empty() && !_keyStates0.empty()) {
			for (int i = 0; i < (int)_keyStates1.size() && i < (int)_keyStates0.size(); ++i) {
				if (!_keyStates1[i] && !!_keyStates0[i])
					return true;
			}

			return false;
		}

		SDL_Scancode scancode = SDL_GetScancodeFromKey((SDL_Keycode)key);
		key = (int)scancode;
		if (key < 0 || key >= (int)_keyStates1.size() || key >= (int)_keyStates0.size())
			return false;

		return !_keyStates1[key] && !!_keyStates0[key];
	}

	virtual bool mouse(
		int idx,
		int* x, int* y,
		bool* b0, bool* b1, bool* b2,
		int* wheelX, int* wheelY
	) const override {
		_activeRequests = (Activities)(_activeRequests | MOUSE_ACTIVE);

		if (x)
			*x = -1;
		if (y)
			*y = -1;
		if (b0)
			*b0 = false;
		if (b1)
			*b1 = false;
		if (b2)
			*b2 = false;
		if (wheelX)
			*wheelX = 0;
		if (wheelY)
			*wheelY = 0;

		if (idx < 0 || idx >= (int)_mouseStates1.size())
			return false;

		const Mouse &t = _mouseStates1[idx];
		if (x)
			*x = t.x;
		if (y)
			*y = t.y;
		if (b0)
			*b0 = t.buttons[0];
		if (b1)
			*b1 = t.buttons[1];
		if (b2)
			*b2 = t.buttons[2];
		if (wheelX)
			*wheelX = t.wheelX;
		if (wheelY)
			*wheelY = t.wheelY;

		return t.valid;
	}

	virtual void sync(void) override {
		LockGuard<decltype(_lock)> guard(_lock);

		_gamepadStates0 = _gamepadStates1;
		_gamepadStates1 = _gamepadStatesNative;

		_controllerStates0 = _controllerStates1;
		_controllerStates1 = _controllerStatesNative;

		_keyStates0 = _keyStates1;
		_keyStates1 = _keyStatesNative;

		_keymodStates0 = _keymodStates1;
		_keymodStates1 = _keymodStatesNative;

		_mouseStates1 = _mouseStatesNative;
		if (_onscreenGamepadPressed > 0)
			_mouseStates1.clear();

		_activeRequested = _activeRequests;
		_activeRequests = INACTIVE;
	}

	virtual Activities active(void) override {
		LockGuard<decltype(_lock)> guard(_lock);

		return (Activities)_activeRequested;
	}

private:
	void clear(bool unplugDevices) {
		LockGuard<decltype(_lock)> guard(_lock);

		_activeRequested = _activeRequests = INACTIVE;

		_mouseStates1.clear();

		_keymodStates1 = KMOD_NONE;
		_keymodStates0 = KMOD_NONE;

		_keyStates1.clear();
		_keyStates0.clear();

		_controllerStates1.clear();
		_controllerStates0.clear();

		_gamepadStates1.clear();
		_gamepadStates0.clear();

		memset(_onscreenGamepadStates1, 0, sizeof(_onscreenGamepadStates1));

		_gamepadStatesNative.clear();
		_controllerStatesNative.clear();
		_keyStatesNative.clear();
		_keymodStatesNative = KMOD_NONE;
		_mouseStatesNative.clear();

		if (unplugDevices) {
			_gamepads.clear();

			for (Controller &c : _controllers)
				SDL_GameControllerClose(c.controller);
			_controllers.clear();

			for (Joystick &j : _joysticks)
				SDL_JoystickClose(j.joystick);
			_joysticks.clear();
		}
	}

	static bool validatePoint(int &touchX, int &touchY, int areaX, int areaY, int areaW, int areaH, int canvasW, int canvasH, int scale) {
		const double dstW = (double)canvasW;
		const double dstH = (double)canvasH;
		double fx = 0, fy = 0;
		if (scale == 1) {
			fx = (double)(touchX - areaX) / areaW * dstW;
			fy = (double)(touchY - areaY) / areaH * dstH;
		} else {
			fx = (double)(touchX / (double)scale - areaX) / areaW * dstW;
			fy = (double)(touchY / (double)scale - areaY) / areaH * dstH;
		}
		if (fx >= 0 && fx < canvasW && fy >= 0 && fy < canvasH) {
			touchX = (int)fx;
			touchY = (int)fy;

			return true;
		}
		touchX = -1;
		touchY = -1;

		return false;
	}
};

Input::Button::Button() {
}

Input::Button::Button(Devices dev, short idx, int val) : device(dev), index(idx), type(VALUE), value(val) {
}

Input::Button::Button(Devices dev, short idx, short hatIdx, Hat::Types hatVal) : device(dev), index(idx), type(HAT) {
	hat.index = hatIdx;
	hat.value = hatVal;
}

Input::Button::Button(Devices dev, short idx, short axisIdx, short axisVal) : device(dev), index(idx), type(AXIS) {
	axis.index = axisIdx;
	axis.value = axisVal;
}

Input::Button &Input::Button::operator = (const Button &other) {
	memcpy(this, &other, sizeof(Button));

	return *this;
}

bool Input::Button::operator == (const Button &other) const {
	return memcmp(this, &other, sizeof(Button)) == 0;
}

bool Input::Button::operator != (const Button &other) const {
	return memcmp(this, &other, sizeof(Button)) != 0;
}

Input::Gamepad::Gamepad() {
}

Input::Gamepad::Gamepad(const Gamepad &other) {
	for (int i = 0; i < BUTTON_COUNT; ++i)
		buttons[i] = other.buttons[i];
}

Input::Gamepad &Input::Gamepad::operator = (const Gamepad &other) {
	for (int i = 0; i < BUTTON_COUNT; ++i)
		buttons[i] = other.buttons[i];

	return *this;
}

bool Input::Gamepad::operator == (const Gamepad &other) const {
	for (int i = 0; i < BUTTON_COUNT; ++i) {
		if (buttons[i] != other.buttons[i])
			return false;
	}

	return true;
}

bool Input::Gamepad::operator != (const Gamepad &other) const {
	for (int i = 0; i < BUTTON_COUNT; ++i) {
		if (buttons[i] != other.buttons[i])
			return true;
	}

	return false;
}

Input* Input::create(void) {
	InputImpl* result = new InputImpl();

	return result;
}

void Input::destroy(Input* ptr) {
	InputImpl* impl = static_cast<InputImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
