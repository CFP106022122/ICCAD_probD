#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <math.h>
#include "macro.h"
#include "graph.h"
#include "corner_stitch/utils/update.h"

extern double chip_width, chip_height, min_spacing, buffer_constraint, powerplan_width, alpha, beta;
extern int micron;

static int collectTiles(Tile *tile, ClientData cdata){
	((vector<Tile*>*) cdata)->push_back(tile);
	return 0;
}

static int collectSolidTiles(Tile *tile, ClientData cdata){
	if(TiGetBody(tile) == SOLID_TILE)
		((vector<Tile*>*) cdata)->push_back(tile);
	return 0;
}

static int buffer_constraint_check(Tile *tile, ClientData cdata){
	if(TiGetBody(tile) != SOLID_TILE){
		*((bool*)cdata) = true;
	}
	return 0;
}

static int collect_neighbor_macro(Tile *tile, ClientData cdata){
	if(TiGetBody(tile) == SOLID_TILE){
		((vector<int>*) cdata)->push_back(TiGetClient(tile));
	}
	return 0;
}

bool Horizontal_update(Plane* horizontal_plane, Plane* vertical_plane,
						Rect& horizontal_region, Rect& vertical_region, double& powerplan_cost){
	// Use boolean for checking if any tiles are inserted in this round
	bool updated = false;

	// Vector stores tiles(in horizontal corner stitch data structure) have overlap with placement region
	vector<Tile*> horizontal_tiles;

	// Find tiles which have overlap with placement region
	TiSrArea(NULL, horizontal_plane, &horizontal_region, collectTiles, (ClientData)&horizontal_tiles);

	for(int i = 0; i < horizontal_tiles.size(); i++){

		// If the tile is solid, means that it is macro or cost_area. We don't need to care if it would become cost_area.
		if(TiGetBody(horizontal_tiles[i]) == SOLID_TILE)
			continue;
		
		// Show this tile's position
		// printf("%d %d %d %d %d %d %d %d\n", LEFT(horizontal_tiles[i]), BOTTOM(horizontal_tiles[i]),
		// 									RIGHT(horizontal_tiles[i]), BOTTOM(horizontal_tiles[i]),
		// 									RIGHT(horizontal_tiles[i]), TOP(horizontal_tiles[i]),
		// 									LEFT(horizontal_tiles[i]), TOP(horizontal_tiles[i]));

		// Decides left, right boundary of tile
		double left = LEFT(horizontal_tiles[i]), right = RIGHT(horizontal_tiles[i]);
		if(left < 0)	left = 0;
		if(right > chip_width) right = chip_width;

		// If tile's width is smaller than powerplan_width, makes the tile become cost_area
		if(right - left < powerplan_width){
			Rect horizontal_cost_tile = { {left, BOTTOM(horizontal_tiles[i])}, {right, TOP(horizontal_tiles[i])} };
			if (InsertTile(&horizontal_cost_tile, horizontal_plane, SOLID_TILE, -1) == NULL) {
				printf("(Horizontal)Invalid insertion due to the overlapping with existing rectangles: (%d, %d) - (%d, %d).\n",
						left, BOTTOM(horizontal_tiles[i]), right, TOP(horizontal_tiles[i]));
			}

			// Needs to add corresponding tile into vertical corner_stitch data structure at the same time
			Rect vertical_cost_tile = { {-TOP(horizontal_tiles[i]), left}, {-BOTTOM(horizontal_tiles[i]), right} };
			if (InsertTile(&vertical_cost_tile, vertical_plane, SOLID_TILE, -1) == NULL) {
				printf("(Vertical)Invalid insertion due to the overlapping with existing rectangles: (%d, %d) - (%d, %d).\n",
						-TOP(horizontal_tiles[i]), left, -BOTTOM(horizontal_tiles[i]), right);
			}

			// Update powerplan_cost
			powerplan_cost += ((double)(right - left) / (double)micron) * ((double)(TOP(horizontal_tiles[i]) - BOTTOM(horizontal_tiles[i])) / (double)micron);
			
			// There are some tiles are inserted in this round
			updated = true;
		}
	}
	return updated;
}

