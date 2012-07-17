#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <gnuplot_i.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>

#include <esn.h>
#include <inv.h>
#include <esn_train.h>

using namespace std;

#define HARD_MACKEY_GLASS  	30
#define SOFT_MACKEY_GLASS	17

//! Adjust the input bias for the neurons
#define INPUT_BIAS			0.2

//! Number of trials (min = 2)
#define NOF_TRIALS			2

//! Define difficulty of the problem
#define MACKEY_GLASS_DIFFICULTY			SOFT_MACKEY_GLASS

/***************************************************************************
 *
 ***************************************************************************/

double mackeyglass_eq(double x_t, double x_t_minus_tau, double a, double b) {
	double x_dot = -b*x_t + a*x_t_minus_tau/(1 + pow(x_t_minus_tau,10));
	return x_dot;
}

double mackeyglass_rk4(double x_t, double x_t_minus_tau, double deltat, double a, double b) {
	double k1 = deltat*mackeyglass_eq(x_t,          x_t_minus_tau, a, b);
	double k2 = deltat*mackeyglass_eq(x_t+0.5*k1,   x_t_minus_tau, a, b);
	double k3 = deltat*mackeyglass_eq(x_t+0.5*k2,   x_t_minus_tau, a, b);
	double k4 = deltat*mackeyglass_eq(x_t+k3,       x_t_minus_tau, a, b);
	double x_t_plus_deltat = (x_t + k1/6 + k2/3 + k3/3 + k4/6);
	return x_t_plus_deltat;
}

/**
 * Generate a Mackey-Glass time series.
 */
void mackey(double *X, double *T, int sample_n) {
	double a        = 0.2;     // value for a in eq (1)
	double b        = 0.1;     // value for b in eq (1)
	int tau      	= MACKEY_GLASS_DIFFICULTY;		// delay constant in eq (1)
	double x0       = 1.2;		// initial condition: x(t=0)=x0
	double deltat   = 0.1;	    // time step size (which coincides with the integration step)
	int interval 	= 1;	    // output is printed at every 'interval' time steps


	double time = 0;
	int index = 1;
	int history_length = floor(tau/deltat);
	double x_history[history_length];
	for (int i = 0; i < x_history[i]; ++i) x_history[i] = 0.0;
	double x_t = x0;
	double x_t_minus_tau, x_t_plus_deltat;

	// Set every value to the default value
	for (int i = 0; i < sample_n; i++) {
		X[i] = x_t;

//		if ((i % interval == 0) && (i > 0)) {
//			printf("%f %f\n", T[i-1], X[i]);
//		}

		if (tau == 0)
			x_t_minus_tau = 0.0;
		else
			x_t_minus_tau = x_history[index];


		x_t_plus_deltat = mackeyglass_rk4(x_t, x_t_minus_tau, deltat, a, b);

		if (tau != 0) {
			x_history[index] = x_t_plus_deltat;
			index = (index % history_length)+1;
		}

		time = time + deltat;
		T[i] = time;
		x_t = x_t_plus_deltat;
	}
}

/***************************************************************************
 *
 ***************************************************************************/

void plot(double *x_axis, double *y_axis, int N, char *file) {
    gnuplot_ctrl * h = NULL;
    h = gnuplot_init() ;
    gnuplot_cmd(h, (char*)"set terminal png");
    char output[256];
    sprintf(output, "set output \"%s\"", file);
    gnuplot_cmd(h, output);
    gnuplot_setstyle(h, (char*)"lines") ;

    char title[100];
    sprintf(title, "A Mackey-Glass time serie");
    gnuplot_plot_xy( h, x_axis, y_axis, N, title);
    gnuplot_close(h) ;
}

template <typename T, typename U >
void convert(T* t, U* u, int len) {
	for (int i = 0; i < len; i++) {
		u[i] = (U)t[i];
	}
}

/**
 * The plotting function knows how to use gnuplot to visualize two lines in the same plot. Be aware
 * that this version of gnuplot_i.c has been adapted to serve this purpose. Gnuplot does only
 * accept doubles, that is the only reason that the values are converted in this function.
 * The function creates its own x-axis with numbers ranging from 0 till N.
 * @param f0		Array of floats for the first graph (will be converted to doubles)
 * @param f1		Array of floats for second graph
 * @param N			Length of both arrays
 * @param title0	Title/index for first graph
 * @param title1	Title/index for second graph
 */
void plot(float *f0, float *f1, int N, string title0, string title1, string file) {
	double T[N];
	double F0[N];
	double F1[N];

	// Fill arrays with doubles
	for (int i = 0; i < N; i++) T[i] = i;
	convert<float,double>(f0, F0, N);
	convert<float,double>(f1, F1, N);

	string file_cmd = "set output \"" + file + "\"";
    gnuplot_ctrl * h = NULL;
    h = gnuplot_init() ;
    gnuplot_cmd(h, (char*)"set terminal png");
    gnuplot_cmd(h, (char*)file_cmd.c_str());

    gnuplot_data data[2];
    data[0].n = N;
    data[0].x = T;
    data[0].y = F0;
    data[0].title = (char*)calloc(title0.length(), sizeof(char));
//    strcpy(data[0].pstyle, "dots");
    strcpy(data[0].pstyle, "lines");
    sprintf(data[0].title, "%s", title0.c_str());
    data[1].n = N;
    data[1].x = T;
    data[1].y = F1;
    data[1].title = (char*)calloc(title1.length(), sizeof(char));
    strcpy(data[1].pstyle, "lines");
    sprintf(data[1].title, "%s", title1.c_str());

    gnuplot_plot_xy_N(h, data, 2);
//    gnuplot_cmd(h, "pause -1"); // if on screen
    gnuplot_close(h) ;

    free (data[0].title);
    free (data[1].title);
}

