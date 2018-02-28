//============================================================================
// Name        : Stimulation_GT.cpp
// Author      : Stanley
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <vector>
#include <string>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <chrono>
#include <random>
using namespace std;


//model independent string comparision
bool icompare_pred(unsigned char a, unsigned char b){
		return tolower(a) == tolower(b);
}
bool icompare(string const& a, string const& b){

		if (a.length()==b.length()) {
				return equal(b.begin(), b.end(),
													 a.begin(),icompare_pred);
		}
		else {
				return false;
		}
}

//normal random variable generator
vector<double> normal_generator(unsigned int n){
		double v1;
		double v2;
		double w;
		vector<double> v;
		for(unsigned int i=0;i<(n/2)+1;i++){
				v1 = 2.0*rand()/RAND_MAX -1;
				v2 = 2.0*rand()/RAND_MAX -1;
				w = pow(v1,2) +pow(v2,2);
				if(w<=1){
					v.push_back(sqrt(-2*log(w)/w)*v1);
					v.push_back(sqrt(-2*log(w)/w)*v2);
				}else{
					--i;
				}
			}
		//size adjustment
		if(v.size()>n+1){
			v.pop_back();
			v.pop_back();
		}else{
			v.pop_back();
		}

		return v;
	}

vector<double> generate_normal(int n = 1, double mean = 0.0, double var = 1.0) {
	int j;
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	normal_distribution<double> distribution(mean, var);
	vector<double> rand_numbers;
	for (j = 0; j < n; j = j + 1) {
		double random_number = distribution(generator);
		rand_numbers.push_back(random_number);
	}
	return rand_numbers;
}


void print(double d) {
	// basic print for double
	cout << d << endl;
}

void print(vector<double> &vec) {
	// basic print for vector
	for (unsigned int i = 0; i < vec.size(); i++) {
		cout << vec[i] << endl;
	}
}

void res_print(vector<double> res) {
	cout << "   estimate = " << res[0] << endl;
	cout << "   time     = " << res[1] << endl;
	cout << "   mean     = " << res[2] << endl;
	cout << "   variance = " << res[3] << endl;

}

double normalCDF(double value) {
	return 0.5 * erfc(-value / sqrt(2));
}

double normalPDF(double value) {
	return (1 / sqrt(2 * M_PI)) * exp(-0.5 * pow(value, 2));
}

double euler_increment(double &s, double &r, double &v, double &dt, double &dt_sqr, double &Z) {
    return r*s*dt + v*s*dt_sqr*Z;
}

double milstein_increment(double &s, double &r, double &v, double &dt, double &dt_sqr, double &Z) {
    return r*s*dt + v*s*dt_sqr*Z  + 0.5 * v * ( v * s ) * dt * ( Z*Z - 1 ) ;
}

class asian_option_geometric{
    double T; // terminal time
    unsigned int N; // number of time partitions
    double K; // strik
    bool is_call;
    
    // this function calculate pay off
    double pay_off(double g_ave){
        if ((is_call&&g_ave>K)) {
            return (g_ave - K);
        }
        else if (!is_call&&g_ave < K) {
            return -(g_ave - K);
        }
        return 0;
    }
    
    void MC_euler_pricing(double S0, double r,double v,unsigned int no_sims,vector<double>& res){
        // prameter initlisation
        clock_t c;
        double duration;
        double price = 0;
        double dt = (double)T/N;
        double dt_sqr = pow((double)T/N,0.5); // ADDED: Casting T to double
        double mean_sqr = 0;

        // run the simulation and use antithetic variance reduction
        for(unsigned int i = 0;i < no_sims;i++) {
            vector<double> z = normal_generator(N); //generate normal vector of size N
            double sum_u = 0;
            double sum_d = 0;
            double s_u = S0;
            double s_d = S0;

            for(unsigned int j=0;j<N;j++){
                s_u += r*s_u*dt + v*s_u*dt_sqr*z[j];
                s_d += r*s_u*dt + v*s_u*dt_sqr*-z[j];
                sum_u += log(s_u);
                sum_d += log(s_d);
            }
            // update price
            price += pay_off(exp(sum_u/(N+1)));
            price += pay_off(exp(sum_d/(N + 1)));
            mean_sqr += pow(pay_off(exp(sum_u / N)), 2);
            mean_sqr += pow(pay_off(exp(sum_d/ N)), 2);

        }
        // record time
        duration = (clock()-c)/(double)CLOCKS_PER_SEC;
        // return results
        // handle edge cases
        res.push_back((price/no_sims/2)*exp(-r*T)); // price
        res.push_back(duration); //time
        res.push_back(price/no_sims/2); //mean
        res.push_back((mean_sqr/no_sims/2)-pow((price/no_sims/2),2)); //variance

    }
    
