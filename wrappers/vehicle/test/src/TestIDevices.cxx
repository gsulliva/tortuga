/*
 * Copyright (C) 2007 Robotics at Maryland
 * Copyright (C) 2007 Joseph Lisee <jlisee@umd.edu>
 * All rights reserved.
 *
 * Author: Joseph Lisee <jlisee@umd.edu>
 * File:  wrapper/vehicle/test/src/TestIVehicle.cxx
 */

// STD Includes
#include <cmath>

// Library Includes
#include <UnitTest++/UnitTest++.h>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/python.hpp>

// Project Includes
#include "vehicle/test/include/MockVehicle.h"
#include "vehicle/test/include/MockThruster.h"

namespace py = boost::python;

struct DeviceFixture
{
    DeviceFixture() :
        main_module(py::import("__main__")),
        main_namespace(main_module.attr("__dict__")),
        eval(boost::bind(py::exec, _1, main_namespace, main_namespace))
    {
        main_namespace["vdev"] = py::import("ext.vehicle_device");
    }

    py::object main_module;
    py::object main_namespace;
    boost::function<py::object (py::str)> eval;
};

TEST(DeviceImport)
{
    py::import("ext.vehicle_device");
}

TEST_FIXTURE(DeviceFixture, Vehicle)
{
    try {
        py::import("ext.vehicle");

        // Create vehicle and add it the python environment
        MockVehicle* vehicle = new MockVehicle;
        boost::shared_ptr<ram::vehicle::IVehicle> vehiclePtr(vehicle);
        main_namespace["vehicle"] = vehiclePtr;

        // Create MockThruster and add it to the MockVehicle
        MockThruster* thruster = new MockThruster("AftThruster");
        ram::vehicle::device::IThrusterPtr thrusterPtr(thruster);
        vehicle->devices[thruster->getName()] = thrusterPtr;

        // Test getForce
        thruster->force = 10;

        eval("thruster = vehicle.getDevice('AftThruster')\n"
             "thruster = vdev.IThruster.castTo(thruster)\n"
             "force = thruster.getForce()");

        int force = py::extract<double>(main_namespace["force"]);
        CHECK_EQUAL(thruster->force, force);

        // Test set force
        eval("thruster.setForce(13)");
        CHECK_EQUAL(13, thruster->force);
    } catch(py::error_already_set err) { PyErr_Print(); throw err; }
}

int main()
{
    // We must register this contverted so we can pass a pointer to python
    py::register_ptr_to_python< boost::shared_ptr<ram::vehicle::IVehicle> >();
    Py_Initialize();
    return UnitTest::RunAllTests();
}
