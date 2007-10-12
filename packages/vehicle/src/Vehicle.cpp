/*
 * Copyright (C) 2007 Robotics at Maryland
 * Copyright (C) 2007 Joseph Lisee <jlisee@umd.edu>
 * All rights reserved.
 *
 * Author: Joseph Lisee <jlisee@umd.edu>
 * File:  packages/vision/src/Vehicle.cpp
 */

// STD Includes
#include <iostream>

// Project Includes
#include "vehicle/include/Vehicle.h"
#include "vehicle/include/device/IDevice.h"
#include "sensorapi/include/sensorapi.h"

namespace ram {
namespace vehicle {

Vehicle::Vehicle(core::ConfigNode config) :
    m_config(config),
    m_sensorFD(-1),
    m_markerNum(0),
    m_depthCalibSlope(m_config["depthCalibSlope"].asDouble()),
    m_depthCalibIntercept(m_config["depthCalibIntercept"].asDouble()),
    m_calibratedDepth(false),
    m_depthOffset(0)
{
    std::string devfile =
        m_config["sensor_board_file"].asString("/dev/sensor");
    
    m_sensorFD = openSensorBoard(devfile.c_str());
    syncBoard(m_sensorFD);

    if (m_sensorFD < -1)
        std::cout << "Could not open sensor board\n";
    else
        unsafeThrusters();
}

Vehicle::~Vehicle()
{
    safeThrusters();
}
    
device::IDevicePtr Vehicle::getDevice(std::string name)
{
    return m_devices[name];
}

double Vehicle::getDepth()
{
    core::ReadWriteMutex::ScopedReadLock lock(m_state_mutex);
    return m_state.depth;
}

void Vehicle::safeThrusters()
{
    boost::mutex::scoped_lock lock(m_sensorBoardMutex);
    
    if (m_sensorFD >= 0)
    {
        // Todo, check whether these succeed
        thrusterSafety(m_sensorFD, CMD_THRUSTER1_OFF);
        thrusterSafety(m_sensorFD, CMD_THRUSTER2_OFF);
        thrusterSafety(m_sensorFD, CMD_THRUSTER3_OFF);
        thrusterSafety(m_sensorFD, CMD_THRUSTER4_OFF);
    }
}

void Vehicle::unsafeThrusters()
{
    boost::mutex::scoped_lock lock(m_sensorBoardMutex);
    
    if (m_sensorFD >= 0)
    {
        // todo check whether these succeed
        thrusterSafety(m_sensorFD, CMD_THRUSTER1_ON);
        thrusterSafety(m_sensorFD, CMD_THRUSTER2_ON);
        thrusterSafety(m_sensorFD, CMD_THRUSTER3_ON);
        thrusterSafety(m_sensorFD, CMD_THRUSTER4_ON);
    }
}

void Vehicle::dropMarker()
{
    boost::mutex::scoped_lock lock(m_sensorBoardMutex);
    ::dropMarker(m_sensorFD, m_markerNum);
}

int Vehicle::startStatus()
{
    core::ReadWriteMutex::ScopedReadLock lock(m_state_mutex);
    return m_state.startSwitch;   
}

void Vehicle::printLine(int line, std::string text)
{
    if (m_sensorFD >= 0)
    {
        boost::mutex::scoped_lock lock(m_sensorBoardMutex);
        displayText(m_sensorFD, line, text.c_str());
    }
}
    
void Vehicle::getState(Vehicle::VehicleState& state)
{
    core::ReadWriteMutex::ScopedReadLock lock(m_state_mutex);
    state = m_state;
}

void Vehicle::setState(const Vehicle::VehicleState& state)
{
    core::ReadWriteMutex::ScopedWriteLock lock(m_state_mutex);
    m_state = state;
}

void Vehicle::_addDevice(device::IDevicePtr device)
{
    m_devices[device->getName()] = device;
}

void Vehicle::update(double timestep)
{
    if (m_sensorFD >= 0)
    {
        core::ReadWriteMutex::ScopedWriteLock lockState(m_state_mutex);
        boost::mutex::scoped_lock lockSensor(m_sensorBoardMutex);

        // Depth
        double rawDepth = readDepth(m_sensorFD);
        m_state.depth = (rawDepth - m_depthCalibIntercept) /
            m_depthCalibSlope - m_depthOffset;;

        // If we aren't calibrated, take values
        if (!m_calibratedDepth)
        {
            m_depthFilter.addValue(m_state.depth);

            // After five values, take the reading
            if (5 == m_depthFilter.getSize())
            {
                m_calibratedDepth = true;
                m_depthOffset = m_depthFilter.getValue();
            }
        }
        
        //m_state.depth = rawDepth;
        // Status register
        int status = readStatus(m_sensorFD);
        m_state.startSwitch = status & STATUS_STARTSW;
    }
}

void Vehicle::calibrateDepth()
{
    m_depthFilter.clear();
    m_calibratedDepth = false;
}
    
} // namespace vehicle
} // namespace ram
