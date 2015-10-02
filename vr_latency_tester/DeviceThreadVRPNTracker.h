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
#include <vrpn_Tracker.h>
#include <vrpn_Generic_server_object.h>
#include <string>

/// Function that returns a pointer to a new object that is derived from
/// vrpn_Tracker that has the specified name and uses the specified
/// connection.  All other constructor parameters must be filled in by
/// the person presenting the function and be appropriate for the specific
/// class being constructed.  This lets us hook an arbitrary vrpn_Tracker
/// server device into a thread with a loopback server and provide a
/// stream of reports in a form needed by the latency-testing applications.

typedef vrpn_Tracker *(*DeviceThreadTrackerCreator)(
          const char *deviceName    //< Name to give the device
          , vrpn_Connection *c      //< Connection for the server to use
);

/// Generic vrpn_Tracker DeviceThread class.  Takes as a parameter
/// to the constructor a function that constructs the desired type of
/// object.

class DeviceThreadVRPNTracker : public DeviceThread {
  public:
    /// @brief Construct a DeviceThreadVRPNTracker using a factory.
    /// This creates a DeviceThread for a generic vrpn_Tracker
    /// device, using its own loopback connection and remote object
    /// to handle the callbacks.
    /// @param deviceMaker [in] Function that can be called to produce
    /// the appropriate object derived from vrpn_Tracker with
    /// the specified name and connection (to be determined by the
    /// DeviceThread class).
    DeviceThreadVRPNTracker(DeviceThreadTrackerCreator deviceMaker);

    /// @brief Construct a DeviceThreadVRPNTracker using a config file.
    /// This creates a DeviceThread for a generic vrpn_Tracker
    /// device, using its own loopback connection and remote object
    /// to handle the callbacks.
    /// @param configFileName [in] Name of config file to parse using a
    /// vrpn_Generic_Server_Object.  This config file should have
    /// exactly one vrpn_Tracker-derived object described.
    /// @param deviceName [in] Name of the device defined in the file.
    DeviceThreadVRPNTracker(std::string configFileName,
      std::string deviceName);

    /// @brief Construct a DeviceThreadVRPNTracker using an external server.
    /// This creates a DeviceThread for a generic vrpn_Tracker
    /// device, using a remote connection to an external server.
    /// @param deviceName [in] Name of device (example: "Tracker0@localhost"
    DeviceThreadVRPNTracker(std::string deviceName);

    ~DeviceThreadVRPNTracker();

    /// The constructor and destructor handle making and tearing
    /// down the class, so we only need to override the ServiceDevice
    /// parent class.
    virtual bool ServiceDevice();

  protected:
    vrpn_Connection *m_connection;  //< Connection to talk over
    vrpn_Tracker    *m_server;      //< Server object
    vrpn_Generic_Server_Object  *m_genericServer;   //< Generic server object
    vrpn_Tracker_Remote  *m_remote;   //< Remote object

    // Closes the devices after the subthread has stopped running
    bool CloseDevice();

    /// Callback handler to get reports from the vrpn_Tracker and
    /// pass them on up to the DeviceThread.
    /// @param userdata [in] 'this' pointer to our object.
    /// @param info [in] Information about the analog values.
    static void VRPN_CALLBACK HandleTrackerCallback(
        void *userdata, const vrpn_TRACKERCB info);
};

