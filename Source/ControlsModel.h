#pragma once
/*
==============================================================================

ControlsModel.h

This file is part of MIDI2LR. Copyright 2015-2017 by Rory Jaffe.

MIDI2LR is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

MIDI2LR is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
MIDI2LR.  If not, see <http://www.gnu.org/licenses/>.
==============================================================================
*/
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <vector>
#include "../JuceLibraryCode/JuceHeader.h"
#include <cereal/access.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/vector.hpp>
#include "MidiUtilities.h"
#include "Utilities/Utilities.h"

namespace RSJ {
  enum struct CCmethod: char {
    absolute, twoscomplement, binaryoffset, signmagnitude
  };
  inline auto now_ms() noexcept {
    return std::chrono::time_point_cast<std::chrono::milliseconds>
      (std::chrono::steady_clock::now()).time_since_epoch().count();
  }
  using timetype = decltype(now_ms());

  struct SettingsStruct {
    short number;//not using size_t so serialized data won't vary if size_t varies
    short high;
    short low;
    RSJ::CCmethod method;
    SettingsStruct(short n = 0, short h = 0x7F, short l = 0, RSJ::CCmethod m = RSJ::CCmethod::absolute):
      number{n}, high{h}, low{l}, method{m} {};
    template<class Archive>    void serialize(Archive & archive, std::uint32_t const version);
  };
}

class ChannelModel {
private:
  constexpr static short kBit14 = 0x2000;
  constexpr static short kBit7 = 0x40;
  constexpr static short kLow13Bits = 0x1FFF;
  constexpr static short kLow6Bits = 0x3F;
  constexpr static short kMaxMIDI = 0x7F;
  constexpr static short kMaxMIDIHalf = kMaxMIDI / 2;
  constexpr static short kMaxNRPN = 0x3FFF;
  constexpr static short kMaxNRPNHalf = kMaxNRPN / 2;
  constexpr static size_t kMaxControls = 0x4000;
  constexpr static RSJ::timetype kUpdateDelay = 250;
public:
  ChannelModel();
  ~ChannelModel() = default;
  //Can write copy and move with special handling for atomics, but in lieu of that, delete
  ChannelModel(const ChannelModel&) = delete; //can't copy atomics
  ChannelModel& operator= (const ChannelModel&) = delete;
  ChannelModel(ChannelModel&&) = delete; //can't move atomics
  ChannelModel& operator=(ChannelModel&&) = delete;
  double ControllerToPlugin(short controltype, size_t controlnumber, short value) noexcept(ndebug);
  RSJ::CCmethod getCCmethod(size_t controlnumber) const noexcept(ndebug);
  short getCCmax(size_t controlnumber) const noexcept(ndebug);
  short getCCmin(size_t controlnumber) const noexcept(ndebug);
  short getPWmax() const noexcept;
  short getPWmin() const noexcept;
  short PluginToController(short controltype, size_t controlnumber, double value) noexcept(ndebug);
  void setCC(size_t controlnumber, short min, short max, RSJ::CCmethod controltype) noexcept;
  void setCCall(size_t controlnumber, short min, short max, RSJ::CCmethod controltype) noexcept;
  void setCCmax(size_t controlnumber, short value) noexcept(ndebug);
  void setCCmethod(size_t controlnumber, RSJ::CCmethod value) noexcept(ndebug);
  void setCCmin(size_t controlnumber, short value) noexcept(ndebug);
  void setPWmax(short value) noexcept(ndebug);
  void setPWmin(short value) noexcept(ndebug);

private:
  friend class cereal::access;
  bool IsNRPN_(size_t controlnumber) const noexcept(ndebug);
  double OffsetResult_(short diff, size_t controlnumber) noexcept(ndebug);
  mutable std::atomic<RSJ::timetype> lastUpdate_{0};
  mutable std::vector<RSJ::SettingsStruct> settingsToSave_{};
  short pitchWheelMax_{kMaxNRPN};
  short pitchWheelMin_{0};
  std::array<RSJ::CCmethod, kMaxControls> ccMethod_;
  std::array<short, kMaxControls> ccHigh_;
  std::array<short, kMaxControls> ccLow_;
  std::array<std::atomic<short>, kMaxControls> currentV_;
  template<class Archive> void load(Archive & archive, std::uint32_t const version);
  template<class Archive> void save(Archive & archive, std::uint32_t const version) const;
  void activeToSaved() const;
  void savedToActive();
};

