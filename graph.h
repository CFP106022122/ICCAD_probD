#include <iostream>
#include <vector>

using namespace std;

class Graph {
private:
	constexpr static const int MAX_N = 105;
	int n;
	struct edge {
		int from, to;
		double weight;
		edge(int u, int v, double w): from{u}, to{v}, weight{w} { }
	};
	vector<edge> g[MAX_N];
public:
	Graph(int _n): n{_n} { }
	
	void add_edge(int u, int v, double w) {
		g[u].push_back(edge(u, v, w));
	}	

	void show() {
		for (int i=1; i<=n; ++i) {
			cout << "macro " << i << "'s neighbors:\n";
			for (auto e:g[i]) {
				cout << "\tmacro " << e.to << " with weight " << e.weight << endl;
			}
			cout << endl;
		}
	}

};
