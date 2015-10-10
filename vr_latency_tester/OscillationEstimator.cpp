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

#include "OscillationEstimator.h"
#include <vrpn_Shared.h>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <algorithm>

OscillationEstimator::OscillationEstimator(double windowSeconds,
  int verbosity)
{
  m_windowSeconds = windowSeconds;
  m_windowReached = false;
  m_verbosity = verbosity;
}

OscillationEstimator::~OscillationEstimator()
{
}

double OscillationEstimator::addReportsAndEstimatePeriod(
  const std::vector<DeviceThreadReport> &reps)
{
  //=======================================================
  // Add reports to our list so long as their value counts
  // match.  If the value counts don't match, discard the
  // old measurements and rest our window statistics.
  for (size_t i = 0; i < reps.size(); i++) {
    addReport(reps[i]);
  }

  //=======================================================
  // If we have reached our window size, attempt to estimate
  // the period.
  return estimatePeriod();
}

bool OscillationEstimator::addReport(const DeviceThreadReport &rep)
{
  // Empty list is easy... we just add the entry.
  if (m_reports.size() == 0) {
    m_reports.push_back(rep);
    return true;
  }

  // Check to make sure we don't try and add an entry with a
  // different number of values than the ones already there.  If
  // so, clear things out and return false (but do add the entry
  // itself onto the list).
  if (rep.values.size() != m_reports.front().values.size()) {
    m_reports.clear();
    m_windowReached = false;

    m_reports.push_back(rep);
    if (m_verbosity >= 0) {
      std::cerr << "OscillationEstimator::addReport: Value vector size differs"
        << std::endl;
    }
    return false;
  }

  // Add the report to the list and see if we've got more than
  // the window's worth of reports.  If so, record that we do and
  // also clear entries until we don't.
  m_reports.push_back(rep);
  while (vrpn_TimevalDurationSeconds(m_reports.back().sampleTime,
      m_reports.front().sampleTime) > m_windowSeconds) {
    m_windowReached = true;
    m_reports.pop_front();
  }
  return true;
}

double OscillationEstimator::estimatePeriod() const
{
  //=======================================================
  // If we haven't reached our window, we can't estimate.
  if (!m_windowReached) { return -1; }

  //=======================================================
  // Figure out which axis has moved the most.  If we have
  // no axes, we can't estimate.
  size_t channel = 0;
  std::vector<double> means;
  std::vector<double> deviations;
  computeValueStatistics(means, deviations);
  if ((means.size() == 0) || (deviations.size() != means.size())) {
    if (m_verbosity >= 0) {
      std::cerr << "OscillationEstimator::estimatePeriod: No measurements"
        << std::endl;
    }
    return -1;
  }
  double maxDev = deviations[0];
  for (size_t i = 1; i < deviations.size(); i++) {
    if (deviations[i] > maxDev) {
      maxDev = deviations[i];
      channel = i;
    }
  }
  if (m_verbosity >= 1) {
    std::cout << "OscillationEstimator::estimatePeriod: Using channel "
      << channel << std::endl;
  }

  //=======================================================
  // Look through the values and make a vector of times at
  // which they cross zero after an excursion of at least
  // half a standard deviation from the mean.  Base the
  // initial sign on the first entry.
  bool beyondHSTD = false;
  bool crossedZero = false;
  double lastVal = m_reports.front().values[channel];
  if (lastVal == 0) { lastVal = 1; }
  std::vector<struct timeval> crossings;
  std::list<DeviceThreadReport>::const_iterator rep;
  double mean = means[channel];
  double deviation = deviations[channel];
  for (rep = m_reports.begin(); rep != m_reports.end(); rep++) {
    // If we have not yet gone beyond the half standard
    // deviation, see if we've done so and mark our state if so.
    double val = rep->values[channel];
    if (!beyondHSTD) {
      if (fabs(val - mean) > deviation / 2) {
        beyondHSTD = true;
        crossedZero = false;
      }
    }
    
    // If we've already crossed zero, then we are waiting
    // until we get beyond a half standard deviation, so we
    // do nothing.
    // Otherwise, see if we just crossed zero and store
    // the time of the crossing and change our state
    else if (!crossedZero) {
      if (val * lastVal < 0) {
        crossedZero = true;
        beyondHSTD = false;
        crossings.push_back(rep->sampleTime);
      }
    }

    // In any case, store our last value
    lastVal = val;
  }
  if (crossings.size() < 2) {
    return -1;
  }
  if (m_verbosity >= 1) {
    std::cout << "OscillationEstimator::estimatePeriod: Found "
      << crossings.size() << " crossings" << std::endl;
  }

  //=======================================================
  // Compute the median period between crossings and return
  // it.
  std::vector<double> durations;
  for (size_t i = 1; i < crossings.size(); i++) {
    durations.push_back(vrpn_TimevalDurationSeconds(
      crossings[i], crossings[i - 1]));
  }
  std::sort(durations.begin(), durations.end());
  return durations[durations.size() / 2];
}

void OscillationEstimator::computeValueStatistics(std::vector<double> &means,
  std::vector<double> &deviations) const
{
  // Clear our vectors and return if we have no data to compute from.
  means.clear();
  deviations.clear();
  if (m_reports.size() == 0) { return; }

  // If we don't have any values in our reports, we also return
  // empty vectors.
  size_t num = m_reports.begin()->values.size();
  if (num == 0) { return; }

  // Sum up the values we'll need to keep to compute our statistics
  // in one pass.  Make vectors to store this information into.
  std::vector<double> sums;
  std::vector<double> squareSums;
  for (size_t i = 0; i < num; i++) {
    sums.push_back(0);
    squareSums.push_back(0);
  }
  std::list<DeviceThreadReport>::const_iterator rep;
  for (rep = m_reports.begin(); rep != m_reports.end(); rep++) {
    for (size_t i = 0; i < num; i++) {
      sums[i] += rep->values[i];
      squareSums[i] += rep->values[i] * rep->values[i];
    }
  }

  // Push back entries for each of the means and standard deviations.
  for (size_t i = 0; i < num; i++) {
    means.push_back(sums[i] / num);
    deviations.push_back(sqrt(squareSums[i] / num - means[i] * means[i]));
  }
}
