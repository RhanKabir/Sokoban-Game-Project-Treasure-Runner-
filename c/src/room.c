#include <stdlib.h>
#include <string.h>

#include "room.h"

Room *room_create(int id, const char *name, int width, int height){
    Room *r = malloc(sizeof(Room));
        if(!r){
            return NULL;
        }

    r->id = id;
    if (width < 1) {
        r->width = 1;
    } else {
        r->width = width;
    }

    if (height < 1) {
        r->height = 1;
    } else {
        r->height = height;
    }

    if(name){
        r->name = malloc(strlen(name) + 1);
        strcpy(r->name, name);
    }
    else{
        r->name = NULL;
    }

    r->floor_grid = NULL;
    r->portals = NULL;
    r->portal_count = 0;
    r->treasures = NULL;
    r->treasure_count = 0;
    r->pushables = NULL;
    r->pushable_count = 0;

    return r;

}

int room_get_width(const Room *r){
    if(!r){
        return 0;
    }

    return r->width;
}

int room_get_height(const Room *r){
    if(!r){
        return 0;
    }

    return r->height;
}

Status room_set_floor_grid(Room *r, bool *floor_grid){
    if(!r){
        return INVALID_ARGUMENT;
    }

    if(r->floor_grid != NULL){
        free(r->floor_grid);
    }

    r->floor_grid = floor_grid;

    return OK;
}

Status room_set_portals(Room *r, Portal *portals, int portal_count){
    if(!r){
        return INVALID_ARGUMENT;
    }

    if(portal_count > 0 && portals == NULL){
        return INVALID_ARGUMENT;
    }

    if(r->portals != NULL){
            for(int i = 0; i < r->portal_count; i++){
                free(r->portals[i].name);
            }

        free(r->portals);
    }

    r->portals = portals;
    r->portal_count = portal_count;

    return OK;
}

Status room_set_treasures(Room *r, Treasure *treasures, int treasure_count){
    if(!r){
        return INVALID_ARGUMENT;
    }

    if(treasure_count > 0 && treasures == NULL){
        return INVALID_ARGUMENT;
    }

    if(r->treasures != NULL){
        for(int i = 0; i < r->treasure_count; i++){
            free(r->treasures[i].name);
        }
        free(r->treasures);
    }

    r->treasures = treasures;
    r->treasure_count = treasure_count;

    return OK;
}

Status room_place_treasure(Room *r, const Treasure *treasure){
    if(!r || !treasure){
        return INVALID_ARGUMENT;
    }

    Treasure *new_arr = realloc(r->treasures, (r->treasure_count + 1) * sizeof(Treasure));
    if(!new_arr){
        return NO_MEMORY;
    }
    r->treasures = new_arr;
    r->treasures[r->treasure_count] = *treasure;
    r->treasure_count++;

    return OK;
}

int room_get_treasure_at(const Room *r, int x, int y){
    if(!r){
        return -1;
    }

    for(int i = 0; i < r->treasure_count; i++){
        if(r->treasures[i].collected == true){ continue; }
        if(r->treasures[i].x == x && r->treasures[i].y == y){
            return r->treasures[i].id;
        }
    }

    return -1;
}

int room_get_portal_destination(const Room *r, int x, int y){
    if(!r){
        return -1;
    }

    for(int i = 0; i < r->portal_count; i++){
        if(r->portals[i].x == x && r->portals[i].y == y){
            return r->portals[i].target_room_id;
        }
    }

    return -1;
}

bool room_is_walkable(const Room *r, int x, int y){
    if(!r){
        return false;
    }

    if(x < 0 || y < 0 || x >= r->width || y >= r->height){
        return false;
    }

    bool is_floor = r->floor_grid[y * r->width + x];

    for(int i = 0; i < r->pushable_count; i++){
        if(r->pushables[i].x == x && r->pushables[i].y == y){
            return false;
        }
    }

    if(is_floor == false){
        return false;
    }

    return true;
}

static RoomTileType check_treasure_tile(const Room *r, int x, int y, int *out_id){
    for(int i = 0; i < r->treasure_count; i++){
        if(r->treasures[i].collected == true){
            continue;
        }
        if(r->treasures[i].x == x && r->treasures[i].y == y){
            if(out_id){
                *out_id = r->treasures[i].id;
            }
            return ROOM_TILE_TREASURE;
        }
    }
    return ROOM_TILE_INVALID;
}

RoomTileType room_classify_tile(const Room *r, int x, int y, int *out_id){

    if(!r){
        return ROOM_TILE_INVALID;
    }

    if(x < 0 || y < 0 || x >= r->width || y >= r->height){
        return ROOM_TILE_INVALID;
    }

    if(out_id){
        *out_id = -1;
    }

    RoomTileType t = check_treasure_tile(r, x, y, out_id);
    if (t != ROOM_TILE_INVALID){
        return t;
    } 

    

    for(int i = 0; i < r->portal_count; i++){
        if(r->portals[i].x == x && r->portals[i].y == y){
            if(out_id){
                *out_id = r->portals[i].target_room_id;
            }
            return ROOM_TILE_PORTAL;
        }
    }

    for(int i = 0; i < r->pushable_count; i++){
        if(r->pushables[i].x == x && r->pushables[i].y == y){
            if(out_id){
                *out_id = i;
            }
            return ROOM_TILE_PUSHABLE;
        }
    }


    bool is_wall = r->floor_grid[y * r->width + x];

    if(is_wall == false){
        return ROOM_TILE_WALL;
    }

    return ROOM_TILE_FLOOR;

}