bool Vertical_update(Plane* horizontal_plane, Plane* vertical_plane,
						Rect& horizontal_region, Rect& vertical_region, double& powerplan_cost){
	// Use boolean for checking if any tiles are inserted in this round
	bool updated = false;

	// Vector stores tiles(in vertical corner stitch data structure) have overlap with placement region
	vector<Tile*> vertical_tiles;

	// Find tiles which have overlap with placement region
	TiSrArea(NULL, vertical_plane, &vertical_region, collectTiles, (ClientData)&vertical_tiles);

	for(int i = 0; i < vertical_tiles.size(); i++){

		// If the tile is solid, means that it is macro or cost_area. We don't need to care if it would become cost_area.		
		if(TiGetBody(vertical_tiles[i]) == SOLID_TILE)
			continue;

		// Decides left, right boundary of tile
		double left = LEFT(vertical_tiles[i]), right = RIGHT(vertical_tiles[i]);
		if(left < -chip_height)	left = -chip_height;
		if(right > 0) right = 0;

		// If tile's width is smaller than powerplan_width, makes the tile become cost_area
		if(right - left < powerplan_width){
			// Needs to add corresponding tile into horizontal corner_stitch data structure at the same time
			Rect horizontal_cost_tile = { {BOTTOM(vertical_tiles[i]), -right}, {TOP(vertical_tiles[i]), -left} };
			if (InsertTile(&horizontal_cost_tile, horizontal_plane, SOLID_TILE, -1) == NULL) {
				printf("(Horizontal)Invalid insertion due to the overlapping with existing rectangles: (%d, %d) - (%d, %d).\n",
						BOTTOM(vertical_tiles[i]), -right, TOP(vertical_tiles[i]), -left);
			}		
			Rect vertical_cost_tile = { {left, BOTTOM(vertical_tiles[i])}, {right, TOP(vertical_tiles[i])} };
			if (InsertTile(&vertical_cost_tile, vertical_plane, SOLID_TILE, -1) == NULL) {
				printf("(Vertical)Invalid insertion due to the overlapping with existing rectangles: (%d, %d) - (%d, %d).\n",
						left, BOTTOM(vertical_tiles[i]), right, TOP(vertical_tiles[i]));
			}

			// Update powerplan_cost
			powerplan_cost += ((double)(right - left) / (double)micron) * ((double)(TOP(vertical_tiles[i]) - BOTTOM(vertical_tiles[i])) / (double)micron);

			// There are some tiles are inserted in this round
			updated = true;
		}
	}
	return updated;
}


double cost_evaluation(vector<Macro*>& macro, Plane* horizontal_plane, Plane* vertical_plane)
{

	// powerplan_cost
	double powerplan_cost = 0;

	// Insert macros into plane
	for(int i = 0; i < macro.size(); i++){

		// In horizontal (left, bottom), (right, top) = (x1, y1), (x2, y2)
		Rect horizontal_rect = { {macro[i]->x1(), macro[i]->y1()}, {macro[i]->x2(), macro[i]->y2()} };
		// In vertical (left, bottom), (right, top) = (-y2, x1), (-y1, x2)
		Rect vertical_rect = { {-macro[i]->y2(), macro[i]->x1()}, {-macro[i]->y1(), macro[i]->x2()} };

		if (InsertTile(&horizontal_rect, horizontal_plane, SOLID_TILE, i) == NULL) {
			printf("(Horizontal)Invalid insertion due to the overlapping with existing rectangles: (%d, %d) - (%d, %d).\n",
					macro[i]->x1(), macro[i]->y1(), macro[i]->x2(), macro[i]->y2());
		}
		if (InsertTile(&vertical_rect, vertical_plane, SOLID_TILE, i) == NULL) {
			printf("(Vertical)Invalid insertion due to the overlapping with existing rectangles: (%d, %d) - (%d, %d).\n",
					-macro[i]->y2(), macro[i]->x1(), -macro[i]->y1(), macro[i]->x2());
		}
	}

	// Rect horizontal(vertical)_region represents place region
	Rect horizontal_region = { {0, 0}, {chip_width, chip_height} };
	Rect vertical_region = { {-chip_height, 0}, {0, chip_width} };

	// Update corner stitch data structure until all cost_area are found
	bool horizontal_updated = true, vertical_updated = true;
	while(horizontal_updated && vertical_updated){
		horizontal_updated = false;
		vertical_updated = false;
		horizontal_updated = Horizontal_update(horizontal_plane, vertical_plane, horizontal_region, vertical_region, powerplan_cost);
		vertical_updated = Vertical_update(horizontal_plane, vertical_plane, horizontal_region, vertical_region, powerplan_cost);
	}

	// Show powerplan cost
	// printf("Powerplan Cost = %lf\n", powerplan_cost);

	return powerplan_cost;
}

vector<int> invalid_check(vector<Macro*>& macro, Plane* horizontal_plane){

	// Vector stores invalid macros found by corner stitch data structure
	vector<int> invalid_macros;
	for(int i = 0; i < macro.size(); i++){
		if(macro[i]->name() == "null")
			continue;
		// Calculate search area boundary for macro
		double left = macro[i]->x1() - buffer_constraint - powerplan_width,
			right = macro[i]->x2() + buffer_constraint + powerplan_width,
			top = macro[i]->y2() + buffer_constraint + powerplan_width,
			bottom = macro[i]->y1() - buffer_constraint - powerplan_width;
		if(left < 0) left = 0;
		if(right > chip_width) right = chip_width;
		if(top > chip_height) top = chip_height;
		if(bottom < 0) bottom = 0;

		// Create search area
		Rect macro_area = { {left, bottom}, {right, top} };

		// Use boolean for checking if this macro is valid
		bool valid = false;

		// Find tiles have overlap with search area, if there are any tile is space tile then the macro is valid
		TiSrArea(NULL, horizontal_plane, &macro_area, buffer_constraint_check, (ClientData)&valid);

		if(!valid){
			invalid_macros.push_back(i);
		}
	}

	// Show all invalid macros
	printf("Invalid macros:\n");
	for(int i = 0; i < invalid_macros.size(); i++){
		cout << "macro " << macro[invalid_macros[i]]->name() << " is invalid" << endl;
	}

	return invalid_macros;
}

