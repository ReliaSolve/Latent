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
#include <vrpn_Shared.h>
#include <vector>

/// This is a structure that describes one set of reports that came in
/// at the same time from a single device.  There should always be the
/// same number of values in the reports from a given object.  Both
/// the estimated time when the values were measured and the time of
/// their arrival in the program should be recorded.
///   Note: If the time that they were measured is not known, the
/// arrival time can be stored in this field as well.
typedef struct {
  std::vector<double> values;     //< Vector of values.
  struct timeval      sampleTime; //< Time when the values were sampled
  struct timeval      arrivalTime;//< Time when the values reached this program.
} DeviceThreadReport;

/// This declares a class that can be used to wrap a thread around an
/// object that emits values.  This is an abstract base class from which
/// you can derive a class to handle an actual object by filling in the
/// appropriate methods.
///   Examples of classes to be derived include VRPN trackers, Ardiuno
/// streaming sensors (which could include potentiometers to detect
/// rotation with very low latency and photodetectors to measure light
/// intensity changes).
///   Each object emits a time-stamped vector of value sets, where each
/// value set is a vector of doubles.
///   Sets of these classes are compared against one another in the
/// various latency-testing applications.

class DeviceThread {
  public:
    /// Construct the thread, which also starts the device running using the
    /// virtual functions below.
    DeviceThread();

    /// Shut down the device, stopping the subthread.  Wait until the
    /// thread finishes and then return.
    virtual ~DeviceThread();

    //=======================================================
    // Methods used to send info back to the application.
    bool IsBroken() const { return m_broken; }
    std::vector<DeviceThreadReport> GetReports();

    //=======================================================
    // All subclasses override these methods if they want to
    // do something during this part of the system operation.

    /// Any device-opening code that should run when the thread
    /// is started that should not happen in the constructor before
    /// the thread is started.
    virtual bool OpenDevice() { return true; }

    /// Service the device; this is called repeatedly while the
    /// thread is running.  It should call the AddReport() method
    /// below whenever a new report comes in from the device.
    virtual bool ServiceDevice() { return true; }

    /// Any device-closing code that should run when the thread
    /// is ending that should not happen in the destructor after
    /// the thread is stopped.
    virtual bool CloseDevice() { return true; }

  protected:
    //=======================================================
    // Thread and associated semaphore handling.
    vrpn_Thread *m_thread;  //< The thread that runs the device
    vrpn_Semaphore  m_quit; //< Main object grabs this to cause thread to quit.
    static void ThreadToRun(vrpn_ThreadData &threadData);
    bool m_broken;          //< The subthread sets this true if trouble.

    //=======================================================
    // Data structures and semaphores to handle reporting data back
    // to the client.
    // @todo Consider using unique_ptr to avoid copies to client.
    std::vector<DeviceThreadReport> m_reports;
    vrpn_Semaphore  m_reportSemaphore;

    //=======================================================
    // Helper functions provided by the base class for derived
    // classes.  They will not normally be overridden by the
    // subclasses.

    /// @brief Add a new report of values.
    /// Add a new report of values to the internally-maintained vector.
    /// @param sampleTime The message time associated with a
    /// VRPN callback handler for the data used to construct the
    /// value or any other estimate of when the actual measurement
    /// was taken before any transmission delays occured.  If unknown,
    /// don't specify a value.
    static const struct timeval NOW;
    virtual void AddReport(
      std::vector<double> values        //< Values to report
      , struct timeval sampleTime = NOW //< When the measurement was taken, if known
    );

};

