/*
  Copyright 2015 ReliaSolve.com

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#pragma once
#include <DeviceThread.h>
#include <vector>

/// Class to handle comparing sets of Arduino values against other
/// devices' reported values to estimate the latency between them.

class ArduinoComparer {
  public:
    ArduinoComparer();

    //=======================================================
    // Methods used to construct the mapping from Arduino to
    // Analog value.  Add as many mappings as desired and
    // then turn into a mapping using constructMapping().

    /// @brief Add an entry to help map arduino values to device values.
    bool addMapping(double arduinoVal, double deviceVal);

    /// @brief Fill in any values without entries, telling how many
    /// @param [out] numInterp How many values had to be interpolated.
    /// @return true on success, false on failure (no entries)
    bool constructMapping(size_t &numInterp);

    /// @brief Tell the minimum Arduino value mapped.
    size_t minArduinoValue() const { return m_minArduinoValue; }

    /// @brief Tell the maximum Arduino value mapped.
    size_t maxArduinoValue() const { return m_maxArduinoValue; }

    /// @brief Read the Analog value associated with this Arduino value.
    /// @return Value associated with specified Arduino value (clamped to
    /// the maximum Arduino value).  If there is not a mapping, this returns 0.
    double getDeviceValueFor(size_t arduinoValue);

    //=======================================================
    // Methods used to store and optimize values to determine
    // latency.  A mapping must have been constructed before
    // these can be used.  Arduino and device reports must be
    // added before the alignment functions can be called.

    /// @brief Add Arduino reports to those used for latency determination.
    bool addArduinoReports(std::vector<DeviceThreadReport> &r);

    /// @brief Add Device reports to those used for latency determination.
    bool addDeviceReports(std::vector<DeviceThreadReport> &r);

    /// @brief Compute the time shift that produces the best alignment.
    /// @param [out] optimalShiftSeconds Optimal shift in seconds to reduce
    ///   the error between the arduino value and the Device value
    ///   recorded at a particular time.  Temporal interpolation is
    ///   used to align values not taken at the same instant.
    /// @return true if a result was found, false if no reports.
    

  protected:
    //=======================================================
    // Data structures and routines to produce a mapping
    // between Arduino values and the reported device values.
    std::vector<std::vector<double> > m_mappingVector;      //< Analog values
    std::vector<double> m_mappingMean;    //< Mean values mapping from Arduino to Analog
    size_t m_minArduinoValue;       //< Minimum mapped Arduino value.
    size_t m_maxArduinoValue;       //< Maximum mapped Arduino value.

    //=======================================================
    // Data structures and routines to enable estimation of
    // latency.
    std::vector<DeviceThreadReport> m_arduinoReports;
    std::vector<DeviceThreadReport> m_deviceReports;
};