void fix_invalid(vector<Macro*>& macro, vector<int>& invalid_macros, Graph& Gh, Graph& Gv){

	vector<edge>* h_edge_list = Gh.get_edge_list();
	vector<edge>* v_edge_list = Gv.get_edge_list();
	vector<edge>* r_h_edge_list = Gh.get_reverse_edge_list();
	vector<edge>* r_v_edge_list = Gv.get_reverse_edge_list();

	map<edge*, int> edges_table;
	for(int i = 0; i < invalid_macros.size(); i++){
		// Set buffer + powerplan boundary of invalid macro
		double left = macro[invalid_macros[i]]->x1() - powerplan_width,
			right = macro[invalid_macros[i]]->x2() + powerplan_width,
			top = macro[invalid_macros[i]]->y2() + powerplan_width,
			bottom = macro[invalid_macros[i]]->y1() - powerplan_width;
		if(left < 0) left = 0;
		if(right > chip_width) right = chip_width;
		if(top > chip_height) top = chip_height;
		if(bottom < 0) bottom = 0;

		// Horizontal_edge : invalid -> neighbor
		for(int j = 0; j < h_edge_list[invalid_macros[i] + 1].size(); j++){
			// ==============================
			// .to need to smaller than n + 1
			// ==============================
			if(h_edge_list[invalid_macros[i] + 1][j].to != macro.size() + 1){
				if(macro[h_edge_list[invalid_macros[i] + 1][j].to - 1]->x1() < right){
					map<edge*, int>::iterator it;
					it = edges_table.find(&h_edge_list[invalid_macros[i] + 1][j]);
					// If the edge hasn't been found, add it into global table
					if(it != edges_table.end()){
						it->second ++;
					}
					else{
						edges_table.insert(make_pair(&h_edge_list[invalid_macros[i] + 1][j], 1));
					}
				}
			}
		}
		// Horizontal_edge : neighbor -> invalid
		for(int j = 0; j < r_h_edge_list[invalid_macros[i] + 1].size(); j++){
			if(r_h_edge_list[invalid_macros[i] + 1][j].from != 0){
				if(macro[r_h_edge_list[invalid_macros[i] + 1][j].from - 1]->x2() > left){
					map<edge*, int>::iterator it;
					it = edges_table.find(&r_h_edge_list[invalid_macros[i] + 1][j]);
					// If the edge hasn't been found, add it into global table
					if(it != edges_table.end()){
						it->second ++;
					}
					else{
						edges_table.insert(make_pair(&r_h_edge_list[invalid_macros[i] + 1][j], 1));
					}
				}
			}
		}
		// Vertical_edge : invalid -> neighbor
		for(int j = 0; j < v_edge_list[invalid_macros[i] + 1].size(); j++){
			if(v_edge_list[invalid_macros[i] + 1][j].to != macro.size() + 1){
				if(macro[v_edge_list[invalid_macros[i] + 1][j].to - 1]->y1() < top){
					map<edge*, int>::iterator it;
					it = edges_table.find(&v_edge_list[invalid_macros[i] + 1][j]);
					// If the edge hasn't been found, add it into global table
					if(it != edges_table.end()){
						it->second ++;
					}
					else{
						edges_table.insert(make_pair(&v_edge_list[invalid_macros[i] + 1][j], 1));
					}
				}
			}
		}		
		// Vertical_edge : neighbor -> invalid
		for(int j = 0; j < r_v_edge_list[invalid_macros[i] + 1].size(); j++){
			if(r_v_edge_list[invalid_macros[i] + 1][j].from != 0){
				if(macro[r_v_edge_list[invalid_macros[i] + 1][j].from - 1]->y2() > bottom){
					map<edge*, int>::iterator it;
					// If the edge hasn't been found, add it into global table
					it = edges_table.find(&r_v_edge_list[invalid_macros[i] + 1][j]);
					if(it != edges_table.end()){
						it->second ++;
					}
					else{
						edges_table.insert(make_pair(&r_v_edge_list[invalid_macros[i] + 1][j], 1));
					}
				}
			}
		}
	}
	for(int i = 0; i < invalid_macros.size(); i++){
		edge* h_target = NULL, *v_target = NULL;
		int h_max_count = 0, v_max_count = 0;
		map<edge*, int>::iterator it;
		for(int j = 0; j < h_edge_list[invalid_macros[i] + 1].size(); j++){
			// Search invalid macro's right edge. If it has maximum count, updates target edge.
			it = edges_table.find(&h_edge_list[invalid_macros[i] + 1][j]);
			if(it != edges_table.end()){
				if(it->second > h_max_count){
					h_target = &h_edge_list[invalid_macros[i] + 1][j];
					h_max_count = it->second;
				}
			}
		}
		for(int j = 0; j < v_edge_list[invalid_macros[i] + 1].size(); j++){
			// Search invalid macro's up edge. If it has maximum count, updates target edge.
			it = edges_table.find(&v_edge_list[invalid_macros[i] + 1][j]);
			if(it != edges_table.end()){
				if(it->second > v_max_count){
					v_target = &v_edge_list[invalid_macros[i] + 1][j];
					v_max_count = it->second;
				}
			}
		}
		for(int j = 0; j < r_h_edge_list[invalid_macros[i] + 1].size(); j++){
			// Search invalid macro's left edge. If it has maximum count, updates target edge.
			it = edges_table.find(&r_h_edge_list[invalid_macros[i] + 1][j]);
			if(it != edges_table.end()){
				if(it->second > h_max_count){
					h_target = &r_h_edge_list[invalid_macros[i] + 1][j];
					h_max_count = it->second;
				}
			}
		}
		for(int j = 0; j < r_v_edge_list[invalid_macros[i] + 1].size(); j++){
			// Search invalid macro's down edge. If it has maximum count, updates target edge.
			it = edges_table.find(&r_v_edge_list[invalid_macros[i] + 1][j]);
			if(it != edges_table.end()){
				if(it->second > v_max_count){
					v_target = &r_v_edge_list[invalid_macros[i] + 1][j];
					v_max_count = it->second;
				}
			}
		}
		// Horizontal target edge has higher count than vertical edge.
		if(h_max_count > v_max_count){
			for(int j = 0; j < h_edge_list[h_target->from].size(); j++){
				if(h_edge_list[h_target->from][j].to == h_target->to){
					h_edge_list[h_target->from][j].weight = (macro[h_target->from - 1]->w() + macro[h_target->to - 1]->w()) / 2 + powerplan_width;
				}
			}
			for(int j = 0; j < r_h_edge_list[h_target->to].size(); j++){
				if(r_h_edge_list[h_target->to][j].from == h_target->from){
					r_h_edge_list[h_target->to][j].weight = (macro[h_target->from - 1]->w() + macro[h_target->to - 1]->w()) / 2 + powerplan_width;
				}
			}
		}
		// Vertical target edge has higher count than horizontal edge.
		else{
			for(int j = 0; j < v_edge_list[v_target->from].size(); j++){
				if(v_edge_list[v_target->from][j].to == v_target->to){
					v_edge_list[v_target->from][j].weight = (macro[v_target->from - 1]->h() + macro[v_target->to - 1]->h()) / 2 + powerplan_width;
				}
			}
			for(int j = 0; j < r_v_edge_list[v_target->to].size(); j++){
				if(r_v_edge_list[v_target->to][j].from == v_target->from){
					r_v_edge_list[v_target->to][j].weight = (macro[v_target->from - 1]->h() + macro[v_target->to - 1]->h()) / 2 + powerplan_width;
				}
			}			
		}
	}
}

