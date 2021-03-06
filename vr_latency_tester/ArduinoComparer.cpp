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

#include "ArduinoComparer.h"
#include <vrpn_Shared.h>
#include <algorithm>
#include <iostream>

static size_t ARDUINO_MAX = 1023;

Trajectory::Trajectory(
      const std::vector<DeviceThreadReport> &reports  //< Holds the values to fill in
      , struct timeval start                    //< Defines 0 seconds
      , int index                               //< Which value to use from the reports
      , bool arrivalTime                        //< Use arrival time rather than reported time
)
{
  if (index < 0) { return; }

  // Insert all reports that have a value with the specified index.
  // Their time is with respect to the base time.
  for (size_t i = 0; i < reports.size(); i++) {
    if (reports[i].values.size() > index) {
      Entry e;
      e.m_value = reports[i].values[index];
      if (arrivalTime) {
        e.m_time = vrpn_TimevalDurationSeconds(reports[i].arrivalTime, start);
      } else {
        e.m_time = vrpn_TimevalDurationSeconds(reports[i].sampleTime, start);
      }
      m_entries.push_back(e);
    }
  }

  // Sort the resulting vector of values.
  std::sort(m_entries.begin(), m_entries.end());
}

double Trajectory::lookup(double seconds) const
{
  // Handle the boundary cases.
  if (m_entries.size() == 0) { return 0; }
  if (seconds <= m_entries.front().m_time) { return m_entries.front().m_value; }
  if (seconds >= m_entries.back().m_time) { return m_entries.back().m_value; }

  // Find the first element in the array that is >= the requested time.
  Entry e;
  e.m_time = seconds;
  std::vector<Entry>::const_iterator ge =
      std::lower_bound(m_entries.begin(), m_entries.end(), e);
  if (ge->m_time == e.m_time) {
    // We found an entry with the same time -- return it.
    return ge->m_value;
  } else {
    // We found the first entry greater than the time requested, and we
    // know that the value is greater than the first entry and less than
    // the last, so there must be a less-than value right before it.
    // Do linear interpolation between the previous and found value and
    // return the result.
    size_t ge_index = ge - m_entries.begin();
    size_t previous_index = ge_index - 1;
    double dT = m_entries[ge_index].m_time - m_entries[previous_index].m_time;
    double dV = m_entries[ge_index].m_value - m_entries[previous_index].m_value;
    double frac = (seconds - m_entries[previous_index].m_time) / dT;
    return m_entries[previous_index].m_value + frac * dV;
  }
}

ArduinoComparer::ArduinoComparer()
{
  // Fill in the mapping vector with empties.
  std::vector<double> emptyVec;
  for (size_t i = 0; i <= ARDUINO_MAX; i++) {
    m_mappingVector.push_back(emptyVec);
  }

  // Fill in the minimum and maximum Arduino values with
  // results that will be overridden whenever an entry is
  // made.
  m_minArduinoValue = ARDUINO_MAX;
  m_maxArduinoValue = 0;
}

ArduinoComparer::~ArduinoComparer()
{
}

bool ArduinoComparer::addMapping(double arduinoVal, double deviceVal)
{
  if ( (arduinoVal < 0) || (arduinoVal > ARDUINO_MAX) ) {
    return false;
  }

  // Add the mapping and keep track of the minimum and maximum
  // Arduino values that have been mapped.
  size_t index = static_cast<size_t>(arduinoVal);
  m_mappingVector[index].push_back(deviceVal);
  if (index < m_minArduinoValue) { m_minArduinoValue = index; }
  if (index > m_maxArduinoValue) { m_maxArduinoValue = index; }
  return true;
}

bool ArduinoComparer::constructMapping(size_t &outNumInterp)
{
  // Make sure we have entries to compute.
  if (m_maxArduinoValue <= m_minArduinoValue) {
    std::cerr << "ArduinoComparer::constructMapping::Insufficient Arduino measurements" << std::endl;
    return false;
  }

  // Compute the range over which we have values and the average value
  // of the readings in each bin to use for our lookup table mapping from
  // Arduino reading to Analog reading.
  for (size_t i = 0; i <= ARDUINO_MAX; i++) {
    double sum = 0;
    size_t count = m_mappingVector[i].size();
    for (size_t j = 0; j < count; j++) {
      sum += m_mappingVector[i][j];
    }
    if (count > 0) {
      m_mappingMean.push_back(sum/count);
    } else {
      m_mappingMean.push_back(0);
    }
  }

  // Find values within the range that were not filled in and fill
  // them in with an interpolated value from the nearest entries above
  // and below them.  Keep track of how many we filled in so we can
  // report it.
  outNumInterp = 0;
  for (size_t i = m_minArduinoValue+1; i < m_maxArduinoValue; i++) {
    if (m_mappingVector[i].size() == 0) {

      // Find the location of the next valid value.
      // Because m_maxArduinoValue has a value, we're guaranteed to
      // find one.
      size_t nextVal = i+1;
      while (m_mappingVector[nextVal].size() == 0) {
        nextVal++;
      }

      // Loop over the empty values and fill in interpolated
      // values.
      size_t base = i-1;
      double gap = static_cast<double>(nextVal - base);
      double baseVal = m_mappingMean[base];
      double diffVal = m_mappingMean[nextVal] - baseVal;
      for (size_t j = i; j < nextVal; j++) {
        m_mappingMean[j] = baseVal + (j-base)/gap * diffVal;
        outNumInterp++;
      }
    }
  }

  return true;
}

