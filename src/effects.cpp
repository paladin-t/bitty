/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "bytes.h"
#include "effects.h"
#include "file_handle.h"
#include "filesystem.h"
#include "renderer.h"
#include "window.h"
#include "workspace.h"
#if BITTY_EFFECTS_ENABLED
#	if defined BITTY_OS_WIN
#		include "../lib/glew/include/GL/glew.h"
#		if defined BITTY_OS_WIN32
#			pragma comment(lib, "lib/glew/lib/Release/Win32/glew32.lib")
#			pragma comment(lib, "Opengl32.lib")
#		endif /* BITTY_OS_WIN32 */
#		if defined BITTY_OS_WIN64
#			pragma comment(lib, "lib/glew/lib/Release/x64/glew32.lib")
#			pragma comment(lib, "Opengl32.lib")
#		endif /* BITTY_OS_WIN64 */
#	elif defined BITTY_OS_MAC
#		include "../lib/gl3w/GL/gl3w.h"
#	elif defined BITTY_OS_LINUX
#		include "../lib/gl3w/GL/gl3w.h"
#	endif /* Platform macro. */
#endif /* BITTY_EFFECTS_ENABLED */
#include "../lib/jpath/jpath.hpp"
#include <SDL.h>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef EFFECTS_DEFAULT_FILE
#	define EFFECTS_DEFAULT_FILE "../effects/default.json"
#endif /* EFFECTS_DEFAULT_FILE */

#ifndef EFFECTS_OFFSETOF
#	define EFFECTS_OFFSETOF(T, M) ((size_t)&(((T*)0)->M))
#endif /* EFFECTS_OFFSETOF */

/* ===========================================================================} */

/*
** {===========================================================================
** Effects
*/

#if BITTY_EFFECTS_ENABLED && (defined BITTY_OS_WIN || defined BITTY_OS_MAC || defined BITTY_OS_LINUX)
class EffectsImpl : public Effects {
private:
	struct Material {
		bool valid = false;

		GLuint program = 0;
		GLuint vert = 0;
		GLuint frag = 0;

		GLuint attribTex = 0;
		GLuint attribResolution = 0;
		GLuint attribTime = 0;
		GLuint attribProjMatrix = 0;
		GLuint attribPosition = 0;
		GLuint attribUv = 0;
		GLuint attribColor = 0;

		GLuint vbo = 0;
		GLuint elements = 0;

		void open(const GLchar* vertSrc, const GLchar* fragSrc, Workspace* workspace) {
			if (!glCreateProgram)
				return;

			const GLchar* verVert[1] = { vertSrc };
			const GLchar* verFrag[1] = { fragSrc };

			program = glCreateProgram();
			vert = glCreateShader(GL_VERTEX_SHADER);
			frag = glCreateShader(GL_FRAGMENT_SHADER);
			glShaderSource(vert, BITTY_COUNTOF(verVert), verVert, NULL);
			glShaderSource(frag, BITTY_COUNTOF(verFrag), verFrag, NULL);
			glCompileShader(vert);
			if (!getError(vert, workspace))
				return;
			glCompileShader(frag);
			if (!getError(frag, workspace))
				return;
			glAttachShader(program, vert);
			glAttachShader(program, frag);
			glLinkProgram(program);

			attribTex = glGetUniformLocation(program, "Texture");
			attribResolution = glGetUniformLocation(program, "Resolution");
			attribTime = glGetUniformLocation(program, "Time");
			attribProjMatrix = glGetUniformLocation(program, "ProjMatrix");
			attribPosition = glGetAttribLocation(program, "Position");
			attribUv = glGetAttribLocation(program, "UV");
			attribColor = glGetAttribLocation(program, "Color");

			glGenBuffers(1, &vbo);
			glGenBuffers(1, &elements);

			valid = true;
		}
		void close(void) {
			if (vbo) {
				glDeleteBuffers(1, &vbo);
				vbo = 0;
			}
			if (elements) {
				glDeleteBuffers(1, &elements);
				elements = 0;
			}

			attribTex = 0;
			attribResolution = 0;
			attribTime = 0;
			attribProjMatrix = 0;
			attribPosition = 0;
			attribUv = 0;
			attribColor = 0;

			if (vert) {
				if (program)
					glDetachShader(program, vert);
				glDeleteShader(vert);
				vert = 0;
			}

			if (frag) {
				if (program)
					glDetachShader(program, frag);
				glDeleteShader(frag);
				frag = 0;
			}

			if (program) {
				glDeleteProgram(program);
				program = 0;
			}

			valid = false;
		}

