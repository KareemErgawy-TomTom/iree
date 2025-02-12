// Copyright 2019 The IREE Authors
//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "iree/compiler/Dialect/VM/Conversion/ConversionTarget.h"

namespace mlir {
namespace iree_compiler {

// static
std::pair<mlir::ModuleOp, mlir::ModuleOp>
VMConversionTarget::nestModuleForConversion(mlir::ModuleOp outerModuleOp) {
  auto innerModuleOp = dyn_cast<ModuleOp>(outerModuleOp.getBody()->front());
  if (!innerModuleOp) {
    innerModuleOp =
        ModuleOp::create(outerModuleOp.getLoc(), outerModuleOp.getName());
    innerModuleOp.getBodyRegion().takeBody(outerModuleOp.getBodyRegion());
    outerModuleOp.getBodyRegion().getBlocks().push_back(new Block());
    outerModuleOp.push_back(innerModuleOp);
  }
  return std::make_pair(outerModuleOp, innerModuleOp);
}

VMConversionTarget::VMConversionTarget(MLIRContext *context)
    : ConversionTarget(*context) {
  addLegalDialect<IREE::VM::VMDialect>();

  // NOTE: we need to allow the outermost std.module to be legal to support the
  // double-nesting (module { vm.module { ... } }).
  addDynamicallyLegalOp<mlir::ModuleOp>(+[](mlir::ModuleOp op) {
    return !op->getParentOp() || !isa<ModuleOp>(op->getParentOp());
  });
}

}  // namespace iree_compiler
}  // namespace mlir
