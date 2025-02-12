func @sort1D() attributes { iree.module.export } {
  %input = iree.unfoldable_constant dense<[3, 2, 1, 4]> : tensor<4xi32>

  %sort = "mhlo.sort"(%input) ( {
  ^bb0(%arg1: tensor<i32>, %arg2: tensor<i32>):  // no predecessors
    %compare = "mhlo.compare"(%arg1, %arg2) {comparison_direction = "GT"} : (tensor<i32>, tensor<i32>) -> tensor<i1>
    "mhlo.return"(%compare) : (tensor<i1>) -> ()
  }) {dimension = 0 : i64, is_stable = false} : (tensor<4xi32>) -> tensor<4xi32>

  check.expect_eq_const(%sort, dense<[1, 2, 3, 4]> : tensor<4xi32>) : tensor<4xi32>
  return
}

func @sort2D() attributes { iree.module.export } {
  %input = iree.unfoldable_constant dense<[[1, 2, 3, 4],
                                           [4, 3, 2, 1]]> : tensor<2x4xi32>

  %sort = "mhlo.sort"(%input) ( {
  ^bb0(%arg1: tensor<i32>, %arg2: tensor<i32>):  // no predecessors
    %compare = "mhlo.compare"(%arg1, %arg2) {comparison_direction = "GT"} : (tensor<i32>, tensor<i32>) -> tensor<i1>
    "mhlo.return"(%compare) : (tensor<i1>) -> ()
  }) {dimension = 1 : i64, is_stable = false} : (tensor<2x4xi32>) -> tensor<2x4xi32>

  check.expect_eq_const(%sort, dense<[[1, 2, 3, 4], [1, 2, 3, 4]]> : tensor<2x4xi32>) : tensor<2x4xi32>
  return
}

func @sort3D() attributes { iree.module.export } {
  %input = iree.unfoldable_constant dense<[[[1, 2, 3, 4],
                                            [4, 3, 2, 1]]]> : tensor<1x2x4xi32>

  %sort = "mhlo.sort"(%input) ( {
  ^bb0(%arg1: tensor<i32>, %arg2: tensor<i32>):  // no predecessors
    %compare = "mhlo.compare"(%arg1, %arg2) {comparison_direction = "GT"} : (tensor<i32>, tensor<i32>) -> tensor<i1>
    "mhlo.return"(%compare) : (tensor<i1>) -> ()
  }) {dimension = 2 : i64, is_stable = false} : (tensor<1x2x4xi32>) -> tensor<1x2x4xi32>

  check.expect_eq_const(%sort, dense<[[[1, 2, 3, 4], [1, 2, 3, 4]]]> : tensor<1x2x4xi32>) : tensor<1x2x4xi32>
  return
}
