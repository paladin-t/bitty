/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "bytes.h"
#include "effects.h"
#include "encoding.h"
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

#if BITTY_EFFECTS_ENABLED
#	pragma message("Effects enabled.")
#endif /* BITTY_EFFECTS_ENABLED */

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
	struct Uniform {
		enum Types {
			NUMBER,
			VEC2,
			VEC3,
			VEC4,
			SAMPLER2D
		};
		union Data {
			GLfloat number;
			Math::Vec2<GLfloat> vec2;
			Math::Vec3<GLfloat> vec3;
			Math::Vec4<GLfloat> vec4;
			GLuint sampler2d;

			Data() {
				vec4 = Math::Vec4<GLfloat>(0, 0, 0, 0);
			}
		};

		Types type = VEC4;
		Data data;

		Uniform() {
		}
		Uniform(const std::string &y) {
			if (y == "number")
				type = NUMBER;
			else if (y == "vec2")
				type = VEC2;
			else if (y == "vec3")
				type = VEC3;
			else if (y == "vec4")
				type = VEC4;
			else if (y == "sampler2d")
				type = SAMPLER2D;
			else
				type = VEC4;
		}
		Uniform(const Uniform &other) {
			type = other.type;
			switch (type) {
			case NUMBER:
				data.number = other.data.number;

				break;
			case VEC2:
				data.vec2 = other.data.vec2;

				break;
			case VEC3:
				data.vec3 = other.data.vec3;

				break;
			case VEC4:
				data.vec4 = other.data.vec4;

				break;
			case SAMPLER2D:
				data.sampler2d = other.data.sampler2d;

				break;
			}
		}

		Uniform &operator = (const Uniform &other) {
			type = other.type;
			switch (type) {
			case NUMBER:
				data.number = other.data.number;

				break;
			case VEC2:
				data.vec2 = other.data.vec2;

				break;
			case VEC3:
				data.vec3 = other.data.vec3;

				break;
			case VEC4:
				data.vec4 = other.data.vec4;

				break;
			case SAMPLER2D:
				data.sampler2d = other.data.sampler2d;

				break;
			}

			return *this;
		}
	};
	struct Material {
		typedef std::vector<GLint> TextureUniforms;
		typedef std::vector<GLuint> TextureHandles;

		typedef std::map<std::string, GLint> ExtraUniforms;
		typedef std::map<std::string, Uniform> ExtraDatas;

		bool valid = false;

		bool hasClearColor = true;
		Math::Vec4<GLclampf> clearColor;
		GLint textureMinFilter = GL_NEAREST;
		GLint textureMagFilter = GL_NEAREST;
		GLint textureWrapS = GL_CLAMP_TO_EDGE;
		GLint textureWrapT = GL_CLAMP_TO_EDGE;

		GLuint program = 0;
		GLuint vert = 0;
		GLuint frag = 0;

		GLint uniformTexture = 0;
		TextureUniforms uniformExtraTextures;
		GLint uniformResolution = 0;
		GLint uniformCanvas = 0;
		GLint uniformTime = 0;
		GLint uniformProjMatrix = 0;
		ExtraUniforms uniformExtraDatas;
		GLint attribPosition = 0;
		GLint attribUv = 0;
		GLint attribColor = 0;

		GLuint vbo = 0;
		GLuint elements = 0;
		GLuint texture = 0;
		TextureHandles extraTextures;
		ExtraDatas extraDatas;

		Material() {
			clearColor = Math::Vec4<GLclampf>(0.118f, 0.118f, 0.118f, 1.0f);
		}

		void open(Workspace* ws, const GLchar* vertSrc, const GLchar* fragSrc, const std::vector<Image::Ptr>* images /* nullable */, const rapidjson::Value* uniforms /* nullable */) {
			if (!glCreateProgram)
				return;

			const GLchar* verVert[1] = { vertSrc };
			const GLchar* verFrag[1] = { fragSrc };

			glEnable(GL_TEXTURE_2D);
			glGenTextures(1, &texture);
			if (images) {
				for (int i = 0; i < (int)images->size(); ++i) {
					GLuint tex = 0;
					glGenTextures(1, &tex);
					extraTextures.push_back(tex);
					const Image::Ptr &img = images->at(i);
					glBindTexture(GL_TEXTURE_2D, tex);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureMinFilter);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureMagFilter);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureWrapS);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureWrapT);
					glTexImage2D(
						GL_TEXTURE_2D, 0, GL_RGBA,
						img->width(), img->height(),
						0,
						GL_RGBA,
						GL_UNSIGNED_BYTE, (const void*)img->pixels()
					);
				}
			}

			program = glCreateProgram();
			vert = glCreateShader(GL_VERTEX_SHADER);
			frag = glCreateShader(GL_FRAGMENT_SHADER);
			glShaderSource(vert, BITTY_COUNTOF(verVert), verVert, NULL);
			glShaderSource(frag, BITTY_COUNTOF(verFrag), verFrag, NULL);
			glCompileShader(vert);
			if (!getError(ws, vert, "Vertex shader:"))
				return;
			glCompileShader(frag);
			if (!getError(ws, frag, "Fragment shader:"))
				return;
			glAttachShader(program, vert);
			glAttachShader(program, frag);
			glLinkProgram(program);

			uniformTexture = glGetUniformLocation(program, "Texture");
			int i = 1;
			while (true) {
				const std::string entry = std::string("Texture") + Text::toString(i);
				const GLint tex = glGetUniformLocation(program, entry.c_str());
				if (tex == -1)
					break;
				uniformExtraTextures.push_back(tex);
				++i;
			}
			uniformResolution = glGetUniformLocation(program, "Resolution");
			uniformCanvas = glGetUniformLocation(program, "Canvas");
			uniformTime = glGetUniformLocation(program, "Time");
			uniformProjMatrix = glGetUniformLocation(program, "ProjMatrix");
			attribPosition = glGetAttribLocation(program, "Position");
			attribUv = glGetAttribLocation(program, "UV");
			attribColor = glGetAttribLocation(program, "Color");
			if (uniforms) {
				for (int j = 0; j < (int)uniforms->Capacity(); ++j) {
					std::string name;
					std::string type;
					if (!Jpath::get(*uniforms, name, j, "name"))
						continue;
					if (!Jpath::get(*uniforms, type, j, "type"))
						continue;
					if (name.empty() || type.empty())
						continue;
					uniformExtraDatas[name] = glGetUniformLocation(program, name.c_str());
					extraDatas[name] = Uniform(type);
					if (type == "sampler2d") {
						Uniform &uniform = extraDatas[name];
						GLuint tex = 0;
						glGenTextures(1, &tex);
						glBindTexture(GL_TEXTURE_2D, tex);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureMinFilter);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureMagFilter);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureWrapS);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureWrapT);
						uniform.data.sampler2d = tex;
					}
				}
			}

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

			uniformTexture = 0;
			uniformExtraTextures.clear();
			uniformResolution = 0;
			uniformCanvas = 0;
			uniformTime = 0;
			uniformProjMatrix = 0;
			uniformExtraDatas.clear();
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

			for (GLuint tex : extraTextures)
				glDeleteTextures(1, &tex);
			extraTextures.clear();
			if (texture) {
				glDeleteTextures(1, &texture);
				texture = 0;
			}
			for (auto kv : extraDatas) {
				Uniform &uniform = kv.second;
				if (uniform.type == Uniform::SAMPLER2D) {
					if (uniform.data.sampler2d)
						glDeleteTextures(1, &uniform.data.sampler2d);
				}
			}
			extraDatas.clear();

			textureMinFilter = GL_NEAREST;
			textureMagFilter = GL_NEAREST;
			textureWrapS = GL_CLAMP_TO_EDGE;
			textureWrapT = GL_CLAMP_TO_EDGE;

			hasClearColor = true;
			clearColor = Math::Vec4<GLclampf>(0.118f, 0.118f, 0.118f, 1.0f);

			valid = false;
		}

		bool getError(Workspace* ws, GLuint obj, const char* prefix) {
			int status = 0;
			glGetShaderiv(obj, GL_COMPILE_STATUS, &status);

			if (status != GL_TRUE) {
				char msg[1024];
				memset(msg, 0, BITTY_COUNTOF(msg));
				GLsizei len = 0;
				glGetShaderInfoLog(obj, BITTY_COUNTOF(msg), &len, msg);
				fprintf(stderr, "%s\n", msg);

				std::string msg_ = prefix;
				msg_ += "\n";
				msg_ += msg;
				ws->error(msg_.c_str());

				return false;
			}

			return true;
		}
	};
	struct Vert {
		Math::Vec2<GLfloat> position;
		Math::Vec2<GLfloat> uv;
		UInt32 color = 0;

		Vert() {
		}
		Vert(const Math::Vec2<GLfloat> &pos, const Math::Vec2<GLfloat> &uv_, UInt32 col) : position(pos), uv(uv_), color(col) {
		}
	};

