// This file is part of the AliceVision project.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "aliceVision/camera/camera.hpp"

#define BOOST_TEST_MODULE pinholeFisheye
#include <boost/test/included/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <aliceVision/unitTest.hpp>


using namespace aliceVision;
using namespace aliceVision::camera;

//-----------------
// Test summary:
//-----------------
// - Create a PinholeFisheye
// - Generate random point inside the image domain
// - Add and remove distortion and assert we found back the generated point
// - Check the last point in the camera & image domain
// - Assert that the tested distortion is not null (in order to ensure validity of the test)
//-----------------
BOOST_AUTO_TEST_CASE(cameraPinholeFisheye_disto_undisto_Fisheye) {

const PinholeFisheye cam(1000, 1000, 1000, 500, 500,
                                    -0.054, 0.014, 0.006, 0.011); // K1, K2, K3, K4

  const double epsilon = 1e-4;
  for (int i = 0; i < 10; ++i)
  {
    // generate random point inside the image domain (last random to avoid 0,0)
    const Vec2 ptImage = (Vec2::Random() * 800./2.) + Vec2(500,500) + Vec2::Random();
    const Vec2 ptCamera = cam.ima2cam(ptImage);
    // Check that adding and removing distortion allow to recover the provided point
    EXPECT_MATRIX_NEAR( ptCamera, cam.remove_disto(cam.add_disto(ptCamera)), epsilon);
    EXPECT_MATRIX_NEAR( ptImage, cam.cam2ima(cam.remove_disto(cam.add_disto(ptCamera))), epsilon);

    // Assert that distortion field is not null and it has moved the initial provided point
    BOOST_CHECK(! (cam.add_disto(ptCamera) == cam.remove_disto(cam.add_disto(ptCamera))) ) ;
  }
}
