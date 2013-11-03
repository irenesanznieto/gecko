#include "HandDescription.h"

//-- Initialization of the private parameters in the constructor
HandDescription:: HandDescription()
{
    //-- Initalize hand parameters
    //-----------------------------------------------------------------------
    _hand_angle=0;
    _hand_center = std::pair <int, int> (0, 0);
    _hand_bounding_box = cv::Rect();
    _hand_gesture = HAND_GESTURE_NONE;
    _hand_num_fingers = -1;
    _hand_found = false;


    //-- Kalman filter setup for estimating hand angle:
    //-----------------------------------------------------------------------

    //-- Create filter:
    kalmanFilterAngle.init( 2, 1, 0);
    kalmanFilterAngle.transitionMatrix = *( cv::Mat_<float>(2, 2) << 1, 1,
								     0, 1);
    //-- Initial state:
    kalmanFilterAngle.statePre.at<float>(0) = 90; //-- initial angle
    kalmanFilterAngle.statePre.at<float>(1) = 0;  //-- initial angular velocity
    
    //-- Set the rest of the matrices:
    cv::setIdentity( kalmanFilterAngle.measurementMatrix );
    cv::setIdentity( kalmanFilterAngle.processNoiseCov, cv::Scalar::all(0.0001));
    cv::setIdentity( kalmanFilterAngle.measurementNoiseCov, cv::Scalar::all(0.1));
    cv::setIdentity( kalmanFilterAngle.errorCovPost, cv::Scalar::all(0.1));



    //-- Kalman filter for estimating hand center
    //---------------------------------------------------------------------

    //-- Create filter:
    kalmanFilterCenter.init( 4, 2, 0);
    kalmanFilterCenter.transitionMatrix = *( cv::Mat_<float>(4, 4) << 1, 0, 1, 0,
								0, 1, 0, 1,
								0, 0, 1, 0,
								0, 0, 0, 1);
    //-- Get mouse position:
    //! \todo Change this for screen center?
    int initial_mouse_x, initial_mouse_y;
    getMousePos( initial_mouse_x, initial_mouse_y);

    //-- Initial state:
    kalmanFilterCenter.statePre.at<float>(0) = initial_mouse_x; //-- x Position
    kalmanFilterCenter.statePre.at<float>(1) = initial_mouse_y; //-- y Position
    kalmanFilterCenter.statePre.at<float>(2) = 0;		  //-- x Velocity
    kalmanFilterCenter.statePre.at<float>(3) = 0;		  //-. y Velocity

    //-- Set the rest of the matrices:
    cv::setIdentity( kalmanFilterCenter.measurementMatrix );
    cv::setIdentity( kalmanFilterCenter.processNoiseCov, cv::Scalar::all(0.0001));
    cv::setIdentity( kalmanFilterCenter.measurementNoiseCov, cv::Scalar::all(0.1));
    cv::setIdentity( kalmanFilterCenter.errorCovPost, cv::Scalar::all(0.1));
}



//-----------------------------------------------------------------------------------------------------------------------
//-- Refresh the detected hand characteristics
//-----------------------------------------------------------------------------------------------------------------------
void HandDescription::operator ()(const cv::Mat& src, const cv::Mat& skinMask )
{
    update ( src, skinMask );
}

void HandDescription::update(const cv::Mat& src, const cv::Mat& skinMask )
{
    //-- Do things to update each parameter
    contourExtraction( skinMask );

    //-- Check if some hand was found:
    if ( _hand_found )
    {
	boundingBoxExtraction(src);

//	angleExtraction();
//	centerExtraction();

//	gestureExtraction();
//	numFingersExtraction();
    }
}



//-----------------------------------------------------------------------------------------------------------------------
//-- Get the characteristics of the hand
//-----------------------------------------------------------------------------------------------------------------------

bool HandDescription::handFound()
{
    return _hand_found;
}

double HandDescription::getHandAngle ()
{
    return _hand_angle;
}

std::pair <int, int> HandDescription::getCenterHand ()
{
    return _hand_center;
}

std::vector< std::vector<cv::Point> > HandDescription::getContours()
{
    return _hand_contour;
}

cv::Rect HandDescription::getBoundingBox()
{
    return _hand_bounding_box;
}

int HandDescription::getGesture()
{
    return _hand_gesture;
}

int HandDescription::getNumFingers()
{
    return _hand_num_fingers;
}



