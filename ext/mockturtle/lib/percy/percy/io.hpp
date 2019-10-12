#pragma once

namespace percy
{

    /***************************************************************************
        Converts a DAG to the folling string format:
        "n,r,j_1,k_1,j_2,k_2,...,j_r,k_r", where
        n is the number of input variables
        r is the number of internal nodes (or steps)
        the r pairs (j_i, k_i) represent the internal nodes and their incoming
        arcs.
        The resulting string is written to the supplied character buffer and
        will be null terminated.
        NOTE: the function assumes that enough memory has been allocated in
        the buffer.
    ***************************************************************************/
    template<class Dag=dag<2>>
    void dag_to_string(const Dag& g, char* buf)
    {
        buf[0] = char(g.nr_vars() + '0');
        buf[1] = ',';
        buf[2] = char(g.nr_vertices() + '0');
        buf[3] = ',';
        int i = 0;
        for (i = 0; i < g.nr_vertices(); i++) {
            const auto& vertex = g.get_vertex(i);
            buf[i*4+4] = char(vertex.first + '0');
            buf[i*4+5] = ',';
            buf[i*4+6] = char(vertex.second + '0');
            if (i != g.nr_vertices() - 1) {
                buf[i*4+7] = ',';
            }
        }
        buf[i*4+3] = '\0';
    }


    /***************************************************************************
        Parses a DAG string into a DAG object.
    ***************************************************************************/
    template<class Dag=dag<2>>
    void dag_read_string(Dag&, char*)
    {
    }

    
}