class ControlsModel {
public:
  ControlsModel();
  ~ControlsModel() = default;
  //Can write copy and move with special handling for atomics, but in lieu of that, delete
  ControlsModel(const ControlsModel&) = delete; //can't copy atomics
  ControlsModel& operator= (const ControlsModel&) = delete;
  ControlsModel(ControlsModel&&) = delete; //can't move atomics
  ControlsModel& operator=(ControlsModel&&) = delete;
  double ControllerToPlugin(short controltype, size_t channel, short controlnumber, short value) noexcept(ndebug) {
    assert(channel <= 15);
    return allControls_[channel].ControllerToPlugin(controltype, controlnumber, value);
  };
  RSJ::CCmethod getCCmethod(size_t channel, short controlnumber) const noexcept(ndebug) {
    assert(channel <= 15);
    return allControls_[channel].getCCmethod(controlnumber);
  };
  short getCCmax(size_t channel, short controlnumber) const noexcept(ndebug) {
    assert(channel <= 15);
    return allControls_[channel].getCCmax(controlnumber);
  };
  short getCCmin(size_t channel, short controlnumber) noexcept(ndebug) {
    assert(channel <= 15);
    return allControls_[channel].getCCmin(controlnumber);
  };
  short getPWmax(size_t channel) const noexcept(ndebug) {
    assert(channel <= 15);
    return allControls_[channel].getPWmax();
  };
  short getPWmin(size_t channel) const noexcept(ndebug) {
    assert(channel <= 15);
    return allControls_[channel].getPWmin();
  };
  short PluginToController(short controltype, size_t channel, short controlnumber, double value) noexcept(ndebug) {
    assert(channel <= 15);
    return allControls_[channel].PluginToController(controltype, controlnumber, value);
  };
  void setCC(size_t channel, short controlnumber, short min, short max, RSJ::CCmethod controltype) noexcept {
    assert(channel <= 15);
    allControls_[channel].setCC(controlnumber, min, max, controltype);
  }
  void setCCall(size_t channel, short controlnumber, short min, short max, RSJ::CCmethod controltype) noexcept(ndebug) {
    assert(channel <= 15);
    allControls_[channel].setCCall(controlnumber, min, max, controltype);
  };
  void setCCmax(size_t channel, short controlnumber, short value) noexcept(ndebug) {
    assert(channel <= 15);
    allControls_[channel].setCCmax(controlnumber, value);
  };
  void setCCmethod(size_t channel, short controlnumber, RSJ::CCmethod value) noexcept(ndebug) {
    assert(channel <= 15);
    allControls_[channel].setCCmethod(controlnumber, value);
  };
  void setCCmin(size_t channel, short controlnumber, short value) noexcept(ndebug) {
    assert(channel <= 15);
    allControls_[channel].setCCmin(controlnumber, value);
  };
  void setPWmax(size_t channel, short value) noexcept(ndebug) {
    assert(channel <= 15);
    allControls_[channel].setPWmax(value);
  };
  void setPWmin(size_t channel, short value) noexcept(ndebug) {
    assert(channel <= 15);
    allControls_[channel].setPWmin(value);
  };
private:
  friend class cereal::access;
  template<class Archive>
  void serialize(Archive & archive, std::uint32_t const version) {
    if (version == 1)// serialize things by passing them to the archive
      archive(allControls_);
  }
  std::array<ChannelModel, 16> allControls_;
};

inline RSJ::CCmethod ChannelModel::getCCmethod(size_t controlnumber) const noexcept(ndebug) {
  assert(controlnumber <= kMaxNRPN);
  return ccMethod_[controlnumber];
}

inline short ChannelModel::getCCmax(size_t controlnumber) const noexcept(ndebug) {
  assert(controlnumber <= kMaxNRPN);
  return ccHigh_[controlnumber];
}

inline short ChannelModel::getCCmin(size_t controlnumber) const noexcept(ndebug) {
  assert(controlnumber <= kMaxNRPN);
  return ccLow_[controlnumber];
}

inline short ChannelModel::getPWmax() const noexcept {
  return pitchWheelMax_;
}

inline short ChannelModel::getPWmin() const noexcept {
  return pitchWheelMin_;
}

