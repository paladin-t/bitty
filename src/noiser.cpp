/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "noiser.h"
#include "../lib/fast_noise/Cpp/FastNoiseLite.h"

/*
** {===========================================================================
** Noiser
*/

class NoiserImpl : public Noiser {
private:
	FastNoiseLite _generator;

public:
	NoiserImpl() {
		_generator.SetSeed(Math::rand());
	}
	virtual ~NoiserImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool option(const std::string &key, const Variant &val) override {
		if (key == "frequency") {
			if (val.isNumber()) {
				_generator.SetFrequency((float)(Variant::Real)val);

				return true;
			}

			return false;
		}
		if (key == "noise_type") {
			const std::string val_ = (std::string)val;
			if (val_ == "open_simplex2") {
				_generator.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

				return true;
			} else if (val_ == "open_simplex2s") {
				_generator.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);

				return true;
			} else if (val_ == "cellular") {
				_generator.SetNoiseType(FastNoiseLite::NoiseType_Cellular);

				return true;
			} else if (val_ == "perlin") {
				_generator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

				return true;
			} else if (val_ == "value_cubic") {
				_generator.SetNoiseType(FastNoiseLite::NoiseType_ValueCubic);

				return true;
			} else if (val_ == "value") {
				_generator.SetNoiseType(FastNoiseLite::NoiseType_Value);

				return true;
			}

			return false;
		}
		if (key == "rotation_type_3d") {
			const std::string val_ = (std::string)val;
			if (val_ == "none") {
				_generator.SetRotationType3D(FastNoiseLite::RotationType3D_None);

				return true;
			} else if (val_ == "improve_xy_planes") {
				_generator.SetRotationType3D(FastNoiseLite::RotationType3D_ImproveXYPlanes);

				return true;
			} else if (val_ == "improve_xz_planes") {
				_generator.SetRotationType3D(FastNoiseLite::RotationType3D_ImproveXZPlanes);

				return true;
			}

			return false;
		}
		if (key == "fractal_type") {
			const std::string val_ = (std::string)val;
			if (val_ == "none") {
				_generator.SetFractalType(FastNoiseLite::FractalType_None);

				return true;
			} else if (val_ == "fbm") {
				_generator.SetFractalType(FastNoiseLite::FractalType_FBm);

				return true;
			} else if (val_ == "ridged") {
				_generator.SetFractalType(FastNoiseLite::FractalType_Ridged);

				return true;
			} else if (val_ == "pingpong") {
				_generator.SetFractalType(FastNoiseLite::FractalType_PingPong);

				return true;
			} else if (val_ == "domain_warp_progressive") {
				_generator.SetFractalType(FastNoiseLite::FractalType_DomainWarpProgressive);

				return true;
			} else if (val_ == "domain_warp_independent") {
				_generator.SetFractalType(FastNoiseLite::FractalType_DomainWarpIndependent);

				return true;
			}

			return false;
		}
		if (key == "fractal_octaves") {
			if (val.isNumber()) {
				_generator.SetFractalOctaves((int)(Variant::Int)val);

				return true;
			}

			return false;
		}
		if (key == "fractal_lacunarity") {
			if (val.isNumber()) {
				_generator.SetFractalLacunarity((float)(Variant::Real)val);

				return true;
			}

			return false;
		}
		if (key == "fractal_gain") {
			if (val.isNumber()) {
				_generator.SetFractalGain((float)(Variant::Real)val);

				return true;
			}

			return false;
		}
		if (key == "fractal_weighted_strength") {
			if (val.isNumber()) {
				_generator.SetFractalWeightedStrength((float)(Variant::Real)val);

				return true;
			}

			return false;
		}
		if (key == "fractal_pingpong_strength") {
			if (val.isNumber()) {
				_generator.SetFractalPingPongStrength((float)(Variant::Real)val);

				return true;
			}

			return false;
		}
		if (key == "cellular_distance_function") {
			const std::string val_ = (std::string)val;
			if (val_ == "euclidean") {
				_generator.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);

				return true;
			} else if (val_ == "euclidean_sq") {
				_generator.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_EuclideanSq);

				return true;
			} else if (val_ == "manhattan") {
				_generator.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Manhattan);

				return true;
			} else if (val_ == "hybrid") {
				_generator.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Hybrid);

				return true;
			}

			return false;
		}
		if (key == "cellular_return_type") {
			const std::string val_ = (std::string)val;
			if (val_ == "cell_value") {
				_generator.SetCellularReturnType(FastNoiseLite::CellularReturnType_CellValue);

				return true;
			} else if (val_ == "distance") {
				_generator.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance);

				return true;
			} else if (val_ == "distance2") {
				_generator.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2);

				return true;
			} else if (val_ == "distance2_add") {
				_generator.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Add);

				return true;
			} else if (val_ == "distance2_sub") {
				_generator.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Sub);

				return true;
			} else if (val_ == "distance2_mul") {
				_generator.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Mul);

				return true;
			} else if (val_ == "distance2_div") {
				_generator.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Div);

				return true;
			}

			return false;
		}
		if (key == "cellular_jitter") {
			if (val.isNumber()) {
				_generator.SetCellularJitter((float)(Variant::Real)val);

				return true;
			}

			return false;
		}
		if (key == "domain_warp_type") {
			const std::string val_ = (std::string)val;
			if (val_ == "open_simplex2") {
				_generator.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);

				return true;
			} else if (val_ == "open_simplex2_reduced") {
				_generator.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2Reduced);

				return true;
			} else if (val_ == "basic_grid") {
				_generator.SetDomainWarpType(FastNoiseLite::DomainWarpType_BasicGrid);

				return true;
			}

			return false;
		}
		if (key == "domain_warp_amplitude") {
			if (val.isNumber()) {
				_generator.SetDomainWarpAmp((float)(Variant::Real)val);

				return true;
			}

			return false;
		}

		return false;
	}

	virtual void seed(int seed) override {
		_generator.SetSeed(seed);
	}

	virtual Real get(const Math::Vec2f &pos) override {
		return _generator.GetNoise(pos.x, pos.y);
	}
	virtual Real get(const Math::Vec3f &pos) override {
		return _generator.GetNoise(pos.x, pos.y, pos.z);
	}

	virtual void domainWarp(Math::Vec2f &pos) override {
		Real x = pos.x;
		Real y = pos.y;
		_generator.DomainWarp(x, y);
		pos.x = x;
		pos.y = y;
	}
	virtual void domainWarp(Math::Vec3f &pos) override {
		Real x = pos.x;
		Real y = pos.y;
		Real z = pos.z;
		_generator.DomainWarp(x, y, z);
		pos.x = x;
		pos.y = y;
		pos.z = z;
	}
};

Noiser* Noiser::create(void) {
	NoiserImpl* p = new NoiserImpl();

	return p;
}

void Noiser::destroy(Noiser* ptr) {
	NoiserImpl* impl = static_cast<NoiserImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