    void MC_euler_pricing_non_anti(double S0, double r, double v, unsigned int no_sims, vector<double>& res) {
        // prameter initlisation
        clock_t c;
        double duration;
        double price = 0;
        double dt = (double)T / N;
        double dt_sqr = pow((double)T / N, 0.5); // ADDED: Casting T to double
        double mean_sqr = 0;
        
        // run the simulation, NO variance reduction
        for (unsigned int i = 0; i < no_sims; i++) {
            vector<double> z = normal_generator(N); //generate normal vector of size N
            double log_sum = 0;
            double s = S0;
            
            for (unsigned int j = 0;j < N;j++) {
                s += r * s*dt + v * s*dt_sqr*z[j];
                log_sum += log(s);
            }
            price += pay_off(exp(log_sum / N));
            mean_sqr += pow(pay_off(exp(log_sum / N)),2);
            
        }
        // record time
        duration = (clock() - c) / (double)CLOCKS_PER_SEC;
        // return results
        // handle edge cases
        res.push_back((price / no_sims)*exp(-r * T)); // price
        res.push_back(duration); //time
        res.push_back(price / no_sims); //mean
        res.push_back((mean_sqr / no_sims) - pow((price / no_sims), 2)); //variance
        
    }

    void MC_milstein_pricing(double S0, double r,double v,unsigned int no_sims,vector<double>& res){
        
        // prameter initlisation
        clock_t c;
        double duration;
        double price = 0;
        double dt = (double)T / N;
        double dt_sqr = pow((double)T / N, 0.5); // ADDED: Casting T to double
        double mean_sqr = 0;
        
        // run the simulation, NO variance reduction
        for (unsigned int i = 0; i < no_sims; i++) {
            vector<double> z = normal_generator(N); //generate normal vector of size N
            double log_sum = 0;
            double s = S0;
            
            for (unsigned int j = 0;j < N;j++) {
                s += r * s*dt + v * s*dt_sqr*z[j] + 0.5 * v * ( v * s ) * dt * ( z[j]*z[j] - 1 ) ;
                log_sum += log(s);
            }
            price += pay_off(exp(log_sum / N));
            mean_sqr += pow(pay_off(exp(log_sum / N )),2);
            
        }
        // record time
        duration = (clock() - c) / (double)CLOCKS_PER_SEC;
        // return results
        // handle edge cases
        res.push_back((price / no_sims)*exp(-r * T)); // price
        res.push_back(duration); //time
        res.push_back(price / no_sims); //mean
        res.push_back((mean_sqr / no_sims) - pow((price / no_sims), 2)); //variance
        
    }
	
	void analytic_solution_pricing(double S0, double r, double v, vector<double>& res){
		clock_t c;
		double t = 0;
		// simulate results
		double duration;
		c = clock();// start the timer

		double G_t = S0;
		double mu_bar = (r - v * v / 2) * pow(T - t, 2) / (2 * T);
		double sigma_bar = sqrt(v*v / (T*T) * pow(T - t, 3) / 3);
		double d2 = 1.0 / sigma_bar * (t / T * log(G_t) + (T - t) / T * log(S0) + mu_bar - log(K));
		double d1 = d2 + sigma_bar;

		double price = exp(-r * (T - t)) * (pow(G_t, t / T) * pow(S0, (T - t) / T) * exp(mu_bar + pow(sigma_bar, 2) / 2) * normalCDF(d1) - K*normalCDF(d2));

		// record time
		duration = (clock() - c) / (double)CLOCKS_PER_SEC;

		res.push_back(price);
		res.push_back(duration);
		res.push_back(price);
		res.push_back(0.0);
	}
    
