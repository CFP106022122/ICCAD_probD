#include "graph.h"

int main()
{
    int V = 15;
    Graph g(V);
    g.add_edge('a' - 'a', 'b' - 'a', 1);
    g.add_edge('a' - 'a', 'd' - 'a', 1);
    g.add_edge('a' - 'a', 'e' - 'a', 1);
    g.add_edge('a' - 'a', 'i' - 'a', 1);
    g.add_edge('a' - 'a', 'f' - 'a', 1);

    g.add_edge('b' - 'a', 'c' - 'a', 1);
    g.add_edge('b' - 'a', 'j' - 'a', 1);
    g.add_edge('b' - 'a', 'g' - 'a', 1);

    g.add_edge('c' - 'a', 'k' - 'a', 1);
    g.add_edge('c' - 'a', 'j' - 'a', 1);

    g.add_edge('d' - 'a', 'j' - 'a', 1);

    g.add_edge('e' - 'a', 'd' - 'a', 1);
    g.add_edge('e' - 'a', 'n' - 'a', 1);

    g.add_edge('f' - 'a', 'h' - 'a', 1);

    g.add_edge('g' - 'a', 'j' - 'a', 1);
    g.add_edge('g' - 'a', 'o' - 'a', 1);

    g.add_edge('h' - 'a', 'i' - 'a', 1);
    g.add_edge('h' - 'a', 'n' - 'a', 1);

    g.add_edge('i' - 'a', 'j' - 'a', 1);
    g.add_edge('i' - 'a', 'm' - 'a', 1);

    g.add_edge('j' - 'a', 'k' - 'a', 1);
    g.add_edge('j' - 'a', 'l' - 'a', 1);
    g.add_edge('j' - 'a', 'm' - 'a', 1);
    g.add_edge('j' - 'a', 'n' - 'a', 1);

    g.add_edge('k' - 'a', 'o' - 'a', 1);
    g.add_edge('l' - 'a', 'o' - 'a', 1);
    g.add_edge('m' - 'a', 'o' - 'a', 1);
    g.add_edge('n' - 'a', 'o' - 'a', 1);

    g.transitive_reduction();
    g.show();
}