void improve_strategy1(vector<Macro*>& macro, vector<Macro*>& native_macro, Plane* horizontal_plane, Graph& Gh, Graph& Gv){

	// Strategy 1: adjust macros near boundary
	vector<edge>* h_edge_list = Gh.get_edge_list();
	vector<edge>* v_edge_list = Gv.get_edge_list();
	vector<edge>* r_h_edge_list = Gh.get_reverse_edge_list();
	vector<edge>* r_v_edge_list = Gv.get_reverse_edge_list();

	// near left
	for(int i = 0; i < h_edge_list[0].size(); i++){
		int m = h_edge_list[0][i].to - 1;
		if(macro[m]->x1() < powerplan_width && macro[m]->x1() != 0 && !macro[m]->is_fixed()){
			// current x-axis displacement + powerplan_cost with left boundary
			double current_cost = alpha * std::abs(native_macro[m]->x1() - macro[m]->x1()) / micron + 
									beta * sqrt((macro[m]->h() / micron ) * (macro[m]->x1() / micron));

			if(native_macro[m]->x1() <= powerplan_width / 2){
				if(alpha * native_macro[m]->x1() / micron <= current_cost){
					macro[m]->updateXY(make_pair(macro[m]->w() / 2, macro[m]->cy()));
				}
			}
			else{
				if(alpha * std::abs(powerplan_width - native_macro[m]->x1()) / micron < current_cost){
					h_edge_list[0][i].weight = macro[m]->w() / 2 + powerplan_width;
					for(int j = 0; j < r_h_edge_list[h_edge_list[0][i].to].size(); j++){
						if(r_h_edge_list[h_edge_list[0][i].to][j].from == 0){
							r_h_edge_list[h_edge_list[0][i].to][j].weight = macro[m]->w() / 2 + powerplan_width;
						}
					}
				}
			}
		}
	}
	// near right
	for(int i = 0; i < r_h_edge_list[macro.size() + 1].size(); i++){
		int m = r_h_edge_list[macro.size() + 1][i].from - 1;
		if((macro[m]->x2() > (chip_width - powerplan_width)) && macro[m]->x2() != chip_width && !macro[m]->is_fixed()){
			// current x-axis displacement + powerplan_cost with right boundary
			double current_cost = alpha * std::abs(native_macro[m]->x1() - macro[m]->x1()) / micron + 
							beta * sqrt((macro[m]->h() / micron ) * ((chip_width - macro[m]->x2()) / micron));

			if(native_macro[m]->x2() >= chip_width - powerplan_width / 2){
				if(alpha * (chip_width - native_macro[m]->x2()) / micron <= current_cost){
					macro[m]->updateXY(make_pair(chip_width - macro[m]->w() / 2, macro[m]->cy()));
				}
			}
			else{
				// In this case, for ensuring that macro will have powerplan_width distance between boundary,
				// needs to add constraint in to LP model by reverse constraint graph.
				// We can only ensure that "a" will at right hand side of "b" by distance at least w(a, b).
				if(alpha * std::abs(native_macro[m]->x2() - (chip_width - powerplan_width)) / micron < current_cost){
					macro[m]->updateXY(make_pair(chip_width - powerplan_width - macro[m]->w() / 2, macro[m]->cy()));
				}
			}
		}
	}
	// near bottom
	for(int i = 0; i < v_edge_list[0].size(); i++){
		int m = v_edge_list[0][i].to - 1;
		if(macro[m]->y1() < powerplan_width && macro[m]->y1() != 0 && !macro[m]->is_fixed()){
			// current y-axis displacement + powerplan_cost with bottom boundary
			double current_cost = alpha * std::abs(native_macro[m]->y1() - macro[m]->y1()) / micron + 
									beta * sqrt((macro[m]->w() / micron) * (macro[m]->y1() / micron));

			if(native_macro[m]->y1() <= powerplan_width / 2){
				if(alpha * native_macro[m]->y1() / micron <= current_cost){
					macro[m]->updateXY(make_pair(macro[m]->cx(), macro[m]->h() / 2));
				}
			}
			else{
				if(alpha * std::abs(powerplan_width - native_macro[m]->y1()) / micron < current_cost){
					v_edge_list[0][i].weight = macro[m]->h() / 2 + powerplan_width;
					for(int j = 0; j < r_v_edge_list[v_edge_list[0][i].to].size(); j++){
						if(r_v_edge_list[v_edge_list[0][i].to][j].from == 0){
							r_v_edge_list[v_edge_list[0][i].to][j].weight = macro[m]->h() / 2 + powerplan_width;
						}
					}					
				}
			}
		}
	}
	// near top
	for(int i = 0; i < r_v_edge_list[macro.size() + 1].size(); i++){
		int m = r_v_edge_list[macro.size() + 1][i].from - 1;
		if(macro[m]->y2() > (chip_height - powerplan_width) && macro[m]->y2() != chip_height && !macro[m]->is_fixed()){
			// current y-axis displacement + powerplan_cost with top boundary
			double current_cost = alpha * std::abs(native_macro[m]->y1() - macro[m]->y1()) / micron + 
									beta * sqrt((macro[m]->w() / micron) * ((chip_height - macro[m]->y2()) / micron));

			if(native_macro[m]->y2() >= chip_height - powerplan_width / 2){
				if(alpha * (chip_height - native_macro[m]->y2()) / micron <= current_cost){
					macro[m]->updateXY(make_pair(macro[m]->cx(), chip_height - macro[m]->h() / 2));
				}
			}
			else{
				// In this case, for ensuring that macro will have powerplan_width distance between boundary,
				// needs to add constraint in to LP model by reverse constraint graph.
				// We can only ensure that "a" will higher than "b" at least w(a, b),
				if(alpha * std::abs(native_macro[m]->y2() - (chip_height - powerplan_width)) / micron < current_cost){
					macro[m]->updateXY(make_pair(macro[m]->cx(), chip_height - powerplan_width - macro[m]->h() / 2));
				}
			}
		}
	}
}

