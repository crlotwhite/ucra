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

    // Bind all components
    bind_types(m);
    bind_curves(m);
    bind_manifest(m);
    bind_engine(m);
}
