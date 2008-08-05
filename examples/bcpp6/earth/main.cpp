//===========================================================================
/*
    This file is part of the CHAI 3D visualization and haptics libraries.
    Copyright (C) 2003-2004 by CHAI 3D. All rights reserved.

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License("GPL") version 2
    as published by the Free Software Foundation.

    For using the CHAI 3D libraries with software that can not be combined
    with the GNU GPL, and for taking advantage of the additional benefits
    of our support services, please contact CHAI 3D about acquiring a
    Professional Edition License.

    \author:    <http://www.chai3d.org>
    \author:    Francois Conti
    \version    1.1
    \date       01/2006
*/
//===========================================================================

//---------------------------------------------------------------------------
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Different compilers like slightly different GLUT's 
#ifdef _MSVC
  #include "../../../external/OpenGL/msvc6/glut.h"
#else
  #ifdef _POSIX
    #include <GL/glut.h>
  #else
    #include "../../../external/OpenGL/bbcp6/glut.h"
  #endif
#endif

//---------------------------------------------------------------------------
#include "CCamera.h"
#include "CLight.h"
#include "CWorld.h"
#include "CMesh.h"
#include "CTriangle.h"
#include "CVertex.h"
#include "CMaterial.h"
#include "CTexture2D.h"
#include "CMatrix3d.h"
#include "CVector3d.h"
#include "CPrecisionClock.h"
#include "CPrecisionTimer.h"
#include "CMeta3dofPointer.h"
#include "CShapeSphere.h"
#include "CBitmap.h"
//---------------------------------------------------------------------------

// the world in which we will create our environment
cWorld* world;

// the camera which is used view the environment in a window
cCamera* camera;

// a light source
cLight *light;

// a simple sphere shape
cShapeSphere* object;

// a little "chai3d" bitmap logo at the bottom of the screen
cBitmap* logo;

// rotational velocity of the torus
cVector3d rotVelocity;

// a 3D cursor which represents the haptic device
cMeta3dofPointer* cursor;

// stores the last position of the cursor
cVector3d lastCursorPos;

// velocity of the cursor
cVector3d cursorVel;

// precision clock to synch dynamic simulation
cPrecisionClock g_clock;
double timeCounter;

// haptic timer callback
cPrecisionTimer timer;

// width and height of the current viewport display
int width   = 0;
int height  = 0;

// menu options
const int OPTION_FULLSCREEN     = 1;
const int OPTION_WINDOWDISPLAY  = 2;

//---------------------------------------------------------------------------

void draw(void)
{
    // set the background color of the world
    cColorf color = camera->getParentWorld()->getBackgroundColor();
    glClearColor(color.getR(), color.getG(), color.getB(), color.getA());

    // clear the color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // render world in the window display
    camera->renderView(width, height);

    // check for any OpenGL errors
    GLenum err;
    err = glGetError();
    if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));

    // Swap buffers
    glutSwapBuffers();
}

//---------------------------------------------------------------------------

void key(unsigned char key, int x, int y)
{
    // "ESC" key is pressed
    if (key == 27)
    {
        // stop the simulation timer
        timer.stop();

        // stop the tool
        cursor->stop();

#ifndef _POSIX 
        // wait for the simulation timer to close
        Sleep(100);
#endif

        // exit application
        exit(0);
    }
}

//---------------------------------------------------------------------------

void rezizeWindow(int w, int h)
{
    // update the size of the viewport
    width = w;
    height = h;

    // update viewport
    glViewport(0, 0, width, height);

    // update the size of the "chai3d" logo
    float scale = (float) w / 1500.0;
    logo->setZoomHV(scale, scale);
}

//---------------------------------------------------------------------------

void updateDisplay(int val)
{
    // draw scene
    draw();

    // update the GLUT timer for the next rendering call
    glutTimerFunc(30, updateDisplay, 0);
}

//---------------------------------------------------------------------------

void setOther(int value)
{
    switch (value)
    {
        case OPTION_FULLSCREEN:
            glutFullScreen();
            break;

        case OPTION_WINDOWDISPLAY:
            glutReshapeWindow(512, 512);
            glutInitWindowPosition(0, 0);
            break;
    }
    
    glutPostRedisplay();
}

//---------------------------------------------------------------------------