		bool getError(GLuint obj, Workspace* workspace) {
			int status = 0;
			glGetShaderiv(obj, GL_COMPILE_STATUS, &status);

			if (status != GL_TRUE) {
				char msg[1024];
				memset(msg, 0, BITTY_COUNTOF(msg));
				GLsizei len = 0;
				glGetShaderInfoLog(obj, BITTY_COUNTOF(msg), &len, msg);
				fprintf(stderr, "%s\n", msg);

				std::string msg_ = "Effects:\n";
				msg_ += msg;
				workspace->error(msg_.c_str());

				return false;
			}

			return true;
		}
	};
	struct Vert {
		Math::Vec2<float> position;
		Math::Vec2<float> uv;
		UInt32 color = 0;

		Vert() {
		}
		Vert(const Math::Vec2<float> &pos, const Math::Vec2<float> &uv_, UInt32 col) : position(pos), uv(uv_), color(col) {
		}
	};

private:
	SDL_GLContext _glContext = nullptr;
	GLuint _glTexture = 0;

	Material _material;
	Math::Vec3<double> _ticks;
	Texture* _texture = nullptr;
	Bytes* _pixels = nullptr;

public:
	EffectsImpl() {
	}
	virtual ~EffectsImpl() {
	}

	virtual bool open(class Window* wnd, class Renderer* rnd, class Workspace* workspace) override {
#if defined BITTY_OS_WIN
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#elif defined BITTY_OS_MAC
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#elif defined BITTY_OS_LINUX
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#endif /* Platform macro. */
		SDL_Window* window = (SDL_Window*)wnd->pointer();
		_glContext = SDL_GL_CreateContext(window);
		if (!_glContext) {
			std::string msg = "Cannot create OpenGL context: ";
			msg += SDL_GetError();
			workspace->error(msg.c_str());
			fprintf(stderr, "%s\n", msg.c_str());

			return false;
		}
#if defined BITTY_OS_WIN
		glewInit();
#elif defined BITTY_OS_MAC
		gl3wInit();
#elif defined BITTY_OS_LINUX
		gl3wInit();
#endif /* Platform macro. */
		SDL_GL_SetSwapInterval(0);
		SDL_GL_MakeCurrent(window, _glContext);

		_texture = Texture::create();
		const Color color[] = { Color(), Color(), Color(), Color() };
		_texture->fromBytes(rnd, Texture::TARGET, (Byte*)color, 2, 2, 0);
		_texture->blend(Texture::BLEND);

		int width = 0, height = 0;
		SDL_GL_GetDrawableSize(window, &width, &height);
		_texture->resize(rnd, width, height);

		_pixels = Bytes::create();

		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &_glTexture);

		_ticks = Math::Vec3<double>(0, 0, 0);
		if (Path::existsFile(EFFECTS_DEFAULT_FILE)) {
			File::Ptr file(File::create());
			if (file->open(EFFECTS_DEFAULT_FILE, Stream::READ)) {
				std::string fx;
				file->readString(fx);
				file->close();

				if (use(fx.c_str(), workspace))
					return true;
			}
		}

