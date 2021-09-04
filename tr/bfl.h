#ifndef BFL_FILE_H_
#define BFL_FILE_H_

#include <iostream>
#include <vector>
#include "../graph.h"
#define K 5
#define D (320 * K)


using namespace std;

struct node {
        vector<int> N_I, N_O;
        int vis;
        union {
                int L_in[K];
                int h_in;
        };
        union {
                int L_out[K];
                int h_out;
        };
        pair<int, int> L_interval;
};
vector<node> nodes;
int vis_cur, cur;

void read_graph(Graph& G) {
        nodes.resize(vertexNum);
        vector<edge>* edge_list = G.get_edge_list();
        for(int i = 0; i < vertexNum; i++){
                for(int j = 0; j < edge_list[i].size(); j++){
                        nodes[i].N_O.push_back(edge_list[i][j].to);
                        nodes[edge_list[i][j].to].N_I.push_back(i);
                }
        }
}

int h_in() {
        static int c = 0, r = rand();
        if (c >= (int) nodes.size() / D) {
                c = 0;
                r = rand();
        }
        c++;
        return r;
}

int h_out() {
        static int c = 0, r = rand();
        if (c >= (int) nodes.size() / D) {
                c = 0;
                r = rand();
        }
        c++;
        return r;
}

void dfs_in(node &u) {
        u.vis = vis_cur;

        if (u.N_I.empty()) {
                u.h_in = h_in() % (K * 32);
        } else {
                for (int i = 0; i < K; i++) {
                        u.L_in[i] = 0;
                }

                for (int i = 0; i < u.N_I.size(); i++) {
                        node &v = nodes[u.N_I[i]];
                        if (v.vis != vis_cur) {
                                dfs_in(v);
                        }
                        if (v.N_I.empty()) {
                                int hu = v.h_in;
                                u.L_in[(hu >> 5) % K] |= 1 << (hu & 31);
                        } else {
                                for (int j = 0; j < K; j++) {
                                        u.L_in[j] |= v.L_in[j];
                                }
                        }
                }

                int hu = h_in();
                u.L_in[(hu >> 5) % K] |= 1 << (hu & 31);
        }
}

void dfs_out(node &u) {
        u.vis = vis_cur;

        u.L_interval.first = cur++;

        if (u.N_O.empty()) {
                u.h_out = h_out() % (K * 32);
        } else {
                for (int i = 0; i < K; i++) {
                        u.L_out[i] = 0;
                }

                for (int i = 0; i < u.N_O.size(); i++) {
                        node &v = nodes[u.N_O[i]];
                        if (v.vis != vis_cur) {
                                dfs_out(v);
                        }
                        if (v.N_O.empty()) {
                                int hu = v.h_out;
                                u.L_out[(hu >> 5) % K] |= 1 << (hu & 31);
                        } else {
                                for (int j = 0; j < K; j++) {
                                        u.L_out[j] |= v.L_out[j];
                                }
                        }
                }

                int hu = h_out();
                u.L_out[(hu >> 5) % K] |= 1 << (hu & 31);
        }

        u.L_interval.second = cur;
}

void index_construction() {
        vis_cur++;
        for (int u = 0; u < nodes.size(); u++) {
                if (nodes[u].N_O.size() == 0) {
                        dfs_in(nodes[u]);
                }
        }
        vis_cur++;
        cur = 0;
        for (int u = 0; u < nodes.size(); u++) {
                if (nodes[u].N_I.size() == 0) {
                        dfs_out(nodes[u]);
                }
        }
        long long index_size = 0;
        for (int u = 0; u < nodes.size(); u++) {
                index_size +=
                                nodes[u].N_I.empty() ?
                                                sizeof(nodes[u].h_in) : sizeof(nodes[u].L_in);
                index_size +=
                                nodes[u].N_O.empty() ?
                                                sizeof(nodes[u].h_out) : sizeof(nodes[u].L_out);
                index_size += sizeof(nodes[u].L_interval);
        }
}

bool reach(node &u, node &v) {
        if (u.L_interval.second < v.L_interval.second) {
                return false;
        } else if (u.L_interval.first <= v.L_interval.first) {
                return true;
        }

        if (v.N_I.empty()) {
                return false;
        }
        if (u.N_O.empty()) {
                return false;
        }
        if (v.N_O.empty()) {
                if ((u.L_out[v.h_out >> 5] & (1 << (v.h_out & 31))) == 0) {
                        return false;
                }
        } else {
                for (int i = 0; i < K; i++) {
                        if ((u.L_out[i] & v.L_out[i]) != v.L_out[i]) {
                                return false;
                        }
                }
        }
        if (u.N_I.empty()) {
                if ((v.L_in[u.h_in >> 5] & (1 << (u.h_in & 31))) == 0) {
                        return false;
                }
        } else {
                for (int i = 0; i < K; i++) {
                        if ((u.L_in[i] & v.L_in[i]) != u.L_in[i]) {
                                return false;
                        }
                }
        }

        for (vector<int>::iterator it = u.N_O.begin(); it != u.N_O.end(); it++) {
                if (nodes[*it].vis != vis_cur) {
                        nodes[*it].vis = vis_cur;
                        if (reach(nodes[*it], v)) {
                                return true;
                        }
                }
        }

        return false;
}

void destroyLabel(){
        vector<node>().swap( nodes );
}

#endif