// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//===- Transforms.h - Transformations common to all backends --------------===//
//
// Defines transformations that are common to backends
//
//===----------------------------------------------------------------------===//
#ifndef IREE_COMPILER_CONVERSION_COMMON_TRANSFORMS_H_
#define IREE_COMPILER_CONVERSION_COMMON_TRANSFORMS_H_

#include "iree/compiler/Conversion/Common/LaunchConfig.h"
#include "mlir/Dialect/Linalg/Analysis/DependenceAnalysis.h"
#include "mlir/Dialect/Linalg/IR/LinalgOps.h"
#include "mlir/Dialect/Linalg/Transforms/Transforms.h"
#include "mlir/IR/Operation.h"

namespace mlir {
namespace iree_compiler {

/// Apply canonicalizations related to tiling to make promotion/vectorization
/// easier.
void applyCanonicalizationPatternsForTiling(MLIRContext *context,
                                            Operation *op);

/// Assuming that `funcOp` contains a single nested scf.for that represented the
/// tiled+fused+distributed loops with the distribution being across workgroups,
/// i.e.
///
/// scf.for ... {
///   ...
///   scf.for ... {
///     ...
///     linalg.
///     ...
///     linalg.
///     ...
///   }
/// }
///
/// Returns the list of linalg operations in the functions. If there are no
/// `scf.for` operations in the function return the linalg operations in the
/// body of the function if it has a single basic block. Return failure in all
/// other cases.
///
/// TODO(ravishankarm) This methods also adds the "workgroup" marker to all ops
/// within the loop. The marker is the way to tie into rest of the
/// codegen. Refactor the downstream passes and get rid of the markers once and
/// for all.
LogicalResult getLinalgOps(FuncOp funcOp,
                           SmallVectorImpl<linalg::LinalgOp> &linalgOps,
                           SmallVectorImpl<Operation *> &tiledLoops);

/// Specifies the number of workgroups to use for a particular entry point
/// function, by updating the `worgroup_count` region in the
/// `hal.executable.entry_point` op for this operation. The method takes a
/// callback function, which computes the workgroup count (x,y,z) given the
/// workload along (x,y,z).
using WorkgroupCountRegionBuilder = std::function<std::array<Value, 3>(
    OpBuilder &b, Location loc, std::array<Value, 3> workload)>;
LogicalResult defineWorkgroupCountRegion(
    OpBuilder &builder, FuncOp funcOp,
    WorkgroupCountRegionBuilder regionBuilder);

/// Using linalg on tensors for dispatch region creation does first-level of
/// tile (fuse and distribute) during dispatch region formation. At that point
/// the workload per workgroup is set to the dynamic value represented by
/// `flow.dispatch.workgroup.size` and is later lowered to
/// `hal.dispatch.workgroup.size`. This method is to materialize the static
/// information of the workload per workgroup determined based on target
/// architecture.  Note that the value of hal.dispatch.workgroup.size is now
/// different after this function is called and represents the actual value used
/// at runtime.
LogicalResult materializeStaticLaunchInformation(
    FuncOp funcOp, ArrayRef<int64_t> workloadPerWorkgroup);

struct TileAndFuseOptions {
  linalg::LinalgLoopDistributionOptions distributionOptions;
  linalg::AllocBufferCallbackFn allocationFn = nullptr;
};
/// Method to tile and fuse sequence of Linalg operations in `linalgOps`. Uses
/// the tile sizes for the first level of tiling specified in
/// `launchConfig`. Proceeds by
/// 1) Find the common loops around `linalgOps` that can be fused.
/// 2) Tile the fusable loops in the last operation in the sequence.
/// 3) Creates tiled version of the other ops within the inter-tile loops
///    generated in step 2.
/// 4) For all the tiled+fused ops, tile the unfused loops as specified by
///    launchconfig.
LogicalResult tileAndFuseLinalgBufferOps(
    FuncOp funcOp, ArrayRef<linalg::LinalgOp> linalgOps,
    const linalg::LinalgDependenceGraph &dependenceGraph,
    const LaunchConfig &launchConfig, const TileAndFuseOptions &options);

}  // namespace iree_compiler
}  // namespace mlir

#endif  // IREE_COMPILER_CONVERSION_COMMON_TRANSFORMS_H_