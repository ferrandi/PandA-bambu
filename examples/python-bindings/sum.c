#include "Python.h"

#include "sum.h"

__attribute__((no_sanitize_address))
long sum(long a, long b)
{
	long sum = 0;
	Py_Initialize();
	PyObject *module_name = PyString_FromString("pysum");
	PyObject *module = PyImport_Import(module_name);
	PyObject *sum_function = PyObject_GetAttrString(module, "PySum");
	PyObject *sum_args = Py_BuildValue("ii", a, b);
	PyObject *result = PyObject_CallObject(sum_function, sum_args);
	sum = PyInt_AsLong(result);
	Py_Finalize();
	return sum;
}
