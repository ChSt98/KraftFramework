#ifndef VEHICLE_TEMPLATE_H
#define VEHICLE_TEMPLATE_H



#include "templates/module_template.h"

#include "data_containers/kinematic_data.h"

#include "guidance_modules/guidance_template.h"
#include "navigation_modules/navigation_template.h"



/**
 * Enum containing all vehicle modes.
 */
enum VEHICLE_MODE {
    //When this is set, the vehicle turns everything off due to a critical error in a subsystem. Should only be resetable from power reset.
    MODE_ERROR,
    //When this is set, the vehicle turns everything off. Should be resetable from vehicle.
    MODE_FAILSAFE,
    //When this is set, the vehicle prepares itsself to arm. Dangerous actuators like motors must be shutoff.
    MODE_DISARM,
    //When this is set, the vehicle turns everything on and follows flight commands.
    MODE_ARM
};



class Vehicle: public Module {
public:

    /**
     * This is where all calculations are done.
     *
     * @param values none.
     * @return none.
     */
    virtual void thread() = 0;

    /**
     * Init function that setups the vehicle.
     *
     * @param values none.
     * @return none.
     */
    virtual void init() = 0;

    /**
     * Returns true if vehicle is ready for flight.
     *
     * @param values none.
     * @return bool.
     */
    virtual bool vehicleReady() = 0;

    /**
     * Returns vehicle kinematics
     *
     * @param values none.
     * @return KinematicData.
     */
    virtual KinematicData getKinematicData() {return _vehicleKinematics;};


protected:

    //Contains current vehicle movement.
    static KinematicData _vehicleKinematics;

    //Contains current forces exerted on the vehicle.
    static VEHICLE_MODE _vehicleMode;

    //Points to the navigation module to use.
    static Navigation *_navigation;

    //Points to the guidance module to use.
    static Guidance *_guidance;


};


//Navigation Vehicle::_navigation = nullptr;




#endif