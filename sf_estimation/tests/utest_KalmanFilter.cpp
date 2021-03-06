#include <gtest/gtest.h>
#include <string>
#include <stdexcept>
#include <Eigen/Dense>

// testing following API
#include "estimation/IEstimator.h"
#include "estimation/KalmanFilter.h"
#include "estimation/Input.h"
#include "estimation/InputValue.h"
#include "estimation/Output.h"
#include "estimation/OutputValue.h"

using namespace std;
using namespace estimation;

namespace KalmanFilterTest 
{
  // -----------------------------------------
  // tests
  // -----------------------------------------
  TEST(KalmanFilterTest, initializationAndValidation)
  {
    KalmanFilter kf;

    // check setting required params
    EXPECT_THROW(kf.validate(), IEstimator::estimator_error);	// "STM missing"
    MatrixXd A_f(2,1); A_f << 1, 0;
    kf.setStateTransitionModel(A_f);
    EXPECT_THROW(kf.validate(), IEstimator::estimator_error);	// "OM missing"
    MatrixXd H(1,2); H << 1,0;
    kf.setObservationModel(H);
    EXPECT_THROW(kf.validate(), IEstimator::estimator_error);	// "PNC missing"
    MatrixXd Q(2,2); Q << 0.1,0,0,0.1;
    kf.setProcessNoiseCovariance(Q);
    EXPECT_THROW(kf.validate(), IEstimator::estimator_error);	// "MNC missing"
    MatrixXd R(1,1); R << 10;
    kf.setMeasurementNoiseCovariance(R);

    EXPECT_THROW(kf.validate(), IEstimator::estimator_error);	// "STM invalid size"
    MatrixXd A(2,2); A << 1,0,0,1;
    kf.setStateTransitionModel(A);
    EXPECT_NO_THROW(kf.validate());			// all required params given

    MatrixXd R_f(2,1); R_f << 10,1;
    kf.setMeasurementNoiseCovariance(R_f);
    EXPECT_THROW(kf.validate(), IEstimator::estimator_error);	// "MNC invalid size"
    kf.setMeasurementNoiseCovariance(R);
    MatrixXd H_f(3,1); H_f << 1,0,2;
    kf.setObservationModel(H_f);
    EXPECT_THROW(kf.validate(), IEstimator::estimator_error);	// "OM invalid size"
    kf.setObservationModel(H);

    // check setting optional params
    MatrixXd B_f(1,1); B_f << 0;
    kf.setControlInputModel(B_f);
    EXPECT_THROW(kf.validate(), IEstimator::estimator_error);	// "CIM invalid size"
    MatrixXd B(2,1); B << 0,0;
    kf.setControlInputModel(B);
    EXPECT_NO_THROW(kf.validate());
    
    VectorXd x(2); x << 0,1;
    kf.setInitialState(x);
    EXPECT_NO_THROW(kf.validate());
  
    // check initialization of output
    Output out = kf.getLastEstimate();
    OutputValue defaultOutVal;
    EXPECT_GT(out.size(), 0);
    EXPECT_DOUBLE_EQ(out.getValue(), defaultOutVal.getValue());
  }

  TEST(KalmanFilterTest, validationEffect)
  {  
    KalmanFilter kf;

    MatrixXd A(1,1); A << 1;
    kf.setStateTransitionModel(A);
    MatrixXd H(1,1); H << 1;
    kf.setObservationModel(H);
    MatrixXd Q(1,1); Q << 0.1;
    kf.setProcessNoiseCovariance(Q);
    MatrixXd R(1,1); R << 10;
    kf.setMeasurementNoiseCovariance(R);

    // validate has an effect?
    InputValue measurement(1);
    Input in(measurement);
    Output out;
  
    EXPECT_THROW(out = kf.estimate(in), IEstimator::estimator_error);	// "not yet validated"
  
    EXPECT_NO_THROW(kf.validate());

    // changing a parameter -> KF must be re-validated
    kf.setProcessNoiseCovariance(Q);
    EXPECT_THROW(out = kf.estimate(in), IEstimator::estimator_error);	// "not yet validated"
    EXPECT_NO_THROW(kf.validate());
  
    // KF should now be released and return an estimate (should not be
    // the default)
    EXPECT_NO_THROW(out = kf.estimate(in));
    EXPECT_GT(out.size(), 0);
    EXPECT_EQ(kf.getState().size(), out.size());
  }

