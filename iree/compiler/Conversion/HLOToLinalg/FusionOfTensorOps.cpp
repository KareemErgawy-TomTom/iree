// Copyright 2020 The IREE Authors
//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

//===--- FusionOfTensorsOps.cpp - Pass to fuse operations on tensors-------===//
//
// Pass to fuse operations on tensors after conversion to Linalg. Uses the
// patterns from MLIR for fusion linalg operations on tensors, and a few
// patterns to fuse these with IREE specific operations.
//
//===----------------------------------------------------------------------===//

#include "iree/compiler/Conversion/PassDetail.h"
#include "iree/compiler/Conversion/Passes.h"
#include "iree/compiler/Dialect/HAL/IR/HALDialect.h"
#include "iree/compiler/Dialect/HAL/IR/HALOps.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Linalg/IR/LinalgOps.h"
#include "mlir/Dialect/Linalg/Transforms/Transforms.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"

static llvm::cl::opt<bool> clEnableFusionWithReductionOps(
    "iree-enable-fusion-with-reduction-ops",
    llvm::cl::desc("Allow fusing generic ops with reductions"),
    llvm::cl::init(false));

namespace mlir {
namespace iree_compiler {
namespace {

using linalg::LinalgOp;

/// Pass to fuse linalg on tensor operations as well as fusion of hal.interface*
/// operations with linalg.tensor_reshape operation.
struct FusionOfTensorOpsPass
    : public FusionOfTensorOpsBase<FusionOfTensorOpsPass> {
  void getDependentDialects(DialectRegistry &registry) const override {
    registry
        .insert<AffineDialect, IREE::HAL::HALDialect, linalg::LinalgDialect>();
  }

  void runOnOperation() override {
    OwningRewritePatternList fusionPatterns(&getContext());
    OwningRewritePatternList interfacePatterns(&getContext());
    Operation *op = getOperation();
    MLIRContext *context = op->getContext();

    // Only fuse operations where all uses of the producer are generic or
    // indexed generic operations. If an operation is used in a named op, it
    // will be computed anyway, so the consumers can just use that value.
    linalg::ControlElementwiseOpsFusionFn controlFn =
        [](const OpResult &producer, const OpOperand &consumer) {
          // TODO(GH-5611): Enable fusion with reduction consumer for all
          // targets. Currently vectorization doesn't handle generic ops with
          // reduction iterators we will disable for now to allow vectorizing
          // producer pointwise ops to avoid performance regressions on CPU.
          if (!clEnableFusionWithReductionOps) {
            auto consumerOp = consumer.getOwner();
            if (isa<linalg::GenericOp, linalg::IndexedGenericOp>(consumerOp) &&
                dyn_cast<LinalgOp>(consumerOp).getNumReductionLoops()) {
              return false;
            }
          }

          llvm::SmallDenseSet<Operation *, 4> numUsers;
          for (Operation *user : producer.getUsers()) {
            if (isa<linalg::GenericOp, linalg::IndexedGenericOp>(user))
              continue;
            numUsers.insert(user);
          }
          return numUsers.empty();
        };
    // Simple heuristic to decide if reshaope should be folded in the linalg.
    // If the source of the reshape is a linalg op fold to potentially allow the
    // two linalg ops to be fused. Otherwise leave it to avoid adding dimensions
    // to the consumer linalg op.
    linalg::ControlElementwiseOpsFusionFn foldReshapeBetweenLinalgFn =
        [](const OpResult &producer, const OpOperand &consumer) {
          auto collapseOp =
              producer.getDefiningOp<linalg::TensorCollapseShapeOp>();
          if (collapseOp)
            return collapseOp.src().getDefiningOp<LinalgOp>() != nullptr;
          auto expandOp = producer.getDefiningOp<linalg::TensorExpandShapeOp>();
          if (expandOp)
            return expandOp.src().getDefiningOp<LinalgOp>() != nullptr;
          return false;
        };
    linalg::populateElementwiseOpsFusionPatterns(
        fusionPatterns,
        linalg::LinalgElementwiseFusionOptions()
            .setControlFoldingReshapes(foldReshapeBetweenLinalgFn)
            .setControlElementwiseOpsFusionFn(controlFn));

    (void)applyPatternsAndFoldGreedily(op->getRegions(),
                                       std::move(fusionPatterns));

    OwningRewritePatternList reshapeCanonicalizations(&getContext());
    linalg::populateFoldUnitDimsReshapeOpsByLinearizationPatterns(
        reshapeCanonicalizations);
    linalg::TensorCollapseShapeOp::getCanonicalizationPatterns(
        reshapeCanonicalizations, context);
    linalg::TensorExpandShapeOp::getCanonicalizationPatterns(
        reshapeCanonicalizations, context);
    (void)applyPatternsAndFoldGreedily(op->getRegions(),
                                       std::move(reshapeCanonicalizations));

    // Push the remaining reshapes down the graphs.
    OwningRewritePatternList pushReshapePatterns(&getContext());
    linalg::populatePushReshapeOpsPatterns(pushReshapePatterns);
    linalg::TensorCollapseShapeOp::getCanonicalizationPatterns(
        pushReshapePatterns, context);
    linalg::TensorExpandShapeOp::getCanonicalizationPatterns(
        pushReshapePatterns, context);
    (void)applyPatternsAndFoldGreedily(op->getRegions(),
                                       std::move(pushReshapePatterns));
  }
};

}  // namespace

std::unique_ptr<Pass> createFusionOfTensorOpsPass() {
  return std::make_unique<FusionOfTensorOpsPass>();
}

}  // namespace iree_compiler
}  // namespace mlir
