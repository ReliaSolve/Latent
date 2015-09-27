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
#include <iostream>

static size_t ARDUINO_MAX = 1023;

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

bool ArduinoComparer::constructMapping(size_t &numInterp, size_t &numNonMonotonic)
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
  numInterp = 0;
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
      double gap = nextVal - base;
      double baseVal = m_mappingMean[base];
      double diffVal = m_mappingMean[nextVal] - baseVal;
      for (size_t j = i; j < nextVal; j++) {
        m_mappingMean[j] = baseVal + (j-base)/gap * diffVal;
        numInterp++;
      }
    }
  }

  // Ensure that the mapping is monotonic.
  bool expectLarger = (m_mappingMean[m_maxArduinoValue] <
      m_mappingMean[m_minArduinoValue]);
  for (size_t i = m_minArduinoValue; i < m_maxArduinoValue; i++) {
    bool larger = (m_mappingMean[i+1] < m_mappingMean[i]);
    if (larger != expectLarger) {
      m_mappingMean[i] = m_mappingMean[i+1];
      numNonMonotonic++;
    }
  }

  return true;
}

double ArduinoComparer::getDeviceValueFor(size_t arduinoValue)
{
  // Make sure we have entries to read.
  if (m_maxArduinoValue <= m_minArduinoValue) {
    return 0.0;
  }

  // Don't read past the end of the array.
  if (arduinoValue > m_maxArduinoValue) {
    arduinoValue = m_maxArduinoValue;
  }

  return m_mappingMean[m_maxArduinoValue];  
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

