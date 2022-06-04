/// \file param.cpp
/// Implementation of camera parameters

#include "param.hpp"
#include "errors.hpp"
#include "log.hpp"

namespace zebral
{
Param::Param(const std::string& name, double value, CurveType curveType, double def, double minVal,
             double maxVal)
    : name_(name),
      value_(value),
      curveType_(curveType),
      def_(def),
      min_(minVal),
      max_(maxVal)
{
}
double Param::Raw() const
{
  return value_;
}

double Param::FromRaw(double raw)
{
  value_ = raw;
  return value_;
}

double Param::FromScaled(double scaled)
{
  value_ = ScaledToRaw(scaled);
  return value_;
}

double Param::ScaledToRaw(double scaled) const
{
  double range = max_ - min_;
  ZBA_ASSERT(range > DBL_EPSILON, "RawToScaled called but range not set for param " + name_);
  return scaled * range + min_;
}

double Param::RawToScaled(double raw) const
{
  double range = max_ - min_;
  ZBA_ASSERT(range > DBL_EPSILON, "RawToScaled called but range not set for param " + name_);
  return (raw - min_) / range;
}

double Param::Scaled()
{
  switch (curveType_)
  {
    case CurveType::Literal:
      return value_;
    case CurveType::Linear:
      return RawToScaled(value_);
    default:
      ZBA_THROW("Unknown CurveType for param " + name_, Result::ZBA_UNDEFINED_VALUE);
  }
}

}  // namespace zebral