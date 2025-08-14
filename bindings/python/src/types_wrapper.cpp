#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <ucra/ucra.h>

namespace py = pybind11;

void bind_types(py::module& m) {
    // UCRA error codes as Python enum
    py::enum_<UCRA_Result>(m, "Result")
        .value("SUCCESS", UCRA_SUCCESS)
        .value("ERR_INVALID_ARGUMENT", UCRA_ERR_INVALID_ARGUMENT)
        .value("ERR_OUT_OF_MEMORY", UCRA_ERR_OUT_OF_MEMORY)
        .value("ERR_NOT_SUPPORTED", UCRA_ERR_NOT_SUPPORTED)
        .value("ERR_INTERNAL", UCRA_ERR_INTERNAL)
        .value("ERR_FILE_NOT_FOUND", UCRA_ERR_FILE_NOT_FOUND)
        .value("ERR_INVALID_JSON", UCRA_ERR_INVALID_JSON)
        .value("ERR_INVALID_MANIFEST", UCRA_ERR_INVALID_MANIFEST)
        .export_values();

    // Custom exception for UCRA errors
    py::register_exception<std::runtime_error>(m, "UcraError");

    // Constants
    m.attr("DEFAULT_SAMPLE_RATE") = 44100;
    m.attr("DEFAULT_CHANNELS") = 1;
    m.attr("DEFAULT_BLOCK_SIZE") = 512;
}