    void MC_fd_delta(string method, double S0, double r, double v, int no_sims, vector<double> &res, double h) {
        
        clock_t c;
        double duration;
        c = clock();// start the timer
        
        double price1_sum = 0, price2_sum = 0, price1, price2, price_dff_sum = 0, price_dff_sqr_sum = 0;
        double dt = (double)T / N;
        double dt_sqr = pow((double)T / N, 0.5); // ADDED: Casting T to double
        double mean_sqr = 0;
        
        double delta, expectation, var;
        
        vector<double> res1,res2;
        
        // run the simulation
        if(icompare(method, "milstein") ) {
            // use milstein if requested
            for (unsigned int i = 0; i < no_sims; i++) {
                vector<double> z = normal_generator(N); //generate normal vector of size N, for both price1 AND price2
                double log_sum1 = 0, log_sum2 = 0;
                double s1 = S0 + h, s2 = S0 - h;
                
                for (unsigned int j = 0;j < N;j++) {
                    s1 += r * s1*dt + v * s1*dt_sqr*z[j] + 0.5 * v * ( v * s1 ) * dt * ( z[j]*z[j] - 1 ) ;
                    s2 += r * s2*dt + v * s2*dt_sqr*z[j] + 0.5 * v * ( v * s2 ) * dt * ( z[j]*z[j] - 1 ) ;
                    log_sum1 += log(s1);
                    log_sum2 += log(s2);
                }
                price_dff_sum += pay_off(exp(log_sum1 / N)) - pay_off(exp(log_sum2 / N));
                price_dff_sqr_sum += pow( pay_off(exp(log_sum1 / N)) - pay_off(exp(log_sum2 / N)) , 2);
                
            }
            delta = ( price_dff_sum / no_sims ) / (2*h);
            
            expectation = delta;
            
            var = price_dff_sqr_sum / ( no_sims * 4*h*h ) - pow( expectation, 2);
            
            // record time
            duration = (clock() - c) / (double)CLOCKS_PER_SEC;
            
        } else if(icompare(method, "euler")){
            
            // use milstein if requested
            for (unsigned int i = 0; i < no_sims; i++) {
                vector<double> z = normal_generator(N); //generate normal vector of size N, for both price1 AND price2
                double log_sum1 = 0, log_sum2 = 0;
                double s1 = S0 + h, s2 = S0 - h;
                
                for (unsigned int j = 0;j < N;j++) {
                    s1 += r * s1*dt + v * s1*dt_sqr*z[j];
                    s2 += r * s2*dt + v * s2*dt_sqr*z[j];
                    log_sum1 += log(s1);
                    log_sum2 += log(s2);
                }
                price_dff_sum += pay_off(exp(log_sum1 / N)) - pay_off(exp(log_sum2 / N));
                price_dff_sqr_sum += pow( pay_off(exp(log_sum1 / N)) - pay_off(exp(log_sum2 / N)) , 2);
            }
            
            delta = ( price_dff_sum / no_sims ) / (2*h);
            
            expectation = delta;
            
            var = price_dff_sqr_sum / ( no_sims * 2*h*h ) - pow( expectation, 2);
            
            // record time
            duration = (clock() - c) / (double)CLOCKS_PER_SEC;
            
        } else {
            // return ERROR
            cout << "ERROR: scheme method not recognized" << endl;
        }
        
        res.push_back( delta );
        res.push_back( duration );
        res.push_back( expectation );
        res.push_back( var );
    }
    
    void MC_pw_delta(string method, double S0, double r, double v, int no_sims, vector<double> &delta) {
        clock_t c;
        double duration;
        c = clock();// start the timer
        
        // prameter initlisation
        clock_t t;
        double price = 0;
        double dt = (double)T/N;
        double dt_sqr = pow((double)T/N,0.5); // ADDED: Casting T to double
        double mean_sqr = 0;
        double sum = 0;
        
        // run the simulation, NO variance reduction
        for (unsigned int i = 0; i < no_sims; i++) {
            vector<double> z = normal_generator(N); //generate normal vector of size N
            double log_sum = log(S0);
            double s = S0;
            
            for (unsigned int j = 0;j < N;j++) {
                s += r * s*dt + v * s*dt_sqr*z[j];
                log_sum += log(s);
            }
            double average = exp(log_sum / (N + 1));
            bool indicator = average > K;
            if( indicator ) {
                sum += average / S0;
                mean_sqr += pow( average / S0,2);
            }
        }
        // record time
        duration = (clock()-t)/(double)CLOCKS_PER_SEC;
        // return results
        // handle edge cases
        delta.push_back((sum/no_sims)); // price
        delta.push_back(duration); //time
        delta.push_back(sum/no_sims); //mean
        delta.push_back((mean_sqr/no_sims)-pow((sum/no_sims),2)); //variance
        
        
        // record time
        duration = (clock() - c) / (double)CLOCKS_PER_SEC;
        
        delta.push_back(0);
        delta.push_back(duration);
        
    }
    