Status room_render(const Room *r, const Charset *charset, char *buffer, int buffer_width, int buffer_height){
    if(!r || !charset || !buffer){
        return INVALID_ARGUMENT;
    }

    int buffer_dimension = buffer_width * buffer_height;
    int room_dimension = r->width * r->height;

    if(buffer_dimension != room_dimension){
        return INVALID_ARGUMENT;
    }
    
    for(int i = 0; i < buffer_dimension; i++){
        if(r->floor_grid[i] == true){
            buffer[i] = charset->floor;
        }
        if(r->floor_grid[i] == false){
            buffer[i] = charset->wall;
        }
    }

    for(int i = 0; i < r->treasure_count; i++){
        if(r->treasures[i].collected == true){
            continue;
        }
        int x = r->treasures[i].x;
        int y = r->treasures[i].y;

        buffer[y * buffer_width + x] = charset->treasure;
    }

    for(int i = 0; i < r->portal_count; i++){
        int x = r->portals[i].x;
        int y = r->portals[i].y;

        buffer[y * buffer_width + x] = charset->portal;
    }

    for(int i = 0; i < r->pushable_count; i++){
        int x = r->pushables[i].x;
        int y = r->pushables[i].y;

        buffer[y * buffer_width + x] = charset->pushable;
    }

    return OK;
}

Status room_get_start_position(const Room *r, int *x_out, int *y_out){
    if(!r || !x_out || !y_out){
        return INVALID_ARGUMENT;
    }

    if(r->portal_count > 0){
        *x_out = r->portals[0].x;
        *y_out = r->portals[0].y;

        return OK;
    }

    for(int x = 0; x < r->width ; x++){
        for(int y = 0; y < r->height; y++){
            if(r->floor_grid[y * r->width + x] == true){
                *x_out = x;
                *y_out = y;

                return OK;
            }
        }
    }


    return ROOM_NOT_FOUND;
}

void room_destroy(Room *r){
    if(!r){
        return;
    }

    free(r->name);

    free(r->floor_grid);

    for(int i = 0; i < r->portal_count; i++){
        free(r->portals[i].name);
    }

    free(r->portals);

    for(int i = 0; i < r->treasure_count; i++){
        free(r->treasures[i].name);
    }

    free(r->treasures);
    free(r->pushables);

    free(r);

}

int room_get_id(const Room *r){
    if(r == NULL){
        return -1;
    }

    return r->id;
}

Status room_pick_up_treasure(Room *r, int treasure_id, Treasure **treasure_out){
    if(r == NULL){
        return INVALID_ARGUMENT;
    }
    if(treasure_out == NULL){
        return INVALID_ARGUMENT;
    }
    if(treasure_id < 0){
        return INVALID_ARGUMENT;
    }

    for(int i = 0; i < r->treasure_count; i++){
        if(r->treasures[i].id == treasure_id){
            if(r->treasures[i].collected == true){
                return INVALID_ARGUMENT;
            }
            *treasure_out = &r->treasures[i];
            r->treasures[i].collected = true;

            return OK;
        }
    }

    return ROOM_NOT_FOUND;

}

void destroy_treasure(Treasure *t){
    if(t == NULL){
        return;
    }
    free(t->name);
    free(t);
}

bool room_has_pushable_at(const Room *r, int x, int y, int *pushable_idx_out){
    if(r == NULL){
        return false;
    }

    for(int i = 0; i < r->pushable_count; i++){
        if(r->pushables[i].x == x && r->pushables[i].y == y){
            if(pushable_idx_out != NULL){
                *pushable_idx_out = i;
            }
            return true;
        }
    }

    return false;

}

Status room_try_push(Room *r, int pushable_idx, Direction dir){
    if(r == NULL){
        return INVALID_ARGUMENT;
    }
    if(pushable_idx < 0 || pushable_idx >= r->pushable_count){
        return INVALID_ARGUMENT;
    }
    if(dir != DIR_NORTH && dir != DIR_SOUTH && dir != DIR_EAST && dir != DIR_WEST){
        return INVALID_ARGUMENT;
    }

    Pushable temp_pushable = r->pushables[pushable_idx];
    int x = temp_pushable.x;
    int y = temp_pushable.y;

    if(dir == DIR_NORTH){
        y = y - 1;
    }
    else if(dir == DIR_SOUTH){
        y = y + 1;
    }
    else if(dir == DIR_EAST){
        x = x + 1; 
    }
    else if(dir == DIR_WEST){
        x = x - 1;
    }

    for(int i = 0; i < r->pushable_count; i++){
        if(i == pushable_idx){
            continue;
        }
        if(r->pushables[i].x == x && r->pushables[i].y == y){
            return ROOM_IMPASSABLE;
        }
    }

    if(room_is_walkable(r, x, y) == false){
            return ROOM_IMPASSABLE;
    }

    r->pushables[pushable_idx].x = x;
    r->pushables[pushable_idx].y = y;
    return OK;
}