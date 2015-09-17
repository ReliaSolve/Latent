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

#include "DeviceThreadVRPNAnalog.h"
#include <string>
#include <iostream>

DeviceThreadVRPNAnalog::DeviceThreadVRPNAnalog(DeviceThreadAnalogCreator deviceMaker)
{
  // Initialize things we don't set in this constructor
  m_genericServer = nullptr;

  // Construct a loopback connection for us to use.
  m_connection = vrpn_create_server_connection("loopback:");

  // Construct the server object and client object, having them
  // use the connection and the same name.
  std::string deviceName = "DeviceThread";
  m_server = deviceMaker(deviceName.c_str(), m_connection);
  m_remote = new vrpn_Analog_Remote(deviceName.c_str(), m_connection);
  if (!m_connection || !m_server || !m_remote) {
    delete m_remote; m_remote = nullptr;
    delete m_server; m_server = nullptr;
    if (m_connection) {
      m_connection->removeReference();
      m_connection = nullptr;
    }
    m_broken = true;
    return;
  }

  // Connect the callback handler for the Analog remote to our static
  // function that handles pushing the reports onto the vector of
  // reports, giving it a pointer to this class instance.
  m_remote->register_change_handler(this, HandleAnalogCallback);
}

DeviceThreadVRPNAnalog::DeviceThreadVRPNAnalog(std::string configFileName,
  std::string deviceName)
{
  // Initialize things we don't set in this constructor
  m_server = nullptr;

  // Construct a loopback connection for us to use.
  m_connection = vrpn_create_server_connection("loopback:");

  // Construct the generic server object and client object, having them
  // use the connection and the same name.
  m_genericServer = new vrpn_Generic_Server_Object(m_connection,
    configFileName.c_str(), true);
  if (!m_genericServer->doing_okay()) {
    m_broken = true;
    delete m_genericServer; m_genericServer = nullptr;
    m_connection->removeReference();
    return;
  }
  m_remote = new vrpn_Analog_Remote(deviceName.c_str(), m_connection);
  if (!m_connection || !m_genericServer || !m_remote) {
    delete m_remote; m_remote = nullptr;
    delete m_genericServer; m_genericServer = nullptr;
    if (m_connection) {
      m_connection->removeReference();
      m_connection = nullptr;
    }
    m_broken = true;
    return;
  }

  // Connect the callback handler for the Analog remote to our static
  // function that handles pushing the reports onto the vector of
  // reports, giving it a pointer to this class instance.
  m_remote->register_change_handler(this, HandleAnalogCallback);
}

// We do this here rather than the destructor because we need to
// make sure the thread has stopped calling our object callbacks
// and Service function.
bool DeviceThreadVRPNAnalog::CloseDevice()
{
// @todo Won't this be called after the destructor?
std::cout << "XXX Closing DeviceThreadVRPNAnalog" << std::endl;
    if (m_remote) {
      m_remote->unregister_change_handler(this, HandleAnalogCallback);
    }
    delete m_remote; m_remote = nullptr;
    delete m_server; m_server = nullptr;
    delete m_genericServer; m_genericServer = nullptr;
    if (m_connection != nullptr) {
      m_connection->removeReference();
    }
    return true;
}

bool DeviceThreadVRPNAnalog::ServiceDevice()
{
  // We just mainloop() all of our objects here, which will cause
  // callbacks to be delivered with new data.
  if (m_server) { m_server->mainloop(); }
  if (m_genericServer) {
    if (!m_genericServer->doing_okay()) {
      m_broken = true;
      delete m_genericServer; m_genericServer = nullptr;
      return false;
    }
    m_genericServer->mainloop();
  }
  if (m_connection) { m_connection->mainloop(); }
  if (m_remote) { m_remote->mainloop(); }
  return true;
}

// Static function
void DeviceThreadVRPNAnalog::HandleAnalogCallback(
        void *userdata, const vrpn_ANALOGCB info)
{
  DeviceThreadVRPNAnalog *me = static_cast<DeviceThreadVRPNAnalog *>(userdata);

  // Construct a vector of values from the analog data, one per
  // each entry.
  std::vector<double> values;
  for (int i = 0; i < info.num_channel; i++) {
    values.push_back(info.channel[i]);
  }

  // Send the new report, using the info time as the sample time.
  me->AddReport(values, info.msg_time);
}