    void analytic_solution_delta(double S0, double r,double sigma, vector<double> &delta) {
        clock_t c;
        double duration;
        c = clock(); // start the timer
        
        double t = 0;
        
        double G_t = S0;
        double mu_bar = (r - sigma * sigma / 2) * pow(T - t, 2) / (2 * T);
        double sigma_bar = sqrt(sigma*sigma / (T*T) * pow(T - t, 3) / 3);
        double d2 = 1.0 / sigma_bar * (t / T * log(G_t) + (T - t) / T * log(S0) + mu_bar - log(K));
        double d1 = d2 + sigma_bar;
        
        double delta_val = exp(mu_bar + pow(sigma_bar, 2) / 2)* normalCDF(d1) + exp(mu_bar + pow(sigma_bar, 2) / 2) / sigma_bar * normalPDF(d1) - K / (sigma_bar * S0) * normalPDF(d2);
        
        // record time
        duration = (clock() - c) / (double)CLOCKS_PER_SEC;
        
        delta.push_back( delta_val );
        delta.push_back( duration );
    }
    
    void MC_fd_vega(string method, double S0, double r, double v, int no_sims, vector<double> &res, double h) {
        
        clock_t c;
        double duration;
        c = clock();// start the timer
        
        double price1_sum = 0, price2_sum = 0, price1, price2, price_dff_sum = 0, price_dff_sqr_sum = 0;
        double dt = (double)T / N;
        double dt_sqr = pow((double)T / N, 0.5); // ADDED: Casting T to double
        double mean_sqr = 0;
        
        double vega, expectation, var;
        
        vector<double> res1,res2;
        
        // run the simulation
        if(icompare(method, "milstein") ) {
            // use milstein if requested
            for (unsigned int i = 0; i < no_sims; i++) {
                vector<double> z = normal_generator(N); //generate normal vector of size N, for both price1 AND price2
                double log_sum1 = 0, log_sum2 = 0;
                double s1 = S0, s2 = S0;
                double v1 = v + h, v2 = v - h;
                
                for (unsigned int j = 0;j < N;j++) {
                    s1 += r * s1*dt + v1 * s1*dt_sqr*z[j] + 0.5 * v1 * ( v1 * s1 ) * dt * ( z[j]*z[j] - 1 ) ;
                    s2 += r * s2*dt + v2 * s2*dt_sqr*z[j] + 0.5 * v2 * ( v2 * s2 ) * dt * ( z[j]*z[j] - 1 ) ;
                    log_sum1 += log(s1);
                    log_sum2 += log(s2);
                }
                price_dff_sum += pay_off(exp(log_sum1 / N)) - pay_off(exp(log_sum2 / N));
                price_dff_sqr_sum += pow( pay_off(exp(log_sum1 / N)) - pay_off(exp(log_sum2 / N)) , 2);
                
            }
            vega = ( price_dff_sum / no_sims ) / (2*h);
            
            expectation = vega;
            
            var = price_dff_sqr_sum / ( no_sims * 4*h*h ) - pow( expectation, 2);
            
            // record time
            duration = (clock() - c) / (double)CLOCKS_PER_SEC;
            
        } else if(icompare(method, "euler")){
            
            // use milstein if requested
            for (unsigned int i = 0; i < no_sims; i++) {
                vector<double> z = normal_generator(N); //generate normal vector of size N, for both price1 AND price2
                double log_sum1 = 0, log_sum2 = 0;
                double s1 = S0, s2 = S0;
                double v1 = v + h, v2 = v - h;
                
                for (unsigned int j = 0;j < N;j++) {
                    s1 += r * s1*dt + v1 * s1*dt_sqr*z[j];
                    s2 += r * s2*dt + v2 * s2*dt_sqr*z[j];
                    log_sum1 += log(s1);
                    log_sum2 += log(s2);
                }
                price_dff_sum += pay_off(exp(log_sum1 / N)) - pay_off(exp(log_sum2 / N));
                price_dff_sqr_sum += pow( pay_off(exp(log_sum1 / N)) - pay_off(exp(log_sum2 / N)) , 2);
                
            }
            
            vega = ( price_dff_sum / no_sims ) / (2*h);
            
            expectation = vega;
            
            var = price_dff_sqr_sum / ( no_sims * 2*h*h ) - pow( expectation, 2);
            
            // record time
            duration = (clock() - c) / (double)CLOCKS_PER_SEC;
            
        } else {
            // return ERROR
            cout << "ERROR: scheme method not recognized" << endl;
        }
        
        res.push_back( vega );
        res.push_back( duration );
        res.push_back( expectation );
        res.push_back( var );
    }
    
    
    void analytic_solution_vega(double S0, double r, double sigma, vector<double> &vega) {
        clock_t c;
        double duration;
        c = clock(); // start the timer
        
        double t = 0;
    
        double G_t = S0;
        double mu_bar = (r - sigma * sigma / 2) * pow(T - t, 2) / (2 * T);
        double sigma_bar = sqrt(sigma*sigma / (T*T) * pow(T - t, 3) / 3);
        double d2 = 1.0 / sigma_bar * (t / T * log(G_t) + (T - t) / T * log(S0) + mu_bar - log(K));
        double d1 = d2 + sigma_bar;
        
        double d_mu_bar = -sigma*T / 2;
        double d_sigma_bar = sqrt(T/3);
        
        double d_d2 = ( d_mu_bar * sigma_bar - ( log(S0) + mu_bar - log(K) ) * d_sigma_bar ) / pow( sigma_bar, 2);
        double d_d1 = d_d2 + d_sigma_bar;
        
        double vega_val =  S0 * exp( mu_bar + pow(sigma_bar,2) / 2 ) * ( d_mu_bar + sigma_bar * d_sigma_bar ) * normalCDF( d1 )
                            + S0 * exp( mu_bar + pow(sigma_bar,2) / 2 ) * normalPDF( d1 ) * d_d1
                            - K * normalPDF( d2 ) * d_d2;
        
        // record time
        duration = (clock() - c) / (double)CLOCKS_PER_SEC;
        
        vega.push_back( vega_val );
        vega.push_back( duration );
        
    }
    