//-----------------------------------------------------------------------------------------------------------------------
//-- Plot characteristics on some image
//-----------------------------------------------------------------------------------------------------------------------

//-- Draws the bounding rectangle of the ROI
void HandDescription::plotBoundingRectangle(const cv::Mat& src, cv::Mat& dst, bool rotated )
{
    //-- Allocate the display matrix if needed:
    if ( dst.total() == 0 )
	dst = src.clone();

    //-- Choose between rotated or std box:
    if ( rotated )
    {
	//-- Draw rotated rectangle:
	cv::Point2f rect_points[4]; _hand_rotated_bounding_box.points( rect_points );
	for( int j = 0; j < 4; j++ )
	    cv::line( dst, rect_points[j], rect_points[(j+1)%4], cv::Scalar(255, 0, 0) , 1, 8 );
    }
    else
    {
	cv::rectangle( dst, _hand_bounding_box, cv::Scalar( 255, 0, 255) );
    }
}


//-- Extracts the contour of the ROI and stores the hand's parameters in the private variables
void HandDescription::plotContours(const cv::Mat& src, cv::Mat& dst)
{
    //-- Allocate the display matrix if needed:
    if ( dst.total() == 0 )
	dst = src.clone();

    //-- Plot contours:
    if ( (int) _hand_contour.size() > 0 )
    {
	//-- Draw contours:
	cv::drawContours( dst, _hand_contour, 0, cv::Scalar( 0, 0, 255), 1, 8);
	//cv::fillConvexPoly( dst, contours[largestId], cv::Scalar( 255, 255, 255));
    }
    else
	std::cerr << "No contours found!" << std::endl;
}


void HandDescription::angleControl(bool show_corrected, bool show_actual, bool show_predicted)
{
    //-- Prints the angle gauge

    //-- Matrix that shows the gauge
    cv::Mat gauge = cv::Mat::zeros( 100, 200, CV_8UC3);

    //-- Define gauge:
    int gauge_l = 60;
    cv::Point gaugeOrigin( 200/2, 80 );

    //-- Calculate and print actual end:
    if ( show_actual )
    {
	double rad_ang = _hand_angle * 3.1415 / 180.0;
	int x_coord = gauge_l * cos( rad_ang);
	int y_coord = gauge_l * sin( rad_ang);
	cv::Point gaugeEnd( 200/2 + x_coord , 80 - y_coord);
	cv::line( gauge, gaugeOrigin, gaugeEnd, cv::Scalar( 255, 0, 0), 1); //-- Actual angle
    }

    //-- Calculate and print predicted end:
    if ( show_predicted)
    {
	double rad_ang_predicted = _hand_angle_prediction * 3.1415 / 180.0;
	int x_coord_predicted = gauge_l * cos( rad_ang_predicted);
	int y_coord_predicted = gauge_l * sin( rad_ang_predicted);
	cv::Point predictedGaugeEnd( 200/2 + x_coord_predicted , 80 - y_coord_predicted);
	cv::line( gauge, gaugeOrigin, predictedGaugeEnd, cv::Scalar( 0, 255, 0), 2); //-- Predicted angle
    }

    //-- Calculate and print estimated end:
    if ( show_corrected )
    {
	double rad_ang_estimated = _hand_angle_estimation * 3.1415 / 180.0;
	int x_coord_estimated = gauge_l * cos( rad_ang_estimated);
	int y_coord_estimated = gauge_l * sin( rad_ang_estimated);
	cv::Point estimatedGaugeEnd( 200/2 + x_coord_estimated , 80 - y_coord_estimated);
	cv::line( gauge, gaugeOrigin, estimatedGaugeEnd, cv::Scalar( 0, 0, 255)), 3; //-- Estimated angle
    }

    cv::imshow( "Gauge", gauge);

}

