func @tensor_float() attributes { iree.module.export } {
  %0 = iree.unfoldable_constant dense<[[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]]> : tensor<2x3xf32>
  %1 = iree.unfoldable_constant dense<[[7.0], [8.0], [9.0]]> : tensor<3x1xf32>
  %result = "tosa.matmul"(%0, %1) : (tensor<2x3xf32>, tensor<3x1xf32>) -> tensor<2x1xf32>
  check.expect_eq_const(%result, dense<[[50.0], [122.0]]> : tensor<2x1xf32>) : tensor<2x1xf32>
  return
}

func @tensor_int() attributes { iree.module.export } {
  %0 = iree.unfoldable_constant dense<[[1, 2, 3], [4, 5, 6]]> : tensor<2x3xi32>
  %1 = iree.unfoldable_constant dense<[[7], [8], [9]]> : tensor<3x1xi32>
  %result = "tosa.matmul"(%0, %1) : (tensor<2x3xi32>, tensor<3x1xi32>) -> tensor<2x1xi32>
  check.expect_eq_const(%result, dense<[[50], [122]]> : tensor<2x1xi32>) : tensor<2x1xi32>
  return
}
