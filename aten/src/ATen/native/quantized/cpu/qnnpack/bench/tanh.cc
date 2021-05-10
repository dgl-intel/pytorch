/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <algorithm>
#include <cmath>
#include <functional>
#include <random>
#include <vector>

#include <pytorch_qnnpack.h>

#include <benchmark/benchmark.h>

static void tanh_q8(benchmark::State& state) {
  const size_t batchSize = static_cast<size_t>(state.range(0));
  const size_t channels = static_cast<size_t>(state.range(1));

  std::random_device randomDevice;
  auto rng = std::mt19937(randomDevice());
  auto u8rng = std::bind(std::uniform_int_distribution<uint8_t>(), rng);

  std::vector<uint8_t> input(batchSize * channels);
  std::vector<uint8_t> output(batchSize * channels);
  std::generate(input.begin(), input.end(), std::ref(u8rng));
  std::fill(output.begin(), output.end(), 0xA5);

  pytorch_qnnp_status status = pytorch_qnnp_initialize();
  if (status != pytorch_qnnp_status_success) {
    state.SkipWithError("failed to initialize QNNPACK");
  }

  pytorch_qnnp_operator_t tanhOperator = nullptr;
  status = pytorch_qnnp_create_tanh_nc_q8(
      channels,
      127 /* input zero point */,
      1.0f /* input scale */,
      0 /* output zero point */,
      1.0f / 256.0f /* output scale */,
      0 /* output min */,
      255 /* output max */,
      0 /* flags */,
      &tanhOperator);
  if (status != pytorch_qnnp_status_success || tanhOperator == nullptr) {
    state.SkipWithError("failed to create TanH operator");
  }

  status = pytorch_qnnp_setup_tanh_nc_q8(
      tanhOperator,
      batchSize,
      input.data(),
      channels /* input:stride */,
      output.data(),
      channels /* output:stride */);
  if (status != pytorch_qnnp_status_success) {
    state.SkipWithError("failed to setup TanH operator");
  }

  for (auto _ : state) {
    status = pytorch_qnnp_run_operator(tanhOperator, nullptr /* thread pool */);
    if (status != pytorch_qnnp_status_success) {
      state.SkipWithError("failed to run TanH operator");
    }
  }

  const size_t itemsPerIteration = batchSize * channels;
  state.SetItemsProcessed(
      int64_t(state.iterations()) * int64_t(itemsPerIteration));

  const size_t bytesPerIteration = 2 * itemsPerIteration * sizeof(uint8_t);
  state.SetBytesProcessed(
      int64_t(state.iterations()) * int64_t(bytesPerIteration));

  status = pytorch_qnnp_delete_operator(tanhOperator);
  if (status != pytorch_qnnp_status_success) {
    state.SkipWithError("failed to delete TanH operator");
  }
}

static void CharacteristicArguments(benchmark::internal::Benchmark* b) {
  b->ArgNames({"N", "C"});

  int32_t c = 16;
  for (int32_t n = 224; n >= 7; n /= 2) {
    b->Args({n * n, c});
    c *= 2;
  }
}

BENCHMARK(tanh_q8)->Apply(CharacteristicArguments);

#ifndef PYTORCH_QNNPACK_BENCHMARK_NO_MAIN
BENCHMARK_MAIN();
#endif
