assert((num_args == 3), "fused_multiply: num_args should be 3")
let arg0 = tvm_struct_get(args, 0, 12)
let arg0.code = arg_type_ids[0]
let arg1 = tvm_struct_get(args, 1, 12)
let arg1.code = arg_type_ids[1]
let arg2 = tvm_struct_get(args, 2, 12)
let arg2.code = arg_type_ids[2]
let placeholder = tvm_struct_get(arg0, 0, 1)
// attr [placeholder] storage_alignment = 64
let arg0.shape = tvm_struct_get(arg0, 0, 2)
let arg0.strides = tvm_struct_get(arg0, 0, 3)
let dev_type = tvm_struct_get(arg0, 0, 10)
let dev_id = tvm_struct_get(arg0, 0, 9)
let placeholder = tvm_struct_get(arg1, 0, 1)
// attr [placeholder] storage_alignment = 64
let arg1.shape = tvm_struct_get(arg1, 0, 2)
let arg1.strides = tvm_struct_get(arg1, 0, 3)
let T_multiply = tvm_struct_get(arg2, 0, 1)
// attr [T_multiply] storage_alignment = 64
let arg2.shape = tvm_struct_get(arg2, 0, 2)
let arg2.strides = tvm_struct_get(arg2, 0, 3)
assert(((((arg0.code == 3) || (arg0.code == 13)) || (arg0.code == 7)) || (arg0.code == 4)), "fused_multiply: Expect arg[0] to be pointer")
assert(((((arg1.code == 3) || (arg1.code == 13)) || (arg1.code == 7)) || (arg1.code == 4)), "fused_multiply: Expect arg[1] to be pointer")
assert(((((arg2.code == 3) || (arg2.code == 13)) || (arg2.code == 7)) || (arg2.code == 4)), "fused_multiply: Expect arg[2] to be pointer")
// attr ["default"] device_context_id = dev_id
// attr ["default"] device_context_type = dev_type
assert((2 == tvm_struct_get(arg0, 0, 4)), "arg0.ndim is expected to equal 2")
assert((((tvm_struct_get(arg0, 0, 5) == (uint8)2) && (tvm_struct_get(arg0, 0, 6) == (uint8)32)) && (tvm_struct_get(arg0, 0, 7) == (uint16)1)), "arg0.dtype is expected to be float32")
assert((1 == int32(arg0.shape[0])), "Argument arg0.shape[0] has an unsatisfied constraint")
assert((8 == int32(arg0.shape[1])), "Argument arg0.shape[1] has an unsatisfied constraint")
if (!tvm_handle_is_null(arg0.strides)) {
  assert(((1 == int32(arg0.strides[1])) && (8 == int32(arg0.strides[0]))), "arg0.strides: expected to be compact array")
  0
}
assert(((uint64)0 == tvm_struct_get(arg0, 0, 8)), "Argument arg0.byte_offset has an unsatisfied constraint")
assert((2 == tvm_struct_get(arg1, 0, 4)), "arg1.ndim is expected to equal 2")
assert((((tvm_struct_get(arg1, 0, 5) == (uint8)2) && (tvm_struct_get(arg1, 0, 6) == (uint8)32)) && (tvm_struct_get(arg1, 0, 7) == (uint16)1)), "arg1.dtype is expected to be float32")
assert((1 == int32(arg1.shape[0])), "Argument arg1.shape[0] has an unsatisfied constraint")
assert((8 == int32(arg1.shape[1])), "Argument arg1.shape[1] has an unsatisfied constraint")
if (!tvm_handle_is_null(arg1.strides)) {
  assert(((1 == int32(arg1.strides[1])) && (8 == int32(arg1.strides[0]))), "arg1.strides: expected to be compact array")
  0
}
assert(((uint64)0 == tvm_struct_get(arg1, 0, 8)), "Argument arg1.byte_offset has an unsatisfied constraint")
assert((dev_type == tvm_struct_get(arg1, 0, 10)), "Argument arg1.device_type has an unsatisfied constraint")
assert((dev_id == tvm_struct_get(arg1, 0, 9)), "Argument arg1.device_id has an unsatisfied constraint")
assert((2 == tvm_struct_get(arg2, 0, 4)), "arg2.ndim is expected to equal 2")
assert((((tvm_struct_get(arg2, 0, 5) == (uint8)2) && (tvm_struct_get(arg2, 0, 6) == (uint8)32)) && (tvm_struct_get(arg2, 0, 7) == (uint16)1)), "arg2.dtype is expected to be float32")
assert((1 == int32(arg2.shape[0])), "Argument arg2.shape[0] has an unsatisfied constraint")
assert((8 == int32(arg2.shape[1])), "Argument arg2.shape[1] has an unsatisfied constraint")
if (!tvm_handle_is_null(arg2.strides)) {
  assert(((1 == int32(arg2.strides[1])) && (8 == int32(arg2.strides[0]))), "arg2.strides: expected to be compact array")
  0
}
assert(((uint64)0 == tvm_struct_get(arg2, 0, 8)), "Argument arg2.byte_offset has an unsatisfied constraint")
assert((dev_type == tvm_struct_get(arg2, 0, 10)), "Argument arg2.device_type has an unsatisfied constraint")
assert((dev_id == tvm_struct_get(arg2, 0, 9)), "Argument arg2.device_id has an unsatisfied constraint")
if ((dev_type != 1)) {
  tvm_call_packed("__tvm_set_device", dev_type, dev_id)
}
// attr [0] compute_scope = "fused_multiply_compute_"
produce T_multiply {
  for (ax1, 0, 8) {
    T_multiply[ax1] = (placeholder[ax1]*placeholder[ax1])
  }
}
