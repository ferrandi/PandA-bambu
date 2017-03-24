#include "reg_binding_cs.hpp"
///HLS/function_allocation include
#include "omp_functions.hpp"

reg_binding_cs::reg_binding_cs(const hlsRef& HLS_, const HLS_managerRef HLSMgr_) :
    reg_binding(HLS_, HLSMgr)
{
}

reg_binding_cs::~reg_binding_cs()
{
}

std::string reg_binding_cs::CalculateRegisterName()
{
    std::string register_type_name;
    auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
    bool found=false;
    if(omp_functions->kernel_functions.find(HLS->functionId) != omp_functions->kernel_functions.end()) found=true;
    if(omp_functions->parallelized_functions.find(HLS->functionId) != omp_functions->kernel_functions.end()) found=true;
    if(omp_functions->atomic_functions.find(HLS->functionId) != omp_functions->kernel_functions.end()) found=true;
    if(found) register_type_name = rams_dist;
    else if(is_without_enable.find(i) != is_without_enable.end())
       register_type_name = register_STD;
    else if(synch_reset == "no")
       register_type_name = register_SE;
    else if(synch_reset == "sync")
          register_type_name = register_SRSE;
    else
       register_type_name = register_SARSE;
}
