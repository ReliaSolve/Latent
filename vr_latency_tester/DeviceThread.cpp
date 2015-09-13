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

#include "DeviceThread.h"

const struct timeval DeviceThread::NOW = { 0, 0};

DeviceThread::DeviceThread()
{
  // Start the device thread and wait until it is running or broken.
  // Give it a pointer to this object so that it can call the
  // appropriate methods.
  vrpn_ThreadData td;
  td.pvUD = this;
  m_thread = new vrpn_Thread(ThreadToRun, td);
  m_broken = false; // Not broken yet.
  m_thread->go();
  while (!m_thread->running() && !m_broken) {};
}

DeviceThread::~DeviceThread()
{
  // Tell our thread it is time to stop running by grabbing its
  // semaphore.  Wait until it has stopped and then delete it.
  m_quit.p();
  while (m_thread->running()) {
    vrpn_SleepMsecs(1);
  }
  delete m_thread;
}

void DeviceThread::ThreadToRun(vrpn_ThreadData &threadData)
{
  DeviceThread *me = static_cast<DeviceThread *>(threadData.pvUD);

  // Open the device.  If this fails, report ourselves as broken.
  if (!me->OpenDevice()) {
    me->m_broken = true;
  }

  // Continue running until our parent yanks our semaphore to get us to quit.
  // We don't sleep here; we're trying to minimize the latency of our
  // measurements even if it eats an entire CPU.
  while (!me->m_broken && (me->m_quit.condP() == 1) ) {
    me->m_quit.v(); // Restore the semaphore for the next time around.

    // If we run into trouble, mark ourselves as broken.
    if (!me->ServiceDevice()) {
      me->m_broken = true;
    }
  }

  // Close the device.  If this fails, report ourselves as broken.
  if (!me->CloseDevice()) {
    me->m_broken = true;
  }
}

void DeviceThread::AddReport(
  std::vector<double> values
  , struct timeval sampleTime)
{
  // The arrival time is always now.
  struct timeval arrivalTime;
  vrpn_gettimeofday(&arrivalTime, NULL);

  // If the sampleTime is NOW, then replace it with the arrival time.
  if ( (sampleTime.tv_sec == NOW.tv_sec) && (sampleTime.tv_usec == NOW.tv_usec)) {
    sampleTime = arrivalTime;
  }

  // This is called in the sub-thread to add a report to the list of available
  // reports.  We yank the report-vector semaphore while we're adding it to
  // avoid races.
  DeviceThreadReport r;
  r.arrivalTime = arrivalTime;
  r.sampleTime = sampleTime;
  r.values = values;
  m_reportSemaphore.p();
  m_reports.push_back(r);
  m_reportSemaphore.v();
}

std::vector<DeviceThreadReport> DeviceThread::GetReports()
{
  // We use the semaphore to keep from reading while the subthread is
  // updating.  We make a copy of the reports to send back to the
  // client and then clear the reports.
  std::vector<DeviceThreadReport> ret;
  m_reportSemaphore.p();
  ret = m_reports;
  m_reports.clear();
  m_reportSemaphore.v();

  return ret;
}

