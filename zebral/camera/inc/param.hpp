/// \file param.hpp
/// Ranged parameters for exposure, gain, etc.
#ifndef LIGHTBOX_CAMERA_PARAM_HPP_
#define LIGHTBOX_CAMERA_PARAM_HPP_

#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include "errors.hpp"
#include "log.hpp"

namespace zebral
{
class Param;
/// Callback function for parameter changes
/// \param param - parameter that changed
/// \param raw_set - true if the value changed from raw value being set.
/// \param auto_mode - if true, the param is in (or should be placed in) auto mode if available.
typedef std::function<void(Param* param, bool raw_set, bool auto_mode)> ParamCb;

/// Named callbacks (for erasing/sorting/debugging)
typedef std::pair<std::string, ParamCb> ParamChangedCb;

/// Compare function for paramer callback functions
struct CompareSubscribers
{
  /// \param p1 - first named callback function
  /// \param p2 - second named callback function
  /// \returns true if p1 < p2 (using the name identifiers)
  bool operator()(const ParamChangedCb& p1, const ParamChangedCb& p2) const noexcept
  {
    return p1.first < p2.first;
  }
};

/// Set of parameter subscribers that receive notifications when the parameters are changed.
typedef std::set<ParamChangedCb, CompareSubscribers> ParamSubscribers;

/// Default RawToScaled function - converts raw value to 0.0 - 1.0 value
double RawToScaledNormal(double value, double minVal, double maxVal);

/// Default ScaledToRaw function - converts scaled 0.0-1.0 value back to linear value between minVal
/// and maxVal
double ScaledToRawNormal(double value, double minVal, double maxVal);

/// The desire is to have a generic parameter type that we can make a list of
/// and pass around that optionally can take a range and convert between a "Device" value and
/// a "GUI" value, without putting too much of a limit on what those can be
/// (see audio params in plugins hard mapped to doubles....)
///
/// When it changes from the GUI side, we notify the Device side and vise-versa.
/// Right now you can also subscribe additional clients to be notified.
///
/// The locking may be overkill for the types we'll be using the most right now, since
/// 32/64-bit values are atomic.  If we have performance issues there look at that.
///
/// Also added an "automode", since the parameters I'm controlling currently can usually
/// be switched between user controlled and automatically adjusted.
class Param
{
 public:
  /// Param constructor
  /// \param name - name of parameter
  /// \param callbacks - list of callbacks to subscribe to (defaults empty)
  Param(const std::string& name, const ParamSubscribers& callbacks = ParamSubscribers())
      : name_(name),
        subscribers_(callbacks.begin(), callbacks.end())
  {
  }
  // dtor
  virtual ~Param() {}

  /// Subscribe to notifications when the parameter changes
  /// \param cb - callback definition
  void Subscribe(const ParamChangedCb& cb)
  {
    std::lock_guard<std::recursive_mutex> lock(val_mutex_);
    // I don't expect will want to register on top of others...
    // {TODO} decide if this is allowed, add a return code, or
    // a normal throw.
    ZBA_ASSERT(subscribers_.count(cb) > 0, "Already registered");
    subscribers_.insert(cb);
  }

  /// Unsubscribe from notifications when the parameter changes
  /// \param cb - callback definition
  void Unsubscribe(const ParamChangedCb& cb)
  {
    std::lock_guard<std::recursive_mutex> lock(val_mutex_);
    subscribers_.erase(cb);
  }

  /// Retrieves the parameter name
  /// \returns std::string
  std::string name() const
  {
    return name_;
  }

  /// Dumps the parameter name to an ostream
  std::ostream& dump(std::ostream& os) const
  {
    os << "Param (" << name_ << ")";
    return os;
  }

 protected:
  std::string name_;                        ///< Name of the parameter
  mutable std::recursive_mutex val_mutex_;  ///< value / subscriber lock
  ParamSubscribers subscribers_;            ///< Notification callbacks
};

/// Param with a value that can be set/retrieved and default value
template <class RawType>
class ParamVal : public Param
{
 public:
  /// Param constructor
  /// \param name - name of parameter
  /// \param callbacks - callbacks to register
  /// \param value - value of parameter
  /// \param def - default value
  ParamVal(const std::string& name, const ParamSubscribers& callbacks, RawType value, RawType def,
           bool auto_mode, bool auto_supported)
      : Param(name, callbacks),
        value_(value),
        def_(def),
        auto_mode_(auto_mode),
        auto_supported_(auto_supported)
  {
  }

  /// virtual dtor
  virtual ~ParamVal() {}

  /// Retrieves the raw value of the param
  /// \returns RawType - returns the raw value
  RawType get() const
  {
    std::lock_guard<std::recursive_mutex> lock(Param::val_mutex_);
    return value_;
  }