std::pair <int, int> HandDescription::getCenterHand (cv:: Mat frame)
{

    //-- Copy the frame to display to draw there the results
    cv:: Mat display;
    frame.copyTo(display);



    if ( (int) _hand_contour.size() > 0 )
    {
	//-- Get image dimensions:
	int imageWidth = frame.cols, imageHeight = frame.rows;
	//std::cout << "Captured image: " << imageWidth << " x " << imageHeight << std::endl;


//	//-- (Optional) Print predicted point on screen:
//	cv::circle( display, predictedPoint, 4, cv::Scalar( 0, 255, 0), 2 );

//	//-- (Optional) Print cog on screen:
//	cv::circle( display, cog, 5, cv::Scalar( 255, 0, 0), 2 );

//	//- (Optional) Print estimation on screen:
//	cv::circle( display, estimationPoint, 3, cv::Scalar( 0, 0, 255), 2 );

	//! \todo this s part of the plotting thing, not the actual center of the hand:
	//-- Get screen dimensions:
	int screenHeight, screenWidth;
	getDisplayDimensions( screenWidth, screenHeight);

	//-- Get new cursor position by mapping the points:
	int x, y;
	//x = cogX * screenWidth / imageWidth;
	//y = cogY * screenHeight / imageHeight;
//	x = estimationPoint.x * screenWidth / imageWidth;
//	y = estimationPoint.y * screenHeight / imageHeight;

	return std::pair<int, int> (x, y);
    }

}



//-----------------------------------------------------------------------------------------------------------------------
//-- Functions that extract characteristics:
//-----------------------------------------------------------------------------------------------------------------------

void HandDescription::contourExtraction(const cv::Mat& skinMask)
{
    //-- Extract skin contours:
    std::vector<std::vector<cv::Point> > raw_contours;
    cv::findContours(skinMask, raw_contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cv::Point(0,0) );

    //-- Filter the contours by size:
    filterContours( raw_contours, _hand_contour);

    //-- Set a flag in case no hands have been found:
    _hand_found = (int) _hand_contour.size() > 0 ;

}

void HandDescription::boundingBoxExtraction(const cv::Mat& src)
{
    //-- Extract minimal rectangle enclosing the hand:
    _hand_rotated_bounding_box = cv::minAreaRect( _hand_contour[0]);

    //-- Extract bounding box:
    _hand_bounding_box  =  cv::boundingRect( _hand_contour[0] );

    //-- Extract hand ROI:
    _hand_ROI = src( _hand_bounding_box ).clone();
    cv::imshow("[Debug] Hand", _hand_ROI);
}

void HandDescription::angleExtraction()
{
    //-- Create matrix for storing the measurement (measured position of hand)
    cv::Mat_<float> angleMeasurement(1, 1);
    angleMeasurement.setTo( cv::Scalar(0));

    //-- Predict angle with Kalman filter:
    cv::Mat anglePrediction = kalmanFilterAngle.predict();
    _hand_angle_prediction = anglePrediction.at<float>(0);

    //-- Measure actual angle:
    double newHandAngle = getAngle(_hand_rotated_bounding_box);
    _hand_angle = newHandAngle < 0 ? _hand_angle : newHandAngle;
    angleMeasurement(0) = _hand_angle;

    //-- Correct prediction:
    cv::Mat angleEstimation = kalmanFilterAngle.correct( angleMeasurement );
    _hand_angle_estimation = angleEstimation.at<float>(0);
}

void HandDescription::centerExtraction()
{
    //-- Create matrix for storing the measurement (measured position of hand)
    cv::Mat_<float> measurement(2, 1);
    measurement.setTo( cv::Scalar(0));

    //-- Predict next center position with kalman filter:
    cv::Mat prediction = kalmanFilterCenter.predict();
    _hand_center_prediction = std::pair <int, int> ( prediction.at<float>(0), prediction.at<float>(1) );

    //-- Measure actual point (uncomment the selected method):
    std::pair <int, int> cog;

    //-------------------- With RotatedRect --------------------------------------
//    cv::Point2f rect_points[4]; _hand_rotated_bounding_box.points( rect_points );
//    for( int j = 0; j < 4; j++ )
//    {
//	cog.first += rect_points[j].x;
//	cog.second += rect_points[j].y;
//    }

//    cog.first /= 4;
//    cog.second /= 4;


    //-------------------- With Bounding Rectangle --------------------------------
    //-- (This is more stable than the rotated rectangle)
    cog.first  = _hand_bounding_box.x + _hand_bounding_box.width  / 2;
    cog.second = _hand_bounding_box.y + _hand_bounding_box.height / 2;

    measurement(0) = cog.first;
    measurement(1) = cog.second;

    //-- Correct estimation:
    cv::Mat estimation = kalmanFilterCenter.correct( measurement);
    _hand_center_estimation = std::pair <int, int> ( estimation.at<float>(0), estimation.at<float>(1) );

}