private:
	Renderer* _renderer = nullptr; // Foreign.

	GLint _glVersion = 0;
	SDL_GLContext _glContext = nullptr;

	Material _material;
	Math::Vec3<double> _ticks;
	Texture* _texture = nullptr;
	Bytes* _pixels = nullptr;

public:
	EffectsImpl() {
	}
	virtual ~EffectsImpl() {
	}

	virtual bool open(class Window* wnd, class Renderer* rnd, class Workspace* ws, bool enabled) override {
		SDL_Window* window = (SDL_Window*)wnd->pointer();

		_renderer = rnd;

		if (!enabled)
			return false;

		_glContext = SDL_GL_CreateContext(window);
		if (!_glContext) {
			std::string msg = "Cannot create OpenGL context: ";
			msg += SDL_GetError();
			ws->error(msg.c_str());
			fprintf(stderr, "%s\n", msg.c_str());

			return false;
		}
		SDL_GL_MakeCurrent(window, _glContext);
		SDL_GL_SetSwapInterval(0);
#if defined BITTY_OS_WIN
		glewInit();
#elif defined BITTY_OS_MAC
		gl3wInit();
#elif defined BITTY_OS_LINUX
		gl3wInit();
#endif /* Platform macro. */

		GLint major = 0;
		GLint minor = 0;
		glGetIntegerv(GL_MAJOR_VERSION, &major);
		glGetIntegerv(GL_MINOR_VERSION, &minor);
		if (major == 0 && minor == 0) {
			const char* glVersion = (const char*)glGetString(GL_VERSION);
			if (glVersion) {
				const Text::Array parts = Text::split(glVersion, ".");
				if (parts.size() >= 2) {
					Text::fromString(parts[0], major);
					Text::fromString(parts[1], minor);
				}
			}
		}
		_glVersion = (GLuint)(major * 100 + minor * 10);
		fprintf(stdout, "OpenGL version: %d.\n", (int)_glVersion);

		int width = 0, height = 0;
		SDL_GL_GetDrawableSize(window, &width, &height);
		fprintf(stdout, "OpenGL initial drawable size: %dx%d.\n", width, height);

		_pixels = Bytes::create();

		_ticks = Math::Vec3<double>(0, 0, 0);
		if (Path::existsFile(EFFECTS_DEFAULT_FILE)) {
			File::Ptr file(File::create());
			if (file->open(EFFECTS_DEFAULT_FILE, Stream::READ)) {
				std::string fx;
				file->readString(fx);
				file->close();

				if (use(ws, fx.c_str()))
					return _material.valid;
			}
		}
		useDefault(ws);

		return _material.valid;
	}
	virtual bool close(void) override {
		_material.close();

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

		_renderer = nullptr;

		return true;
	}

	virtual bool valid(void) const override {
		return !!_glContext && _material.valid && _glVersion >= 300;
	}
	virtual bool use(class Workspace* ws, const char* material) override {
		// Use default material.
		if (!material) {
			useDefault(ws);

			_ticks = Math::Vec3<double>(0, 0, 0);

			return true;
		}

		// Prepare.
		Json::Ptr json(Json::create());
		if (!json->fromString(material))
			return false;
		rapidjson::Document doc;
		if (!json->toJson(doc))
			return false;

		// Load shader source.
		std::string vert, frag;
		if (!Jpath::get(doc, vert, "vs_source")) {
			if (!Jpath::get(doc, vert, "vs"))
				return false;
			File::Ptr file(File::create());
			if (!file->open(vert.c_str(), Stream::READ))
				return false;
			if (!file->readString(vert)) {
				file->close();

				return false;
			}
			file->close();
		}
		if (!Jpath::get(doc, frag, "fs_source")) {
			if (!Jpath::get(doc, frag, "fs"))
				return false;
			File::Ptr file(File::create());
			if (!file->open(frag.c_str(), Stream::READ))
				return false;
			if (!file->readString(frag)) {
				file->close();

				return false;
			}
			file->close();
		}

		// Load extra textures.
		std::vector<Image::Ptr> images;
		Text::Array textures;
		Jpath::get(doc, textures, "textures");
		for (const std::string &tex : textures) {
			if (Path::existsFile(tex.c_str())) {
				File::Ptr file(File::create());
				if (!file->open(tex.c_str(), Stream::READ))
					break;
				Bytes::Ptr bytes(Bytes::create());
				file->readBytes(bytes.get());
				Image::Ptr img(Image::create(nullptr));
				if (img->fromBytes(bytes.get()))
					images.push_back(img);
				file->close();
			} else {
				Bytes::Ptr bytes(Bytes::create());
				if (!Base64::toBytes(bytes.get(), tex))
					break;
				Image::Ptr img(Image::create(nullptr));
				if (img->fromBytes(bytes.get()))
					images.push_back(img);
			}
			if (1 + images.size() >= 32) // Up to 32 textures totally.
				break;
		}

		// Load extra uniforms.
		const rapidjson::Value* uniforms = nullptr;
		Jpath::get(doc, uniforms, "uniforms");

		// Load rendering attributes.
		Material mat;
		std::vector<int> color;
		if (Jpath::get(doc, color, "clear_color") && color.size() >= 4) {
			mat.hasClearColor = true;
			mat.clearColor = Math::Vec4<GLclampf>(
				Math::clamp((GLclampf)(color[0] / 255.0f), (GLclampf)0, (GLclampf)1),
				Math::clamp((GLclampf)(color[1] / 255.0f), (GLclampf)0, (GLclampf)1),
				Math::clamp((GLclampf)(color[2] / 255.0f), (GLclampf)0, (GLclampf)1),
				Math::clamp((GLclampf)(color[3] / 255.0f), (GLclampf)0, (GLclampf)1)
			);
		} else {
			mat.hasClearColor = false;
		}
		std::string param;
		if (Jpath::get(doc, param, "texture_min_filter")) {
			if (param == "nearest")
				mat.textureMinFilter = GL_NEAREST;
			else if (param == "linear")
				mat.textureMinFilter = GL_LINEAR;
		}
		if (Jpath::get(doc, param, "texture_mag_filter")) {
			if (param == "nearest")
				mat.textureMagFilter = GL_NEAREST;
			else if (param == "linear")
				mat.textureMagFilter = GL_LINEAR;
		}
		if (Jpath::get(doc, param, "texture_wrap_s")) {
			if (param == "repeat")
				mat.textureWrapS = GL_REPEAT;
#if defined GL_CLAMP
			else if (param == "clamp")
				mat.textureWrapS = GL_CLAMP;
#endif /* GL_CLAMP */
			else if (param == "clamp_to_edge")
				mat.textureWrapS = GL_CLAMP_TO_EDGE;
		}
		if (Jpath::get(doc, param, "texture_wrap_t")) {
			if (param == "repeat")
				mat.textureWrapT = GL_REPEAT;
#if defined GL_CLAMP
			else if (param == "clamp")
				mat.textureWrapT = GL_CLAMP;
#endif /* GL_CLAMP */
			else if (param == "clamp_to_edge")
				mat.textureWrapT = GL_CLAMP_TO_EDGE;
		}
		mat.open(ws, vert.c_str(), frag.c_str(), &images, uniforms);
		if (!mat.valid)
			return false;

		// Active the material.
		std::swap(mat, _material);
		mat.close();

		// Initialize ticks.
		_ticks = Math::Vec3<double>(0, 0, 0);

		// Finish.
		return true;
	}

	virtual bool inject(const char* entry, float arg) override {
		if (!_material.valid)
			return false;
		if (_glVersion < 300)
			return false;
		Material::ExtraDatas::iterator it = _material.extraDatas.find(entry);
		if (it == _material.extraDatas.end())
			return false;

		Uniform &uniform = it->second;
		uniform.data.number = (GLfloat)arg;

		return true;
	}
	virtual bool inject(const char* entry, const Math::Vec2f &arg) override {
		if (!_material.valid)
			return false;
		if (_glVersion < 300)
			return false;
		Material::ExtraDatas::iterator it = _material.extraDatas.find(entry);
		if (it == _material.extraDatas.end())
			return false;

		Uniform &uniform = it->second;
		uniform.data.vec2 = Math::Vec2<GLfloat>((GLfloat)arg.x, (GLfloat)arg.y);

		return true;
	}
	virtual bool inject(const char* entry, const Math::Vec3f &arg) override {
		if (!_material.valid)
			return false;
		if (_glVersion < 300)
			return false;
		Material::ExtraDatas::iterator it = _material.extraDatas.find(entry);
		if (it == _material.extraDatas.end())
			return false;

		Uniform &uniform = it->second;
		uniform.data.vec3 = Math::Vec3<GLfloat>((GLfloat)arg.x, (GLfloat)arg.y, (GLfloat)arg.z);

		return true;
	}
	virtual bool inject(const char* entry, const Math::Vec4f &arg) override {
		if (!_material.valid)
			return false;
		if (_glVersion < 300)
			return false;
		Material::ExtraDatas::iterator it = _material.extraDatas.find(entry);
		if (it == _material.extraDatas.end())
			return false;

		Uniform &uniform = it->second;
		uniform.data.vec4 = Math::Vec4<GLfloat>((GLfloat)arg.x, (GLfloat)arg.y, (GLfloat)arg.z, (GLfloat)arg.w);

		return true;
	}
	virtual bool inject(const char* entry, const Resources::Texture::Ptr &arg) override {
		if (!arg)
			return false;

		Image::Ptr src = arg->source.lock();
		if (src)
			return inject(entry, src);

		if (!_material.valid)
			return false;
		if (_glVersion < 300)
			return false;
		Material::ExtraDatas::iterator it = _material.extraDatas.find(entry);
		if (it == _material.extraDatas.end())
			return false;

		Texture::Ptr ptr = arg->pointer;
		if (!ptr)
			return false;
		Uniform &uniform = it->second;
		Bytes::Ptr pixels(Bytes::create());
		pixels->resize(ptr->width() * ptr->height() * sizeof(Color));
		if (ptr->toBytes(_renderer, pixels->pointer()) == 0)
			return false;
		glBindTexture(GL_TEXTURE_2D, uniform.data.sampler2d);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _material.textureMinFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _material.textureMagFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _material.textureWrapS);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _material.textureWrapT);
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGBA,
			ptr->width(), ptr->height(),
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE, (const void*)pixels->pointer()
		);

		return true;
	}
	virtual bool inject(const char* entry, const Image::Ptr &arg) override {
		if (!arg)
			return false;

		if (!_material.valid)
			return false;
		if (_glVersion < 300)
			return false;
		Material::ExtraDatas::iterator it = _material.extraDatas.find(entry);
		if (it == _material.extraDatas.end())
			return false;

		Uniform &uniform = it->second;
		glBindTexture(GL_TEXTURE_2D, uniform.data.sampler2d);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _material.textureMinFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _material.textureMagFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _material.textureWrapS);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _material.textureWrapT);
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGBA,
			arg->width(), arg->height(),
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE, (const void*)arg->pixels()
		);

		return true;
	}

	virtual void prepare(class Window* wnd, class Renderer* rnd, class Workspace*, double delta) override {
		// Prepare.
		SDL_Window* window = (SDL_Window*)wnd->pointer();

		if (!_glContext) {
			rnd->target(nullptr);

			return;
		}
		if (!_material.valid) {
			rnd->target(nullptr);

			return;
		}
		if (_glVersion < 300) {
			rnd->target(nullptr);

			return;
		}

		// Tick.
		_ticks.x += delta;
		if (_ticks.x >= 1) {
			_ticks.x -= 1;
			_ticks.y += 1;
			if (_ticks.y >= 60) {
				_ticks.y -= 60;
				_ticks.z += 1;
			}
		}

		// Initialize frame target.
		if (!_texture) {
			_texture = Texture::create();
			const Color color[] = { Color(), Color(), Color(), Color() };
			_texture->fromBytes(rnd, Texture::TARGET, (Byte*)color, 2, 2, 0, Texture::NEAREST);
			_texture->blend(Texture::BLEND);
		}
		int width = 0, height = 0;
		SDL_GL_GetDrawableSize(window, &width, &height);
		if (_texture->width() != width || _texture->height() != height)
			_texture->resize(rnd, width, height);

		// Set frame target.
		rnd->target(_texture);
	}
	virtual void finish(class Window* wnd, class Renderer* rnd, class Workspace* ws) override {
		// Prepare.
		SDL_Window* window = (SDL_Window*)wnd->pointer();

		if (!_glContext) {
			rnd->flush();

			return;
		}
		if (!_material.valid) {
			rnd->flush();

			return;
		}
		if (_glVersion < 300) {
			rnd->flush();

			return;
		}

		SDL_GL_MakeCurrent(window, _glContext);
		int width = 0, height = 0;
		SDL_GL_GetDrawableSize(window, &width, &height);

		// Reserve render states.
		GLenum lastActiveTexture; glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&lastActiveTexture);
		GLuint lastProgram; glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&lastProgram);
		GLuint lastTexture; glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&lastTexture);
