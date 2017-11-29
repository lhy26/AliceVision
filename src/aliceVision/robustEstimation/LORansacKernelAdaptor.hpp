// This file is part of the AliceVision project.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <aliceVision/robustEstimation/ACRansacKernelAdaptator.hpp>
#include <aliceVision/numeric/numeric.hpp>
#include <aliceVision/multiview/conditioning.hpp>

namespace aliceVision {
namespace robustEstimation {


/**
 * @brief A generic kernel used for the LORANSAC framework.
 *
 * @tparam minSample The minimum number of samples that allows to solve the problem
 * @tparam minSampleLS The minimum number of samples that allows to solve the problem in a least squared manner.
 * @tparam Model The class representing the model to estimate.
 */
template <int minSample,
          int minSampleLS,
          typename Model>
class LORansacGenericKernel
{
  public:
  
  enum
  {
    MINIMUM_SAMPLES = minSample,
    MINIMUM_LSSAMPLES = minSampleLS
  };

  /**
   * @brief The constructor;
   */
  LORansacGenericKernel() = default;

  /**
   * @brief This function is called to estimate the model from the minimum number
   * of sample \p minSample (i.e. minimal problem solver).
   * @param[in] samples A vector containing the indices of the data to be used for
   * the minimal estimation.
   * @param[out] models The model(s) estimated by the minimal solver.
   */
  virtual void Fit(const std::vector<std::size_t> &samples, std::vector<Model> *models) const = 0;

  /**
   * @brief This function is called to estimate the model using a least squared
   * algorithm from a minumum of \p minSampleLS.
   * @param[in] inliers An array containing the indices of the data to use.
   * @param[out] models The model(s) estimated using the least squared algorithm.
   * @param[in] weights An optional array of weights, one for each sample
   */
  virtual void FitLS(const std::vector<std::size_t> &inliers, 
                      std::vector<Model> *models, 
                      const std::vector<double> *weights = nullptr) const = 0; 

  /**
   * @brief Function used to estimate the weights, typically used by the least square algorithm.
   * @param[in] model The model against which the weights are computed.
   * @param[in] inliers The array of the indices of the data to be used.
   * @param[out] vec_weights The array of weight of the same size as \p inliers.
   * @param[in] eps An optional threshold to max out the value of the threshold (typically
   * to avoid division by zero or too small numbers).
   */
  virtual void computeWeights(const Model & model, 
                              const std::vector<std::size_t> &inliers, 
                              std::vector<double> & vec_weights, 
                              const double eps = 0.001) const = 0; 

  /**
   * @brief Function that computes the estimation error for a given model and a given element.
   * @param[in] sample The index of the element for which the error is computed.
   * @param[in] model The model to consider.
   * @return The estimation error for the given element and the given model.
   */
  virtual double Error(std::size_t sample, const Model &model) const = 0;

  /**
   * @brief Function that computes the estimation error for a given model and all the elements.
   * @param[in] model The model to consider.
   * @param[out] vec_errors The vector containing all the estimation errors for every element.
   */
  virtual void Errors(const Model & model, std::vector<double> & vec_errors) const = 0;

  /**
   * @brief Function used to unnormalize the model.
   * @param[in,out] model The model to unnormalize.
   */
  virtual  void Unnormalize(Model * model) const = 0;

  /**
   * @brief The number of elements in the data.
   * @return the number of elements in the data.
   */
  virtual std::size_t NumSamples() const = 0; 

  /**
   * @brief The destructor.
   */
  virtual ~LORansacGenericKernel( ) = default;

};

/**
 * @brief The generic kernel to be used for LORansac framework.
 * 
 * @tparam SolverArg The minimal solver able to find a solution from a
 * minimum set of points.
 * @tparam ErrorArg The functor computing the error for each data sample with
 * respect to the estimated model.
 * @tparam UnnormalizerArg The functor used to normalize the data before the 
 * estimation of the model.
 * @tparam ModelArg = Mat3 The type of the model to estimate.
 * @tparam SolverLSArg = SolverArg The least square solver that is used to find
 * a solution from any set of data larger than the minimum required.
 */
template <typename SolverArg,
  typename ErrorArg,
  typename UnnormalizerArg,
  typename ModelArg = Mat3,
  typename SolverLSArg = SolverArg>
class KernelAdaptorLoRansac : public ACKernelAdaptor<SolverArg, ErrorArg, UnnormalizerArg, ModelArg>
{
public:
  typedef SolverArg Solver;
  typedef ModelArg Model;
  typedef ErrorArg ErrorT;
  typedef SolverLSArg SolverLS;
  
  enum
  {
    MINIMUM_SAMPLES = Solver::MINIMUM_SAMPLES,
    MINIMUM_LSSAMPLES = SolverLS::MINIMUM_SAMPLES
  };