inline void ChannelModel::setCC(size_t controlnumber, short min, short max, RSJ::CCmethod controltype) noexcept {
  setCCmin(controlnumber, min);
  setCCmax(controlnumber, max);
  setCCmethod(controlnumber, controltype);
}

inline void ChannelModel::setCCall(size_t controlnumber, short min, short max, RSJ::CCmethod controltype) noexcept {
  if (IsNRPN_(controlnumber))
    for (short a = kMaxMIDI + 1; a <= kMaxNRPN; ++a)
      setCC(a, min, max, controltype);
  else
    for (short a = 0; a <= kMaxMIDI; ++a)
      setCC(a, min, max, controltype);
}

inline void ChannelModel::setCCmax(size_t controlnumber, short value) noexcept(ndebug) {
  assert(controlnumber <= kMaxNRPN);
  assert(value <= kMaxNRPN);
  assert(value >= 0);
  if (ccMethod_[controlnumber] != RSJ::CCmethod::absolute) {
    ccHigh_[controlnumber] = (value < 0) ? 1000 : value;
  }
  else {
    short max = (IsNRPN_(controlnumber) ? kMaxNRPN : kMaxMIDI);
    ccHigh_[controlnumber] = (value <= ccLow_[controlnumber] || value > max) ? max : value;
  }
  currentV_[controlnumber].store((ccHigh_[controlnumber] - ccLow_[controlnumber]) / 2, std::memory_order_release);
}

inline void ChannelModel::setCCmethod(size_t controlnumber, RSJ::CCmethod value) noexcept(ndebug) {
  assert(controlnumber <= kMaxNRPN);
  ccMethod_[controlnumber] = value;
}

inline void ChannelModel::setCCmin(size_t controlnumber, short value) noexcept(ndebug) {
  assert(controlnumber <= kMaxNRPN);
  assert(value <= kMaxNRPN);
  assert(value >= 0);
  if (ccMethod_[controlnumber] != RSJ::CCmethod::absolute)
    ccLow_[controlnumber] = 0;
  else
    ccLow_[controlnumber] = (value < 0 || value >= ccHigh_[controlnumber]) ? 0 : value;
  currentV_[controlnumber].store((ccHigh_[controlnumber] - ccLow_[controlnumber]) / 2, std::memory_order_release);
}

inline void ChannelModel::setPWmax(short value) noexcept(ndebug) {
  assert(value <= kMaxNRPN);
  assert(value >= 0);
  if (value > kMaxNRPN || value <= pitchWheelMin_)
    pitchWheelMax_ = kMaxNRPN;
  else
    pitchWheelMax_ = value;
}

inline void ChannelModel::setPWmin(short value) noexcept(ndebug) {
  assert(value <= kMaxNRPN);
  assert(value >= 0);
  if (value < 0 || value >= pitchWheelMax_)
    pitchWheelMin_ = 0;
  else
    pitchWheelMin_ = value;
}

inline bool ChannelModel::IsNRPN_(size_t controlnumber) const noexcept(ndebug) {
  assert(controlnumber <= kMaxNRPN);
  return controlnumber > kMaxMIDI;
}

inline double ChannelModel::OffsetResult_(short diff, size_t controlnumber) noexcept(ndebug) {
  assert(ccHigh_[controlnumber] > 0); //CCLow will always be 0 for offset controls
  assert(diff <= kMaxNRPN && diff >= -kMaxNRPN);
  assert(controlnumber <= kMaxNRPN);
  lastUpdate_.store(RSJ::now_ms(), std::memory_order_release);
  short cv = currentV_[controlnumber].fetch_add(diff, std::memory_order_relaxed) + diff;
  if (cv < 0) {//fix currentV unless another thread has already altered it
    currentV_[controlnumber].compare_exchange_strong(cv, static_cast<short>(0),
      std::memory_order_relaxed, std::memory_order_relaxed);
    return 0.0;
  }
  if (cv > ccHigh_[controlnumber]) {//fix currentV unless another thread has already altered it
    currentV_[controlnumber].compare_exchange_strong(cv, ccHigh_[controlnumber],
      std::memory_order_relaxed, std::memory_order_relaxed);
    return 1.0;
  }
  return static_cast<double>(cv) / static_cast<double>(ccHigh_[controlnumber]);
}

CEREAL_CLASS_VERSION(ChannelModel, 2);
CEREAL_CLASS_VERSION(ControlsModel, 1);
CEREAL_CLASS_VERSION(RSJ::SettingsStruct, 1);