#if defined GL_VERSION_3_3
		GLuint lastSampler; if (_glVersion >= 330) { glGetIntegerv(GL_SAMPLER_BINDING, (GLint*)&lastSampler); } else { lastSampler = 0; }
#endif /* GL_VERSION_3_3 */
		GLuint lastArrayBuffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint*)&lastArrayBuffer);
		GLuint lastVertexArrayObject; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&lastVertexArrayObject);
#if defined GL_POLYGON_MODE
		GLint lastPolygonMode[2]; glGetIntegerv(GL_POLYGON_MODE, lastPolygonMode);
#endif /* GL_POLYGON_MODE */
		GLint lastViewport[4]; glGetIntegerv(GL_VIEWPORT, lastViewport);
		GLint lastScissorBox[4]; glGetIntegerv(GL_SCISSOR_BOX, lastScissorBox);
		GLenum lastBlendSrcRgb; glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&lastBlendSrcRgb);
		GLenum lastBlendDstRgb; glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&lastBlendDstRgb);
		GLenum lastBlendSrcAlpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&lastBlendSrcAlpha);
		GLenum lastBlendDstAlpha; glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&lastBlendDstAlpha);
		GLenum lastBlendEquationRgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&lastBlendEquationRgb);
		GLenum lastBlendEquationAlpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&lastBlendEquationAlpha);
		GLboolean lastEnableBlend = glIsEnabled(GL_BLEND);
		GLboolean lastEnableCullFace = glIsEnabled(GL_CULL_FACE);
		GLboolean lastEnableDepthTest = glIsEnabled(GL_DEPTH_TEST);
		GLboolean lastEnableStencilTest = glIsEnabled(GL_STENCIL_TEST);
		GLboolean lastEnableScissorTest = glIsEnabled(GL_SCISSOR_TEST);
