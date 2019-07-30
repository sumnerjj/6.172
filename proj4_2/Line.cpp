#include <stdlib.h>
#include <iostream>
#include "Line.h"
#include "LineDemo.h"

/*
 * The PROFILE_BUILD preprocessor define is used to indicate we are
 * building for profiling, so don't include any graphics or Cilk functions.
 */

#ifndef PROFILE_BUILD

#include "GraphicStuff.h"
#include <cilkview.h>
#define MAIN cilk_main

#else

#define MAIN main

#endif

using namespace std;

// For non-graphic version
void lineMain(LineDemo *lineDemo)
{
   // Loop for updating line movement simulation
   while (true) {
      if (!lineDemo->update()) {
         break;
      }
   }
}


int MAIN(int argc, char** argv)
{
   int optchar;
   bool graphicDemoFlag = false, imageOnlyFlag = false;
   unsigned int numFrames = 1;

   // process command line options
   while ((optchar = getopt(argc, argv, "gi")) != -1) {
      switch (optchar) {
         case 'g':
            graphicDemoFlag = true;
            break;
         case 'i':
            imageOnlyFlag = true;
            graphicDemoFlag = true;
            break;
         default:
            cout << "Ignoring unrecognized option: " << optchar << endl;
            continue;
      }
   }

   if (!imageOnlyFlag) {
      // shift remaining arguments over
      int remaining_args = argc - optind;
      for(int i = 1; i <= remaining_args; ++i ) {
         argv[i] = argv[i+optind-1];
      }

      // check to make sure number of arguments is correct
      if (remaining_args != 1) {
         cout << "Usage: " << argv[0] << " [-g] [-i] <numFrames>" << endl;
         cout << "  -g : show graphics" << endl;
         cout << "  -i : show first image only (ignore numFrames)" << endl;
         exit(-1);
      }

      numFrames = atoi(argv[1]);
      cout << "Number of frames = " << numFrames << endl;
   }

   // Create and initialize the Line simulation environment
   LineDemo *lineDemo = new LineDemo();
   lineDemo->initLine();
   lineDemo->setNumFrames(numFrames);

#ifndef PROFILE_BUILD
   // start cilkview performance analysis
   cilk::cilkview cv;
   cv.start();

   // run demo
   if (graphicDemoFlag) {
      graphicMain(argc, argv, lineDemo, imageOnlyFlag);
   } else {
      lineMain(lineDemo);
   }

   // stop cilkview performance analysis
   cv.stop();
#else
   lineMain(lineDemo);
#endif

   // output results
   cout << lineDemo->getNumLineWallCollisions()
        << " Line-Wall Collisions" << endl;
   cout << lineDemo->getNumLineLineCollisions()
        << " Line-Line Collisions" << endl;

#ifndef PROFILE_BUILD
   cout << cv.accumulated_milliseconds() / 1000.0
        << " seconds in total" << endl;

   // dump cilkview analysis information
   cv.dump("linedemo");
#endif

   // delete objects
   delete lineDemo;

   return 0;
}