bool search_area(vector<Macro*>& macro, Plane* horizontal_plane, Rect& region,
					double left, double right, double top, double bottom, Graph& Gh, Graph& Gv, double threshold){
	vector<edge>* h_edge_list = Gh.get_edge_list();
	vector<edge>* v_edge_list = Gv.get_edge_list();
	vector<edge>* r_h_edge_list = Gh.get_reverse_edge_list();
	vector<edge>* r_v_edge_list = Gv.get_reverse_edge_list();
	bool found = false;

	vector<Tile*> horizontal_tiles;
	// Find out all solid tiles in this subregion
	TiSrArea(NULL, horizontal_plane, &region, collectSolidTiles, (ClientData)&horizontal_tiles);

	double solid_area = 0;
	vector<Tile*> cost_tile;
	// Iterative go through solid tiles, calculate macros total area in this subregion, and find out cost tiles
	for(int i = 0; i < horizontal_tiles.size(); i++){
		if(TiGetClient(horizontal_tiles[i]) == -1)
			cost_tile.push_back(horizontal_tiles[i]);
		else{
			double tile_left = (LEFT(horizontal_tiles[i]) < left) ? left : LEFT(horizontal_tiles[i]);
			double tile_right = (RIGHT(horizontal_tiles[i]) > right) ? right : RIGHT(horizontal_tiles[i]);
			double tile_top = (TOP(horizontal_tiles[i]) > top) ? top : TOP(horizontal_tiles[i]);
			double tile_bottom = (BOTTOM(horizontal_tiles[i]) < bottom) ? bottom : BOTTOM(horizontal_tiles[i]);
			solid_area += (tile_right - tile_left) * (tile_top - tile_bottom);
		}
	}
	// Check if the subregion is sparse
	if(solid_area <= threshold * (right - left) * (top - bottom)){
		// for each cost tile in suregion, find macros which cause this cost_tile
		// And adjust edge's weight which connect to these two macros
		for(int i = 0; i < cost_tile.size(); i++){
			// if(rand() % 10 > int(threshold * 10) + 2)
			// 	continue;
			if(TiGetBody(BL(cost_tile[i])) == SOLID_TILE && TiGetBody(TR(cost_tile[i])) == SOLID_TILE &&
				TiGetClient(BL(cost_tile[i])) != -1 && TiGetClient(TR(cost_tile[i])) != -1){
				bool pre = false;
				if(macro[TiGetClient(TR(cost_tile[i]))]->y2() > macro[TiGetClient(BL(cost_tile[i]))]->y2()){
					if(macro[TiGetClient(BL(cost_tile[i]))]->y2() - macro[TiGetClient(TR(cost_tile[i]))]->y1() <
						powerplan_width - (macro[TiGetClient(TR(cost_tile[i]))]->x1() - macro[TiGetClient(BL(cost_tile[i]))]->x2())){
						Gv.add_edge(TiGetClient(BL(cost_tile[i])) + 1, TiGetClient(TR(cost_tile[i])) + 1, (macro[TiGetClient(BL(cost_tile[i]))]->h() + macro[TiGetClient(TR(cost_tile[i]))]->h()) / 2);
						// cout << "move " << macro[TiGetClient(BL(cost_tile[i]))]->name() << " " << macro[TiGetClient(TR(cost_tile[i]))]->name() << endl;
						pre = true;
					}
				}
				else if(macro[TiGetClient(BL(cost_tile[i]))]->y2() > macro[TiGetClient(TR(cost_tile[i]))]->y2()){
					if(macro[TiGetClient(TR(cost_tile[i]))]->y2() - macro[TiGetClient(BL(cost_tile[i]))]->y1() <
						powerplan_width - (macro[TiGetClient(TR(cost_tile[i]))]->x1() - macro[TiGetClient(BL(cost_tile[i]))]->x2())){
						Gv.add_edge(TiGetClient(TR(cost_tile[i])) + 1, TiGetClient(BL(cost_tile[i])) + 1, (macro[TiGetClient(TR(cost_tile[i]))]->h() + macro[TiGetClient(BL(cost_tile[i]))]->h()) / 2);
						pre = true;
					}
				}
				if(!pre){
					// cout << "space h " << macro[TiGetClient(BL(cost_tile[i]))]->name() << " " << macro[TiGetClient(TR(cost_tile[i]))]->name() << endl;
					for(int j = 0; j < h_edge_list[TiGetClient(BL(cost_tile[i])) + 1].size(); j++){
						if(h_edge_list[TiGetClient(BL(cost_tile[i])) + 1][j].to - 1 == TiGetClient(TR(cost_tile[i]))){
							h_edge_list[TiGetClient(BL(cost_tile[i])) + 1][j].weight = (macro[TiGetClient(BL(cost_tile[i]))]->w() + macro[TiGetClient(TR(cost_tile[i]))]->w()) / 2 + powerplan_width;
						}
					}
					for(int j = 0; j < r_h_edge_list[TiGetClient(TR(cost_tile[i])) + 1].size(); j++){
						if(r_h_edge_list[TiGetClient(TR(cost_tile[i])) + 1][j].from - 1 == TiGetClient(BL(cost_tile[i]))){
							r_h_edge_list[TiGetClient(TR(cost_tile[i])) + 1][j].weight = (macro[TiGetClient(BL(cost_tile[i]))]->w() + macro[TiGetClient(TR(cost_tile[i]))]->w()) / 2 + powerplan_width;
						}
					}
				}
			}
			if(TiGetBody(LB(cost_tile[i])) == SOLID_TILE && TiGetBody(RT(cost_tile[i])) == SOLID_TILE &&
				TiGetClient(LB(cost_tile[i])) != -1 && TiGetClient(RT(cost_tile[i])) != -1){
				bool pre = false;
				if(macro[TiGetClient(LB(cost_tile[i]))]->x2() > macro[TiGetClient(RT(cost_tile[i]))]->x2()){
					if(macro[TiGetClient(LB(cost_tile[i]))]->x2() - macro[TiGetClient(RT(cost_tile[i]))]->x1() <
						powerplan_width - (macro[TiGetClient(RT(cost_tile[i]))]->y1() - macro[TiGetClient(LB(cost_tile[i]))]->y2())){
						Gh.add_edge(TiGetClient(LB(cost_tile[i])) + 1, TiGetClient(RT(cost_tile[i])) + 1, (macro[TiGetClient(LB(cost_tile[i]))]->w() + macro[TiGetClient(RT(cost_tile[i]))]->h()) / 2);
						// cout << "move " << macro[TiGetClient(LB(cost_tile[i]))]->name() << " " << macro[TiGetClient(RT(cost_tile[i]))]->name() << endl;
						pre = true;
					}
				}
				else if(macro[TiGetClient(LB(cost_tile[i]))]->x2() > macro[TiGetClient(RT(cost_tile[i]))]->x2()){
					if(macro[TiGetClient(RT(cost_tile[i]))]->y2() - macro[TiGetClient(LB(cost_tile[i]))]->y1() <
						powerplan_width - (macro[TiGetClient(RT(cost_tile[i]))]->y1() - macro[TiGetClient(LB(cost_tile[i]))]->y2())){
						Gh.add_edge(TiGetClient(RT(cost_tile[i])) + 1, TiGetClient(LB(cost_tile[i])) + 1, (macro[TiGetClient(RT(cost_tile[i]))]->w() + macro[TiGetClient(LB(cost_tile[i]))]->w()) / 2);
						pre = true;
					}
				}					
				// cout << "space v " << macro[TiGetClient(LB(cost_tile[i]))]->name() << " " << macro[TiGetClient(RT(cost_tile[i]))]->name() << endl;
				for(int j = 0; j < v_edge_list[TiGetClient(LB(cost_tile[i])) + 1].size(); j++){
					if(v_edge_list[TiGetClient(LB(cost_tile[i])) + 1][j].to - 1 == TiGetClient(RT(cost_tile[i]))){
						v_edge_list[TiGetClient(LB(cost_tile[i])) + 1][j].weight = (macro[TiGetClient(LB(cost_tile[i]))]->h() + macro[TiGetClient(RT(cost_tile[i]))]->h()) / 2 + powerplan_width;
					}
				}
				for(int j = 0; j < r_v_edge_list[TiGetClient(RT(cost_tile[i])) + 1].size(); j++){
					if(r_v_edge_list[TiGetClient(RT(cost_tile[i])) + 1][j].from - 1 == TiGetClient(LB(cost_tile[i]))){
						r_v_edge_list[TiGetClient(RT(cost_tile[i])) + 1][j].weight = (macro[TiGetClient(LB(cost_tile[i]))]->h() + macro[TiGetClient(RT(cost_tile[i]))]->h()) / 2 + powerplan_width;
					}
				}
			}
		}
		found = true;
	}
	return found;
}