#if defined GL_VERSION_3_1
		GLboolean lastEnablePrimitiveRestart = _glVersion >= 310 ? glIsEnabled(GL_PRIMITIVE_RESTART) : GL_FALSE;
#endif /* GL_VERSION_3_1 */
		// Render.
		{
			// Fill frame buffer.
			_pixels->resize(_texture->width() * _texture->height() * sizeof(Color));
			_texture->toBytes(rnd, _pixels->pointer());
			glBindTexture(GL_TEXTURE_2D, _material.texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _material.textureMinFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _material.textureMagFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _material.textureWrapS);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _material.textureWrapT);
			glTexImage2D(
				GL_TEXTURE_2D, 0, GL_RGBA,
				_texture->width(), _texture->height(),
				0,
				GL_RGBA,
				GL_UNSIGNED_BYTE, (const void*)_pixels->pointer()
			);

			// Initialize render states.
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_CULL_FACE);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_SCISSOR_TEST);
#if defined GL_POLYGON_MODE
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif /* GL_POLYGON_MODE */

			// Initialize rendering data.
			glViewport(0, 0, width, height);
			if (_material.hasClearColor) {
				glClearColor(_material.clearColor.x, _material.clearColor.y, _material.clearColor.z, _material.clearColor.w);
				glClear(GL_COLOR_BUFFER_BIT);
			}
			glUseProgram(_material.program);
			const GLfloat resolution[4] = {
				(GLfloat)width, (GLfloat)height, 1.0f / width, 1.0f / height
			};
			glUniform4fv(_material.uniformResolution, 1, (GLfloat*)&resolution);
			const GLfloat canvas[4] = {
				(GLfloat)ws->canvasValidation().x, (GLfloat)ws->canvasValidation().y, 1.0f / ws->canvasValidation().x, 1.0f / ws->canvasValidation().y
			};
			glUniform4fv(_material.uniformCanvas, 1, (GLfloat*)&canvas);
			const GLfloat time[3] = {
				(GLfloat)_ticks.x, (GLfloat)_ticks.y, (GLfloat)_ticks.z
			};
			glUniform3fv(_material.uniformTime, 1, (GLfloat*)&time);
			const GLfloat orthoProjection[4][4] = {
				{ 2.0f / width, 0.0f,            0.0f, 0.0f },
				{ 0.0f,         2.0f / -height,  0.0f, 0.0f },
				{ 0.0f,         0.0f,           -1.0f, 0.0f },
				{ -1.0f,        1.0f,            0.0f, 1.0f }
			};
			glUniformMatrix4fv(_material.uniformProjMatrix, 1, GL_FALSE, &orthoProjection[0][0]);
			int textureIndex = 0;
			glUniform1i(_material.uniformTexture, textureIndex); ++textureIndex;
			for (int i = 0; i < (int)_material.uniformExtraTextures.size(); ++i) {
				glUniform1i(_material.uniformExtraTextures[i], textureIndex); ++textureIndex;
			}
			for (Material::ExtraDatas::iterator it = _material.extraDatas.begin(); it != _material.extraDatas.end(); ++it) {
				const std::string &name = it->first;
				const Uniform &uniform = it->second;
				Material::ExtraUniforms::iterator uit = _material.uniformExtraDatas.find(name);
				if (uit == _material.uniformExtraDatas.end())
					continue;

				GLint handle = uit->second;
				switch (uniform.type) {
				case Uniform::NUMBER:
					glUniform1fv(handle, 1, (GLfloat*)&uniform.data.number);

					break;
				case Uniform::VEC2:
					glUniform2fv(handle, 1, (GLfloat*)&uniform.data.vec2);

					break;
				case Uniform::VEC3:
					glUniform3fv(handle, 1, (GLfloat*)&uniform.data.vec3);

					break;
				case Uniform::VEC4:
					glUniform4fv(handle, 1, (GLfloat*)&uniform.data.vec4);

					break;
				case Uniform::SAMPLER2D:
					glUniform1i(handle, textureIndex); ++textureIndex;

					break;
				}
			}
