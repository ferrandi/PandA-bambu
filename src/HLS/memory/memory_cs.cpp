#include "memory_cs.hpp"

memory_cs::memory_cs(const tree_managerRef TreeM, unsigned int off_base_address, unsigned int max_bram, bool null_pointer_check, bool initial_internal_address_p, unsigned int initial_internal_address, unsigned int &_address_bitsize) :
    memory(_TreeM, _off_base_address, max_bram, _null_pointer_check, initial_internal_address_p, initial_internal_address,&_address_bitsize)
{
    bus_tag_bitsize=0;
}

memory_cs::~memory_cs()
{

}