  /// Converts the value to a string
  virtual std::string to_string() const
  {
    return std::to_string(get());
  }

  /// Sets the raw value
  /// \param raw - raw value to set
  /// \returns bool - true if value was clamped, false otherwise
  ///
  /// If ParamVal is used directly, it's never clamped - but ParamRange may clamp it.
  virtual bool set(RawType raw)
  {
    bool changed = false;
    {
      std::lock_guard<std::recursive_mutex> lock(Param::val_mutex_);
      changed = (value_ != raw);
      value_  = raw;
    }
    if (changed) OnChanged(true);
    return false;
  }

  /// Get the default value
  /// \return RawType - default value for the parameter
  RawType default_value() const
  {
    return def_;
  }
  /// Returns true if we can support auto switching
  bool autoSupported() const
  {
    return auto_supported_;
  }

  // Gets the auto mode
  bool getAuto() const
  {
    std::lock_guard<std::recursive_mutex> lock(Param::val_mutex_);
    return auto_mode_;
  }

  /// Sets the auto mode
  void setAuto(bool auto_mode, bool fire_event = true)
  {
    bool changed = false;
    {
      std::lock_guard<std::recursive_mutex> lock(Param::val_mutex_);
      if (auto_mode != auto_mode_)
      {
        changed    = true;
        auto_mode_ = auto_mode;
      }
    }
    // For now, we assume sets are from scaled side.
    if (fire_event)
    {
      if (changed) OnChanged(false);
    }
  }

  /// Called when a value is changed
  /// \param from_raw - true if changed by raw value being set (e.g. from set())
  void OnChanged(bool from_raw)
  {
    std::lock_guard<std::recursive_mutex> lock(Param::val_mutex_);
    for (const auto& callback : subscribers_)
    {
      callback.second(this, from_raw, auto_mode_);
    }
  }

  /// Dumps the unique part of ParamVal to an ostream
  std::ostream& dump(std::ostream& os) const
  {
    os << "     Value:" << value_ << " Def:" << def_ << " Auto Support: " << auto_supported_
       << " Auto:" << auto_mode_;
    return os;
  }

 protected:
  RawType value_;        ///< Raw value of the parameter
  RawType def_;          ///< Default value of the parameter
  bool auto_mode_;       ///< Is the param in automatic mode (handled by hardware/driver)?
  bool auto_supported_;  ///< Does the control have an auto setting
};

/// Ranged Parameter that accepts functions to convert between raw and scaled (device and gui).
/// Now also takes a min/max value and clamps to that range.
template <class RawType, class ScaledType>
class ParamRanged : public ParamVal<RawType>
{
 public:
  /// Function to go from a raw value and range to scaled value
  typedef std::function<ScaledType(RawType value, RawType minVal, RawType maxVal)> RawToScaledFunc;

  /// Function to go from a scaled value to the raw value
  typedef std::function<RawType(ScaledType value, RawType minVal, RawType maxVal)> ScaledToRawFunc;

  /// Param constructor
  /// \param name - name of parameter
  /// \param callbacks - callbacks to register
  /// \param value - value of parameter
  /// \param def - default value
  /// \param minVal - minimum value
  /// \param maxVal - maximum value
  /// \param step - step size to see a change
  /// \param auto_mode - is it in auto mode?
  /// \param r2sfunc - rawToScaled function, defaults to normalized linear 0-1
  /// \param s2rfunc - ScaledToRaw function, defaults to inverse of normalized linear 0-1
  ParamRanged(const std::string& name, const ParamSubscribers& callbacks, RawType value,
              RawType def, RawType minVal, RawType maxVal, RawType step, bool auto_mode,
              bool autoSupport, RawToScaledFunc r2sfunc = RawToScaledNormal,
              ScaledToRawFunc s2rfunc = ScaledToRawNormal)
      : ParamVal<RawType>(name, callbacks, value, def, auto_mode, autoSupport),
        minVal_(minVal),
        maxVal_(maxVal),
        stepVal_(step),
        stepScaled_(step / (maxVal - minVal)),
        ToScaled_(r2sfunc),
        ToRaw_(s2rfunc)
  {
  }

  /// dtor
  virtual ~ParamRanged() {}

  /// Override of ParamVal's set() to add clamping
  /// \param raw - value to set
  /// \returns bool - true if clamped, false if in range.
  bool set(RawType raw) override
  {
    bool clamped = false;
    {
      std::lock_guard<std::recursive_mutex> lock(Param::val_mutex_);
      clamped = (raw < minVal_) || (raw > maxVal_);
      ParamVal<RawType>::set(std::clamp<RawType>(raw, minVal_, maxVal_));
    }
    return clamped;
  }

