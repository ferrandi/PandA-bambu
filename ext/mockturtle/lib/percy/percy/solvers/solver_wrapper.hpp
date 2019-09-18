#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
//#pragma GCC diagnostic ignored "-Wzero-length-array"
#include <abc/satSolver.h>
#pragma GCC diagnostic pop

#include <thread>

namespace percy
{
    enum synth_result
    {
        success,
        failure,
        timeout
    };

    class solver_wrapper
    {
    public:
        virtual ~solver_wrapper() ///< Virtual constructor allows for cleanup in derived class destructors
        {

        }
        virtual void restart() = 0;
        virtual void set_nr_vars(int nr_vars) = 0;
        virtual int  nr_vars() = 0;
        virtual int  nr_clauses() = 0;
        virtual int  nr_conflicts() = 0;
        virtual void add_var() = 0;
        virtual int  add_clause(pabc::lit* begin, pabc::lit* end) = 0;
        virtual int  var_value(int var) = 0;
        virtual synth_result solve(int conflict_limit = 0) = 0;
        virtual synth_result solve(pabc::lit* begin, pabc::lit* end, int conflict_limit = 0) = 0;
    };
	
}
