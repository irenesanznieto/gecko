//------------------------------------------------------------------------------
//-- BackgroundSubstractor
//------------------------------------------------------------------------------
//--
//-- Extracts the background using the Mixture Of Gaussians method
//--
//------------------------------------------------------------------------------
//--
//-- This file belongs to the "Gecko - Gesture Recognition" project
//-- (https://github.com/David-Estevez/gecko)
//--
//------------------------------------------------------------------------------
//-- Authors: David Estevez Fernandez
//--          Irene Sanz Nieto
//--
//-- Released under the GPL license (more info on LICENSE.txt file)
//------------------------------------------------------------------------------

/*! \file backgroundSubstractor.h
 *  \brief Extracts the background using the Mixture Of Gaussians method
 *
 * \author David Estevez Fernandez ( http://github.com/David-Estevez )
 * \author Irene Sanz Nieto ( https://github.com/irenesanznieto )
 * \date Dec 12th, 2013
 */


#ifndef BACKGROUNDSUBSTRACTOR_H
#define BACKGROUNDSUBSTRACTOR_H

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

class backgroundSubstractor: public cv::BackgroundSubtractorMOG2
{
public:
    void setbackgroundRatio(float a){backgroundRatio = a;}
};

#endif // BACKGROUNDSUBSTRACTOR_H
