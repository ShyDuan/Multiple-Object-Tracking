#pragma once

#include <iostream>
#include <Eigen/Dense>

// kalman 滤波器的抽象类
// 实现可以是 KF/EKF/UKF...
class KalmanFilter {
public:
    /**
     * 用户需要定义H矩阵和R矩阵
     * @param num_states
     * @param num_obs
     */
    // 构造函数、析构函数
    explicit KalmanFilter(unsigned int num_states, unsigned int num_obs);
    virtual ~KalmanFilter() = default;

    /**
     * Coast state and state covariance using the process model
     * User can use this function without change the internal
     * tracking state x_
     */
    virtual void Coast();

    /**
     * Predict without measurement update
     */
    void Predict();

    /**
     * This function maps the true state space into the observed space
     * using the observation model
     * User can implement their own method for more complicated models
     */
    virtual Eigen::VectorXd PredictionToObservation(const Eigen::VectorXd &state);

    /**
     * Updates the state by using Extended Kalman Filter equations
     * @param z The measurement at k+1
     */
    virtual void Update(const Eigen::VectorXd &z);

    /**
     * Calculate marginal log-likelihood to evaluate different parameter choices
     */
    float CalculateLogLikelihood(const Eigen::VectorXd& y, const Eigen::MatrixXd& S);

    // 状态向量
    Eigen::VectorXd x_, x_predict_;

    // 误差协方差矩阵
    Eigen::MatrixXd P_, P_predict_;

    // 状态转移矩阵
    Eigen::MatrixXd F_;

    // 过程噪声协方差矩阵
    Eigen::MatrixXd Q_;

    // 测量矩阵
    Eigen::MatrixXd H_;

    // 观测噪声协方差矩阵
    Eigen::MatrixXd R_;

    unsigned int num_states_, num_obs_;

    float log_likelihood_delta_;

    float NIS_;
};
