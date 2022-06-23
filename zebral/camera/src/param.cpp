#include "param.hpp"

namespace zebral
{
double RawToScaledNormal(double value, double minVal, double maxVal)
{
  if (minVal == maxVal)
  {
    ZBA_THROW("MinVal == MaxVal for parameter", Result::ZBA_INVALID_RANGE);
  }
  return (value - minVal) / (maxVal - minVal);
}

double ScaledToRawNormal(double value, double minVal, double maxVal)
{
  if (minVal == maxVal)
  {
    ZBA_THROW("MinVal == MaxVal for parameter", Result::ZBA_INVALID_RANGE);
  }
  return (value * (maxVal - minVal) + minVal);
}

std::ostream& operator<<(std::ostream& os, const std::shared_ptr<Param>& param)
{
  param->dump(os);

  auto val = dynamic_cast<ParamVal<double>*>(param.get());
  if (val)
  {
    os << std::endl;
    val->dump(os);
  }

  auto menu = dynamic_cast<ParamMenu*>(param.get());
  if (menu)
  {
    os << std::endl;
    menu->dump(os);
  }

  auto ranged = dynamic_cast<ParamRanged<double, double>*>(param.get());
  if (ranged)
  {
    os << std::endl;
    ranged->dump(os);
  }

  return os;
}

}  // namespace zebral