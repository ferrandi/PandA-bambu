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

void reg_binding::add_to_SM(structural_objectRef clock_port, structural_objectRef selector_register_file)
{
   const structural_managerRef& SM = HLS->datapath;

   const structural_objectRef& circuit = SM->get_circ();

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug, "reg_binding::add_registers - Start");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug, "Number of registers: " +
                                                         boost::lexical_cast<std::string>(get_used_regs()));
   compute_is_without_enable();
   /// define boolean type for command signals
   for (unsigned int i = 0; i < get_used_regs(); i++)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug, "Allocating register number: " + boost::lexical_cast<std::string>(i));
      generic_objRef regis = get(i);
      std::string name = regis->get_string();
      std::string register_type_name=CalculateRegisterName(i);
      std::string library = HLS->HLS_T->get_technology_manager()->get_library(register_type_name);
      structural_objectRef reg_mod = SM->add_module_from_technology_library(name, register_type_name, library, circuit, HLS->HLS_T->get_technology_manager());
      this->specialise_reg(reg_mod, i);
      structural_objectRef port_ck = reg_mod->find_member(CLOCK_PORT_NAME, port_o_K, reg_mod);
      SM->add_connection(clock_port, port_ck);
      structural_objectRef port_selector = reg_mod->find_member(SELECTOR_PORT_NAME, port_o_K, reg_mod);
      SM->add_connection(selector_register_file_port, port_selector);
      regis->set_structural_obj(reg_mod);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug, "Register " + boost::lexical_cast<std::string>(i) + " successfully allocated");
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug, "reg_binding::add_registers - End");
   if (HLS->output_level >= OUTPUT_LEVEL_MINIMUM)
   {
      unsigned int number_ff = 0;
      for (unsigned int r = 0; r < get_used_regs(); r++)
      {
         number_ff += get_bitsize(r);
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, HLS->output_level, "---Total number of flip-flop registers: " + STR(number_ff));
   }
}
