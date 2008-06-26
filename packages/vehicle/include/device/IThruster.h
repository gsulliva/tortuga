/*
 * Copyright (C) 2007 Robotics at Maryland
 * Copyright (C) 2007 Joseph Lisee <jlisee@umd.edu>
 * All rights reserved.
 *
 * Author: Joseph Lisee <jlisee@umd.edu>
 * File:  packages/vision/include/device/Thruster.h
 */

#ifndef RAM_VEHICLE_DEVICE_ITHRUSTER_06_25_2007
#define RAM_VEHICLE_DEVICE_ITHRUSTER_06_25_2007

// STD Includes
#include <string>

// Project Includes
#include "core/include/Event.h"
#include "vehicle/include/device/IDevice.h"
#include "vehicle/include/device/ICurrentProvider.h"

// Must Be Included last
#include "vehicle/include/Export.h"

namespace ram {
namespace vehicle {
namespace device {

class RAM_EXPORT IThruster : public IDevice, // boost::noncopyable
                             public ICurrentProvider
{
public:
    static const core::Event::EventType FORCE_UPDATE;
    
    virtual ~IThruster();

    /** Sets the current thruster force of the thrusters */
    virtual void setForce(double newtons) = 0;

    /** Return the current force the thrusters are outputing in newtons */
    virtual double getForce() = 0;

    /** The maximum positive force in Newtons the thruster produces */
    virtual double getMaxForce() = 0;

    /** The maximum negative force in Newtons the thruster produces */
    virtual double getMinForce() = 0;

    /** Whether or not the thruster will produce any force */
    virtual bool isEnabled() = 0;

    /** Enables of disables the thruster
     *
     *  @param state
     *      True to enable the thruster, false to disable
     */
    virtual void setEnabled(bool state) = 0;
    
    /** Returns the offset from axis perpendicular to axis of inducded rotation
     *
     *  The means the port thruster which rotates around Z-axis and is in the
     *  X-Y plane would give report its distance off the X axis.
     */
    virtual double getOffset() = 0;

protected:
    IThruster(core::EventHubPtr eventHub = core::EventHubPtr());  
};
    
} // namespace device
} // namespace vehicle
} // namespace ram

#endif // RAM_VEHICLE_DEVICE_ITHRUSTER_06_25_2007
