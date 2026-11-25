#pragma once
#include <memory>
#include <vector>
#include <cstdint>
#include <array>
namespace skellay {
enum class Model { classic=2, modern=3, bank=4 };
struct Configuration { Model model; std::vector<std::uint8_t> bytes; };
struct LiveControls { Model model=Model::classic; std::array<int,133> values{}; };
int liveControlCount(Model) noexcept;
int liveControlMin(Model,int) noexcept;
int liveControlMax(Model,int) noexcept;
LiveControls liveControls(const Configuration&); // non-RT, validated input
bool validLiveControls(const LiveControls&) noexcept;
// Preparation is non-real-time. Tempo/control changes are audio-thread-only, between blocks.
class Engine {
public:
 virtual ~Engine()=default;
 // Audio-thread-only; engines requiring preparation ignore this.
 virtual void updateTempo(double) noexcept {}
 virtual bool updateControls(const LiveControls&) noexcept {return false;}
 virtual bool process(float* left,float* right,int samples) noexcept=0;
};
Configuration defaults(Model);
bool validate(const Configuration&);
std::unique_ptr<Engine> prepare(const Configuration&, int rate, double bpm, int block);
}
