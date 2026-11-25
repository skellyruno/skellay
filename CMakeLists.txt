cmake_minimum_required(VERSION 3.22)
project(Skellay VERSION 0.1.0 LANGUAGES CXX)
# Fail closed: the supplied DSP is external, not a generic substitute.
set(SKELLAY_PRIVATE_SOURCE_DIR "" CACHE PATH "Owner-controlled native source checkout")
if(NOT EXISTS "${SKELLAY_PRIVATE_SOURCE_DIR}/CMakeLists.txt")
  message(FATAL_ERROR "Set SKELLAY_PRIVATE_SOURCE_DIR to the private native package. No substitute DSP is supplied.")
endif()
enable_testing()
add_subdirectory("${SKELLAY_PRIVATE_SOURCE_DIR}" private-native)
# Future JUCE wrapper will use the ecosystem pin, 8.0.12.
# No plugin binary target until safe live parameter/tempo updates are implemented.