    void MC_fd_gamma(string method, double S0, double r, double v, int no_sims, vector<double> &res, double h) {
        
        clock_t c;
        double duration;
        c = clock();// start the timer
        
        double price_dff_sum = 0, price_dff_sqr_sum = 0, numerator;
        double dt = (double)T / N;
        double dt_sqr = pow((double)T / N, 0.5); // ADDED: Casting T to double
        double mean_sqr = 0;
        
        double res_val, expectation, var;
        
        vector<double> res1,res2;
        
        // run the simulation
        if(icompare(method, "milstein") ) {
            // use milstein if requested
            for (unsigned int i = 0; i < no_sims; i++) {
                vector<double> z = normal_generator(N); //generate normal vector of size N, for both price1 AND price2
                double log_sum1 = 0, log_sum2 = 0, log_sum3 = 0;
                double s1 = S0 + h, s2 = S0, s3 = S0 - h;
                
                for (unsigned int j = 0;j < N;j++) {
                    s1 += r * s1*dt + v * s1*dt_sqr*z[j] + 0.5 * v * ( v * s1 ) * dt * ( z[j]*z[j] - 1 ) ;
                    s2 += r * s2*dt + v * s2*dt_sqr*z[j] + 0.5 * v * ( v * s2 ) * dt * ( z[j]*z[j] - 1 ) ;
                    s3 += r * s3*dt + v * s3*dt_sqr*z[j] + 0.5 * v * ( v * s3 ) * dt * ( z[j]*z[j] - 1 ) ;
                    log_sum1 += log(s1);
                    log_sum2 += log(s2);
                    log_sum3 += log(s3);
                }
                numerator = pay_off(exp(log_sum1 / N)) - 2*pay_off(exp(log_sum2 / N)) + pay_off(exp(log_sum3 / N ));
                price_dff_sum += numerator;
                price_dff_sqr_sum += pow( numerator , 2);
                
            }
            res_val = ( price_dff_sum / no_sims ) / pow(h,2);
            
            expectation = res_val;
            
            var = price_dff_sqr_sum / ( no_sims * pow(h,4) ) - pow( expectation, 2);
            
            // record time
            duration = (clock() - c) / (double)CLOCKS_PER_SEC;
            
        } else if(icompare(method, "euler")){
            
            // use milstein if requested
            for (unsigned int i = 0; i < no_sims; i++) {
                vector<double> z = normal_generator(N); //generate normal vector of size N, for both price1 AND price2
                double log_sum1 = 0, log_sum2 = 0, log_sum3 = 0;
                double s1 = S0 + h, s2 = S0, s3 = S0 - h;
                
                for (unsigned int j = 0;j < N;j++) {
                    s1 += r * s1*dt + v * s1*dt_sqr*z[j];
                    s2 += r * s2*dt + v * s2*dt_sqr*z[j];
                    s3 += r * s3*dt + v * s3*dt_sqr*z[j];
                    log_sum1 += log(s1);
                    log_sum2 += log(s2);
                    log_sum3 += log(s3);
                }
                numerator = pay_off(exp(log_sum1 / N)) - 2*pay_off(exp(log_sum2 / N)) + pay_off(exp(log_sum3 / N ));
                price_dff_sum += numerator;
                price_dff_sqr_sum += pow( numerator , 2);
                
            }
            res_val = ( price_dff_sum / no_sims ) / pow(h,2);
            
            expectation = res_val;
            
            var = price_dff_sqr_sum / ( no_sims * pow(h,4) ) - pow( expectation, 2);
            
            // record time
            duration = (clock() - c) / (double)CLOCKS_PER_SEC;
            
        } else {
            // return ERROR
            cout << "ERROR: scheme method not recognized" << endl;
        }
        
        res.push_back( res_val );
        res.push_back( duration );
        res.push_back( expectation );
        res.push_back( var );
    }
    
