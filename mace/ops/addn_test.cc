//
// Copyright (c) 2017 XiaoMi All rights reserved.
//

#include "mace/core/operator.h"
#include "mace/ops/ops_test_util.h"

namespace mace {
namespace ops {
namespace test {

class AddnOpTest : public OpsTestBase {};

template <DeviceType D>
void SimpleAdd2() {
  // Construct graph
  OpsTestNet net;
  OpDefBuilder("AddN", "AddNTest")
      .Input("Input1")
      .Input("Input2")
      .Output("Output")
      .Finalize(net.NewOperatorDef());

  // Add input data
  net.AddInputFromArray<D, float>("Input1", {1, 2, 3, 1}, {1, 2, 3, 4, 5, 6});
  net.AddInputFromArray<D, float>("Input2", {1, 2, 3, 1}, {1, 2, 3, 4, 5, 6});

  // Run
  net.RunOp(D);

  auto expected = CreateTensor<float>({1, 2, 3, 1}, {2, 4, 6, 8, 10, 12});

  ExpectTensorNear<float>(*expected, *net.GetOutput("Output"), 1e-5);
}

TEST_F(AddnOpTest, CPUSimpleAdd2) { SimpleAdd2<DeviceType::CPU>(); }

template <DeviceType D>
void SimpleAdd3() {
  // Construct graph
  OpsTestNet net;
  OpDefBuilder("AddN", "AddNTest")
      .Input("Input1")
      .Input("Input2")
      .Input("Input3")
      .Output("Output")
      .Finalize(net.NewOperatorDef());

  // Add input data
  net.AddInputFromArray<D, float>("Input1", {1, 2, 3, 1}, {1, 2, 3, 4, 5, 6});
  net.AddInputFromArray<D, float>("Input2", {1, 2, 3, 1}, {1, 2, 3, 4, 5, 6});
  net.AddInputFromArray<D, float>("Input3", {1, 2, 3, 1}, {1, 2, 3, 4, 5, 6});

  // Run
  net.RunOp(D);

  auto expected = CreateTensor<float>({1, 2, 3, 1}, {3, 6, 9, 12, 15, 18});

  ExpectTensorNear<float>(*expected, *net.GetOutput("Output"), 1e-5);
}

TEST_F(AddnOpTest, CPUSimpleAdd3) { SimpleAdd3<DeviceType::CPU>(); }

template <DeviceType D>
void RandomTest() {
  testing::internal::LogToStderr();
  srand(time(NULL));

  for (int round = 0; round < 10; ++round) {
    // generate random input
    static unsigned int seed = 123;
    index_t n = 1 + (rand_r(&seed) % 5);
    index_t h = 1 + (rand_r(&seed) % 100);
    index_t w = 1 + (rand_r(&seed) % 100);
    index_t c = 1 + (rand_r(&seed) % 32);
    int input_num = 2 + rand_r(&seed) % 3;
    // Construct graph
    OpsTestNet net;
    auto op_def = OpDefBuilder("AddN", "AddNTest");
    for (int i = 0; i < input_num; ++i) {
      op_def.Input(MakeString("Input", i));
    }
    op_def.Output("Output").Finalize(net.NewOperatorDef());

    // Add input data
    for (int i = 0; i < input_num; ++i) {
      net.AddRandomInput<D, float>(MakeString("Input", i), {n, h, w, c});
    }

    // run on cpu
    net.RunOp();
    // Check
    Tensor expected;
    expected.Copy(*net.GetOutput("Output"));

    // run on gpu
    for (int i = 0; i < input_num; ++i) {
      BufferToImage<D, half>(net, MakeString("Input", i),
                             MakeString("InputImage", i),
                             kernels::BufferType::IN_OUT_CHANNEL);
    }

    auto op_def_cl = OpDefBuilder("AddN", "AddNTest");
    for (int i = 0; i < input_num; ++i) {
      op_def_cl.Input(MakeString("InputImage", i));
    }
    op_def_cl.Output("OutputImage")
        .AddIntArg("T", static_cast<int>(DataType::DT_HALF))
        .Finalize(net.NewOperatorDef());

    // Run on device
    net.RunOp(D);

    ImageToBuffer<D, float>(net, "OutputImage", "OPENCLOutput",
                            kernels::BufferType::IN_OUT_CHANNEL);

    ExpectTensorNear<float>(expected, *net.GetOutput("OPENCLOutput"), 0.1);
  }
}

TEST_F(AddnOpTest, OPENCLRandom) { RandomTest<DeviceType::OPENCL>(); }

}  // namespace test
}  // namespace ops
}  // namespace mace