/**
 * Simple test that does not use real input, but just faked input. It can used to easy
 * test if the matrices calculations are done correct. The indices need to show up
 * somehow.
 */
void test_regression() {
	cout << "Start test" << endl;
	ESNPrediction pred(2,0.5);
	int N = 6;
	float in[N]; float out[N]; out[0] = 0;
	for (int i = 0; i < N; i++) {
		in[i] = i;
		out[i] = i;
	}
	pred.AddTrial(in, out, N);
	pred.AddTrial(in, out, N);
	pred.AddTrial(in, out, N);
	pred.RunTrials();
}

/***************************************************************************
 *
 ***************************************************************************/

/**
 * Create and run an ESN. 
 */
int main(int argc, char*argv[]) {
//#define TEST

#ifdef TEST
	test_regression();
	return 1;
#endif

	int sample_all = 10000;	// total no. of samples, excluding the given initial condition
	assert (sample_all >= 2000); // if sample_n < 2000 then the time series is incorrect!!
	double M[sample_all];
	double T[sample_all];
	for (int i = 0; i < sample_all; ++i) M[i] = 0.0;
	for (int i = 0; i < sample_all; ++i) T[i] = 0.0;

	printf("Start mackey\n");
	mackey(M,T,sample_all);

	// Downsample (very important!)
	int down_sample = 10;
	int sample_n = sample_all / down_sample;

	// Normalize mackey to -1 1 using hyperbolic tangent
	double X[sample_n];
	for (int i = 0; i < sample_n; i++) {
		X[i] = tanh(M[i*down_sample] - 1.0);
	}

	int trial_len = 1000 / down_sample;
	int nof_trials = NOF_TRIALS; // test and training trials (minimum 2)
	srand(time(NULL));

	// Create input for the reservoir, just a bias of 0.2
	float bias[trial_len];
	for (int i = 0; i < trial_len; i++) {
		bias[i] = INPUT_BIAS;
	}

	// Make the array a bit larger so we can use additional values for forecasting
	float S[nof_trials][trial_len];
	int trial_id[nof_trials];

	int r = rand() % (sample_n - trial_len*2); // pick an index (max sample_n - size)

	for (int t = 0; t < nof_trials; t++) {
		trial_id[t] = r;
		for (int i = 0; i < trial_len; i++) {
			S[t][i] = (float)X[r+i];
		}
//		r+=1000;
	}

	int nof_neurons = 200, K = 20;
//	nof_neurons = 12; K = 2;
//	nof_neurons = 60; K = 8;
	float connectivity = K / (float)nof_neurons;

	connectivity = 0.1;
	ESNPrediction pred(nof_neurons, connectivity);

	for (int t = 0; t < nof_trials; t++) {
		pred.AddTrial(bias, S[t], trial_len, trial_id[t]);
	}

	pred.RunTrials();

	std::vector<Trial*> &testSet = pred.GetTestSet();
	if (testSet.empty()) {
		cout << "Test set is empty" << endl;
		return EXIT_SUCCESS;
	}

	assert (trial_len == testSet[0]->sampleSize);

	cout << "Trial length is " << trial_len << endl;

	float input[trial_len];
	float output[trial_len];

	pred.GetESN().saveESN("mackey_glass.esn");

	float states[trial_len*pred.GetESN().getReservoirSize()];

	for (int t = 0; t < testSet.size(); t++) {
		pred.RunTest(t, input, output, states);
		char file[64];
		sprintf(file, "graph%i.png", t);
		cout << "Plot result to file " << file << endl;
		string title0 = "A Mackey-Glass time serie";
		string title1 = "ESN prediction";
		plot(input, output, trial_len, title0, title1, file);
	}

	string ppm_file = "reservoir_state.ppm";
	FILE *stream;
	stream = fopen(ppm_file.c_str(), "wb" );
	fprintf( stream, "P6\n%d %d\n255\n", trial_len, pred.GetESN().getReservoirSize());

	int sections = 4; int nof_colors = sections*255;
	char rgb[3];
	for(int i = 0; i < pred.GetESN().getReservoirSize(); ++i) {
		for (int j = 0; j < trial_len; ++j) {
			int value = ((states[i+j*pred.GetESN().getReservoirSize()]) *nof_colors); //255*3=765, //255*4=1020
//			value = abs(value);
			if(value < 0) {
				rgb[0] = 0;
				rgb[1] = 0;
				rgb[2] = 0;
			} else
			if(value < 256)
			{
				rgb[0] = 0;
				rgb[1] = value; // 0 b is bluest, and up to g+b=cyan
				rgb[2] = 255;
			}
			else if(value < 511)
			{
				rgb[0] = 0;
				rgb[1] = 255;
				rgb[2] = 511 - value; // 255 is g+b=cyan, 511 g is greenest
			}
			else if (value < 766)
			{
				rgb[0] = value - 511;
				rgb[1] = 255; // 511 g is greenest, 765 is r+g=yellow
				rgb[2] = 0;
			}
			else if (value < 1021)
			{
				rgb[0] = 255;
				rgb[1] = 1020 - value; // 765 is r+g=yellow, 1020 is reddest
				rgb[2] = 0;
			}
			// 511 is r+g=yellow, 765 is reddest
			fwrite(&rgb[0], 1, 1, stream);
			fwrite(&rgb[1], 1, 1, stream);
			fwrite(&rgb[2], 1, 1, stream);
		}
	}
	fclose( stream );
}