  TEST(KalmanFilterTest, functionality)
  {
    KalmanFilter kf;

    MatrixXd A(1,1); A << 1;
    kf.setStateTransitionModel(A);
    MatrixXd H(1,1); H << 1;
    kf.setObservationModel(H);
    MatrixXd Q(1,1); Q << 0.1;
    kf.setProcessNoiseCovariance(Q);
    MatrixXd R(1,1); R << 10;
    kf.setMeasurementNoiseCovariance(R);

    kf.validate();
    
    // check if the calculation of an estimate is correct
    InputValue measurement(1);
    Input in(measurement);
    Output out = kf.estimate(in);
  
    EXPECT_EQ(out.size(), 1);
    EXPECT_NEAR(out[0].getValue(), 0.0099, 0.0001);
    EXPECT_NEAR(out[0].getVariance(), 0.0990, 0.0001);

    in[0].setValue(5);
    EXPECT_NO_THROW(out = kf.estimate(in));
  
    EXPECT_NEAR(out[0].getValue(), 0.1073, 0.0001);
    EXPECT_NEAR(out[0].getVariance(), 0.1951, 0.0001);

    // missing measurement
    InputValue missingValue;
    Input inMissing(missingValue);
    EXPECT_NO_THROW(out = kf.estimate(inMissing));
  
    EXPECT_NEAR(out[0].getValue(), 0.1073, 0.0001);
    EXPECT_NEAR(out[0].getVariance(), 0.2866, 0.0001);

    // another example ---------------------------------
    KalmanFilter kf2;
    MatrixXd A2(2,2); A2 << 1,0.1,0,1;
    kf2.setStateTransitionModel(A2);
    MatrixXd H2(1,2); H2 << 0,1;
    kf2.setObservationModel(H2);
    MatrixXd Q2(2,2); Q2 << 0.1,0,0,0.1;
    kf2.setProcessNoiseCovariance(Q2);
    MatrixXd R2(1,1); R2 << 10;
    kf2.setMeasurementNoiseCovariance(R2);

    kf2.validate();

    in[0].setValue(1);
    EXPECT_NO_THROW(out = kf2.estimate(in));
    EXPECT_EQ(out.size(), 2);
    
    EXPECT_NEAR(out[0].getValue(), 0.0, 0.0001);
    EXPECT_NEAR(out[0].getVariance(), 0.1, 0.0001);
    EXPECT_NEAR(out[1].getValue(), 0.0099, 0.0001);
    EXPECT_NEAR(out[1].getVariance(), 0.0990, 0.0001);

    // add control input
    MatrixXd B2(2,1); B2 << 0,1;
    kf2.setControlInputModel(B2);	// needs to be validated
    InputValue ctrl(0.5);
    Input in_ctrl(ctrl);
    kf2.setControlInput(in_ctrl);

    in[0].setValue(5);
    EXPECT_NO_THROW(kf2.validate());
    EXPECT_NO_THROW(out = kf2.estimate(in));
    
    EXPECT_NEAR(out[0].getValue(), 0.0053, 0.0001);
    EXPECT_NEAR(out[0].getVariance(), 0.20098, 0.0001);
    EXPECT_NEAR(out[1].getValue(), 0.5975, 0.0001);
    EXPECT_NEAR(out[1].getVariance(), 0.1951, 0.0001);

    // pass control input with invalid size
    in_ctrl.add(ctrl);	// -> u.size = 2

    // setting invalid control input throws exception
    EXPECT_THROW(kf2.setControlInput(in_ctrl), length_error);

    // changing control input model works
    MatrixXd B3(2,2); B3 << 0,0,0,0;
    EXPECT_NO_THROW(kf2.setControlInputModel(B3));
    EXPECT_ANY_THROW(out = kf2.estimate(in));		// not validated
    EXPECT_NO_THROW(kf2.validate()); 

    // missing measurement
    EXPECT_NO_THROW(out = kf2.estimate(inMissing));
  
    EXPECT_NEAR(out[0].getValue(), 0.0651, 0.0001);
    EXPECT_NEAR(out[0].getVariance(), 0.3048, 0.0001);
    EXPECT_NEAR(out[1].getValue(), 0.5975, 0.0001);
    EXPECT_NEAR(out[1].getVariance(), 0.2867, 0.0001);
  }
}
