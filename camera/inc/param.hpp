/// \file param.hpp
/// Ranged parameters for exposure, gain, etc.
#ifndef LIGHTBOX_CAMERA_PARAM_HPP_
#define LIGHTBOX_CAMERA_PARAM_HPP_

#include <limits>
#include "platform.hpp"

namespace zebral
{
/// Scaled parameter type
/// Work in progress, just an idea right now
class Param
{
 public:
  enum class CurveType
  {
    Literal,  ///< Take value as-is
    Linear,   ///< Map to 0.0f-100.0f
    // dB
  };

  Param(const std::string& name, double value = 0.0, CurveType curveType = CurveType::Literal,
        double def = 0.0, double minVal = 0.0, double maxVal = 0.0);

  double Raw() const;
  double Scaled();
  double FromScaled(double scaled);
  double FromRaw(double raw);
  double ScaledToRaw(double scaled) const;
  double RawToScaled(double raw) const;

 protected:
  std::string name_;
  double value_;
  CurveType curveType_;
  double def_;
  double min_;
  double max_;
};

}  // namespace zebral

#endif  // LIGHTBOX_CAMERA_PARAM_HPP_