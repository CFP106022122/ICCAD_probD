#ifndef _CORNER_STITCH_H
#define	_CORNER_STITCH_H

#include "corner_stitch/utils/update.h"

double cost_evaluation(vector<Macro*>& macro, Plane* horizontal_plane, Plane* vertical_plane);
vector<int> invalid_check(vector<Macro*>& macro, Plane* horizontal_plane);
void fix_invalid(vector<Macro*>& macro, vector<int>& invalid_macros, Graph& Gh, Graph& Gv);
void improve_strategy1(vector<Macro*>& macro, vector<Macro*>& native_macro, Plane* horizontal_plane, Graph& Gh, Graph& Gv);
void improve_strategy2(vector<Macro*>& macro, Plane* horizontal_plane, Graph& Gh, Graph& Gv, int SA_COUNT);

#endif