void search_sparse(vector<Macro*>& macro, Plane* horizontal_plane, double left, double right, double bottom, double top,
						Graph& Gh, Graph& Gv, double threshold, bool& pre_found){
	Rect left_top = { {left, (top + bottom) / 2}, {(left + right) / 2, top} };
	Rect right_top = { {(left + right) / 2, (top + bottom) / 2}, {right, top} };
	Rect right_bottom = { {(left + right) / 2, bottom}, {right, (top + bottom) / 2} };
	Rect left_bottom = { {left, bottom}, {(left + right) / 2, (top + bottom) / 2} };

	if(!pre_found){
		pre_found = search_area(macro, horizontal_plane, left_top, left, (left + right) / 2, top, (top + bottom) / 2, Gh, Gv, threshold);
	}
	if(!pre_found){
		pre_found = search_area(macro, horizontal_plane, right_top, (left + right) / 2, right, top, (top + bottom) / 2, Gh, Gv, threshold);
	}
	if(!pre_found){
		pre_found = search_area(macro, horizontal_plane, right_bottom, (left + right) / 2, right, (top + bottom) / 2, bottom, Gh, Gv, threshold);
	}
	if(!pre_found){
		pre_found = search_area(macro, horizontal_plane, left_bottom, left, (left + right) / 2, (top + bottom) / 2, bottom, Gh, Gv, threshold);
	}
}

