/* Copyright 2023 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/compiler/mlir/tfrt/transforms/ifrt/ifrt_backend_compiler.h"

#include <memory>
#include <string>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "mlir/IR/BuiltinOps.h"  // from @llvm-project
#include "mlir/IR/DialectRegistry.h"  // from @llvm-project
#include "mlir/IR/MLIRContext.h"  // from @llvm-project
#include "mlir/IR/OwningOpRef.h"  // from @llvm-project
#include "mlir/InitAllDialects.h"  // from @llvm-project
#include "mlir/Parser/Parser.h"  // from @llvm-project
#include "tensorflow/compiler/mlir/tensorflow/dialect_registration.h"
#include "tensorflow/core/platform/resource_loader.h"
#include "tensorflow/core/platform/test.h"
#include "tensorflow/core/tfrt/graph_executor/graph_execution_options.h"
#include "tensorflow/core/tfrt/ifrt/ifrt_model_context.h"
#include "tensorflow/core/tfrt/runtime/runtime.h"
#include "tensorflow/core/tfrt/saved_model/saved_model_testutil.h"
#include "tsl/lib/core/status_test_util.h"
#include "tfrt/host_context/resource_context.h"  // from @tf_runtime

namespace tensorflow {
namespace ifrt_serving {

TEST(IfrtBackendCompilerTest, Basic) {
  // Create test input module
  constexpr absl::string_view kDataDirectory =
      "tensorflow/compiler/mlir/tfrt/transforms/ifrt/testdata";
  std::string mlir_module_path = tensorflow::GetDataDependencyFilepath(
      absl::StrCat(kDataDirectory, "/ifrt_cluster.mlir"));

  mlir::DialectRegistry registry;
  mlir::registerAllDialects(registry);
  mlir::RegisterAllTensorFlowDialects(registry);

  mlir::MLIRContext context(registry);

  mlir::OwningOpRef<mlir::ModuleOp> mlir_module =
      mlir::parseSourceFile<mlir::ModuleOp>(mlir_module_path, &context);

  ASSERT_TRUE(mlir_module);
  ASSERT_TRUE(mlir_module.get() != nullptr);

  // Create contexts required for the compiler execution.
  IfrtModelContext model_context;

  std::unique_ptr<tensorflow::tfrt_stub::Runtime> runtime =
      tensorflow::tfrt_stub::DefaultTfrtRuntime(/*num_threads=*/1);
  tensorflow::tfrt_stub::GraphExecutionOptions graph_execution_options(
      runtime.get());
  tfrt::ResourceContext resource_context;
  tensorflow::tfrt_stub::ModelRuntimeContext runtime_context(
      &graph_execution_options, /*export_dir=*/"", &resource_context);

  runtime_context.resource_context().CreateResource<IfrtModelContext>(
      "IfrtModelContext", std::move(model_context));

  IfrtBackendCompiler compiler;
  TF_ASSERT_OK(compiler.CompileTensorflow(runtime_context, mlir_module.get()));
}

}  // namespace ifrt_serving
}  // namespace tensorflow
