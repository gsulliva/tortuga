#ifndef _RAM_SIM_CAMERA_H
#define _RAM_SIM_CAMERA_H

#include <btBulletDynamicsCommon.h>

namespace ram {
    namespace sim {
        class Camera {
        private:
            float yaw, pitch, roll, dollyDist;
            //btTransform transform;
        public:
            Camera();
            
            /// Tumble the camera about the +z axis.
            void tumbleYaw(float degrees);
            
            /// Tumble the camera about the +y axis.
            void tumblePitch(float degrees);
            
            /// Tumble the camera about the +x axis.
            void tumbleRoll(float degrees);
            
            /// Dolly in (move straight into the screen)
            void dollyIn(float meters);
            
            /// Dolly out (move straight out of the screen)
            void dollyOut(float meters);
            
            
            
            /// Transform the current OpenGL matrix to look through this camera
            void glTransform();
        };
    }
}

#endif