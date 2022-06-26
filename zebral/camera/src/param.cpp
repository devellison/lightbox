#include "param.hpp"
#include "errors.hpp"
#include "log.hpp"

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
/// \returns index from value, or -1 if invalid
int ParamMenu::getIndex() const
{
  std::lock_guard<std::recursive_mutex> lock(Param::val_mutex_);
  for (int i = 0; i < static_cast<int>(menu_descs_.size()); ++i)
  {
    if (value_ == menu_values_[i])
    {
      return i;
    }
  }
  ZBA_ERR("Invalid menu item, can't find index for {}:{}!", name_, value_);
  return -1;
}

/// Retrieve the number of menu items
int ParamMenu::getCount() const
{
  std::lock_guard<std::recursive_mutex> lock(Param::val_mutex_);
  return static_cast<int>(menu_descs_.size());
}

bool ParamMenu::setIndex(int idx)
{
  bool changed = false;
  bool err     = false;
  {
    std::lock_guard<std::recursive_mutex> lock(Param::val_mutex_);
    if ((idx >= 0) && (idx < static_cast<int>(menu_descs_.size())))
    {
      changed               = true;
      ParamVal<int>::value_ = menu_values_[idx];
    }
    else
    {
      ZBA_ERR("Invalid menu index for {}({})", name_, idx);
      err = true;
    }
  }
  if (changed) ParamVal<int>::OnChanged(false);
  return err;
}

}  // namespace zebral