  /// Retrieves the scaled value
  /// \returns ScaledType - scaled value
  ScaledType getScaled() const
  {
    std::lock_guard<std::recursive_mutex> lock(Param::val_mutex_);
    return ToScaled_(ParamVal<RawType>::value_, minVal_, maxVal_);
  }

  /// Retrieve the size of step in scaled values
  ScaledType getScaledStep() const
  {
    return stepScaled_;
  }

  /// Sets the value from a scaled value.
  /// /param scaled - scaled value
  /// \returns true - if true, the value was in range. If false, it was out of range and clamped.
  bool setScaled(ScaledType scaled)
  {
    bool changed = false;
    bool clamped = false;
    {
      std::lock_guard<std::recursive_mutex> lock(Param::val_mutex_);
      RawType raw               = ToRaw_(scaled, minVal_, maxVal_);
      changed                   = (ParamVal<RawType>::value_ != raw);
      clamped                   = (raw < minVal_) || (raw > maxVal_);
      ParamVal<RawType>::value_ = std::clamp<RawType>(raw, minVal_, maxVal_);
    }
    if (changed) ParamVal<RawType>::OnChanged(false);
    return clamped;
  }

  /// Dumps the unique part of ParamRanged to an ostream
  std::ostream& dump(std::ostream& os) const
  {
    os << "     Min:" << minVal_ << " Max:" << maxVal_ << " Scaled:" << getScaled();
    return os;
  }

 protected:
  RawType minVal_;         ///< Minimum raw value for the param
  RawType maxVal_;         ///< Maximum raw value for the param
  RawType stepVal_;        ///< Size of step (raw)
  ScaledType stepScaled_;  ///< Size of step (scaled)

  RawToScaledFunc ToScaled_;  ///< Function to convert a raw value to scaled (device to gui)
  ScaledToRawFunc ToRaw_;     ///< Function to convert a scaled value to a raw (gui to device)
};

/// Menu parameter that allows selection between a set of values.
class ParamMenu : public ParamVal<int>
{
 public:
  /// Param constructor
  /// \param name - name of parameter
  /// \param callbacks - callbacks to register
  /// \param value - value of parameter
  /// \param def - default value
  /// \param minVal - minimum value
  /// \param maxVal - maximum value
  ParamMenu(const std::string& name, const ParamSubscribers& callbacks, int value, int def)
      : ParamVal<int>(name, callbacks, value, def, false, false)
  {
  }

  /// dtor
  virtual ~ParamMenu() {}

  /// Override of ParamVal's set() to add clamping
  /// \param raw - value to set
  /// \returns bool - true if clamped, false if in range.
  bool set(int raw) override
  {
    std::lock_guard<std::recursive_mutex> lock(Param::val_mutex_);
    for (size_t i = 0; i < menu_descs_.size(); ++i)
    {
      if (raw == menu_values_[i])
      {
        ParamVal<int>::set(raw);
        return false;
      }
    }
    return true;
  }

  /// Convert the current value to a string
  std::string to_string() const override
  {
    std::lock_guard<std::recursive_mutex> lock(Param::val_mutex_);
    int i = getIndex();
    if (i >= 0)
    {
      return menu_descs_[i];
    }
    return "INVALID";
  }

  /// Add a new value to the menu
  void addValue(const std::string& desc, int value)
  {
    std::lock_guard<std::recursive_mutex> lock(Param::val_mutex_);
    menu_descs_.emplace_back(desc);
    menu_values_.emplace_back(value);
  }

  /// \returns index from value, or -1 if invalid
  int getIndex() const;

  /// Retrieve the number of menu items
  int getCount() const;

  /// Sets the value from a scaled value.
  /// /param scaled - scaled value
  /// \returns true - if true, the value was in range. If false, it was out of range and clamped.
  bool setIndex(int idx);

  /// Dumps the unique part of ParamMenu to an ostream
  std::ostream& dump(std::ostream& os) const
  {
    std::lock_guard<std::recursive_mutex> lock(Param::val_mutex_);
    for (size_t i = 0; i < menu_descs_.size(); ++i)
    {
      if (i != 0) os << std::endl;

      os << "     Item " << i << ": " << menu_descs_[i] << " (" << std::hex
         << static_cast<unsigned int>(menu_values_[i]) << ")" << std::dec;
    }
    return os;
  }

 protected:
  std::vector<std::string> menu_descs_;  ///< list of menu descriptions
  std::vector<int> menu_values_;         ///< matching list of menu values
};

/// Dumps a shared_ptr<Param> to an ostream. Works for inherited param types.
std::ostream& operator<<(std::ostream& os, const std::shared_ptr<Param>& param);

}  // namespace zebral

#endif  // LIGHTBOX_CAMERA_PARAM_HPP_