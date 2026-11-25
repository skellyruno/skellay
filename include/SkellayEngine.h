#pragma once
#include <memory>
#include <vector>
#include <cstdint>
namespace skellay {
enum class Model { classic=2, modern=3, bank=4 };
struct Configuration { Model model; std::vector<std::uint8_t> bytes; };
// Preparation is non-real-time. Frozen processing accepts planar stereo buffers.
// No parameter or tempo change is allowed through this API during processing.
class Engine {
public:
 virtual ~Engine()=default;
 virtual bool process(float* left,float* right,int samples) noexcept=0;
};
Configuration defaults(Model);
bool validate(const Configuration&);
std::unique_ptr<Engine> prepare(const Configuration&, int rate, double bpm, int block);
}
