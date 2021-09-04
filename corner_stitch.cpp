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

static int empty_region(Tile *tile, ClientData cdata){
	if(TiGetBody(tile) == SOLID_TILE)
		*((bool*)cdata) = false;
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

		// Decides left, right boundary of tile
		double left = LEFT(horizontal_tiles[i]), right = RIGHT(horizontal_tiles[i]);
		if(left < 0)	left = 0;
		if(right > chip_width) right = chip_width;

		// If tile's width is smaller than powerplan_width, makes the tile become cost_area
		if(right - left < powerplan_width){
			Rect horizontal_cost_tile = { {left, BOTTOM(horizontal_tiles[i])}, {right, TOP(horizontal_tiles[i])} };
			if (InsertTile(&horizontal_cost_tile, horizontal_plane, SOLID_TILE, -1) == NULL) {
				printf("(Horizontal)Invalid insertion due to the overlapping with existing rectangles: (%lf, %lf) - (%lf, %lf).\n",
						left, BOTTOM(horizontal_tiles[i]), right, TOP(horizontal_tiles[i]));
			}

			// Needs to add corresponding tile into vertical corner_stitch data structure at the same time
			Rect vertical_cost_tile = { {-TOP(horizontal_tiles[i]), left}, {-BOTTOM(horizontal_tiles[i]), right} };
			if (InsertTile(&vertical_cost_tile, vertical_plane, SOLID_TILE, -1) == NULL) {
				printf("(Vertical)Invalid insertion due to the overlapping with existing rectangles: (%lf, %lf) - (%lf, %lf).\n",
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
				printf("(Horizontal)Invalid insertion due to the overlapping with existing rectangles: (%lf, %lf) - (%lf, %lf).\n",
						BOTTOM(vertical_tiles[i]), -right, TOP(vertical_tiles[i]), -left);
			}		
			Rect vertical_cost_tile = { {left, BOTTOM(vertical_tiles[i])}, {right, TOP(vertical_tiles[i])} };
			if (InsertTile(&vertical_cost_tile, vertical_plane, SOLID_TILE, -1) == NULL) {
				printf("(Vertical)Invalid insertion due to the overlapping with existing rectangles: (%lf, %lf) - (%lf, %lf).\n",
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
			printf("(Horizontal)Invalid insertion due to the overlapping with existing rectangles: (%lf, %lf) - (%lf, %lf).\n",
					macro[i]->x1(), macro[i]->y1(), macro[i]->x2(), macro[i]->y2());
		}
		if (InsertTile(&vertical_rect, vertical_plane, SOLID_TILE, i) == NULL) {
			printf("(Vertical)Invalid insertion due to the overlapping with existing rectangles: (%lf, %lf) - (%lf, %lf).\n",
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

	// Return powerplan cost
	return powerplan_cost;
}

vector<int> invalid_check(vector<Macro*>& macro, Plane* horizontal_plane){

	// Vector stores invalid macros found by corner stitch data structure
	vector<int> invalid_macros;
	for(int i = 0; i < macro.size(); i++){
		if(macro[i]->name() == "null")
			continue;
		// Determine search area boundary for macro
		double left = macro[i]->x1() - buffer_constraint,
			right = macro[i]->x2() + buffer_constraint,
			top = macro[i]->y2() + buffer_constraint,
			bottom = macro[i]->y1() - buffer_constraint;
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
	if(!invalid_macros.empty()){
		printf("Invalid macros:\n");
	}
	for(int i = 0; i < invalid_macros.size(); i++){
		cout << "macro " << macro[invalid_macros[i]]->name() << " is invalid" << endl;
	}

	return invalid_macros;
}

void fix_invalid(vector<Macro*>& macro, vector<int>& invalid_macros, Graph& Gh, Graph& Gv){

	// Read edges list
	vector<edge>* h_edge_list = Gh.get_edge_list();
	vector<edge>* v_edge_list = Gv.get_edge_list();
	vector<edge>* r_h_edge_list = Gh.get_reverse_edge_list();
	vector<edge>* r_v_edge_list = Gv.get_reverse_edge_list();

	// Use a table for recording potential edges which 
	map<edge*, int> edges_table;
	for(int i = 0; i < invalid_macros.size(); i++){
		// Set [buffer + powerplan] boundary of invalid macro
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
			// .to can't be sink
			if(h_edge_list[invalid_macros[i] + 1][j].to != macro.size() + 1){
				// right neighbor overlaps with region created by invalid_macro
				if(macro[h_edge_list[invalid_macros[i] + 1][j].to - 1]->x1() < right){
					// If the edge hasn't been found, add it into global table
					map<edge*, int>::iterator it;
					it = edges_table.find(&h_edge_list[invalid_macros[i] + 1][j]);
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
			// .from can't be source
			if(r_h_edge_list[invalid_macros[i] + 1][j].from != 0){
				// left neighbor overlaps with region created by invalid_macro
				if(macro[r_h_edge_list[invalid_macros[i] + 1][j].from - 1]->x2() > left){
					// If the edge hasn't been found, add it into global table
					map<edge*, int>::iterator it;
					it = edges_table.find(&r_h_edge_list[invalid_macros[i] + 1][j]);
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
			// .to can't be sink
			if(v_edge_list[invalid_macros[i] + 1][j].to != macro.size() + 1){
				// top neighbor overlaps with region created by invalid_macro
				if(macro[v_edge_list[invalid_macros[i] + 1][j].to - 1]->y1() < top){
					// If the edge hasn't been found, add it into global table
					map<edge*, int>::iterator it;
					it = edges_table.find(&v_edge_list[invalid_macros[i] + 1][j]);
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
			// .from can't be source
			if(r_v_edge_list[invalid_macros[i] + 1][j].from != 0){
				// bottom neighbor overlaps with region created by invalid_macro
				if(macro[r_v_edge_list[invalid_macros[i] + 1][j].from - 1]->y2() > bottom){
					// If the edge hasn't been found, add it into global table					
					map<edge*, int>::iterator it;
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
	// For each invalid_macro, we change edge's weight which has largest count
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
			if(alpha * macro[m]->x1() < beta * sqrt(macro[m]->x1() * macro[m]->h())){
				macro[m]->updateXY(make_pair(macro[m]->w() / 2, macro[m]->cy()));
			}
		}
	}
	// near right
	for(int i = 0; i < r_h_edge_list[macro.size() + 1].size(); i++){
		int m = r_h_edge_list[macro.size() + 1][i].from - 1;
		if((macro[m]->x2() > (chip_width - powerplan_width)) && macro[m]->x2() != chip_width && !macro[m]->is_fixed()){
			if(alpha * macro[m]->x2() < beta * sqrt(macro[m]->x2() * macro[m]->h())){
				macro[m]->updateXY(make_pair(chip_width - macro[m]->w() / 2, macro[m]->cy()));
			}
		}
	}
	// near bottom
	for(int i = 0; i < v_edge_list[0].size(); i++){
		int m = v_edge_list[0][i].to - 1;
		if(macro[m]->y1() < powerplan_width && macro[m]->y1() != 0 && !macro[m]->is_fixed()){
			if(alpha * macro[m]->y1() < beta * sqrt(macro[m]->y1() * macro[m]->w())){
				macro[m]->updateXY(make_pair(macro[m]->cx(), macro[m]->h() / 2));
			}
		}
	}
	// near top
	for(int i = 0; i < r_v_edge_list[macro.size() + 1].size(); i++){
		int m = r_v_edge_list[macro.size() + 1][i].from - 1;
		if(macro[m]->y2() > (chip_height - powerplan_width) && macro[m]->y2() != chip_height && !macro[m]->is_fixed()){
			if(alpha * macro[m]->y2() < beta * sqrt(macro[m]->y2() * macro[m]->w())){
				macro[m]->updateXY(make_pair(macro[m]->cx(), chip_height - macro[m]->h() / 2));
			}
		}
	}
}

bool cost_tile_cmp(const Tile* a, const Tile* b){
	double a_area = std::abs(a->ti_rt->ti_ll.p_x - a->ti_ll.p_x) * std::abs(a->ti_tr->ti_ll.p_y - a->ti_ll.p_y),
			b_area = std::abs(b->ti_rt->ti_ll.p_x - b->ti_ll.p_x) * std::abs(b->ti_tr->ti_ll.p_y - b->ti_ll.p_y);
	return a_area < b_area;
}

void improve_strategy2(vector<Macro*>& macro, Plane* horizontal_plane, Plane* vertical_plane, Graph& Gh, Graph& Gv){

	vector<edge>* h_edge_list = Gh.get_edge_list();
	vector<edge>* v_edge_list = Gv.get_edge_list();
	vector<edge>* r_h_edge_list = Gh.get_reverse_edge_list();
	vector<edge>* r_v_edge_list = Gv.get_reverse_edge_list();

	vector<Tile*> horizontal_tiles;
	vector<Tile*> vertical_tiles;
	Rect horizontal_region = { {0, 0}, {chip_width, chip_height} };
	Rect vertical_region = {{-chip_height, 0}, {0, chip_width}};	
	// Find out all solid tiles in this subregion
	TiSrArea(NULL, horizontal_plane, &horizontal_region, collectSolidTiles, (ClientData)&horizontal_tiles);
	TiSrArea(NULL, vertical_plane, &vertical_region, collectSolidTiles, (ClientData)&vertical_tiles);

	vector<Tile*> h_cost_tile;
	// Iterative go through solid tiles, calculate macros total area in this subregion, and find out cost tiles
	for(int i = 0; i < horizontal_tiles.size(); i++){
		if(TiGetClient(horizontal_tiles[i]) == -1){
			h_cost_tile.push_back(horizontal_tiles[i]);
		}
	}
	// std::sort(h_cost_tile.begin(), h_cost_tile.end(), cost_tile_cmp);
	std::random_shuffle (h_cost_tile.begin(), h_cost_tile.end());
	vector<Tile*> v_cost_tile;
	for(int i = 0; i < vertical_tiles.size(); i++){
		if(TiGetClient(vertical_tiles[i]) == -1){
			v_cost_tile.push_back(vertical_tiles[i]);
		}
	}
	// std::sort(v_cost_tile.begin(), v_cost_tile.end(), cost_tile_cmp);
	std::random_shuffle (v_cost_tile.begin(), v_cost_tile.end());
	// And adjust edge's weight which connect to these two macros
	if(rand() % 2){
		for(int i = 0; i < h_cost_tile.size(); i++){
			// This cost tile is generated by two macros(left - right)
			if(TiGetBody(BL(h_cost_tile[i])) == SOLID_TILE && TiGetBody(TR(h_cost_tile[i])) == SOLID_TILE &&
				TiGetClient(BL(h_cost_tile[i])) != -1 && TiGetClient(TR(h_cost_tile[i])) != -1){
				// Do not space e(macro, null)
				if(macro[TiGetClient(BL(h_cost_tile[i]))]->name() != "null" && macro[TiGetClient(TR(h_cost_tile[i]))]->name() != "null"){
					// Determine if left macro is the higher one or right macro.
					bool left_higher = macro[TiGetClient(BL(h_cost_tile[i]))]->y1() > macro[TiGetClient(TR(h_cost_tile[i]))]->y1() && 
										macro[TiGetClient(BL(h_cost_tile[i]))]->y2() > macro[TiGetClient(TR(h_cost_tile[i]))]->y2(),
							right_higher = macro[TiGetClient(TR(h_cost_tile[i]))]->y1() > macro[TiGetClient(BL(h_cost_tile[i]))]->y1() && 
										macro[TiGetClient(TR(h_cost_tile[i]))]->y2() > macro[TiGetClient(BL(h_cost_tile[i]))]->y2();
					if(left_higher){
						// Calculate width and height of cost region
						double diff_h = macro[TiGetClient(TR(h_cost_tile[i]))]->x1() - macro[TiGetClient(BL(h_cost_tile[i]))]->x2(),
								diff_v =  macro[TiGetClient(TR(h_cost_tile[i]))]->y2() - macro[TiGetClient(BL(h_cost_tile[i]))]->y1();
						if(alpha * diff_v < beta * sqrt(diff_h * diff_v)){
							// moving macro vertically is supposed to gain benefit
							double left_l = macro[TiGetClient(BL(h_cost_tile[i]))]->x1() - min_spacing,
									left_r = macro[TiGetClient(BL(h_cost_tile[i]))]->x2() + min_spacing,
									left_t = macro[TiGetClient(BL(h_cost_tile[i]))]->y2() + 2 * powerplan_width,
									left_b = macro[TiGetClient(BL(h_cost_tile[i]))]->y2();
							if(left_l < 0) left_l = 0;
							if(left_r > chip_width) left_r = chip_width;
							if(left_t > chip_height) left_t = chip_height;
							double right_l = macro[TiGetClient(TR(h_cost_tile[i]))]->x1() - min_spacing,
									right_r = macro[TiGetClient(TR(h_cost_tile[i]))]->x2() + min_spacing,
									right_t = macro[TiGetClient(TR(h_cost_tile[i]))]->y1(),
									right_b = macro[TiGetClient(TR(h_cost_tile[i]))]->y1() - 2 * powerplan_width;
							if(right_l < 0) right_l = 0;
							if(right_r > chip_width) right_r = chip_width;
							if(right_b < 0) right_b = 0;
							bool left_e = true, right_e = true;
							if(left_t - left_b - diff_v < powerplan_width){
								left_e = false;
							}
							else{
								Rect left_ = { {left_l, left_b}, {left_r, left_t} };
								TiSrArea(NULL, horizontal_plane, &left_, empty_region, (ClientData)&left_e);
							}
							if(right_t - right_b - diff_v < powerplan_width){
								right_e = false;
							}
							else{
								Rect right_ = { {right_l, right_b}, {right_r, right_t} };
								TiSrArea(NULL, horizontal_plane, &right_, empty_region, (ClientData)&right_e);
							}
							if(left_e && !macro[TiGetClient(BL(h_cost_tile[i]))]->is_fixed()){
								// If left macro can move up(left macro is higher)
								macro[TiGetClient(BL(h_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(BL(h_cost_tile[i]))]->cx(), macro[TiGetClient(BL(h_cost_tile[i]))]->cy() + diff_v));
								break;
							}
							else if(right_e && !macro[TiGetClient(TR(h_cost_tile[i]))]->is_fixed()){
								// If right macro can move down
								macro[TiGetClient(TR(h_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(TR(h_cost_tile[i]))]->cx(), macro[TiGetClient(TR(h_cost_tile[i]))]->cy() - diff_v));
								break;
							}
						}
					}
					else if(right_higher){
						// Calculate width and height of cost region
						double diff_h = macro[TiGetClient(TR(h_cost_tile[i]))]->x1() - macro[TiGetClient(BL(h_cost_tile[i]))]->x2(),
								diff_v =  macro[TiGetClient(BL(h_cost_tile[i]))]->y2() - macro[TiGetClient(TR(h_cost_tile[i]))]->y1();
						if(alpha * diff_v < beta * sqrt(diff_h * diff_v)){
							// moving macro vertically is supposed to gain benefit
							double left_l = macro[TiGetClient(BL(h_cost_tile[i]))]->x1() - min_spacing,
									left_r = macro[TiGetClient(BL(h_cost_tile[i]))]->x2() + min_spacing,
									left_t = macro[TiGetClient(BL(h_cost_tile[i]))]->y1(),
									left_b = macro[TiGetClient(BL(h_cost_tile[i]))]->y1() - 2 * powerplan_width;
							if(left_l < 0) left_l = 0;
							if(left_r > chip_width) left_r = chip_width;
							if(left_b < 0) left_b = 0;
							double right_l = macro[TiGetClient(TR(h_cost_tile[i]))]->x1() - min_spacing,
									right_r = macro[TiGetClient(TR(h_cost_tile[i]))]->x2() + min_spacing,
									right_t = macro[TiGetClient(TR(h_cost_tile[i]))]->y2() + 2 * powerplan_width,
									right_b = macro[TiGetClient(TR(h_cost_tile[i]))]->y2();
							if(right_l < 0) right_l = 0;
							if(right_r > chip_width) right_r = chip_width;
							if(right_t > chip_height) right_t = chip_height;
							bool left_e = true, right_e = true;
							if(left_t - left_b - diff_v < powerplan_width){
								left_e = false;
							}
							else{
								Rect left_ = { {left_l, left_b}, {left_r, left_t} };
								TiSrArea(NULL, horizontal_plane, &left_, empty_region, (ClientData)&left_e);
							}
							if(right_t - right_b - diff_v < powerplan_width){
								right_e = false;
							}
							else{
								Rect right_ = { {right_l, right_b}, {right_r, right_t} };
								TiSrArea(NULL, horizontal_plane, &right_, empty_region, (ClientData)&right_e);
							}
							if(left_e && !macro[TiGetClient(BL(h_cost_tile[i]))]->is_fixed()){
								// If left macro can move down(left macro is higher)
								macro[TiGetClient(BL(h_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(BL(h_cost_tile[i]))]->cx(), macro[TiGetClient(BL(h_cost_tile[i]))]->cy() - diff_v));
								break;
							}
							else if(right_e && !macro[TiGetClient(TR(h_cost_tile[i]))]->is_fixed()){
								// If right macro can move up(right macro is higher)
								macro[TiGetClient(TR(h_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(TR(h_cost_tile[i]))]->cx(), macro[TiGetClient(TR(h_cost_tile[i]))]->cy() + diff_v));
								break;
							}
						}
					}
					// moving macros vertically can't get good benefit, we try to seperate two macros horizontally
					for(int j = 0; j < h_edge_list[TiGetClient(BL(h_cost_tile[i])) + 1].size(); j++){
						if(h_edge_list[TiGetClient(BL(h_cost_tile[i])) + 1][j].to - 1 == TiGetClient(TR(h_cost_tile[i]))){
							h_edge_list[TiGetClient(BL(h_cost_tile[i])) + 1][j].weight = (macro[TiGetClient(BL(h_cost_tile[i]))]->w() + macro[TiGetClient(TR(h_cost_tile[i]))]->w()) / 2 + powerplan_width;
						}
					}
					for(int j = 0; j < r_h_edge_list[TiGetClient(TR(h_cost_tile[i])) + 1].size(); j++){
						if(r_h_edge_list[TiGetClient(TR(h_cost_tile[i])) + 1][j].from - 1 == TiGetClient(BL(h_cost_tile[i]))){
							r_h_edge_list[TiGetClient(TR(h_cost_tile[i])) + 1][j].weight = (macro[TiGetClient(BL(h_cost_tile[i]))]->w() + macro[TiGetClient(TR(h_cost_tile[i]))]->w()) / 2 + powerplan_width;
						}
					}
					break;
				}
			}
		}
	}
	else{
		for(int i = 0; i < v_cost_tile.size(); i++){
			// This cost tile is generated by two macros(left - right)
			if(TiGetBody(BL(v_cost_tile[i])) == SOLID_TILE && TiGetBody(TR(v_cost_tile[i])) == SOLID_TILE &&
				TiGetClient(BL(v_cost_tile[i])) != -1 && TiGetClient(TR(v_cost_tile[i])) != -1){
				// Do not space e(macro, null)
				if(macro[TiGetClient(BL(v_cost_tile[i]))]->name() != "null" && macro[TiGetClient(TR(v_cost_tile[i]))]->name() != "null"){
					// Determine if top macro more close to right hand side one or bottom macro.
					bool bottom_at_right = macro[TiGetClient(TR(v_cost_tile[i]))]->x1() > macro[TiGetClient(BL(v_cost_tile[i]))]->x1() && 
										macro[TiGetClient(TR(v_cost_tile[i]))]->x2() > macro[TiGetClient(BL(v_cost_tile[i]))]->x2(),
							top_at_right = macro[TiGetClient(BL(v_cost_tile[i]))]->x1() > macro[TiGetClient(TR(v_cost_tile[i]))]->x1() && 
										macro[TiGetClient(BL(v_cost_tile[i]))]->x2() > macro[TiGetClient(TR(v_cost_tile[i]))]->x2();

					if(bottom_at_right){
						// Calculate width and height of cost region
						double diff_h = macro[TiGetClient(BL(v_cost_tile[i]))]->x2() - macro[TiGetClient(TR(v_cost_tile[i]))]->x1(),
								diff_v =  macro[TiGetClient(BL(v_cost_tile[i]))]->y1() - macro[TiGetClient(TR(v_cost_tile[i]))]->y2();
						if(alpha * diff_h < beta * sqrt(diff_h * diff_v)){
							// moving macro horizontally is supposed to gain benefit
							double bottom_l = macro[TiGetClient(TR(v_cost_tile[i]))]->x2(),
									bottom_r = macro[TiGetClient(TR(v_cost_tile[i]))]->x2() + 2 * powerplan_width,
									bottom_t = macro[TiGetClient(TR(v_cost_tile[i]))]->y2() + min_spacing,
									bottom_b = macro[TiGetClient(TR(v_cost_tile[i]))]->y1() - min_spacing;
							if(bottom_r > chip_width) bottom_r = chip_width;
							if(bottom_b < 0) bottom_b = 0;
							if(bottom_t > chip_height) bottom_t = chip_height;
							double top_l = macro[TiGetClient(BL(v_cost_tile[i]))]->x1() - 2 * powerplan_width,
									top_r = macro[TiGetClient(BL(v_cost_tile[i]))]->x1(),
									top_t = macro[TiGetClient(BL(v_cost_tile[i]))]->y2() + min_spacing,
									top_b = macro[TiGetClient(BL(v_cost_tile[i]))]->y1() - min_spacing;
							if(top_l < 0) top_l = 0;
							if(top_b < 0) top_b = 0;
							if(top_t > chip_height) top_t = chip_height;
							bool bottom_e = true, top_e = true;
							if(bottom_r - bottom_l - diff_h < powerplan_width){
								bottom_e = false;
							}
							else{
								Rect bottom_ = { {bottom_l, bottom_b}, {bottom_r, bottom_t} };
								TiSrArea(NULL, vertical_plane, &bottom_, empty_region, (ClientData)&bottom_e);
							}
							if(top_r - top_l - diff_h < powerplan_width){
								top_e = false;
							}
							else{
								Rect top_ = { {top_l, top_b}, {top_r, top_t} };
								TiSrArea(NULL, vertical_plane, &top_, empty_region, (ClientData)&top_e);
							}
							if(bottom_e && !macro[TiGetClient(TR(v_cost_tile[i]))]->is_fixed()){
								// If bottom macro can move right(bottom macro is more close to right hand side)
								macro[TiGetClient(TR(v_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(TR(v_cost_tile[i]))]->cx() + diff_h, macro[TiGetClient(TR(v_cost_tile[i]))]->cy()));
								break;
							}
							else if(top_e && !macro[TiGetClient(BL(v_cost_tile[i]))]->is_fixed()){
								// If top macro can move left
								macro[TiGetClient(BL(v_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(BL(v_cost_tile[i]))]->cx() - diff_h, macro[TiGetClient(BL(v_cost_tile[i]))]->cy()));
								break;
							}
						}
					}
					else if(top_at_right){
						// Calculate width and height of cost region
						double diff_h = macro[TiGetClient(TR(v_cost_tile[i]))]->x2() - macro[TiGetClient(BL(v_cost_tile[i]))]->x1(),
								diff_v =  macro[TiGetClient(BL(v_cost_tile[i]))]->y1() - macro[TiGetClient(TR(v_cost_tile[i]))]->y2();
						if(alpha * diff_h < beta * sqrt(diff_h * diff_v)){
							// moving macro horizontally is supposed to gain benefit
							double bottom_l = macro[TiGetClient(TR(v_cost_tile[i]))]->x1() - 2 * powerplan_width,
									bottom_r = macro[TiGetClient(TR(v_cost_tile[i]))]->x1(),
									bottom_t = macro[TiGetClient(TR(v_cost_tile[i]))]->y2() + min_spacing,
									bottom_b = macro[TiGetClient(TR(v_cost_tile[i]))]->y1() - min_spacing;
							if(bottom_l < 0) bottom_l = 0;
							if(bottom_b < 0) bottom_b = 0;
							if(bottom_t > chip_height) bottom_t = chip_height;
							double top_l = macro[TiGetClient(BL(v_cost_tile[i]))]->x2(),
									top_r = macro[TiGetClient(BL(v_cost_tile[i]))]->x2() + 2 * powerplan_width,
									top_t = macro[TiGetClient(BL(v_cost_tile[i]))]->y2() + min_spacing,
									top_b = macro[TiGetClient(BL(v_cost_tile[i]))]->y1() - min_spacing;
							if(top_r > chip_width) top_r = chip_width;
							if(top_b < 0) top_b = 0;
							if(top_t > chip_height) top_t = chip_height;
							bool bottom_e = true, top_e = true;
							if(bottom_r - bottom_l - diff_h < powerplan_width){
								bottom_e = false;
							}
							else{
								Rect bottom_ = { {bottom_l, bottom_b}, {bottom_r, bottom_t} };
								TiSrArea(NULL, vertical_plane, &bottom_, empty_region, (ClientData)&bottom_e);
							}
							if(top_r - top_l - diff_h < powerplan_width){
								top_e = false;
							}
							else{
								Rect top_ = { {top_l, top_b}, {top_r, top_t} };
								TiSrArea(NULL, vertical_plane, &top_, empty_region, (ClientData)&top_e);
							}
							if(bottom_e && !macro[TiGetClient(TR(v_cost_tile[i]))]->is_fixed()){
								// If bottom macro can move left(bottom macro is more close to left hand side)
								macro[TiGetClient(TR(v_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(TR(v_cost_tile[i]))]->cx() - diff_h, macro[TiGetClient(TR(v_cost_tile[i]))]->cy()));
								break;
							}
							else if(top_e && !macro[TiGetClient(BL(v_cost_tile[i]))]->is_fixed()){
								// If top macro can move right
								macro[TiGetClient(BL(v_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(BL(v_cost_tile[i]))]->cx() + diff_h, macro[TiGetClient(BL(v_cost_tile[i]))]->cy()));
								break;
							}
						}
					}
					// moving macros horizontally can't get good benefit, we try to seperate two macros vertically
					for(int j = 0; j < v_edge_list[TiGetClient(TR(v_cost_tile[i])) + 1].size(); j++){
						if(v_edge_list[TiGetClient(TR(v_cost_tile[i])) + 1][j].to - 1 == TiGetClient(BL(v_cost_tile[i]))){
							v_edge_list[TiGetClient(TR(v_cost_tile[i])) + 1][j].weight = (macro[TiGetClient(TR(v_cost_tile[i]))]->h() + macro[TiGetClient(BL(v_cost_tile[i]))]->h()) / 2 + powerplan_width;
						}
					}
					for(int j = 0; j < r_v_edge_list[TiGetClient(BL(v_cost_tile[i])) + 1].size(); j++){
						if(r_v_edge_list[TiGetClient(BL(v_cost_tile[i])) + 1][j].from - 1 == TiGetClient(TR(v_cost_tile[i]))){
							r_v_edge_list[TiGetClient(BL(v_cost_tile[i])) + 1][j].weight = (macro[TiGetClient(TR(v_cost_tile[i]))]->h() + macro[TiGetClient(BL(v_cost_tile[i]))]->h()) / 2 + powerplan_width;
						}
					}
					break;
				}
			}
		}
	}
	// Updates zero slack edges
	while(Gh.longest_path(true) > chip_width){
		vector<edge*> h_zero_slack;
		h_zero_slack = Gh.zero_slack();
		for(int i = 0; i < h_zero_slack.size(); i++){
			// restore some of edges in critical path instead of restoring all.
			if(rand() % 5)
				continue;
			if(h_zero_slack[i]->from == 0){
				if(h_zero_slack[i]->weight == macro[h_zero_slack[i]->to - 1]->w() / 2 + powerplan_width){
					h_zero_slack[i]->weight = macro[h_zero_slack[i]->to - 1]->w() / 2;
				}
			}			
			if(h_zero_slack[i]->from != 0 && h_zero_slack[i]->to != macro.size() + 1){
				if(h_zero_slack[i]->weight == (macro[h_zero_slack[i]->from - 1]->w() + macro[h_zero_slack[i]->to - 1]->w()) / 2 + powerplan_width){
					h_zero_slack[i]->weight = (macro[h_zero_slack[i]->from - 1]->w() + macro[h_zero_slack[i]->to - 1]->w()) / 2 + min_spacing;
					// Also set reverse constraint graph
					for(int j = 0; j < r_h_edge_list[h_zero_slack[i]->to].size(); j++){
						if(r_h_edge_list[h_zero_slack[i]->to][j].from == h_zero_slack[i]->from)
							r_h_edge_list[h_zero_slack[i]->to][j].weight = (macro[h_zero_slack[i]->from - 1]->w() + macro[h_zero_slack[i]->to - 1]->w()) / 2 + min_spacing;
					}
				}
				if(h_zero_slack[i]->weight == (macro[h_zero_slack[i]->from - 1]->w() + macro[h_zero_slack[i]->to - 1]->w()) / 2 &&
					macro[h_zero_slack[i]->from - 1]->name() != "null" && macro[h_zero_slack[i]->to - 1]->name() != "null"){
					Gh.remove_edge(h_zero_slack[i]->from, h_zero_slack[i]->to);
				}
			}
		}
	}
	while(Gv.longest_path(false) > chip_height){
		vector<edge*> v_zero_slack;
		v_zero_slack = Gv.zero_slack();
		for(int i = 0; i < v_zero_slack.size(); i++){
			// restore some of edges in critical path instead of restoring all.
			if(rand() % 5)
				continue;
			if(v_zero_slack[i]->from == 0){
				if(v_zero_slack[i]->weight == macro[v_zero_slack[i]->to - 1]->h() / 2 + powerplan_width){
					v_zero_slack[i]->weight = macro[v_zero_slack[i]->to - 1]->h() / 2;
				}
			}
			if(v_zero_slack[i]->from != 0 && v_zero_slack[i]->to != macro.size() + 1){
				if(v_zero_slack[i]->weight == (macro[v_zero_slack[i]->from - 1]->h() + macro[v_zero_slack[i]->to - 1]->h()) / 2 + powerplan_width){
					v_zero_slack[i]->weight = (macro[v_zero_slack[i]->from - 1]->h() + macro[v_zero_slack[i]->to - 1]->h()) / 2 + min_spacing;
					// Also set reverse constraint graph
					for(int j = 0; j < r_v_edge_list[v_zero_slack[i]->to].size(); j++){
						if(r_v_edge_list[v_zero_slack[i]->to][j].from == v_zero_slack[i]->from)
							r_v_edge_list[v_zero_slack[i]->to][j].weight = (macro[v_zero_slack[i]->from - 1]->h() + macro[v_zero_slack[i]->to - 1]->h()) / 2 + min_spacing;
					}					
				}
				if(v_zero_slack[i]->weight == (macro[v_zero_slack[i]->from - 1]->h() + macro[v_zero_slack[i]->to - 1]->h()) / 2 &&
					macro[v_zero_slack[i]->from - 1]->name() != "null" && macro[v_zero_slack[i]->to - 1]->name() != "null"){
					Gv.remove_edge(v_zero_slack[i]->from, v_zero_slack[i]->to);
				}
			}
		}
	}
}


//=========TODO========
//check what kind of edges' weight maybe changed. ex. source->null, source->fixed, placed->placed
//all of changes should can be change to initial value if the edge involve in critical path

void improve_strategy3(vector<Macro*>& macro, Plane* horizontal_plane, Plane* vertical_plane){

	vector<Tile*> horizontal_tiles;
	vector<Tile*> vertical_tiles;

	Rect horizontal_region = { {0, 0}, {chip_width, chip_height} };
	Rect vertical_region = {{-chip_height, 0}, {0, chip_width}};	
	
	TiSrArea(NULL, horizontal_plane, &horizontal_region, collectSolidTiles, (ClientData)&horizontal_tiles);
	TiSrArea(NULL, vertical_plane, &vertical_region, collectSolidTiles, (ClientData)&vertical_tiles);
	vector<Tile*> h_cost_tile;
	// Iterative go through solid tiles find out cost tiles
	for(int i = 0; i < horizontal_tiles.size(); i++){
		if(TiGetClient(horizontal_tiles[i]) == -1)
			h_cost_tile.push_back(horizontal_tiles[i]);
	}
	// std::sort(h_cost_tile.begin(), h_cost_tile.end(), cost_tile_cmp);
	std::random_shuffle (h_cost_tile.begin(), h_cost_tile.end());
	vector<Tile*> v_cost_tile;
	for(int i = 0; i < vertical_tiles.size(); i++){
		if(TiGetClient(vertical_tiles[i]) == -1)
			v_cost_tile.push_back(vertical_tiles[i]);
	}
	// std::sort(v_cost_tile.begin(), v_cost_tile.end(), cost_tile_cmp);
	std::random_shuffle (v_cost_tile.begin(), v_cost_tile.end());

	bool change_hcg = false, change_vcg = false;

	if(rand() % 2){
		for(int i = 0; i < h_cost_tile.size(); i++){
			// This cost tile is generated by two macros(left - right)
			if(TiGetBody(BL(h_cost_tile[i])) == SOLID_TILE && TiGetBody(TR(h_cost_tile[i])) == SOLID_TILE &&
				TiGetClient(BL(h_cost_tile[i])) != -1 && TiGetClient(TR(h_cost_tile[i])) != -1){
				// Do not space e(macro, null)
				if(macro[TiGetClient(BL(h_cost_tile[i]))]->name() != "null" && macro[TiGetClient(TR(h_cost_tile[i]))]->name() != "null"){
					double left_left = macro[TiGetClient(BL(h_cost_tile[i]))]->x1() - 2 * powerplan_width,
							left_right = macro[TiGetClient(BL(h_cost_tile[i]))]->x1(),
							left_top = macro[TiGetClient(BL(h_cost_tile[i]))]->y2() + min_spacing,
							left_bottom = macro[TiGetClient(BL(h_cost_tile[i]))]->y1() - min_spacing;
					if(left_left < 0) left_left = 0;
					if(left_top > chip_height) left_top = chip_height;
					if(left_bottom < 0) left_bottom = 0;
					double right_left = macro[TiGetClient(TR(h_cost_tile[i]))]->x2(),
							right_right = macro[TiGetClient(TR(h_cost_tile[i]))]->x2() + 2 * powerplan_width,
							right_top = macro[TiGetClient(TR(h_cost_tile[i]))]->y2() + min_spacing,
							right_bottom = macro[TiGetClient(TR(h_cost_tile[i]))]->y1() - min_spacing;
					if(right_right > chip_width) right_right = chip_width;
					if(right_top > chip_height) right_top = chip_height;
					if(right_bottom < 0) right_bottom = 0;
					bool left_empty = true, right_empty = true;
					if(left_right - left_left < powerplan_width){
						left_empty = false;
						if(alpha * (left_right - left_left) < beta * sqrt((left_right - left_left) * macro[TiGetClient(BL(h_cost_tile[i]))]->h())
							&& !macro[TiGetClient(BL(h_cost_tile[i]))]->is_fixed()){
							macro[TiGetClient(BL(h_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(BL(h_cost_tile[i]))]->w() / 2, macro[TiGetClient(BL(h_cost_tile[i]))]->cy()));
						}
					}
					else{
						Rect left = { {left_left, left_bottom}, {left_right, left_top} };
						TiSrArea(NULL, horizontal_plane, &left, empty_region, (ClientData)&left_empty);
					}
					if(right_right - right_left < powerplan_width){
						right_empty = false;
						if(alpha * (right_right - right_left) < beta * sqrt((right_right - right_left) * macro[TiGetClient(TR(h_cost_tile[i]))]->h())
							&& !macro[TiGetClient(TR(h_cost_tile[i]))]->is_fixed()){
							macro[TiGetClient(TR(h_cost_tile[i]))]->updateXY(make_pair(chip_width - macro[TiGetClient(TR(h_cost_tile[i]))]->w() / 2, macro[TiGetClient(TR(h_cost_tile[i]))]->cy()));
						}
					}
					else{
						Rect right = { {right_left, right_bottom}, {right_right, right_top} };
						TiSrArea(NULL, horizontal_plane, &right, empty_region, (ClientData)&right_empty);
					}

					bool left_higher = macro[TiGetClient(BL(h_cost_tile[i]))]->y1() > macro[TiGetClient(TR(h_cost_tile[i]))]->y1() && 
										macro[TiGetClient(BL(h_cost_tile[i]))]->y2() > macro[TiGetClient(TR(h_cost_tile[i]))]->y2(),
							right_higher = macro[TiGetClient(TR(h_cost_tile[i]))]->y1() > macro[TiGetClient(BL(h_cost_tile[i]))]->y1() && 
										macro[TiGetClient(TR(h_cost_tile[i]))]->y2() > macro[TiGetClient(BL(h_cost_tile[i]))]->y2();

					double diff_h = macro[TiGetClient(TR(h_cost_tile[i]))]->x1() - macro[TiGetClient(BL(h_cost_tile[i]))]->x2();
					
					if(diff_h >= powerplan_width){
						continue;
					}

					if(left_higher){
						double diff_v =  macro[TiGetClient(TR(h_cost_tile[i]))]->y2() - macro[TiGetClient(BL(h_cost_tile[i]))]->y1();
						if(powerplan_width - diff_h < diff_v){
							if(alpha * (powerplan_width - diff_h) < beta * sqrt(diff_h * diff_v)){
								if(left_empty && !macro[TiGetClient(BL(h_cost_tile[i]))]->is_fixed()){
									macro[TiGetClient(BL(h_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(BL(h_cost_tile[i]))]->cx() - (powerplan_width - diff_h), macro[TiGetClient(BL(h_cost_tile[i]))]->cy()));
									break;
								}
								else if(right_empty && !macro[TiGetClient(TR(h_cost_tile[i]))]->is_fixed()){
									macro[TiGetClient(TR(h_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(TR(h_cost_tile[i]))]->cx() + (powerplan_width - diff_h), macro[TiGetClient(TR(h_cost_tile[i]))]->cy()));
									break;
								}
							}
						}
						else{
							if(alpha * diff_v < beta * sqrt(diff_h * diff_v)){
								double left_l = macro[TiGetClient(BL(h_cost_tile[i]))]->x1() - min_spacing,
										left_r = macro[TiGetClient(BL(h_cost_tile[i]))]->x2() + min_spacing,
										left_t = macro[TiGetClient(BL(h_cost_tile[i]))]->y2() + 2 * powerplan_width,
										left_b = macro[TiGetClient(BL(h_cost_tile[i]))]->y2();
								if(left_l < 0) left_l = 0;
								if(left_r > chip_width) left_r = chip_width;
								if(left_t > chip_height) left_t = chip_height;
								double right_l = macro[TiGetClient(TR(h_cost_tile[i]))]->x1() - min_spacing,
										right_r = macro[TiGetClient(TR(h_cost_tile[i]))]->x2() + min_spacing,
										right_t = macro[TiGetClient(TR(h_cost_tile[i]))]->y1(),
										right_b = macro[TiGetClient(TR(h_cost_tile[i]))]->y1() - 2 * powerplan_width;
								if(right_l < 0) right_l = 0;
								if(right_r > chip_width) right_r = chip_width;
								if(right_b < 0) right_b = 0;
								bool left_e = true, right_e = true;
								if(left_t - left_b - diff_v < powerplan_width){
									left_e = false;
								}
								else{
									Rect left_ = { {left_l, left_b}, {left_r, left_t} };
									TiSrArea(NULL, horizontal_plane, &left_, empty_region, (ClientData)&left_e);
								}
								if(right_t - right_b - diff_v < powerplan_width){
									right_e = false;
								}
								else{
									Rect right_ = { {right_l, right_b}, {right_r, right_t} };
									TiSrArea(NULL, horizontal_plane, &right_, empty_region, (ClientData)&right_e);
								}
								if(left_e && !macro[TiGetClient(BL(h_cost_tile[i]))]->is_fixed()){
									macro[TiGetClient(BL(h_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(BL(h_cost_tile[i]))]->cx(), macro[TiGetClient(BL(h_cost_tile[i]))]->cy() + diff_v));
									break;
								}
								else if(right_e && !macro[TiGetClient(TR(h_cost_tile[i]))]->is_fixed()){
									macro[TiGetClient(TR(h_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(TR(h_cost_tile[i]))]->cx(), macro[TiGetClient(TR(h_cost_tile[i]))]->cy() - diff_v));
									break;
								}
							}
						}
					}
					else if(right_higher){
						double diff_v =  macro[TiGetClient(BL(h_cost_tile[i]))]->y2() - macro[TiGetClient(TR(h_cost_tile[i]))]->y1();
						if(powerplan_width - diff_h < diff_v){
							if(alpha * (powerplan_width - diff_h) < beta * sqrt(diff_h * diff_v)){
								if(left_empty && !macro[TiGetClient(BL(h_cost_tile[i]))]->is_fixed()){
									macro[TiGetClient(BL(h_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(BL(h_cost_tile[i]))]->cx() - (powerplan_width - diff_h), macro[TiGetClient(BL(h_cost_tile[i]))]->cy()));
									break;
								}
								else if(right_empty && !macro[TiGetClient(TR(h_cost_tile[i]))]->is_fixed()){
									macro[TiGetClient(TR(h_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(TR(h_cost_tile[i]))]->cx() + (powerplan_width - diff_h), macro[TiGetClient(TR(h_cost_tile[i]))]->cy()));
									break;
								}
							}
						}
						else{
							if(alpha * diff_v < beta * sqrt(diff_h * diff_v)){
								double left_l = macro[TiGetClient(BL(h_cost_tile[i]))]->x1() - min_spacing,
										left_r = macro[TiGetClient(BL(h_cost_tile[i]))]->x2() + min_spacing,
										left_t = macro[TiGetClient(BL(h_cost_tile[i]))]->y1(),
										left_b = macro[TiGetClient(BL(h_cost_tile[i]))]->y1() - 2 * powerplan_width;
								if(left_l < 0) left_l = 0;
								if(left_r > chip_width) left_r = chip_width;
								if(left_b < 0) left_b = 0;
								double right_l = macro[TiGetClient(TR(h_cost_tile[i]))]->x1() - min_spacing,
										right_r = macro[TiGetClient(TR(h_cost_tile[i]))]->x2() + min_spacing,
										right_t = macro[TiGetClient(TR(h_cost_tile[i]))]->y2() + 2 * powerplan_width,
										right_b = macro[TiGetClient(TR(h_cost_tile[i]))]->y2();
								if(right_l < 0) right_l = 0;
								if(right_r > chip_width) right_r = chip_width;
								if(right_t > chip_height) right_t = chip_height;
								bool left_e = true, right_e = true;
								if(left_t - left_b - diff_v < powerplan_width){
									left_e = false;
								}
								else{
									Rect left_ = { {left_l, left_b}, {left_r, left_t} };
									TiSrArea(NULL, horizontal_plane, &left_, empty_region, (ClientData)&left_e);
								}
								if(right_t - right_b - diff_v < powerplan_width){
									right_e = false;
								}
								else{
									Rect right_ = { {right_l, right_b}, {right_r, right_t} };
									TiSrArea(NULL, horizontal_plane, &right_, empty_region, (ClientData)&right_e);
								}
								if(left_e && !macro[TiGetClient(BL(h_cost_tile[i]))]->is_fixed()){
									macro[TiGetClient(BL(h_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(BL(h_cost_tile[i]))]->cx(), macro[TiGetClient(BL(h_cost_tile[i]))]->cy() - diff_v));
									break;
								}
								else if(right_e && !macro[TiGetClient(TR(h_cost_tile[i]))]->is_fixed()){
									macro[TiGetClient(TR(h_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(TR(h_cost_tile[i]))]->cx(), macro[TiGetClient(TR(h_cost_tile[i]))]->cy() + diff_v));
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	else{
		for(int i = 0; i < v_cost_tile.size(); i++){
			// This cost tile is generated by two macro(top - bottom)
			if(TiGetBody(BL(v_cost_tile[i])) == SOLID_TILE && TiGetBody(TR(v_cost_tile[i])) == SOLID_TILE &&
				TiGetClient(BL(v_cost_tile[i])) != -1 && TiGetClient(TR(v_cost_tile[i])) != -1){
				// Do not space e(macro, null)
				if(macro[TiGetClient(BL(v_cost_tile[i]))]->name() != "null" && macro[TiGetClient(TR(v_cost_tile[i]))]->name() != "null"){
					
					double bottom_left = macro[TiGetClient(TR(v_cost_tile[i]))]->x1() - min_spacing,
							bottom_right = macro[TiGetClient(TR(v_cost_tile[i]))]->x2() + min_spacing,
							bottom_top = macro[TiGetClient(TR(v_cost_tile[i]))]->y1(),
							bottom_bottom = macro[TiGetClient(TR(v_cost_tile[i]))]->y1() - 2 * powerplan_width;
					if(bottom_bottom < 0) bottom_bottom = 0;
					if(bottom_left < 0) bottom_left = 0;
					if(bottom_right > chip_width) bottom_right = chip_width;
					double top_left = macro[TiGetClient(BL(v_cost_tile[i]))]->x1() - min_spacing,
							top_right = macro[TiGetClient(BL(v_cost_tile[i]))]->x2() + min_spacing,
							top_top = macro[TiGetClient(BL(v_cost_tile[i]))]->y2() + 2 * powerplan_width,
							top_bottom = macro[TiGetClient(BL(v_cost_tile[i]))]->y2();
					if(top_top > chip_height) top_top = chip_height;
					if(top_left < 0) top_left = 0;
					if(top_right > chip_width) top_right = chip_width;

					bool bottom_empty = true, top_empty = true;
					if(bottom_top - bottom_bottom < powerplan_width){
						bottom_empty = false;
						if(alpha * (bottom_top - bottom_bottom) < beta * sqrt((bottom_top - bottom_bottom) * macro[TiGetClient(TR(v_cost_tile[i]))]->w())
							&& !macro[TiGetClient(TR(v_cost_tile[i]))]->is_fixed()){
							macro[TiGetClient(TR(v_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(TR(v_cost_tile[i]))]->cx(), macro[TiGetClient(TR(v_cost_tile[i]))]->h() / 2));
						}
					}
					else{
						Rect bottom = { {-bottom_top, bottom_left}, {-bottom_bottom, bottom_right} };
						TiSrArea(NULL, vertical_plane, &bottom, empty_region, (ClientData)&bottom_empty);
					}
					if(top_top - top_bottom < powerplan_width){
						top_empty = false;
						if(alpha * (top_top - top_bottom) < beta * sqrt((top_top - top_bottom) * macro[TiGetClient(BL(v_cost_tile[i]))]->w())
							&& !macro[TiGetClient(BL(v_cost_tile[i]))]->is_fixed()){
							macro[TiGetClient(BL(v_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(BL(v_cost_tile[i]))]->cx(), chip_height - macro[TiGetClient(BL(v_cost_tile[i]))]->h() / 2));
						}
					}
					else{
						Rect top = { {-top_top, top_left}, {-top_bottom, top_right} };
						TiSrArea(NULL, vertical_plane, &top, empty_region, (ClientData)&top_empty);
					}

					bool bottom_at_right = macro[TiGetClient(TR(v_cost_tile[i]))]->x1() > macro[TiGetClient(BL(v_cost_tile[i]))]->x1() && 
										macro[TiGetClient(TR(v_cost_tile[i]))]->x2() > macro[TiGetClient(BL(v_cost_tile[i]))]->x2(),
							top_at_right = macro[TiGetClient(BL(v_cost_tile[i]))]->x1() > macro[TiGetClient(TR(v_cost_tile[i]))]->x1() && 
										macro[TiGetClient(BL(v_cost_tile[i]))]->x2() > macro[TiGetClient(TR(v_cost_tile[i]))]->x2();

					double diff_v =  macro[TiGetClient(BL(v_cost_tile[i]))]->y1() - macro[TiGetClient(TR(v_cost_tile[i]))]->y2();

					if(diff_v >= powerplan_width){
						continue;
					}

					if(bottom_at_right){
						double diff_h = macro[TiGetClient(BL(v_cost_tile[i]))]->x2() - macro[TiGetClient(TR(v_cost_tile[i]))]->x1();
						if(powerplan_width - diff_v < diff_h){
							if(alpha * (powerplan_width - diff_v) < beta * sqrt(diff_h * diff_v)){
								if(top_empty && !macro[TiGetClient(BL(v_cost_tile[i]))]->is_fixed()){
									macro[TiGetClient(BL(v_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(BL(v_cost_tile[i]))]->cx(), macro[TiGetClient(BL(v_cost_tile[i]))]->cy() + (powerplan_width - diff_v)));
									break;
								}
								else if(bottom_empty && !macro[TiGetClient(TR(v_cost_tile[i]))]->is_fixed()){
									macro[TiGetClient(TR(v_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(TR(v_cost_tile[i]))]->cx(), macro[TiGetClient(TR(v_cost_tile[i]))]->cy() - (powerplan_width - diff_v)));
									break;
								}
							}
						}
						else{
							if(alpha * diff_h < beta * sqrt(diff_h * diff_v)){
								double bottom_l = macro[TiGetClient(TR(v_cost_tile[i]))]->x2(),
										bottom_r = macro[TiGetClient(TR(v_cost_tile[i]))]->x2() + 2 * powerplan_width,
										bottom_t = macro[TiGetClient(TR(v_cost_tile[i]))]->y2() + min_spacing,
										bottom_b = macro[TiGetClient(TR(v_cost_tile[i]))]->y1() - min_spacing;
								if(bottom_r > chip_width) bottom_r = chip_width;
								if(bottom_b < 0) bottom_b = 0;
								if(bottom_t > chip_height) bottom_t = chip_height;
								double top_l = macro[TiGetClient(BL(v_cost_tile[i]))]->x1() - 2 * powerplan_width,
										top_r = macro[TiGetClient(BL(v_cost_tile[i]))]->x1(),
										top_t = macro[TiGetClient(BL(v_cost_tile[i]))]->y2() + min_spacing,
										top_b = macro[TiGetClient(BL(v_cost_tile[i]))]->y1() - min_spacing;
								if(top_l < 0) top_l = 0;
								if(top_b < 0) top_b = 0;
								if(top_t > chip_height) top_t = chip_height;
								bool bottom_e = true, top_e = true;
								if(bottom_r - bottom_l - diff_h < powerplan_width){
									bottom_e = false;
								}
								else{
									Rect bottom_ = { {bottom_l, bottom_b}, {bottom_r, bottom_t} };
									TiSrArea(NULL, vertical_plane, &bottom_, empty_region, (ClientData)&bottom_e);
								}
								if(top_r - top_l - diff_h < powerplan_width){
									top_e = false;
								}
								else{
									Rect top_ = { {top_l, top_b}, {top_r, top_t} };
									TiSrArea(NULL, vertical_plane, &top_, empty_region, (ClientData)&top_e);
								}
								if(bottom_e && !macro[TiGetClient(TR(v_cost_tile[i]))]->is_fixed()){
									macro[TiGetClient(TR(v_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(TR(v_cost_tile[i]))]->cx() + diff_h, macro[TiGetClient(TR(v_cost_tile[i]))]->cy()));
									break;
								}
								else if(top_e && !macro[TiGetClient(BL(v_cost_tile[i]))]->is_fixed()){
									macro[TiGetClient(BL(v_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(BL(v_cost_tile[i]))]->cx() - diff_h, macro[TiGetClient(BL(v_cost_tile[i]))]->cy()));
									break;
								}
							}
						}
					}
					else if(top_at_right){
						double diff_h = macro[TiGetClient(TR(v_cost_tile[i]))]->x2() - macro[TiGetClient(BL(v_cost_tile[i]))]->x1();
						if(powerplan_width - diff_v < diff_h){
							if(alpha * (powerplan_width - diff_v) < beta * sqrt(diff_h * diff_v)){
								if(top_empty && !macro[TiGetClient(BL(v_cost_tile[i]))]->is_fixed()){
									macro[TiGetClient(BL(v_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(BL(v_cost_tile[i]))]->cx(), macro[TiGetClient(BL(v_cost_tile[i]))]->cy() + (powerplan_width - diff_v)));
									break;
								}
								else if(bottom_empty && !macro[TiGetClient(TR(v_cost_tile[i]))]->is_fixed()){
									macro[TiGetClient(TR(v_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(TR(v_cost_tile[i]))]->cx(), macro[TiGetClient(TR(v_cost_tile[i]))]->cy() - (powerplan_width - diff_v)));
									break;
								}
							}
						}
						else{
							if(alpha * diff_h < beta * sqrt(diff_h * diff_v)){
								double bottom_l = macro[TiGetClient(TR(v_cost_tile[i]))]->x1() - 2 * powerplan_width,
										bottom_r = macro[TiGetClient(TR(v_cost_tile[i]))]->x1(),
										bottom_t = macro[TiGetClient(TR(v_cost_tile[i]))]->y2() + min_spacing,
										bottom_b = macro[TiGetClient(TR(v_cost_tile[i]))]->y1() - min_spacing;
								if(bottom_l < 0) bottom_l = 0;
								if(bottom_b < 0) bottom_b = 0;
								if(bottom_t > chip_height) bottom_t = chip_height;
								double top_l = macro[TiGetClient(BL(v_cost_tile[i]))]->x2(),
										top_r = macro[TiGetClient(BL(v_cost_tile[i]))]->x2() + 2 * powerplan_width,
										top_t = macro[TiGetClient(BL(v_cost_tile[i]))]->y2() + min_spacing,
										top_b = macro[TiGetClient(BL(v_cost_tile[i]))]->y1() - min_spacing;
								if(top_r > chip_width) top_r = chip_width;
								if(top_b < 0) top_b = 0;
								if(top_t > chip_height) top_t = chip_height;
								bool bottom_e = true, top_e = true;
								if(bottom_r - bottom_l - diff_h < powerplan_width){
									bottom_e = false;
								}
								else{
									Rect bottom_ = { {bottom_l, bottom_b}, {bottom_r, bottom_t} };
									TiSrArea(NULL, vertical_plane, &bottom_, empty_region, (ClientData)&bottom_e);
								}
								if(top_r - top_l - diff_h < powerplan_width){
									top_e = false;
								}
								else{
									Rect top_ = { {top_l, top_b}, {top_r, top_t} };
									TiSrArea(NULL, vertical_plane, &top_, empty_region, (ClientData)&top_e);
								}
								if(bottom_e && !macro[TiGetClient(TR(v_cost_tile[i]))]->is_fixed()){
									macro[TiGetClient(TR(v_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(TR(v_cost_tile[i]))]->cx() - diff_h, macro[TiGetClient(TR(v_cost_tile[i]))]->cy()));
									break;
								}
								else if(top_e && !macro[TiGetClient(BL(v_cost_tile[i]))]->is_fixed()){
									macro[TiGetClient(BL(v_cost_tile[i]))]->updateXY(make_pair(macro[TiGetClient(BL(v_cost_tile[i]))]->cx() + diff_h, macro[TiGetClient(BL(v_cost_tile[i]))]->cy()));
									break;
								}
							}
						}
					}
				}
			}
		}
	}
}

int improve_strategy4(vector<Macro*>& macro, const vector<Macro*>& native_macro){
	vector<double> displacement;
	double mean = 0;
	for(int i = 0; i < macro.size(); i++){
		double dist = std::abs(native_macro[i]->x1() - macro[i]->x1()) + std::abs(native_macro[i]->y1() - macro[i]->y1());
		displacement.push_back(dist);
		mean += dist;
	}
	mean = mean / macro.size();
	double vari = 0;
    for (int i = 0; i < macro.size(); i++){
        vari += pow(displacement[i] - mean, 2);
	}
	vari = vari / macro.size();
	double SD = sqrt(vari); // Standard Deviation
	vector<int> access_order;
	for(int i = 0; i < macro.size(); i++){
		access_order.push_back(i);
	}
	std::random_shuffle (access_order.begin(), access_order.end());
	for(int i = 0; i < macro.size(); i++){
		if(displacement[access_order[i]] - mean > 0 && !macro[access_order[i]]->is_fixed()){
			return access_order[i];
			// macro[access_order[i]]->updateXY(make_pair(native_macro[access_order[i]]->cx(), native_macro[access_order[i]]->cy()));
			// break;
		}
	}
	return -1;
}

void improve_strategy5(vector<Macro*>& macro, Plane* horizontal_plane, Plane* vertical_plane, Graph& Gh, Graph& Gv){

	vector<edge>* h_edge_list = Gh.get_edge_list();
	vector<edge>* v_edge_list = Gv.get_edge_list();
	vector<edge>* r_h_edge_list = Gh.get_reverse_edge_list();
	vector<edge>* r_v_edge_list = Gv.get_reverse_edge_list();

	vector<Tile*> horizontal_tiles;
	vector<Tile*> vertical_tiles;

	Rect horizontal_region = { {0, 0}, {chip_width, chip_height} };
	Rect vertical_region = {{-chip_height, 0}, {0, chip_width}};	
	
	TiSrArea(NULL, horizontal_plane, &horizontal_region, collectSolidTiles, (ClientData)&horizontal_tiles);
	TiSrArea(NULL, vertical_plane, &vertical_region, collectSolidTiles, (ClientData)&vertical_tiles);
	vector<Tile*> h_cost_tile;
	// Iterative go through solid tiles find out cost tiles
	for(int i = 0; i < horizontal_tiles.size(); i++){
		if(TiGetClient(horizontal_tiles[i]) == -1)
			h_cost_tile.push_back(horizontal_tiles[i]);
	}
	// std::sort(h_cost_tile.begin(), h_cost_tile.end(), cost_tile_cmp);
	std::random_shuffle (h_cost_tile.begin(), h_cost_tile.end());
	vector<Tile*> v_cost_tile;
	for(int i = 0; i < vertical_tiles.size(); i++){
		if(TiGetClient(vertical_tiles[i]) == -1)
			v_cost_tile.push_back(vertical_tiles[i]);
	}
	// std::sort(v_cost_tile.begin(), v_cost_tile.end(), cost_tile_cmp);
	std::random_shuffle (v_cost_tile.begin(), v_cost_tile.end());
	bool found = false;
	if(rand() % 2){
		for(int i = 0; i < h_cost_tile.size(); i++){
			// This cost tile is generated by two macros(left - right)
			if(TiGetBody(BL(h_cost_tile[i])) == SOLID_TILE && TiGetBody(TR(h_cost_tile[i])) == SOLID_TILE &&
				TiGetClient(BL(h_cost_tile[i])) != -1 && TiGetClient(TR(h_cost_tile[i])) != -1){
				// Do not space e(macro, null)
				if(macro[TiGetClient(BL(h_cost_tile[i]))]->name() != "null" && macro[TiGetClient(TR(h_cost_tile[i]))]->name() != "null"){		
					for(int j = 0; j < h_edge_list[TiGetClient(BL(h_cost_tile[i])) + 1].size(); j++){
						if(h_edge_list[TiGetClient(BL(h_cost_tile[i])) + 1][j].to - 1 == TiGetClient(TR(h_cost_tile[i]))){
							h_edge_list[TiGetClient(BL(h_cost_tile[i])) + 1][j].weight = (macro[TiGetClient(BL(h_cost_tile[i]))]->w() + macro[TiGetClient(TR(h_cost_tile[i]))]->w()) / 2 + powerplan_width;
						}
					}
					for(int j = 0; j < r_h_edge_list[TiGetClient(TR(h_cost_tile[i])) + 1].size(); j++){
						if(r_h_edge_list[TiGetClient(TR(h_cost_tile[i])) + 1][j].from - 1 == TiGetClient(BL(h_cost_tile[i]))){
							r_h_edge_list[TiGetClient(TR(h_cost_tile[i])) + 1][j].weight = (macro[TiGetClient(BL(h_cost_tile[i]))]->w() + macro[TiGetClient(TR(h_cost_tile[i]))]->w()) / 2 + powerplan_width;
							found = true;
						}
					}
				}
			}
			if(found){
				break;
			}
		}
	}
	else{
		for(int i = 0; i < v_cost_tile.size(); i++){
			if(TiGetBody(BL(v_cost_tile[i])) == SOLID_TILE && TiGetBody(TR(v_cost_tile[i])) == SOLID_TILE &&
				TiGetClient(BL(v_cost_tile[i])) != -1 && TiGetClient(TR(v_cost_tile[i])) != -1){
				// Do not space e(macro, null)
				if(macro[TiGetClient(BL(v_cost_tile[i]))]->name() != "null" && macro[TiGetClient(TR(v_cost_tile[i]))]->name() != "null"){
					for(int j = 0; j < v_edge_list[TiGetClient(TR(v_cost_tile[i])) + 1].size(); j++){
						if(v_edge_list[TiGetClient(TR(v_cost_tile[i])) + 1][j].to - 1 == TiGetClient(BL(v_cost_tile[i]))){
							v_edge_list[TiGetClient(TR(v_cost_tile[i])) + 1][j].weight = (macro[TiGetClient(TR(v_cost_tile[i]))]->h() + macro[TiGetClient(BL(v_cost_tile[i]))]->h()) / 2 + powerplan_width;
						}
					}
					for(int j = 0; j < r_v_edge_list[TiGetClient(BL(v_cost_tile[i])) + 1].size(); j++){
						if(r_v_edge_list[TiGetClient(BL(v_cost_tile[i])) + 1][j].from - 1 == TiGetClient(TR(v_cost_tile[i]))){
							r_v_edge_list[TiGetClient(BL(v_cost_tile[i])) + 1][j].weight = (macro[TiGetClient(TR(v_cost_tile[i]))]->h() + macro[TiGetClient(BL(v_cost_tile[i]))]->h()) / 2 + powerplan_width;
							found = true;
						}
					}
				}
			}
			if(found){
				break;
			}
		}
	}
	// Updates zero slack edges
	while(Gh.longest_path(true) > chip_width){
		vector<edge*> h_zero_slack;
		h_zero_slack = Gh.zero_slack();
		for(int i = 0; i < h_zero_slack.size(); i++){
			// restore some of edges in critical path instead of restoring all.
			if(rand() % 5)
				continue;
			if(h_zero_slack[i]->from == 0){
				if(h_zero_slack[i]->weight == macro[h_zero_slack[i]->to - 1]->w() / 2 + powerplan_width){
					h_zero_slack[i]->weight = macro[h_zero_slack[i]->to - 1]->w() / 2;
				}
			}			
			if(h_zero_slack[i]->from != 0 && h_zero_slack[i]->to != macro.size() + 1){
				if(h_zero_slack[i]->weight == (macro[h_zero_slack[i]->from - 1]->w() + macro[h_zero_slack[i]->to - 1]->w()) / 2 + powerplan_width){
					h_zero_slack[i]->weight = (macro[h_zero_slack[i]->from - 1]->w() + macro[h_zero_slack[i]->to - 1]->w()) / 2 + min_spacing;
					// Also set reverse constraint graph
					for(int j = 0; j < r_h_edge_list[h_zero_slack[i]->to].size(); j++){
						if(r_h_edge_list[h_zero_slack[i]->to][j].from == h_zero_slack[i]->from)
							r_h_edge_list[h_zero_slack[i]->to][j].weight = (macro[h_zero_slack[i]->from - 1]->w() + macro[h_zero_slack[i]->to - 1]->w()) / 2 + min_spacing;
					}
				}
				if(h_zero_slack[i]->weight == (macro[h_zero_slack[i]->from - 1]->w() + macro[h_zero_slack[i]->to - 1]->w()) / 2 &&
					macro[h_zero_slack[i]->from - 1]->name() != "null" && macro[h_zero_slack[i]->to - 1]->name() != "null"){
					Gh.remove_edge(h_zero_slack[i]->from, h_zero_slack[i]->to);
				}
			}
		}
	}
	while(Gv.longest_path(false) > chip_height){
		vector<edge*> v_zero_slack;
		v_zero_slack = Gv.zero_slack();
		for(int i = 0; i < v_zero_slack.size(); i++){
			// restore some of edges in critical path instead of restoring all.
			if(rand() % 5)
				continue;
			if(v_zero_slack[i]->from == 0){
				if(v_zero_slack[i]->weight == macro[v_zero_slack[i]->to - 1]->h() / 2 + powerplan_width){
					v_zero_slack[i]->weight = macro[v_zero_slack[i]->to - 1]->h() / 2;
				}
			}
			if(v_zero_slack[i]->from != 0 && v_zero_slack[i]->to != macro.size() + 1){
				if(v_zero_slack[i]->weight == (macro[v_zero_slack[i]->from - 1]->h() + macro[v_zero_slack[i]->to - 1]->h()) / 2 + powerplan_width){
					v_zero_slack[i]->weight = (macro[v_zero_slack[i]->from - 1]->h() + macro[v_zero_slack[i]->to - 1]->h()) / 2 + min_spacing;
					// Also set reverse constraint graph
					for(int j = 0; j < r_v_edge_list[v_zero_slack[i]->to].size(); j++){
						if(r_v_edge_list[v_zero_slack[i]->to][j].from == v_zero_slack[i]->from)
							r_v_edge_list[v_zero_slack[i]->to][j].weight = (macro[v_zero_slack[i]->from - 1]->h() + macro[v_zero_slack[i]->to - 1]->h()) / 2 + min_spacing;
					}					
				}
				if(v_zero_slack[i]->weight == (macro[v_zero_slack[i]->from - 1]->h() + macro[v_zero_slack[i]->to - 1]->h()) / 2 &&
					macro[v_zero_slack[i]->from - 1]->name() != "null" && macro[v_zero_slack[i]->to - 1]->name() != "null"){
					Gv.remove_edge(v_zero_slack[i]->from, v_zero_slack[i]->to);
				}
			}
		}
	}	
}