double ArduinoComparer::getDeviceValueFor(size_t arduinoValue) const
{
  // Make sure we have entries to read.
  if (m_maxArduinoValue <= m_minArduinoValue) {
    arduinoValue = m_minArduinoValue;
  }

  // Don't read past the end of the array.
  if (arduinoValue > m_maxArduinoValue) {
    arduinoValue = m_maxArduinoValue;
  }

  return m_mappingMean[arduinoValue];  
}

bool ArduinoComparer::addArduinoReports(std::vector<DeviceThreadReport> &r)
{
  // Make sure we have entries to read.
  if (m_maxArduinoValue <= m_minArduinoValue) {
    return false;
  }

  m_arduinoReports.insert(m_arduinoReports.end(), r.begin(), r.end());
  return true;
}

bool ArduinoComparer::addDeviceReports(std::vector<DeviceThreadReport> &r)
{
  // Make sure we have entries to read.
  if (m_maxArduinoValue <= m_minArduinoValue) {
    return false;
  }

  m_deviceReports.insert(m_deviceReports.end(), r.begin(), r.end());
  return true;
}

bool ArduinoComparer::computeLatency(
          int arduinoChannel
          , int deviceChannel
          , double &outLatencySeconds
          , bool arrivalTime ) const
{
  // Bogus value in case we have to bail.
  outLatencySeconds = -10e10;
  if ( (m_deviceReports.size() == 0) || (m_arduinoReports.size() == 0) ) {
    return false;
  }

  // Compute the start time, which is the lowest time value in either
  // of the report lists.
  struct timeval start;
  if (arrivalTime) {
    start = m_deviceReports[0].arrivalTime;
    if (vrpn_TimevalGreater(start, m_arduinoReports[0].arrivalTime)) {
      start = m_arduinoReports[0].arrivalTime;
    }
  } else {
    start = m_deviceReports[0].sampleTime;
    if (vrpn_TimevalGreater(start, m_arduinoReports[0].sampleTime)) {
      start = m_arduinoReports[0].sampleTime;
    }
  }

  // Construct trajectories for both the Arduino and the device.
  Trajectory arduinoTrajectory(m_arduinoReports, start, arduinoChannel, arrivalTime);
  Trajectory deviceTrajectory(m_deviceReports, start, deviceChannel, arrivalTime);

  // Compute the sum of squared differences between the device values and
  // the expected mapping for the nearest-time Arduino values for a temporal
  // offset of 0.
  double minOffset = 0;
  double minError = computeError(arduinoTrajectory, deviceTrajectory,
    minOffset);

  // @todo Turn this from a brute-force search into an optimization routine.
  for (double offset = -300e-3; offset <= 300e-3; offset += 1e-3) {
    double err = computeError(arduinoTrajectory, deviceTrajectory, offset);
    //std::cout << "XXX offset vs. error: " << offset << ", " << err << std::endl;
    if (err < minError) {
      minError = err;
      minOffset = offset;
    }
  }
  outLatencySeconds = minOffset;

  return true;
}

double ArduinoComparer::computeError(
                const Trajectory &aT
                , const Trajectory &dT
                , double offsetSeconds
  ) const
{
  double sum = 0;

  // Check each reading in the device trajectory.  Find its time and
  // shift it.  Then read the value from the Arduino trace at the
  // shifted time and run its value through the mapping to find the
  // expected device value at that time.  Compute the sum of squared
  // errors.
  //  Because we want a positive offset to correspond to the device
  // measurements being behind the Arduino, we need to offset the
  // Arduino measurements by the negative of the offset.
  for (size_t i = 0; i < dT.m_entries.size(); i++) {
    double deviceValue = dT.m_entries[i].m_value;
    double timeShifted = dT.m_entries[i].m_time - offsetSeconds;
    size_t arduinoValue = static_cast<size_t>(aT.lookup(timeShifted));
    double expectedDeviceValue = m_mappingMean[arduinoValue];
    sum += (expectedDeviceValue - deviceValue) * 
           (expectedDeviceValue - deviceValue);    
  }

  return sum;
}

