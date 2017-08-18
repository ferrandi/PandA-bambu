#include "DiscrepancyOpInfo.hpp"

bool operator != (const DiscrepancyOpInfo & a, const DiscrepancyOpInfo &b)
{
   return (a.stg_fun_id != b.stg_fun_id) || (a.op_id != b.op_id);
}

bool operator == (const DiscrepancyOpInfo & a, const DiscrepancyOpInfo &b)
{
   return !(a != b);
}

bool operator < (const DiscrepancyOpInfo & a, const DiscrepancyOpInfo &b)
{
   if (a.stg_fun_id == b.stg_fun_id)
      return a.op_id < b.op_id;
   else
      return a.stg_fun_id < b.stg_fun_id;
}

bool operator <= (const DiscrepancyOpInfo & a, const DiscrepancyOpInfo &b)
{
   return (a == b) || (a < b);
}

bool operator > (const DiscrepancyOpInfo & a, const DiscrepancyOpInfo &b)
{
   if (a.stg_fun_id == b.stg_fun_id)
      return a.op_id > b.op_id;
   else
      return a.stg_fun_id > b.stg_fun_id;
}

bool operator >= (const DiscrepancyOpInfo & a, const DiscrepancyOpInfo &b)
{
   return (a == b) || (a > b);
}
