//
// Created by Marco Siracusa on 3/26/20.
//

#ifndef SCALAR_REPLACEMENT_OF_AGGREGATES_FPGA_CALLBACKS_HPP
#define SCALAR_REPLACEMENT_OF_AGGREGATES_FPGA_CALLBACKS_HPP

unsigned long MaxNumScalarTypes = 32;
unsigned long MaxTypeByteSize = 128;

unsigned long long get_first_type_bitwidth(llvm::Type* ty)
{
   unsigned long long bitwidth = 0;
   if(ty->isIntegerTy())
   {
      bitwidth = ty->getIntegerBitWidth();
   }
   else if(ty->isFloatTy())
   {
      bitwidth = 32;
   }
   else if(ty->isDoubleTy())
   {
      bitwidth = 64;
   }

   return bitwidth;
}

unsigned long get_num_elements(llvm::Type* ty, unsigned long decayed_dim_if_any = 1)
{
   std::vector<llvm::Type*> contained_types;

   for(unsigned long long e_idx = 0; e_idx < decayed_dim_if_any; e_idx++)
   {
      contained_types.push_back(ty);
   }

   unsigned long long non_aggregate_types = 0;

   for(auto c_idx = 0u; c_idx < contained_types.size(); c_idx++)
   {
      llvm::Type* el_ty = contained_types.at(c_idx);

      if(el_ty->isAggregateType())
      {
         if(el_ty->isStructTy())
         {
            for(unsigned int e_idx = 0; e_idx < el_ty->getStructNumElements(); ++e_idx)
            {
               contained_types.push_back(el_ty->getStructElementType(e_idx));
            }
         }
         else if(el_ty->isArrayTy())
         {
            for(unsigned long long e_idx = 0; e_idx < el_ty->getArrayNumElements(); ++e_idx)
            {
               contained_types.push_back(el_ty->getArrayElementType());
            }
         }
      }
      else
      {
         ++non_aggregate_types;
      }
   }

   return non_aggregate_types;
}

Expandability compute_alloca_expandability_profit(const llvm::AllocaInst* alloca_inst, const llvm::DataLayout& DL, std::string& msg)
{
   llvm::Type* allocated_type = alloca_inst->getAllocatedType();

   unsigned long num_elements = get_num_elements(allocated_type);
   unsigned long size = DL.getTypeAllocSize(allocated_type);
   bool expandable_size = num_elements <= MaxNumScalarTypes and size <= MaxTypeByteSize;

   if(!expandable_size)
   {
      msg = "# aggregate types is " + std::to_string(num_elements) + " (allowed " + std::to_string(MaxNumScalarTypes) + ") and allocates size is " + std::to_string(size) + "(allowed " + std::to_string(MaxTypeByteSize) + ")";
   }

   double area_revenue = (double)size * 8.0;
   double area_cost = (double)size * 8.0; // TODO what about here? It's always 0
   double area_profit = area_revenue - area_cost;

   double latency_revenue = 0.0;
   double latency_cost = 0.0;
   double latency_profit = latency_revenue - latency_cost;

   return Expandability(expandable_size, area_profit, latency_profit);
}

Expandability compute_global_expandability_profit(llvm::GlobalVariable* g_var, const llvm::DataLayout& DL, std::string& msg)
{
   llvm::Type* allocated_type = g_var->getValueType();

   unsigned long num_elements = get_num_elements(allocated_type);
   unsigned long size = DL.getTypeAllocSize(allocated_type);
   bool expandable_size = num_elements <= MaxNumScalarTypes and size <= MaxTypeByteSize;

   if(!expandable_size)
   {
      msg = "# aggregate types is " + std::to_string(num_elements) + " (allowed " + std::to_string(MaxNumScalarTypes) + ") and allocates size is " + std::to_string(size) + "(allowed " + std::to_string(MaxTypeByteSize) + ")";
   }

   double area_revenue = (double)size * 8.0;
   double area_cost = (double)size * 8.0; // TODO what about here? It's always 0
   double area_profit = area_revenue - area_cost;

   double latency_revenue = 0.0;
   double latency_cost = 0.0;
   double latency_profit = latency_revenue - latency_cost;
   return Expandability(expandable_size, area_profit, latency_profit);
}