    void analytic_solution_gamma(double S0, double r, double sigma, vector<double> &res) {
        clock_t c;
        double duration;
        c = clock(); // start the timer
        
        double t = 0;
        
        double G_t = S0;
        double mu_bar = (r - sigma * sigma / 2) * pow(T - t, 2) / (2 * T);
        double sigma_bar = sqrt(sigma*sigma / (T*T) * pow(T - t, 3) / 3);
        double d2 = 1.0 / sigma_bar * (t / T * log(G_t) + (T - t) / T * log(S0) + mu_bar - log(K));
        double d1 = d2 + sigma_bar;
        
        double d_d = 1.0 / ( sigma * S0 );
        
        double res_val =    exp( mu_bar + pow( sigma_bar,2 ) ) * normalPDF( d1 ) * d_d
                            + exp( mu_bar + pow(sigma_bar,2)) / sigma_bar  * ( -d1 ) * normalPDF(d1) * d_d
                            - ( -d2 * d_d * K * normalPDF(d2) * sigma_bar * S0 - K * normalPDF(d2) * sigma_bar ) / pow( sigma_bar * S0, 2);
        
        // record time
        duration = (clock() - c) / (double)CLOCKS_PER_SEC;
        
        res.push_back( res_val );
        res.push_back( duration );
        
    }

public :
    // an asian option has time to maturity T,K,initial price and interest rate
    asian_option_geometric(const string& type,double T,int N, double K):T(T),N(N),K(K){
        if(icompare(type,"call")){
            is_call = true;
        }
        else if(icompare(type,"put")){
            is_call = false;
        } else {
            // edge case: given comtract type known
        }
    }

    // this function calculate the option price at time 0 and analytic statistics
    vector<double> calculate_price(const string& method, double S0, double r, double v, int no_sims = 100000) {
        // return containeer
        vector<double> res;

        //Use selected method to calcuate c_0
        if (icompare(method, "euler")) {
            MC_euler_pricing_non_anti(S0, r, v, no_sims, res);
        } else if (icompare(method,"milstein")){
            MC_milstein_pricing(S0,r,v,no_sims, res);
        } else if (icompare(method,"analytic")){
            analytic_solution_pricing(S0, r, v, res);
        } else {
            // edge case: prcing method known
        }
        return res;
    }

