/// \file zebralpy.cpp
/// Create python bindings for zebral library

// Disable unused variable warning from pybind11...
#if _WIN32
#pragma warning(disable : 4189)
#endif

#include <pybind11/complex.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "camera.hpp"
#include "camera_frame.hpp"
#include "camera_info.hpp"
#include "camera_manager.hpp"
#include "camera_platform.hpp"

namespace py = pybind11;
using namespace zebral;

/// Define a module for using the zebral libs from python
/// {TODO} Lots of work to be done here, but may want to
///        wait until the zebral library is more mature.
///        This much allows camera/format selection and
///        basic frame capture.
PYBIND11_MODULE(zebralpy, m)
{
  m.doc() = "Zebral libraries (camera,serial,etc)";
  py::class_<FormatInfo>(m, "FormatInfo")
      .def(py::init<>())
      .def(py::init<int, int, float, const std::string &>())
      .def_readwrite("width", &FormatInfo::width)
      .def_readwrite("height", &FormatInfo::height)
      .def_readwrite("fps", &FormatInfo::fps)
      .def_readwrite("channels", &FormatInfo::channels)
      .def_readwrite("bytespppc", &FormatInfo::bytespppc)
      .def_readwrite("format", &FormatInfo::format)
      .def("__repr__", [](const FormatInfo &f) {
        std::stringstream ss;
        ss << f;
        return "<" + ss.str() + ">";
      });

  py::class_<CameraInfo>(m, "CameraInfo")
      .def(py::init<int, const std::string &, const std::string &, const std::string &,
                    const std::string &, uint16_t, uint16_t>())
      .def_readwrite("index", &CameraInfo::index)
      .def_readwrite("name", &CameraInfo::name)
      .def_readwrite("bus", &CameraInfo::bus)
      .def_readwrite("path", &CameraInfo::path)
      .def_readwrite("driver", &CameraInfo::driver)
      .def_readwrite("vid", &CameraInfo::vid)
      .def_readwrite("pid", &CameraInfo::pid)
      .def_readwrite("selected_format", &CameraInfo::selected_format)
      .def("__repr__", [](const CameraInfo &f) {
        std::stringstream ss;
        ss << f;
        return "<" + ss.str() + ">";
      });

  py::class_<CameraFrame>(m, "CameraFrame")
      .def(py::init<>())
      .def(py::init<int, int, int, int, bool, bool, const uint8_t *>())
      .def("clear", &CameraFrame::clear)
      .def("reset", &CameraFrame::reset)
      .def("empty", &CameraFrame::empty)
      .def("width", &CameraFrame::width)
      .def("height", &CameraFrame::height)
      .def("channels", &CameraFrame::channels)
      .def("bytes_per_channel", &CameraFrame::bytes_per_channel)
      .def("is_signed", &CameraFrame::is_signed)
      .def("is_floating", &CameraFrame::is_floating)
      .def("data_size", &CameraFrame::data_size)
      .def("data", [](CameraFrame &f) {
        return pybind11::array_t<unsigned char>({f.data_size()}, {1}, f.data());
      });

  py::class_<CameraManager>(m, "CameraManager")
      .def(py::init<>())
      .def("Enumerate", &CameraManager::Enumerate)
      .def("Create", &CameraManager::Create);

  py::class_<CameraPlatform, std::shared_ptr<CameraPlatform>> camera(m, "Camera");
  camera.def(py::init<const CameraInfo &>())
      .def("Start", &CameraPlatform::Start)
      .def("Stop", &CameraPlatform::Stop)
      .def("GetNewFrame", &CameraPlatform::GetNewFrame)
      .def("GetLastFrame", &CameraPlatform::GetLastFrame)
      .def("IsRunning", &CameraPlatform::IsRunning)
      .def("GetCameraInfo", &CameraPlatform::GetCameraInfo)
      .def("GetAllModes", &CameraPlatform::GetAllModes)
      .def("SetFormat", &CameraPlatform::SetFormat)
      .def("GetFormat", &CameraPlatform::GetFormat);

  py::enum_<Camera::DecodeType>(camera, "DecodeType")
      .value("INTERNAL", Camera::DecodeType::INTERNAL)
      .value("SYSTEM", Camera::DecodeType::SYSTEM)
      .value("NONE", Camera::DecodeType::NONE)
      .export_values();
}