void improve_strategy2(vector<Macro*>& macro, Plane* horizontal_plane, Graph& Gh, Graph& Gv, int SA_COUNT){

	vector<edge>* h_edge_list = Gh.get_edge_list();
	vector<edge>* v_edge_list = Gv.get_edge_list();
	vector<edge>* r_h_edge_list = Gh.get_reverse_edge_list();
	vector<edge>* r_v_edge_list = Gv.get_reverse_edge_list();	
	Rect left_top = { {0, chip_height / 2}, {chip_width / 2, chip_height} };
	Rect right_top = { {chip_width / 2, chip_height / 2}, {chip_width, chip_height} };
	Rect right_bottom = { {chip_width / 2, 0}, {chip_width, chip_height / 2} };
	Rect left_bottom = { {0, 0}, {chip_width / 2, chip_height / 2} };
	
	double threshold = 0.1 * double(SA_COUNT);
	// Check if left_top subregion is sparse. If yes, set left_top_sparse = true. And do not search into next level
	bool left_top_sparse = false;
	left_top_sparse = search_area(macro, horizontal_plane, left_top, 0, chip_width / 2, chip_height, chip_height / 2, Gh, Gv, threshold);
	if(!left_top_sparse){
		search_sparse(macro, horizontal_plane, 0, chip_width / 2, chip_height / 2, chip_height, Gh, Gv, threshold, left_top_sparse);
	}

	// Check if right_top subregion is sparse. If yes, set right_top_sparse = true. And do not search into next level
	bool right_top_sparse = false;
	right_top_sparse = search_area(macro, horizontal_plane, right_top, chip_width / 2, chip_width, chip_height, chip_height / 2, Gh, Gv, threshold);
	if(!right_top_sparse){
		search_sparse(macro, horizontal_plane, chip_width / 2, chip_width, chip_height / 2, chip_height, Gh, Gv, threshold, right_top_sparse);
	}

	// Check if right_bottom subregion is sparse. If yes, set right_bottom_sparse = true. And do not search into next level
	bool right_bottom_sparse = false;
	right_bottom_sparse = search_area(macro, horizontal_plane, right_bottom, chip_width / 2, chip_width, chip_height / 2, 0, Gh, Gv, threshold);
	if(!right_bottom_sparse){
		search_sparse(macro, horizontal_plane, chip_width / 2, chip_width, 0, chip_height / 2, Gh, Gv, threshold, right_bottom_sparse);
	}

	// Check if left_bottom subregion is sparse. If yes, set left_bottom_sparse = true. And do not search into next level
	bool left_bottom_sparse = false;
	left_bottom_sparse = search_area(macro, horizontal_plane, left_bottom, 0, chip_width / 2, chip_height / 2, 0, Gh, Gv, threshold);
	if(!left_bottom_sparse){
		search_sparse(macro, horizontal_plane, 0, chip_width / 2, 0, chip_height / 2, Gh, Gv, threshold, left_bottom_sparse);
	}	

	// Updates zero slack edges
	while(Gh.longest_path(true) > chip_width){
		vector<edge*> h_zero_slack;
		h_zero_slack = Gh.zero_slack();
		for(int i = 0; i < h_zero_slack.size(); i++){
			if(h_zero_slack[i]->from != 0 && h_zero_slack[i]->to != macro.size() + 1){
				if(h_zero_slack[i]->weight == (macro[h_zero_slack[i]->from - 1]->w() + macro[h_zero_slack[i]->to - 1]->w()) / 2 + powerplan_width){
					// cout << macro[h_zero_slack[i]->from - 1]->name() << " " << macro[h_zero_slack[i]->to - 1]->name() << " go back" << endl;
					h_zero_slack[i]->weight = (macro[h_zero_slack[i]->from - 1]->w() + macro[h_zero_slack[i]->to - 1]->w()) / 2 + min_spacing;
					// Also set reverse constraint graph
					for(int j = 0; j < r_h_edge_list[h_zero_slack[i]->to].size(); j++){
						if(r_h_edge_list[h_zero_slack[i]->to][j].from == h_zero_slack[i]->from)
							r_h_edge_list[h_zero_slack[i]->to][j].weight = (macro[h_zero_slack[i]->from - 1]->w() + macro[h_zero_slack[i]->to - 1]->w()) / 2 + min_spacing;
					}
				}
				if(h_zero_slack[i]->weight == (macro[h_zero_slack[i]->from - 1]->w() + macro[h_zero_slack[i]->to - 1]->w()) / 2){
					Gh.remove_edge(h_zero_slack[i]->from, h_zero_slack[i]->to);
				}
			}
			// ================================
			// needs also update reversed graph
			// ================================
		}
	}
	while(Gv.longest_path(false) > chip_height){
		vector<edge*> v_zero_slack;
		v_zero_slack = Gv.zero_slack();
		for(int i = 0; i < v_zero_slack.size(); i++){
			if(v_zero_slack[i]->from != 0 && v_zero_slack[i]->to != macro.size() + 1){
				if(v_zero_slack[i]->weight == (macro[v_zero_slack[i]->from - 1]->h() + macro[v_zero_slack[i]->to - 1]->h()) / 2 + powerplan_width){
					// cout << macro[v_zero_slack[i]->from - 1]->name() << " " << macro[v_zero_slack[i]->to - 1]->name() << " go back" << endl;
					v_zero_slack[i]->weight = (macro[v_zero_slack[i]->from - 1]->h() + macro[v_zero_slack[i]->to - 1]->h()) / 2 + min_spacing;
					// Also set reverse constraint graph
					for(int j = 0; j < r_v_edge_list[v_zero_slack[i]->to].size(); j++){
						if(r_v_edge_list[v_zero_slack[i]->to][j].from == v_zero_slack[i]->from)
							r_v_edge_list[v_zero_slack[i]->to][j].weight = (macro[v_zero_slack[i]->from - 1]->h() + macro[v_zero_slack[i]->to - 1]->h()) / 2 + min_spacing;
					}					
				}
				if(v_zero_slack[i]->weight == (macro[v_zero_slack[i]->from - 1]->h() + macro[v_zero_slack[i]->to - 1]->h()) / 2){
					Gv.remove_edge(v_zero_slack[i]->from, v_zero_slack[i]->to);
				}
			}
			// ================================
			// needs also update reversed graph
			// ================================
		}
	}
}