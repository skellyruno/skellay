#pragma once
#include "SkellayEngine.h"
#include <cstddef>
#include <string>
#include <optional>
namespace skellay {
// Offline only. The FL container scanner recognizes the named wrapper layout
// observed in earlier research; unknown wrappers and versions are not guessed.
struct PresetEntry { Configuration state; int track, slot; std::size_t offset; };
struct ImportResult { std::vector<PresetEntry> entries; std::string error; unsigned rejected=0; bool legacyDelaySeen=false; };
ImportResult scanFlPreset(const std::vector<std::uint8_t>& data);
// Raw state records preserve exact bytes; distinct sizes identify the supported model.
std::optional<Configuration> importRaw(const std::vector<std::uint8_t>& data);
std::optional<Configuration> importRaw(Model, const std::vector<std::uint8_t>&);
}
