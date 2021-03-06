
#ifndef SIMULATIONMETHODS_MCMODEL_H
#define SIMULATIONMETHODS_MCMODEL_H

#include "ModelResult.h"
#include "Model.h"
#include "Simulator.h"
#include "SensitivityModel.h"

#include <vector>
#include <ctime>
#include <algorithm>
#include <numeric>

using namespace std;

template<class OPTION>
class MCModel: public Model<OPTION> {
    using Model<OPTION>::m_Option;
    using Model<OPTION>::m_S0;
    using Model<OPTION>::m_Sigma;
    using Model<OPTION>::discount;
    using Model<OPTION>::m_h;
    using Model<OPTION>::m_r;
    using Model<OPTION>::m_Solver;
protected:
    vector<Path> simulation_vector;

    function<const double (const Path&)> control_variate;
    double control_variate_mean = 0.0, control_variate_beta = 1.0;

    SensitivityMethod m_SensitivityMethod = SensitivityMethod::FiniteDifference;
    double drift(double t, Bump bump) {
        double price_bump = 1.0, sigma_bump = 1.0;
        switch(bump) {
            case Price_Up:
                price_bump = (1.0+m_h);
                break;
            case Price_Down:
                price_bump = (1.0-m_h);
                break;
            case Sigma_Up:
                sigma_bump = (1.0+m_h);
                break;
            case Sigma_Down:
                sigma_bump = (1.0-m_h);
                break;
            case None:
                break;
        }
        double sigma = m_Sigma * sigma_bump;
        return m_r - sigma * sigma;
    }
    double diffusion(double t, Bump bump) {
        double price_bump = 1.0, sigma_bump = 1.0;
        switch(bump) {
            case Price_Up:
                price_bump = (1.0+m_h);
                break;
            case Price_Down:
                price_bump = (1.0-m_h);
                break;
            case Sigma_Up:
                sigma_bump = (1.0+m_h);
                break;
            case Sigma_Down:
                sigma_bump = (1.0-m_h);
                break;
            case None:
                break;
        }
        return m_Sigma * sqrt(t) * sigma_bump;
    }
public:
    MCModel(OPTION &option, double S0, double sigma, double r, double h = 0.01, SDESolver sdeSolver = Explicit): Model<OPTION>(option, S0, sigma, r) {
        this->m_h = h;
        this->m_Solver = sdeSolver;
        this->m_dim = option.getDim();
    }

    void define_control_variate(function<const double (const Path&)> control_variate, double control_variate_mean, double beta = 1.0) {
        this->control_variate = control_variate;
        this->control_variate_mean = control_variate_mean;
        this->control_variate_beta = beta;
    }

    ModelResult simulate(Simulator simulator, SensitivityModel<OPTION> &sensitivityModel, int simulations, int path_size = 1) {
        clock_t start = clock();
        this->simulation_vector.clear();
        this->simulation_vector = simulator.simulate(*this, simulations, path_size);
        auto price = this->calcPrice();
        auto delta = this->calcDelta(sensitivityModel);
        auto gamma = this->calcGamma(sensitivityModel);
        auto vega  = this->calcVega(sensitivityModel);
        ModelResult result;
        result.setModelType(ModelType::MonteCarlo);
        result.setAntitheticVariate(simulator.is_Antithetic());
        result.setControlVariate(control_variate?true:false);
        result.setSimulations(simulations);
        result.setGreeksMethod(sensitivityModel.getSensitivityMethod());
        result.setPrice(price.first);
        result.setPriceVariance(price.second);
        result.setDelta(delta.first);
        result.setDeltaVariance(delta.second);
        result.setGamma(gamma.first);
        result.setGammaVariance(gamma.second);
        result.setVega(vega.first);
        result.setVegaVariance(vega.second);
        result.setCalcTime((std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000));
        return result;
    }

