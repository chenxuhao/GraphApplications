// Marathon Match - CCL - Directional Propagation Labelling

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <list>
#include <algorithm>
#include <utility>
#include <cmath>
#include <functional>
#include <cstring>
#include <cmath>
#include <limits>

#define NOMINMAX

#ifdef _MSC_VER
#include <ctime>
inline double get_time()
{
        return static_cast<double>(std::clock()) / CLOCKS_PER_SEC;
}
#else
#include <sys/time.h>
inline double get_time()
{
        timeval tv;
        gettimeofday(&tv, 0);
        return tv.tv_sec + 1e-6 * tv.tv_usec;
}
#endif

using namespace std;

void init_CCL(int L[], int N)
{
	for (int id = 0; id < N; id++) L[id] = id;
}

inline int diff(int d1, int d2)
{
	return abs(((d1>>16) & 0xff) - ((d2>>16) & 0xff)) + abs(((d1>>8) & 0xff) - ((d2>>8) & 0xff)) + abs((d1 & 0xff) - (d2 & 0xff));
}

void kernel(int I, int D[], int L[], bool& m, int N, int W, int th)
{
	int H = N / W;
	int M = (I & 1) ? H : W;

	for (int id = 0; id < M; id++) {
		int S, E, step;
		switch (I) {
		case 0:
			S = id;
			E = W * (H - 1) + id;
			step = W;
			break;
		case 1:
			S = id * W;
			E = S + W - 1;
			step = 1;
			break;
		case 2:
			S = W * (H - 1) + id;
			E = id;
			step = -W;
			break;
		case 3:
			S = (id + 1) * W - 1;
			E = id * W;
			step = -1;
			break;
		}

		int label = L[S];
		for (int n = S + step; n != E + step; n += step) {
			if (diff(D[n], D[n-step]) <= th && label < L[n]) {
				L[n] = label;
				m = true;
			} else label = L[n];
		}
	}
}

void kernel8(int I, int D[], int L[], bool& m, int N, int W, int th)
{
	int H = N / W;
	int M = W + H - 1;

	for (int id = 0; id < M; id++) {
		int S, E1, E2, step;
		switch (I) {
		case 0:
			if (id < W) S = id;
			else S = (id - W + 1) * W;
			E1 = W - 1; // % W
			E2 = H - 1; // / W
			step = W + 1;
			break;
		case 1:
			if (id < W) S = W * (H - 1) + id;
			else S = (id - W + 1) * W;
			E1 = W - 1; // % W
			E2 = 0; // / W
			step = -W + 1;
			break;
		case 2:
			if (id < W) S = W * (H - 1) + id;
			else S = (id - W) * W + W - 1;
			E1 = 0; // % W
			E2 = 0; // / W
			step = -(W + 1);
			break;
		case 3:
			if (id < W) S = id;
			else S = (id - W + 1) * W + W - 1;
			E1 = 0; // % W
			E2 = H - 1; // / W
			step = W - 1;
			break;
		}

		if (E1 == S % W || E2 == S / W) continue;
		int label = L[S];
		for (int n = S + step;; n += step) {
			if (diff(D[n], D[n-step]) <= th && label < L[n]) {
				L[n] = label;
				m = true;
			} else label = L[n];
			if (E1 == n % W || E2 == n / W) break;
		}
	}
}

class CCL {
public:
	vector<int> cuda_ccl(vector<int>& image, int W, int degree_of_connectivity, int threshold);
};

vector<int> CCL::cuda_ccl(vector<int>& image, int W, int degree_of_connectivity, int threshold)
{
	int* D = static_cast<int*>(&image[0]);
	int N = image.size();
	int* L = new int[N];

	init_CCL(L, N);

	for (bool m = true; m;) {
		m = false;
		for (int i = 0; i < 4; i++) {
			kernel(i, D, L, m, N, W, threshold);
			if (degree_of_connectivity == 8) kernel8(i, D, L, m, N, W, threshold);
		}
	}

	vector<int> result(L, L + N);

	delete [] L;

	return result;
}

void read_data(const string filename, vector<int>& image, int& W, int& degree_of_connectivity, int& threshold)
{
	fstream fs(filename.c_str(), ios_base::in);
	string line;
	stringstream ss;
	int data;

	getline(fs, line);
	ss.str(line);
	ss >> W >> degree_of_connectivity >> threshold;
	getline(fs, line);
	ss.str("");  ss.clear();
	for (ss.str(line); ss >> data; image.push_back(data));
}

int main(int argc, char* argv[])
{
	ios_base::sync_with_stdio(false);

	if (argc < 2) {
		cerr << "Usage: " << argv[0] << " input_file" << endl;
		exit(1);
	}

	vector<int> image;
	int W, degree_of_connectivity, threshold;
	read_data(argv[1], image, W, degree_of_connectivity, threshold);

	CCL ccl;

	double start = get_time();
	vector<int> result(ccl.cuda_ccl(image, W, degree_of_connectivity, threshold));
	double end = get_time();
	cerr << "Time: " << end - start << endl;

	cout << result.size() << endl; /// number of pixels
	cout << W << endl; /// width
	for (int i = 0; i < static_cast<int>(result.size()) / W; i++) {
		for (int j = 0; j < W; j++) cout << result[i*W+j] << " ";
		cout << endl;
	}

	return 0;
}