  KernelAdaptorLoRansac(const Mat &x1, int w1, int h1,
                        const Mat &x2, int w2, int h2, 
                        bool bPointToLine = true) 
          : ACKernelAdaptor<SolverArg, ErrorArg, UnnormalizerArg, ModelArg>(x1, w1, h1, x2, w2, h2, bPointToLine)
  {
  }
  
  void FitLS(const std::vector<std::size_t> &inliers, 
              std::vector<Model> *models, 
              const std::vector<double> *weights = nullptr) const
  {
    const Mat x1 = ExtractColumns(this->x1_, inliers);
    const Mat x2 = ExtractColumns(this->x2_, inliers);
    SolverLS::Solve(x1, x2, models, weights);
  }
  
  /**
   * @brief Given a model and the associated inliers, it computes the weight for
   * each inlier as the squared inverse of the associated error.
   * 
   * @param[in] model The model to against which to compute the weights.
   * @param[in] inliers The inliers associated to the model.
   * @param[out] vec_weights The weights associated to each inlier.
   * @param[in] eps Each inlier having an error below this value will be assigned
   * a weight of 1/eps^2 (to avoid division by zero).
   */
  void computeWeights(const Model & model, 
                      const std::vector<std::size_t> &inliers, 
                      std::vector<double> & vec_weights, 
                      const double eps = 0.001) const
  {
    const auto numInliers = inliers.size();
    vec_weights.resize(numInliers);
    for(std::size_t sample = 0; sample < numInliers; ++sample)
    {
      const auto idx = inliers[sample];
      vec_weights[sample] = ErrorT::Error(model, this->x1_.col(idx), this->x2_.col(idx));
      // avoid division by zero
      vec_weights[sample] = 1.0 / std::pow(std::max(eps, vec_weights[sample]), 2.0);
    }
  }

};

/**
 * @brief The kernel for the resection with known intrinsics (PnP) to be used with
 * the LORansac framework.
 * 
 * @tparam SolverArg The minimal solver able to find a solution from a
 * minimum set of points, usually any PnP solver.
 * @tparam ErrorArg The functor computing the error for each data sample with
 * respect to the estimated model, usually a reprojection error functor.
 * @tparam UnnormalizerArg The functor used to normalize the data before the 
 * estimation of the model, usually a functor that normalize the point in camera
 * coordinates (ie multiply by the inverse of the calibration matrix).
 * @tparam ModelArg = Mat34 The type of the model to estimate, the projection matrix.
 * @tparam SolverLSArg = SolverArg The least square solver that is used to find
 * a solution from any set of data larger than the minimum required, usually the 
 * 6 point algorithm which solves the resection problem by means of LS.
 */
template <typename SolverArg,
typename ErrorArg,
typename UnnormalizerArg,
typename SolverLSArg,
typename ModelArg = Mat34>
class KernelAdaptorResectionLORansac_K : 
      public ACKernelAdaptorResection_K<SolverArg, ErrorArg, UnnormalizerArg, ModelArg>
{
  public:
  typedef SolverArg Solver;
  typedef ModelArg Model;
  typedef ErrorArg ErrorT;
  typedef SolverLSArg SolverLS;
  
  enum
  {
    MINIMUM_SAMPLES = Solver::MINIMUM_SAMPLES,
    MINIMUM_LSSAMPLES = SolverLS::MINIMUM_SAMPLES
  };
  
  KernelAdaptorResectionLORansac_K(const Mat &x2d, const Mat &x3D, const Mat3 & K) : 
      ACKernelAdaptorResection_K<SolverArg, ErrorArg, UnnormalizerArg, ModelArg>(x2d, x3D, K)
  {}
  
  void FitLS(const std::vector<std::size_t> &inliers, 
              std::vector<Model> *models, 
              const std::vector<double> *weights = nullptr) const
  {
    const Mat x1 = ExtractColumns(this->x2d_, inliers);
    const Mat x2 = ExtractColumns(this->x3D_, inliers);
    SolverLS::Solve(x1, x2, models, weights);
  }
  
  void computeWeights(const Model & model, 
                      const std::vector<std::size_t> &inliers, 
                      std::vector<double> & vec_weights, 
                      const double eps = 0.001) const
  {
    const auto numInliers = inliers.size();
    vec_weights.resize(numInliers);
    for(std::size_t sample = 0; sample < numInliers; ++sample)
    {
      const auto idx = inliers[sample];
      vec_weights[sample] = ErrorT::Error(model, this->x2d_.col(idx), this->x3D_.col(idx));
      // avoid division by zero
      vec_weights[sample] = 1/std::pow(std::max(eps, vec_weights[sample]), 2);
    }
  }
};




} // namespace robustEstimation
} // namespace aliceVision
