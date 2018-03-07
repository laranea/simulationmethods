
#ifndef SIMULATIONMETHODS_SIMULATOR_H
#define SIMULATIONMETHODS_SIMULATOR_H

#include "Path.h"
#include "Random.h"

using namespace std;

/**
 * The job of the Simulator is generate the simulated paths.
 */
class Simulator {
protected:
    Random &m_Rng;
    bool m_Antithetic;
public:
    Simulator(Random &rng, bool antithetic = false): m_Rng(rng), m_Antithetic(antithetic) {}
    vector<Path> simulate(Model &model, int simulations, int path_size = 1) {
        vector<Path> sims;
        for (int i = 0; i < simulations; ++i) {
            sims.push_back(Path(model, m_Rng.generate(path_size), m_Antithetic));
        }
        return sims;
    }
};


#endif //SIMULATIONMETHODS_SIMULATOR_H