Expandability compute_operand_expandability_profit(llvm::Use* op_use, const llvm::DataLayout& DL, unsigned long long decayed_dim, std::string& msg)
{
   llvm::Type* allocated_type = op_use->get()->getType()->getPointerElementType();

   unsigned long num_elements = get_num_elements(allocated_type) * decayed_dim;
   unsigned long size = static_cast<unsigned long>(DL.getTypeAllocSize(allocated_type)) * decayed_dim;
   bool expandable_size = num_elements <= MaxNumScalarTypes and size <= MaxTypeByteSize;

   if(!expandable_size)
   {
      msg = "# aggregate types is " + std::to_string(num_elements) + " (allowed " + std::to_string(MaxNumScalarTypes) + ") and allocates size is " + std::to_string(size) + "(allowed " + std::to_string(MaxTypeByteSize) + ")";
   }

   double area_revenue = (double)size * 8.0; // TODO sure about that?
   double area_cost = 0.0;
   double area_profit = area_revenue - area_cost;

   double latency_revenue = 0.0;
   double latency_cost = 0.0;
   double latency_profit = latency_revenue - latency_cost;

   return Expandability(expandable_size, area_profit, latency_profit);
}

Expandability compute_gepi_expandability_profit(llvm::GEPOperator* gep_op, std::string& msg)
{
   if(gep_op->hasAllConstantIndices())
   {
      double area_revenue = 3.0 * 32.0 * gep_op->getNumIndices();
      double area_cost = 0.0;
      double area_profit = area_revenue - area_cost;

      double latency_revenue = 0.0;
      double latency_cost = 0.0;
      double latency_profit = latency_revenue - latency_cost;

      return Expandability(true, area_profit, latency_profit);
   }
   else
   {
      double area_revenue = 0.0; // 3.0 * 32.0 * gep_op->getNumIndices();
      double area_cost = 128.0 * 3.0 * 32.0 * 32.0;
      double area_profit = area_revenue - area_cost;

      double latency_revenue = 0.0;
      double latency_cost = 0.0;
      double latency_profit = latency_revenue - latency_cost;

      return Expandability(true, area_profit, latency_profit);
   }
}

Expandability compute_function_versioning_cost(llvm::Function* function)
{
   unsigned long long area_cost = 0;
   unsigned long long latency_cost = 0;

   for(llvm::BasicBlock& bb : *function)
   {
      for(llvm::Instruction& i : bb)
      {
         if(i.isBinaryOp())
         {
            bool all_const_ops = true;

            for(llvm::Use& op : i.operands())
            {
               if(!llvm::isa<llvm::Constant>(op.get()))
               {
                  all_const_ops = false;
                  break;
               }
            }

            if(!all_const_ops)
            {
               unsigned long long inst_cost = get_first_type_bitwidth(i.getType());

               if(i.isShift())
               {
                  inst_cost *= 0;
               }
               else if(i.getOpcodeName() == std::string("add") or i.getOpcodeName() == std::string("fadd"))
               {
                  inst_cost *= 1;
               }
               else if(i.getOpcodeName() == std::string("sub") or i.getOpcodeName() == std::string("fsub"))
               {
                  inst_cost *= 1;
               }
               else if(i.getOpcodeName() == std::string("mul") or i.getOpcodeName() == std::string("fmul"))
               {
                  inst_cost *= 100;
               }
               else
               {
                  inst_cost *= 200;
               }

               area_cost += inst_cost;
            }
         }
      }
   }

   return Expandability(true, area_cost, latency_cost);
}

Expandability compute_load_expandability_profit(llvm::LoadInst* load_inst, std::string& msg)
{
   double area_revenue = 3.0 * get_first_type_bitwidth(load_inst->getType());
   double area_cost = 0.0;
   double area_profit = area_revenue - area_cost;

   double latency_revenue = 0.0; // 2.0 * get_first_type_bitwidth(load_inst->getType());
   double latency_cost = 0.0;
   double latency_profit = latency_revenue - latency_cost;

   return Expandability(true, area_profit, latency_profit);
}

Expandability compute_store_expandability_profit(llvm::StoreInst* store_inst, std::string& msg)
{
   double area_revenue = 2.0 * get_first_type_bitwidth(store_inst->getType());
   double area_cost = 0.0;
   double area_profit = area_revenue - area_cost;

   double latency_revenue = 0.0;
   double latency_cost = 0.0; // get_first_type_bitwidth(store_inst->getType());
   double latency_profit = latency_revenue - latency_cost;

   return Expandability(true, area_profit, latency_profit);
}

#endif // SCALAR_REPLACEMENT_OF_AGGREGATES_FPGA_CALLBACKS_HPP