    // this function calculate delta at time 0 and return analytic statistics
    vector<double> calculate_delta(string method, string method2, double S0, double r, double v, int no_sims = 100000, double h = 0.01) {
        // method2 is the user choice of between finite difference/ pathwise/ likelihood ratio
        vector<double> delta;
        
        if(icompare(method2, "fd")) {
            MC_fd_delta(method, S0, r, v, no_sims, delta, h);
        } else if(icompare(method2, "pw")) {
            MC_pw_delta(method, S0, r, v, no_sims, delta);
        } else if(icompare(method2, "analytic") ) {
            analytic_solution_delta(S0,r,v,delta);
        } else {
            // edge case:
        }
    
        return delta;
    };
    
	// this function calculate vega at time 0 and return analytic statistics
    vector<double> calculate_vega(string method, string method2, double S0, double r, double v, int no_sims = 100000, double h = 0.01) {
        vector<double> vega;
        
        if(icompare(method2, "fd")) {
            MC_fd_vega(method, S0, r, v, no_sims, vega, h );
        } else if(icompare(method2, "analytic")) {
            analytic_solution_vega(S0,r,v,vega);
        } else {
            // edge case:
        }
        return vega;
    };
    
    // this function calculate vega at time 0 and return analytic statistics
    vector<double> calculate_gamma(string method, string method2, double S0, double r, double v, int no_sims = 100000, double h = 0.01) {
        vector<double> gamma;
        
        if(icompare(method2, "fd")) {
            MC_fd_gamma(method, S0, r, v, no_sims, gamma, h );
        } else if(icompare(method2, "analytic")) {
            analytic_solution_gamma(S0,r,v,gamma);
        } else {
            // edge case:
        }
        return gamma;
    };
    
};

void hline(int n) {
    for(int i = 0; i < n; i++ ) {
        cout << "-";
    }
    cout << endl;
}

int main() {
	int T = 1;
	unsigned int N = 100;
	double K = 100;
	string method = "euler";
	string type = "call";
	double s0 = 100;
	double r = 0.05;
	double v = 0.4;
	int no_sims = 100000;
    double h = 0.1;
    vector<double> res, delta, price, vega,gamma;
    

	srand(time(NULL));
	asian_option_geometric opt(type,T,N,K);
    
    
    hline(20);
	cout << "Analytic: " << endl;
    hline(20);
    res = opt.calculate_price("analytic", s0, r, v);
    cout << "price = " << res[0] << endl;
    delta = opt.calculate_delta("analytic","analytic", s0, r,v);
    cout << "delta = " << delta[0] << endl;
    gamma = opt.calculate_gamma("analytic","analytic", s0, r, v);
    cout << "gamma = " << gamma[0] << endl;
    vega = opt.calculate_vega("analytic","analytic", s0, r, v);
    cout << "vega = " << vega[0] << endl;
	cout << endl;
    

    hline(20);
	cout << "Euler NA: " << "n = " << no_sims << endl;
    hline(20);
    cout << "price = " << endl << endl;
    price = opt.calculate_price("euler", s0, r, v, no_sims);
	res_print(price);
    cout << endl;
    
    cout << "delta_fd = " << endl << endl;
    delta = opt.calculate_delta("euler", "fd", s0, r,v , no_sims, h);
    res_print(delta);
    cout << endl;
    
    cout << "gamma_fd = " << endl << endl;
    delta = opt.calculate_gamma("euler", "fd", s0, r,v , no_sims, h);
    res_print(delta);
    cout << endl;
    
    vega = opt.calculate_vega("euler", "fd", s0, r, v, no_sims, h);
    cout << "vega_fd = " << endl << endl;
    res_print(vega);
    cout << endl;

    hline(20);
    cout << "Milstein: " << "n = " << no_sims << endl;
    hline(20);
    cout << "price = " << endl << endl;
    res = opt.calculate_price("milstein", s0, r, v, no_sims);
    res_print(res);
    cout << endl;
    
    delta = opt.calculate_delta("milstein", "fd", s0, r,v , no_sims, h);
    cout << "delta_fd = " << endl << endl;
    res_print(delta);
    cout << endl;
    
    cout << "gamma_fd = " << endl << endl;
    delta = opt.calculate_gamma("milstein", "fd", s0, r,v , no_sims, h);
    res_print(delta);
    cout << endl;
    
    vega = opt.calculate_vega("milstein", "fd", s0, r, v, no_sims, h);
    cout << "vega_fd = " << endl << endl;
    res_print(vega);
    cout << endl;
    
	int dummy;
	cin >> dummy;
	return 0;
}
