/// \file param.hpp
/// Ranged parameters for exposure, gain, etc.
#ifndef LIGHTBOX_CAMERA_PARAM_HPP_
#define LIGHTBOX_CAMERA_PARAM_HPP_

#include <cmath>
#include <functional>
#include <limits>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include "platform.hpp"

namespace zebral
{
class Param;
/// Callback function for parameter changes
/// \param param - parameter that changed
/// \param raw_set - true if the value changed from raw value being set.
typedef std::function<void(Param* param, bool raw_set)> ParamCb;

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

/// OnParamChanged callback
/// \param param - reference to parameter that changed
/// \param raw_set - true if set via set() (device side).
///                  false if set via setScaled() (user side).

/// Work in progress, just an idea right now
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
  ParamVal(const std::string& name, const ParamSubscribers& callbacks, RawType value, RawType def)
      : Param(name, callbacks),
        value_(value),
        def_(def)
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

  /// Called when a value is changed
  /// \param from_raw - true if changed by raw value being set (e.g. from set())
  void OnChanged(bool from_raw)
  {
    std::lock_guard<std::recursive_mutex> lock(Param::val_mutex_);
    for (const auto& callback : subscribers_)
    {
      callback.second(this, from_raw);
    }
  }

 protected:
  RawType value_;  ///< Raw value of the parameter
  RawType def_;    ///< Default value of the parameter
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
  ParamRanged(const std::string& name, const ParamSubscribers& callbacks, RawType value,
              RawType def, RawType minVal, RawType maxVal, RawToScaledFunc r2sfunc,
              ScaledToRawFunc s2rfunc)
      : ParamVal<RawType>(name, callbacks, value, def),
        minVal_(minVal),
        maxVal_(maxVal),
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

 protected:
  RawType minVal_;  ///< Minimum raw value for the param
  RawType maxVal_;  ///< Maximum raw value for the param

  RawToScaledFunc ToScaled_;  ///< Function to convert a raw value to scaled (device to gui)
  ScaledToRawFunc ToRaw_;     ///< Function to convert a scaled value to a raw (gui to device)
};

}  // namespace zebral

#endif  // LIGHTBOX_CAMERA_PARAM_HPP_