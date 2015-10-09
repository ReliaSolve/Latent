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

/// Class to keep track of a set of changing values over time.  It is
/// constructed based on a set of reports, a definition of 0 time, and
/// an index telling which value to use.  It produces a sorted vector
/// of values by reading the specified index and then allows lookup
/// into those values over time, interpolating when between times and
/// clamping to the ends when beyond the bounds.

class Trajectory {
  public:
    Trajectory(
      const std::vector<DeviceThreadReport> &reports  //< Holds the values to fill in
      , struct timeval start                    //< Defines 0 seconds
      , int index                               //< Which value to use from the reports
      , bool arrivalTime = false                //< Use arrival time rather than reported time
    );

    /// @brief Look up an interpolated value at specified seconds past start time.
    /// @param [in] seconds Time in seconds since the start time passed to the constructor.
    /// @return If seconds is before the beginning of time, returns the value
    ///   associated with the first entry in the trajectory.  If beyond the
    ///   end of time, the last.  If there are no entries, returns 0.
    ///   If the time is between two entries in the list of reports, it will
    ///   linearly interpolate the value.  If there are multiple entries at the
    ///   same time, it picks one of them and returns it.
    double lookup(double seconds) const;

    /// Class describing one entry in the trajectory vector.
    class Entry {
    public:
      double m_time;
      double m_value;
      bool operator < (const Entry &e) const { return m_time < e.m_time; }
    };
    std::vector<Entry> m_entries;   //< Sorted list of values from base time.
};

/// Class to handle comparing sets of Arduino values against other
/// devices' reported values to estimate the latency between them.

class ArduinoComparer {
  public:
    ArduinoComparer();
    ~ArduinoComparer();

    //=======================================================
    // Methods used to construct the mapping from Arduino to
    // Device value.  Add as many mappings as desired and
    // then turn into a mapping using constructMapping().

    /// @brief Add an entry to help map arduino values to device values.
    bool addMapping(double arduinoVal, double deviceVal);

    /// @brief Fill in any values without entries, telling how many
    /// @param [out] outNumInterp How many values had to be interpolated.
    /// @return true on success, false on failure (no entries)
    bool constructMapping(size_t &outNumInterp);

    /// @brief Tell the minimum Arduino value mapped.
    size_t minArduinoValue() const { return m_minArduinoValue; }

    /// @brief Tell the maximum Arduino value mapped.
    size_t maxArduinoValue() const { return m_maxArduinoValue; }

    /// @brief Read the Device value associated with this Arduino value.
    /// @return Value associated with specified Arduino value (clamped to
    /// the maximum Arduino value).  If there is not a mapping, this returns 0.
    double getDeviceValueFor(size_t arduinoValue) const;

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
    /// @param [in] arduinoChannel Channel to read values from for the Arduino
    /// @param [out] deviceChannel Channel to read values from for the Device
    /// @param [out] optimalShiftSeconds Optimal shift in seconds to reduce
    ///   the error between the arduino value and the Device value
    ///   recorded at a particular time.  Temporal interpolation is
    ///   used to align values not taken at the same instant.  Positive
    ///   shift means that the device values were later than the
    ///   Arduino values, and is what is expected.
    /// @param [in] arrivalTime Use arrival time rather than report time
    /// @return true if a result was found, false if no reports.
    bool computeLatency(
          int arduinoChannel
          , int deviceChannel
          , double &outLatencySeconds
          , bool arrivalTime = false
      ) const;

  protected:
    //=======================================================
    // Data structures and routines to produce a mapping
    // between Arduino values and the reported device values.
    std::vector<std::vector<double> > m_mappingVector;      //< device values
    std::vector<double> m_mappingMean;    //< Mean values mapping from Arduino to device
    size_t m_minArduinoValue;       //< Minimum mapped Arduino value.
    size_t m_maxArduinoValue;       //< Maximum mapped Arduino value.

    //=======================================================
    // Data structures and routines to enable estimation of
    // latency.
    std::vector<DeviceThreadReport> m_arduinoReports;
    std::vector<DeviceThreadReport> m_deviceReports;

    /// @brief Compute the sum of squared errors for trajectories given offset.
    /// @param [in] aT Trajectory to use for the Arduino values
    /// @param [in] dT Trajectory to use for the Device values
    /// @param [in] offsetSeconds How far to shift the device values into the
    ///   future when computing the differences.
    /// @return The sum of squared differences between the device values
    ///   shifted in time by the specified offset and the expected mapping
    ///   for the nearest-time Arduino values.
    double computeError(
                const Trajectory &aT
                , const Trajectory &dT
                , double offsetSeconds
           ) const;
};

