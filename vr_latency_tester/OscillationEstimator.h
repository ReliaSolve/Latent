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
#include <list>

/// Class to handle estimating the period of oscillation of motion
/// over time.

class OscillationEstimator {
  public:
    /// @Brief construct an estimator of oscillation period.
    /// @param [in] Time window over which to estimate.
    OscillationEstimator(double windowSeconds = 1.0, int verbosity = -1);
    virtual ~OscillationEstimator();

    /// @brief Add reports from the device.
    /// @param [in] reps Reports from the device.
    /// All of the reports must be from the same device and
    /// they must all have the same number of values.
    /// @return Period of oscillation in seconds (if valid)
    /// or -1 (if insufficient accumulated reports, or if
    /// the reports do not all have the same number of
    /// entries, or if there is not oscillation happening
    /// during the window.
    double addReportsAndEstimatePeriod(const std::vector<DeviceThreadReport> &reps);

  protected:
    //=======================================================
    // Keep track of our window of estimates
    int m_verbosity;            //< How verbose to be in printing info?
    double m_windowSeconds;     //< Time window of measurements to keep
    bool m_windowReached;       //< Have we gotten enough samples to fill our window?
    std::list<DeviceThreadReport> m_reports; //< Our window full of reports.

    /// @brief Add a report to our list
    /// Add a report to our list, keeping track of whether we have
    /// reached our window or not.  If we try to insert a report
    /// with a different number of values than the others in the
    /// list, clear the list and start over.
    /// @param [in] rep Report to add.
    /// @return True on success, false if we had to reset the list.
    bool addReport(const DeviceThreadReport &rep);

    /// @brief Estimate the period based on the window of reports
    /// @return Report length in seconds in success, -1 on failure.
    double estimatePeriod() const;

    /// @brief Compute the mean and standard deviation for each channel.
    /// @param means [out] The mean for each value in the vector.
    /// Empty if no values or no reports.
    /// @param deviations [out] The standard deviation for each value in the vector.
    /// Empty if no values or no reports.
    void computeValueStatistics(std::vector<double> &means,
      std::vector<double> &deviations) const;
};