#if defined GL_VERSION_3_3
			if (_glVersion >= 330)
				glBindSampler(0, 0);
#endif /* GL_VERSION_3_3 */

			// Bind rendering data.
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

			// Draw elements.
			const Vert vertexes[] = {
				Vert(Math::Vec2<GLfloat>(0, 0),                            Math::Vec2<GLfloat>(0, 0), 0xffffffff),
				Vert(Math::Vec2<GLfloat>((GLfloat)width, 0),               Math::Vec2<GLfloat>(1, 0), 0xffffffff),
				Vert(Math::Vec2<GLfloat>(0, (GLfloat)height),              Math::Vec2<GLfloat>(0, 1), 0xffffffff),
				Vert(Math::Vec2<GLfloat>((GLfloat)width, (GLfloat)height), Math::Vec2<GLfloat>(1, 1), 0xffffffff)
			};
			const GLushort indices[] = {
				0, 2, 3,
				0, 3, 1
			};
			glBindBuffer(GL_ARRAY_BUFFER, _material.vbo);
			glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)BITTY_COUNTOF(vertexes) * sizeof(Vert), (const GLvoid*)vertexes, GL_STREAM_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _material.elements);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)BITTY_COUNTOF(indices) * sizeof(GLushort), (const GLvoid*)indices, GL_STREAM_DRAW);
			textureIndex = 0;
			glActiveTexture(GL_TEXTURE0 + textureIndex); ++textureIndex;
			glBindTexture(GL_TEXTURE_2D, _material.texture);
			for (int i = 0; i < (int)_material.extraTextures.size(); ++i) {
				glActiveTexture(GL_TEXTURE0 + textureIndex); ++textureIndex;
				glBindTexture(GL_TEXTURE_2D, _material.extraTextures[i]);
			}
			for (Material::ExtraDatas::iterator it = _material.extraDatas.begin(); it != _material.extraDatas.end(); ++it) {
				const std::string &name = it->first;
				const Uniform &uniform = it->second;
				Material::ExtraUniforms::iterator uit = _material.uniformExtraDatas.find(name);
				if (uit == _material.uniformExtraDatas.end())
					continue;

				switch (uniform.type) {
				case Uniform::SAMPLER2D:
					glActiveTexture(GL_TEXTURE0 + textureIndex); ++textureIndex;
					glBindTexture(GL_TEXTURE_2D, uniform.data.sampler2d);

					break;
				default:
					break;
				}
			}