    pair<double,double> calcPrice() const override;
    pair<double,double> calcDelta() const override {
        Greeks_by_FD::CentralDifferencesSensitivityModel<OPTION> fd_method(*this,this->m_h);
        return calcDelta(fd_method);
    };
    pair<double,double> calcGamma() const override {
        Greeks_by_FD::CentralDifferencesSensitivityModel<OPTION> fd_method(*this,this->m_h);
        return calcGamma(fd_method);
    };
    pair<double,double> calcVega() const override {
        Greeks_by_FD::CentralDifferencesSensitivityModel<OPTION> fd_method(*this,this->m_h);
        return calcVega(fd_method);
    };
    pair<double,double> calcDelta(SensitivityModel<OPTION> &sensitivityModel) const;
    pair<double,double> calcGamma(SensitivityModel<OPTION> &sensitivityModel) const;
    pair<double,double> calcVega(SensitivityModel<OPTION> &sensitivityModel) const;
};

template<class OPTION> pair<double,double> MCModel<OPTION>::calcPrice() const {
    if(!control_variate) {
        double sum = 0.0, sum_of_squares = 0.0;
        for(auto path: simulation_vector) {
            double discounted = discount(m_Option.payoff(path));
            sum += discounted;
            sum_of_squares += discounted * discounted;
        }
        double size = simulation_vector.size();
        double estimate = size > 0 ? sum / size : NAN;
        double variance = ((sum_of_squares / size) - estimate * estimate) / size;
        return pair<double, double>(estimate, variance);
    } else {
        vector<double> payoffs, control_variates;
        transform(simulation_vector.begin(), simulation_vector.end(), back_inserter(control_variates), control_variate);

        double size = control_variates.size();

        for(int i = 0 ; i< simulation_vector.size(); i++) {
            payoffs.push_back(m_Option.payoff(simulation_vector[i], None) - control_variate_beta*control_variates[i]);
        }
        double sum = 0.0, sum_of_squares = 0.0;
        for (int i =0; i<payoffs.size(); i++) {
            double discounted = discount(payoffs[i]);
            sum += discounted;
            sum_of_squares += discounted * discounted;
        }
        double estimate = size > 0 ? sum / size : NAN;
        double variance = ((sum_of_squares / size) - estimate * estimate) / size;
        return pair<double, double>(estimate+control_variate_beta*control_variate_mean, variance);
    }
}

template<class OPTION>
pair<double,double> MCModel<OPTION>::calcDelta(SensitivityModel<OPTION> &sensitivityModel) const {
    double sum = 0.0, sum_of_squares = 0.0;
    double size = simulation_vector.size();
    double divisor = sensitivityModel.deltaDivisor();
    for (auto path : simulation_vector) {
        double greek = sensitivityModel.calcDelta(path);
        sum += greek;
        sum_of_squares += greek*greek/(divisor*divisor);
    }
    double estimate = (sum/size)/divisor;
    double variance = ((sum_of_squares / size) - estimate*estimate) / size;
    return pair<double,double>(size>0? estimate: NAN, variance);
};

template<class OPTION>
pair<double,double> MCModel<OPTION>::calcGamma(SensitivityModel<OPTION> &sensitivityModel) const {
    double sum = 0.0, sum_of_squares = 0.0;
    double size = simulation_vector.size();
    double divisor = sensitivityModel.gammaDivisor();
    for (auto path : simulation_vector) {
        double greek = sensitivityModel.calcGamma(path);
        sum += greek;
        sum_of_squares += greek*greek/(divisor*divisor);
    }
    double estimate = (sum/size)/divisor;
    double variance = ((sum_of_squares / size) - estimate*estimate) / size;
    return pair<double,double>(size>0? estimate: NAN, variance);
};

template<class OPTION>
pair<double,double> MCModel<OPTION>::calcVega(SensitivityModel<OPTION> &sensitivityModel) const {
    double sum = 0.0, sum_of_squares = 0.0;
    double size = simulation_vector.size();
    double divisor = sensitivityModel.vegaDivisor();
    for (auto path : simulation_vector) {
        double greek = sensitivityModel.calcVega(path);
        sum += greek;
        sum_of_squares += greek*greek/(divisor*divisor);
    }
    double estimate = (sum/size)/divisor;
    double variance = ((sum_of_squares / size) - estimate*estimate) / size;
    return pair<double,double>(size>0? estimate: NAN, variance);
};

#endif //SIMULATIONMETHODS_MCMODEL_H
