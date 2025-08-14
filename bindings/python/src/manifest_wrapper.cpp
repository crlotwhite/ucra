#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <ucra/ucra.h>
#include <stdexcept>

namespace py = pybind11;

// Helper function for result checking (defined in engine_wrapper.cpp)
extern void check_ucra_result(UCRA_Result result, const std::string& operation);

// Python wrapper for UCRA_Manifest
class PyManifest {
private:
    UCRA_Manifest* manifest_;

public:
    PyManifest(const std::string& manifest_path) {
        if (manifest_path.empty()) {
            throw std::invalid_argument("Manifest path cannot be empty");
        }

        UCRA_Result result = ucra_manifest_load(manifest_path.c_str(), &manifest_);
        check_ucra_result(result, "Manifest loading");
    }

    ~PyManifest() {
        if (manifest_) {
            ucra_manifest_free(manifest_);
        }
    }

    // Delete copy constructor and assignment operator
    PyManifest(const PyManifest&) = delete;
    PyManifest& operator=(const PyManifest&) = delete;

    std::string get_name() const {
        return std::string(manifest_->name ? manifest_->name : "");
    }

    std::string get_version() const {
        return std::string(manifest_->version ? manifest_->version : "");
    }

    std::string get_vendor() const {
        return std::string(manifest_->vendor ? manifest_->vendor : "");
    }

    std::string get_license() const {
        return std::string(manifest_->license ? manifest_->license : "");
    }

    // Internal access to raw pointer
    UCRA_Manifest* get_raw() const { return manifest_; }
};

void bind_manifest(py::module& m) {
    py::class_<PyManifest>(m, "Manifest")
        .def(py::init<const std::string&>(),
             py::arg("manifest_path"),
             "Load a manifest from file")
        .def_property_readonly("name", &PyManifest::get_name, "Manifest name")
        .def_property_readonly("version", &PyManifest::get_version, "Manifest version")
        .def_property_readonly("vendor", &PyManifest::get_vendor, "Manifest vendor")
        .def_property_readonly("license", &PyManifest::get_license, "Manifest license");
}
