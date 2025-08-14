#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <ucra/ucra.h>
#include <stdexcept>
#include <memory>

namespace py = pybind11;

// Helper function for result checking (defined in engine_wrapper.cpp)
extern void check_ucra_result(UCRA_Result result, const std::string& operation);

// Python wrapper for UCRA_F0Curve with NumPy integration
class PyF0Curve {
private:
    UCRA_F0Curve curve_;
    std::vector<float> time_storage_;
    std::vector<float> f0_storage_;

public:
    PyF0Curve(py::array_t<float> time_sec, py::array_t<float> f0_hz) {
        // Validate input arrays
        if (time_sec.ndim() != 1 || f0_hz.ndim() != 1) {
            throw std::invalid_argument("Arrays must be 1-dimensional");
        }
        if (time_sec.size() != f0_hz.size()) {
            throw std::invalid_argument("Time and F0 arrays must have the same length");
        }
        if (time_sec.size() == 0) {
            throw std::invalid_argument("Arrays cannot be empty");
        }

        // Get direct access to data
        auto time_buf = time_sec.request();
        auto f0_buf = f0_hz.request();

        // Copy data to internal storage
        time_storage_.resize(time_sec.size());
        f0_storage_.resize(f0_hz.size());

        std::memcpy(time_storage_.data(), time_buf.ptr, time_sec.size() * sizeof(float));
        std::memcpy(f0_storage_.data(), f0_buf.ptr, f0_hz.size() * sizeof(float));

        // Set up UCRA_F0Curve structure
        curve_.time_sec = time_storage_.data();
        curve_.f0_hz = f0_storage_.data();
        curve_.length = static_cast<uint32_t>(time_sec.size());
    }

    // Delete copy constructor and assignment operator
    PyF0Curve(const PyF0Curve&) = delete;
    PyF0Curve& operator=(const PyF0Curve&) = delete;

    uint32_t get_length() const {
        return curve_.length;
    }

    py::array_t<float> get_time_sec() const {
        return py::array_t<float>(
            curve_.length,
            curve_.time_sec
        );
    }

    py::array_t<float> get_f0_hz() const {
        return py::array_t<float>(
            curve_.length,
            curve_.f0_hz
        );
    }

    // Internal access to raw pointer
    const UCRA_F0Curve* get_raw() const { return &curve_; }
};

// Python wrapper for UCRA_EnvCurve with NumPy integration
class PyEnvCurve {
private:
    UCRA_EnvCurve curve_;
    std::vector<float> time_storage_;
    std::vector<float> value_storage_;

public:
    PyEnvCurve(py::array_t<float> time_sec, py::array_t<float> value) {
        // Validate input arrays
        if (time_sec.ndim() != 1 || value.ndim() != 1) {
            throw std::invalid_argument("Arrays must be 1-dimensional");
        }
        if (time_sec.size() != value.size()) {
            throw std::invalid_argument("Time and value arrays must have the same length");
        }
        if (time_sec.size() == 0) {
            throw std::invalid_argument("Arrays cannot be empty");
        }

        // Get direct access to data
        auto time_buf = time_sec.request();
        auto value_buf = value.request();

        // Copy data to internal storage
        time_storage_.resize(time_sec.size());
        value_storage_.resize(value.size());

        std::memcpy(time_storage_.data(), time_buf.ptr, time_sec.size() * sizeof(float));
        std::memcpy(value_storage_.data(), value_buf.ptr, value.size() * sizeof(float));

        // Set up UCRA_EnvCurve structure
        curve_.time_sec = time_storage_.data();
        curve_.value = value_storage_.data();
        curve_.length = static_cast<uint32_t>(time_sec.size());
    }

    // Delete copy constructor and assignment operator
    PyEnvCurve(const PyEnvCurve&) = delete;
    PyEnvCurve& operator=(const PyEnvCurve&) = delete;

    uint32_t get_length() const {
        return curve_.length;
    }

    py::array_t<float> get_time_sec() const {
        return py::array_t<float>(
            curve_.length,
            curve_.time_sec
        );
    }

    py::array_t<float> get_value() const {
        return py::array_t<float>(
            curve_.length,
            curve_.value
        );
    }

    // Internal access to raw pointer
    const UCRA_EnvCurve* get_raw() const { return &curve_; }
};

void bind_curves(py::module& m) {
    py::class_<PyF0Curve>(m, "F0Curve")
        .def(py::init<py::array_t<float>, py::array_t<float>>(),
             py::arg("time_sec"), py::arg("f0_hz"),
             "Create an F0 curve from NumPy arrays")
        .def_property_readonly("length", &PyF0Curve::get_length, "Number of points in the curve")
        .def_property_readonly("time_sec", &PyF0Curve::get_time_sec, "Time points as NumPy array")
        .def_property_readonly("f0_hz", &PyF0Curve::get_f0_hz, "F0 values as NumPy array");

    py::class_<PyEnvCurve>(m, "EnvCurve")
        .def(py::init<py::array_t<float>, py::array_t<float>>(),
             py::arg("time_sec"), py::arg("value"),
             "Create an envelope curve from NumPy arrays")
        .def_property_readonly("length", &PyEnvCurve::get_length, "Number of points in the curve")
        .def_property_readonly("time_sec", &PyEnvCurve::get_time_sec, "Time points as NumPy array")
        .def_property_readonly("value", &PyEnvCurve::get_value, "Value points as NumPy array");
}