#if defined GL_VERSION_3_2
			if (_glVersion >= 320)
				glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)6, GL_UNSIGNED_SHORT, 0, 0);
			else
				glDrawElements(GL_TRIANGLES, (GLsizei)6, GL_UNSIGNED_SHORT, 0);
#else /* GL_VERSION_3_2 */
			glDrawElements(GL_TRIANGLES, (GLsizei)6, GL_UNSIGNED_SHORT, 0);
#endif /* GL_VERSION_3_2 */

			// Unbind rendering data.
			glDeleteVertexArrays(1, &vao);
		}
		// Restore render states.
		glUseProgram(lastProgram);
		glBindTexture(GL_TEXTURE_2D, lastTexture);
#if defined GL_VERSION_3_3
		if (_glVersion >= 330) glBindSampler(0, lastSampler);
#endif /* GL_VERSION_3_3 */
		glActiveTexture(lastActiveTexture);
		glBindVertexArray(lastVertexArrayObject);
		glBindBuffer(GL_ARRAY_BUFFER, lastArrayBuffer);
		glBlendEquationSeparate(lastBlendEquationRgb, lastBlendEquationAlpha);
		glBlendFuncSeparate(lastBlendSrcRgb, lastBlendDstRgb, lastBlendSrcAlpha, lastBlendDstAlpha);
		if (lastEnableBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
		if (lastEnableCullFace) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
		if (lastEnableDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
		if (lastEnableStencilTest) glEnable(GL_STENCIL_TEST); else glDisable(GL_STENCIL_TEST);
		if (lastEnableScissorTest) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
#if defined GL_VERSION_3_1
		if (_glVersion >= 310) { if (lastEnablePrimitiveRestart) glEnable(GL_PRIMITIVE_RESTART); else glDisable(GL_PRIMITIVE_RESTART); }
#endif /* GL_VERSION_3_1 */
#if defined GL_POLYGON_MODE
		glPolygonMode(GL_FRONT_AND_BACK, (GLenum)lastPolygonMode[0]);
#endif /* GL_POLYGON_MODE */
		glViewport(lastViewport[0], lastViewport[1], (GLsizei)lastViewport[2], (GLsizei)lastViewport[3]);
		glScissor(lastScissorBox[0], lastScissorBox[1], (GLsizei)lastScissorBox[2], (GLsizei)lastScissorBox[3]);

		// Finish.
		SDL_GL_SwapWindow(window);

		rnd->target(nullptr);

		if (ws->effectCustomized()) {
			const std::string &material = ws->effectConfig();
			use(ws, material.empty() ? nullptr : material.c_str());
			ws->effectCustomized(false);
			ws->effectConfig().clear();
		}
	}

	virtual void renderTargetsReset(void) override {
		if (_material.texture) {
			glDeleteTextures(1, &_material.texture);
			_material.texture = 0;
		}
		glGenTextures(1, &_material.texture);

		if (_texture) {
			Texture::destroy(_texture);
			_texture = nullptr;
		}
	}

private:
	void useDefault(Workspace* ws) {
#if defined BITTY_OS_WIN
#	define _GLSL_VERSION "130"
#elif defined BITTY_OS_MAC
#	define _GLSL_VERSION "150"
#elif defined BITTY_OS_LINUX
#	define _GLSL_VERSION "130"
#endif /* Platform macro. */

		const GLchar* vertSrc =
			"#version " _GLSL_VERSION "\n"
			"uniform mat4 ProjMatrix;\n"
			"in vec2 Position;\n"
			"in vec2 UV;\n"
			"in vec4 Color;\n"
			"out vec2 Frag_UV;\n"
			"out vec4 Frag_Color;\n"
			"void main()\n"
			"{\n"
			"	Frag_UV = UV;\n"
			"	Frag_Color = Color;\n"
			"	gl_Position = ProjMatrix * vec4(Position.xy, 0, 1);\n"
			"}\n";
		const GLchar* fragSrc =
			"#version " _GLSL_VERSION "\n"
			"uniform sampler2D Texture;\n"
			"in vec2 Frag_UV;\n"
			"in vec4 Frag_Color;\n"
			"out vec4 Out_Color;\n"
			"void main()\n"
			"{\n"
			"	Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
			"}\n";
		_material.close();
		_material.open(ws, vertSrc, fragSrc, nullptr, nullptr);

#undef _GLSL_VERSION
	}
};
#else /* BITTY_EFFECTS_ENABLED && Platform macro. */
class EffectsImpl : public Effects {
public:
	EffectsImpl() {
	}
	virtual ~EffectsImpl() {
	}

	virtual bool open(class Window*, class Renderer*, class Workspace*, bool) override {
		return false;
	}
	virtual bool close(void) override {
		return false;
	}

	virtual bool valid(void) const override {
		return false;
	}
	virtual bool use(class Workspace*, const char*) override {
		return false;
	}

	virtual bool inject(const char*, float) override {
		return false;
	}
	virtual bool inject(const char*, const Math::Vec2f &) override {
		return false;
	}
	virtual bool inject(const char*, const Math::Vec3f &) override {
		return false;
	}
	virtual bool inject(const char*, const Math::Vec4f &) override {
		return false;
	}
	virtual bool inject(const char*, const Resources::Texture::Ptr &) override {
		return false;
	}
	virtual bool inject(const char*, const Image::Ptr &) override {
		return false;
	}

	virtual void prepare(class Window*, class Renderer* rnd, class Workspace*, double) override {
		rnd->target(nullptr);
	}
	virtual void finish(class Window*, class Renderer* rnd, class Workspace*) override {
		rnd->flush();
	}

	virtual void renderTargetsReset(void) override {
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
