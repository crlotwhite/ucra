#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

// Forward declarations for binding functions
void bind_engine(pybind11::module& m);
void bind_curves(pybind11::module& m);
void bind_manifest(pybind11::module& m);
void bind_types(pybind11::module& m);

namespace py = pybind11;

PYBIND11_MODULE(ucra, m) {
    m.doc() = "UCRA Python bindings - Audio synthesis and rendering library";
    m.attr("__version__") = "1.0.0";

    // Define a simple ndarray subclass to carry audio metadata attributes
    // This enables tests to access attributes like sample_rate/frames/channels
    py::object np = py::module::import("numpy");
    py::object ndarray = np.attr("ndarray");
    py::object dict = py::dict();
    py::object bases = py::make_tuple(ndarray);
    py::object AudioArray = py::reinterpret_borrow<py::object>(PyObject_CallFunctionObjArgs((PyObject*)&PyType_Type,
                                                                                           py::str("AudioArray").ptr(),
                                                                                           bases.ptr(),
                                                                                           dict.ptr(),
                                                                                           nullptr));
    m.attr("AudioArray") = AudioArray;

    // Bind all components
    bind_types(m);
    bind_curves(m);
    bind_manifest(m);
    bind_engine(m);
}