void hapticsLoop(void* a_pUserData)
{
    // read the position of the haptic device
    cursor->updatePose();

    // compute forces between the cursor and the environment
    cursor->computeForces();

    // stop the simulation clock
    g_clock.stop();

    // read the time increment in seconds
    double increment = g_clock.getCurrentTime() / 1000000.0;

    // restart the simulation clock
    g_clock.initialize();
    g_clock.start();

    // get position of cursor in global coordinates
    cVector3d cursorPos = cursor->m_deviceGlobalPos;

    // compute velocity of cursor;
    timeCounter = timeCounter + increment;
    if (timeCounter > 0.01)
    {
        cursorVel = (cursorPos - lastCursorPos) / timeCounter;
        lastCursorPos = cursorPos;
        timeCounter = 0;
    }

    // get position of torus in global coordinates
    cVector3d objectPos = object->getGlobalPos();

    // compute the velocity of the sphere at the contact point
    cVector3d contactVel = cVector3d(0.0, 0.0, 0.0);
    if (rotVelocity.length() > CHAI_SMALL)
    {
        cVector3d projection = cProjectPointOnLine(cursorPos, objectPos, rotVelocity);
        cVector3d vpc = cursorPos - projection;
        if (vpc.length() > CHAI_SMALL)
        {
            contactVel = vpc.length() * rotVelocity.length() * cNormalize(cCross(rotVelocity, vpc));
        }
    }

    // get the last force applied to the cursor in global coordinates
    cVector3d cursorForce = cursor->m_lastComputedGlobalForce;

    // compute friction force
    cVector3d friction = -40.0 * cursorForce.length() * cProjectPointOnPlane((cursorVel - contactVel), cVector3d(0.0, 0.0, 0.0), (cursorPos - objectPos));

    // add friction force to cursor
    cursor->m_lastComputedGlobalForce.add(friction);

    // update rotational velocity
    if (friction.length() > CHAI_SMALL)
    {
        rotVelocity.add( cMul(-10.0 * increment, cCross(cSub(cursorPos, objectPos), friction)));
    }

    // add some damping...
    //rotVelocity.mul(1.0 - increment);
    
    // compute the next rotation of the torus
    if (rotVelocity.length() > CHAI_SMALL)
    {
        object->rotate(cNormalize(rotVelocity), increment * rotVelocity.length());
    }

    // send forces to haptic device
    cursor->applyForces();
}

//---------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // display pretty message
    printf ("\n");
    printf ("  ===================================\n");
    printf ("  CHAI 3D\n");
    printf ("  Earth Demo\n");
    printf ("  Copyright 2006\n");
    printf ("  ===================================\n");
    printf ("\n");

    // create a new world
    world = new cWorld();

    // set background color
    world->setBackgroundColor(0.2f,0.2f,0.2f);

    // create a camera
    camera = new cCamera(world);
    world->addChild(camera);

    // create a torus like shape
    object = new cShapeSphere(0.18);
    world->addChild(object);

    // set material stiffness of object
    object->m_material.setStiffness(30.0);
    
    object->m_material.m_ambient.set(0.3, 0.3, 0.3);
    object->m_material.m_diffuse.set(0.8, 0.8, 0.8);
    object->m_material.m_specular.set(1.0, 1.0, 1.0);
    object->m_material.setShininess(100);

    // let's project a world map onto the sphere
    cTexture2D* texture = new cTexture2D();
    texture->loadFromFile("./resources/images/earth.bmp");
    texture->setEnvironmentMode(GL_MODULATE);
    object->m_texture = texture;

    // initialize the object's rotational velocity
    rotVelocity.set(0,0,0.001);

    // position a camera
    camera->set( cVector3d (1.0, 0.0, 0.0),
                 cVector3d (0.0, 0.0, 0.0),
                 cVector3d (0.0, 0.0, 1.0));

    // set the near and far clipping planes of the camera
    camera->setClippingPlanes(0.01, 10.0);

    // Create a light source and attach it to the camera
    light = new cLight(world);
    light->setEnabled(true);
    light->setPos(cVector3d(2,0.5,1));
    light->setDir(cVector3d(-2,0.5,1));
    camera->addChild(light);

    // load a little chai bitmap logo which will located at the bottom of the screen
    logo = new cBitmap();
    logo->m_image.loadFromFile("./resources/images/chai3d.bmp");
    logo->setPos(10,10,0);
    camera->m_front_2Dscene.addChild(logo);

    // we replace the backround color of the logo (black) with a transparent color.
    // we also enable transparency
    logo->m_image.replace(cColorb(0,0,0), cColorb(0,0,0,0));
    logo->enableTransparency(true);

    // create a cursor and add it to the world.
    cursor = new cMeta3dofPointer(world, 0);
    world->addChild(cursor);
    cursor->setPos(0.0, 0.0, 0.0);

    // set up a nice-looking workspace for the cursor so it fits nicely with our
    // cube models we will be builing
    cursor->setWorkspace(1.0,1.0,1.0);

    // set the diameter of the ball representing the cursor
    cursor->setRadius(0.01);

    // set up the device
    cursor->initialize();

    // open communication to the device
    cursor->start();

    // start haptic timer callback
    timer.set(0, hapticsLoop, NULL);

    // initialize the GLUT windows
    glutInit(&argc, argv);
    glutInitWindowSize(512, 512);
    glutInitWindowPosition(0, 0);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(draw);
    glutKeyboardFunc(key);
    glutReshapeFunc(rezizeWindow);
    glutSetWindowTitle("CHAI 3D");

    // create a mouse menu
    glutCreateMenu(setOther);
    glutAddMenuEntry("Full Screen", OPTION_FULLSCREEN);
    glutAddMenuEntry("Window Display", OPTION_WINDOWDISPLAY);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    // update display
    glutTimerFunc(30, updateDisplay, 0);

    // start main graphic rendering loop
    glutMainLoop();
    return 0;
}

//---------------------------------------------------------------------------
