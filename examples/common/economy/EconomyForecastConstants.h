#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace sc2
{

static constexpr size_t ForecastHorizonCountValue = 3U;
static constexpr size_t ShortForecastHorizonIndexValue = 0U;
static constexpr size_t MediumForecastHorizonIndexValue = 1U;
static constexpr size_t LongForecastHorizonIndexValue = 2U;
static constexpr std::array<uint64_t, ForecastHorizonCountValue> ForecastHorizonGameLoopsValue = {96U, 224U, 672U};

}  // namespace sc2