		return true;
	}
	virtual bool close(void) override {
		_material.close();

		if (_glTexture) {
			glDeleteTextures(1, &_glTexture);
			_glTexture = 0;
		}

		if (_pixels) {
			Bytes::destroy(_pixels);
			_pixels = nullptr;
		}

		if (_texture) {
			Texture::destroy(_texture);
			_texture = nullptr;
		}

		if (_glContext) {
			SDL_GL_DeleteContext(_glContext);
			_glContext = nullptr;
		}

		return true;
	}

	virtual bool use(const char* config, class Workspace* workspace) override {
		Json::Ptr json(Json::create());
		if (!json->fromString(config))
			return false;
		rapidjson::Document doc;
		if (!json->toJson(doc))
			return false;

		std::string vert, frag;
		if (!Jpath::get(doc, vert, "vert"))
			return false;
		if (!Jpath::get(doc, frag, "frag"))
			return false;

		File::Ptr file(File::create());
		if (!file->open(vert.c_str(), Stream::READ))
			return false;
		if (!file->readString(vert)) {
			file->close();

			return false;
		}
		file->close();
		if (!file->open(frag.c_str(), Stream::READ))
			return false;
		if (!file->readString(frag)) {
			file->close();

			return false;
		}
		file->close();

		Material mat;
		mat.open(vert.c_str(), frag.c_str(), workspace);
		if (!mat.valid)
			return false;

		std::swap(mat, _material);
		mat.close();

		return true;
	}

	virtual void prepare(class Window* wnd, class Renderer* rnd, double delta) override {
		_ticks.x += delta;
		if (_ticks.x >= 1) {
			_ticks.x -= 1;
			_ticks.y += 1;
			if (_ticks.y >= 60) {
				_ticks.y -= 60;
				_ticks.z += 1;
			}
		}

		if (!_glContext || !_texture) {
			rnd->target(nullptr);

			return;
		}
		if (!_material.valid) {
			rnd->flush();

			return;
		}

		SDL_Window* window = (SDL_Window*)wnd->pointer();

		int width = 0, height = 0;
		SDL_GL_GetDrawableSize(window, &width, &height);
		if (_texture->width() != width || _texture->height() != height)
			_texture->resize(rnd, width, height);

		rnd->target(_texture);
	}
	virtual void finish(class Window* wnd, class Renderer* rnd) override {
		if (!_glContext || !_texture) {
			rnd->flush();

			return;
		}
		if (!_material.valid) {
			rnd->flush();

			return;
		}

		SDL_Window* window = (SDL_Window*)wnd->pointer();

		SDL_GL_MakeCurrent(window, _glContext);
		int width = 0, height = 0;
		SDL_GL_GetDrawableSize(window, &width, &height);

		_pixels->resize(width * height * sizeof(Color));
		_texture->toBytes(rnd, _pixels->pointer());
		glBindTexture(GL_TEXTURE_2D, _glTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGBA,
			width, height,
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE, _pixels->pointer()
		);

		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_SCISSOR_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glViewport(0, 0, width, height);
		const GLfloat resolution[4] = {
			(GLfloat)width, (GLfloat)height, 1.0f / width, 1.0f / height
		};
		const GLfloat time[3] = {
			(GLfloat)_ticks.x, (GLfloat)_ticks.y, (GLfloat)_ticks.z
		};
		const GLfloat orthoProjection[4][4] = {
			{ 2.0f / width, 0.0f,            0.0f, 0.0f },
			{ 0.0f,         2.0f / -height,  0.0f, 0.0f },
			{ 0.0f,         0.0f,           -1.0f, 0.0f },
			{ -1.0f,        1.0f,            0.0f, 1.0f }
		};
		glUseProgram(_material.program);
		glUniform1i(_material.attribTex, 0);
		glUniform4fv(_material.attribResolution, 1, (GLfloat*)&resolution);
		glUniform3fv(_material.attribTime, 1, (GLfloat*)&time);
		glUniformMatrix4fv(_material.attribProjMatrix, 1, GL_FALSE, &orthoProjection[0][0]);
		glBindSampler(0, 0);

		GLuint vao = 0;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, _material.vbo);
		glEnableVertexAttribArray(_material.attribPosition);
		glEnableVertexAttribArray(_material.attribUv);
		glEnableVertexAttribArray(_material.attribColor);
		glVertexAttribPointer(_material.attribPosition, 2, GL_FLOAT, GL_FALSE, sizeof(Vert), (GLvoid*)EFFECTS_OFFSETOF(Vert, position));
		glVertexAttribPointer(_material.attribUv, 2, GL_FLOAT, GL_FALSE, sizeof(Vert), (GLvoid*)EFFECTS_OFFSETOF(Vert, uv));
		glVertexAttribPointer(_material.attribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vert), (GLvoid*)EFFECTS_OFFSETOF(Vert, color));

		const Vert vertexes[] = {
			Vert(Math::Vec2<float>(0, 0), Math::Vec2<float>(0, 0), 0xffffffff),
			Vert(Math::Vec2<float>((float)width, 0), Math::Vec2<float>(1, 0), 0xffffffff),
			Vert(Math::Vec2<float>(0, (float)height), Math::Vec2<float>(0, 1), 0xffffffff),
			Vert(Math::Vec2<float>((float)width, (float)height), Math::Vec2<float>(1, 1), 0xffffffff)
		};
		const unsigned short indices[] = {
			0, 2, 3,
			0, 3, 1
		};
		glBindBuffer(GL_ARRAY_BUFFER, _material.vbo);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)BITTY_COUNTOF(vertexes) * sizeof(Vert), (const GLvoid*)vertexes, GL_STREAM_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _material.elements);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)BITTY_COUNTOF(indices) * sizeof(unsigned short), (const GLvoid*)indices, GL_STREAM_DRAW);
		glBindTexture(GL_TEXTURE_2D, _glTexture);
		glDrawElements(GL_TRIANGLES, (GLsizei)6, GL_UNSIGNED_SHORT, 0);

		glDeleteVertexArrays(1, &vao);

		SDL_GL_SwapWindow(window);
	}
};
#else /* BITTY_EFFECTS_ENABLED && Platform macro. */
class EffectsImpl : public Effects {
public:
	EffectsImpl() {
	}
	virtual ~EffectsImpl() {
	}

	virtual bool open(class Window*, class Renderer*, class Workspace*) override {
		return false;
	}
	virtual bool close(void) override {
		return false;
	}

	virtual bool use(const char*, class Workspace*) override {
		return false;
	}

	virtual void prepare(class Window*, class Renderer* rnd, double) override {
		rnd->target(nullptr);
	}
	virtual void finish(class Window*, class Renderer* rnd) override {
		rnd->flush();
	}
};
#endif /* BITTY_EFFECTS_ENABLED && Platform macro. */

Effects* Effects::create(void) {
	EffectsImpl* result = new EffectsImpl();

	return result;
}

void Effects::destroy(Effects* ptr) {
	EffectsImpl* impl = static_cast<EffectsImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
