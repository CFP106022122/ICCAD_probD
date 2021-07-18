#include <iostream>
#include <vector>
#include "macro.h"
#include "corner_stitch/utils/update.h"

extern double chip_width, chip_height, buffer_constraint, powerplan_width;
extern int micron;

static int collectTiles(Tile *tile, ClientData cdata){
	((vector<Tile*>*) cdata)->push_back(tile);
	return 0;
}

static int buffer_constraint_check(Tile *tile, ClientData cdata){
	if(TiGetBody(tile) != SOLID_TILE){
		*((bool*)cdata) = true;
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
		int left = LEFT(horizontal_tiles[i]), right = RIGHT(horizontal_tiles[i]);
		if(left < 0)	left = 0;
		if(right > (int)chip_width) right = (int)chip_width;

		// If tile's width is smaller than powerplan_width, makes the tile become cost_area
		if(right - left < (int)powerplan_width){
			Rect horizontal_cost_tile = { {left, BOTTOM(horizontal_tiles[i])}, {right, TOP(horizontal_tiles[i])} };
			if (InsertTile(&horizontal_cost_tile, horizontal_plane) == NULL) {
				printf("(Horizontal)Invalid insertion due to the overlapping with existing rectangles: (%d, %d) - (%d, %d).\n",
						left, BOTTOM(horizontal_tiles[i]), right, TOP(horizontal_tiles[i]));
			}

			// Needs to add corresponding tile into vertical corner_stitch data structure at the same time
			Rect vertical_cost_tile = { {-TOP(horizontal_tiles[i]), left}, {-BOTTOM(horizontal_tiles[i]), right} };
			if (InsertTile(&vertical_cost_tile, vertical_plane) == NULL) {
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

		// Show this tile's position
		// printf("%d %d %d %d %d %d %d %d\n", LEFT(vertical_tiles[i]), BOTTOM(vertical_tiles[i]),
		// 									RIGHT(vertical_tiles[i]), BOTTOM(vertical_tiles[i]),
		// 									RIGHT(vertical_tiles[i]), TOP(vertical_tiles[i]),
		// 									LEFT(vertical_tiles[i]), TOP(vertical_tiles[i]));

		// Decides left, right boundary of tile
		int left = LEFT(vertical_tiles[i]), right = RIGHT(vertical_tiles[i]);
		if(left < -(int)chip_height)	left = -(int)chip_height;
		if(right > 0) right = 0;

		// If tile's width is smaller than powerplan_width, makes the tile become cost_area
		if(right - left < (int)powerplan_width){
			// Needs to add corresponding tile into horizontal corner_stitch data structure at the same time
			Rect horizontal_cost_tile = { {BOTTOM(vertical_tiles[i]), -right}, {TOP(vertical_tiles[i]), -left} };
			if (InsertTile(&horizontal_cost_tile, horizontal_plane) == NULL) {
				printf("(Horizontal)Invalid insertion due to the overlapping with existing rectangles: (%d, %d) - (%d, %d).\n",
						BOTTOM(vertical_tiles[i]), -right, TOP(vertical_tiles[i]), -left);
			}		
			Rect vertical_cost_tile = { {left, BOTTOM(vertical_tiles[i])}, {right, TOP(vertical_tiles[i])} };
			if (InsertTile(&vertical_cost_tile, vertical_plane) == NULL) {
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


void corner_stitch(vector<Macro*>& macro)
{

	// powerplan_cost
	double powerplan_cost = 0;

	// Create horizontal, vertical corner stitch data structure
	Plane* horizontal_plane = CreateTilePlane();
	Plane* vertical_plane = CreateTilePlane();

	// Insert macros into plane
	for(int i = 0; i < macro.size(); i++){

		// In horizontal (left, bottom), (right, top) = (x1, y1), (x2, y2)
		Rect horizontal_rect = { {(int)macro[i]->x1(), (int)macro[i]->y1()}, {(int)macro[i]->x2(), (int)macro[i]->y2()} };
		// In vertical (left, bottom), (right, top) = (-y2, x1), (-y1, x2)
		Rect vertical_rect = { {-(int)macro[i]->y2(), (int)macro[i]->x1()}, {-(int)macro[i]->y1(), (int)macro[i]->x2()} };

		if (InsertTile(&horizontal_rect, horizontal_plane) == NULL) {
			printf("(Horizontal)Invalid insertion due to the overlapping with existing rectangles: (%d, %d) - (%d, %d).\n",
					(int)macro[i]->x1(), (int)macro[i]->y1(), (int)macro[i]->x2(), (int)macro[i]->y2());
		}
		if (InsertTile(&vertical_rect, vertical_plane) == NULL) {
			printf("(Vertical)Invalid insertion due to the overlapping with existing rectangles: (%d, %d) - (%d, %d).\n",
					-(int)macro[i]->y2(), (int)macro[i]->x1(), -(int)macro[i]->y1(), (int)macro[i]->x2());
		}
	}

	// Rect horizontal(vertical)_region represents place region
	Rect horizontal_region = { {0, 0}, {(int)chip_width, (int)chip_height} };
	Rect vertical_region = { {-(int)chip_height, 0}, {0, (int)chip_width} };

	// Update corner stitch data structure until all cost_area are found
	bool horizontal_updated = true, vertical_updated = true;
	while(horizontal_updated && vertical_updated){
		horizontal_updated = false;
		vertical_updated = false;
		horizontal_updated = Horizontal_update(horizontal_plane, vertical_plane, horizontal_region, vertical_region, powerplan_cost);
		vertical_updated = Vertical_update(horizontal_plane, vertical_plane, horizontal_region, vertical_region, powerplan_cost);
	}

	// Show powerplan cost
	printf("Powerplan Cost = %lf\n", powerplan_cost);

	// Vector stores invalid macros found by corner stitch data structure
	vector<int> invalid_macros;
	for(int i = 0; i < macro.size(); i++){
		// Calculate search area boundary for macro
		int left = (int)(macro[i]->x1() - buffer_constraint - powerplan_width),
			right = (int)(macro[i]->x2() + buffer_constraint + powerplan_width),
			top = (int)(macro[i]->y2() + buffer_constraint + powerplan_width),
			bottom = (int)(macro[i]->y1() - buffer_constraint - powerplan_width);
		if(left < 0) left = 0;
		if(right > (int)chip_width) right = (int)chip_width;
		if(top > (int)chip_height) top = (int)chip_height;
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
		printf("macro[%d] is invalid\n", i);
	}

	// Remove tile plane
	RemoveTilePlane(horizontal_plane);
	RemoveTilePlane(vertical_plane);
}