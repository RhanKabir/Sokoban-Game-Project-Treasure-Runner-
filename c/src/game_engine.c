#include "game_engine.h"
#include "world_loader.h"
#include "room.h"
#include "player.h"
#include "graph.h"

#include <stdlib.h>

Status game_engine_create(const char *config_file_path, GameEngine **engine_out){
    if(!config_file_path || !engine_out){
        return INVALID_ARGUMENT;
    }

    *engine_out = NULL;
    GameEngine *eng = malloc(sizeof(GameEngine));
    if (!eng){
        return NO_MEMORY;
    } 

    Graph *g = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset cs;

    Status s = loader_load_world(config_file_path, &g, &first_room, &num_rooms, &cs);
    if(s != OK){
        free(eng);
        return s;
    }

    eng->graph = g;
    eng->charset = cs;
    eng->room_count = num_rooms;

    int x = 0;
    int y = 0;

    s = room_get_start_position(first_room, &x, &y);
    if(s != OK){
        free(eng);
        return s;
    }

    eng->initial_player_x = x;
    eng->initial_player_y = y;
    eng->initial_room_id = first_room->id;

    s = player_create(eng->initial_room_id, x, y, &eng->player);
    if(s != OK){
        free(eng);
        return s;
    }

    *engine_out = eng;

    return OK;

}

void game_engine_destroy(GameEngine *eng){
    if(!eng){
        return;
    }

    player_destroy(eng->player);
    graph_destroy(eng->graph);

    free(eng);
}

const Player *game_engine_get_player(const GameEngine *eng){
    if(!eng){
        return NULL;
    }

    Player *p = eng->player;

    return p;
}

static void calc_target(Direction dir, int px, int py, int *tx, int *ty) {

    if(dir == DIR_NORTH)      { *ty = py - 1; }
    else if(dir == DIR_SOUTH) { *ty = py + 1; }
    else if(dir == DIR_EAST)  { *tx = px + 1; }
    else if(dir == DIR_WEST)  { *tx = px - 1; }
}

Status game_engine_move_player(GameEngine *eng, Direction dir){
    if(!eng){
        return INVALID_ARGUMENT;
    }
    if(dir != DIR_NORTH && dir != DIR_SOUTH && dir != DIR_EAST && dir != DIR_WEST){
        return INVALID_ARGUMENT;
    }

    int room_id = player_get_room(eng->player);

    Room key;
    key.id = room_id;

    Room *curr_room = (Room *)graph_get_payload(eng->graph, &key);
    if (!curr_room){
        return GE_NO_SUCH_ROOM;
    }

    int px = 0;
    int py = 0;

    Status s = player_get_position(eng->player, &px, &py);
    if(s != OK){
        return s;
    }

    int tx = px;
    int ty = py;

    calc_target(dir, px, py, &tx, &ty);

    int treasure_id = room_get_treasure_at(curr_room, tx, ty);
    if(treasure_id >= 0){
        Treasure *treasure = NULL;
        Status ts = room_pick_up_treasure(curr_room, treasure_id, &treasure);
        if(ts == OK && treasure != NULL){
            treasure->collected = false;
            player_try_collect(eng->player, treasure);
        }
        return OK;
    }

    int pushable_idx = -1;
    if(room_has_pushable_at(curr_room, tx, ty, &pushable_idx)){
        s = room_try_push(curr_room, pushable_idx, dir);
        if(s != OK){
            return ROOM_IMPASSABLE;
        }
    } else {
        if(!room_is_walkable(curr_room, tx, ty)){
            return ROOM_IMPASSABLE;
        }
    }

    eng->player->x = tx;
    eng->player->y = ty;

    int dest_room_id = room_get_portal_destination(curr_room, tx, ty);
    if(dest_room_id >= 0){

        Room key2;
        key2.id = dest_room_id;

        Room *dest_room = (Room *)graph_get_payload(eng->graph, &key2);
        if(!dest_room){
            return GE_NO_SUCH_ROOM;
        }

        int sx = 0;
        int sy = 0;

        s = room_get_start_position(dest_room, &sx, &sy);
        if(s != OK){
            return s;
        }

        s = player_move_to_room(eng->player, dest_room_id);
        if(s != OK){
            return s;
        }

        s = player_set_position(eng->player, sx, sy);
        if(s != OK){
            return s;
        }
    }

    return OK;
}

Status game_engine_get_room_count(const GameEngine *eng, int *count_out){
    if(!eng){
        return INVALID_ARGUMENT;
    }
    if(!count_out){
        return NULL_POINTER;
    }

    *count_out = eng->room_count;

    return OK;
}

Status game_engine_get_room_dimensions(const GameEngine *eng, int *width_out, int *height_out){
    if(!eng){
        return INVALID_ARGUMENT;
    }
    if(!width_out || !height_out){
        return NULL_POINTER;
    }
    if (!eng->player || !eng->graph) {
        return INTERNAL_ERROR;
    }

    int room_id = player_get_room(eng->player);

    Room key;
    key.id = room_id;

    Room *curr_room = (Room *)graph_get_payload(eng->graph, &key);
    if(!curr_room){
        return GE_NO_SUCH_ROOM;
    }

    *width_out = curr_room->width;
    *height_out = curr_room->height;

    return OK;
}

Status game_engine_reset(GameEngine *eng){
    if(!eng){
        return INVALID_ARGUMENT;
    }

    if (!eng->player) {
        return INTERNAL_ERROR;
    }

    eng->player->room_id = eng->initial_room_id;
    eng->player->x = eng->initial_player_x;
    eng->player->y = eng->initial_player_y;

    eng->player->collected_count = 0;
    free(eng->player->collected_treasures);
    eng->player->collected_treasures = NULL;

    int room_count = 0;
    const void * const *payloads = NULL;
    
    graph_get_all_payloads(eng->graph, &payloads, &room_count);

    for(int i = 0; i < eng->room_count; i++){
        Room *r = (Room *)payloads[i];
        for(int j = 0; j < r->treasure_count; j++){
            r->treasures[j].x = r->treasures[j].initial_x;
            r->treasures[j].y = r->treasures[j].initial_y;
            r->treasures[j].collected = false;
        }
        for(int k = 0; k < r->pushable_count; k++){
            r->pushables[k].x = r->pushables[k].initial_x;
            r->pushables[k].y = r->pushables[k].initial_y;
        }
    }

    return OK;
}

Status game_engine_render_current_room(const GameEngine *eng, char **str_out){
    if (!eng){ 
        return INVALID_ARGUMENT;
    }
    if (!str_out){
        return NULL_POINTER;
    }

    int room_id = player_get_room(eng->player);

    Room key;
    key.id = room_id;

    Room *curr_room = (Room *)graph_get_payload(eng->graph, &key);

    char *buffer = malloc((size_t)curr_room->width * (size_t)curr_room->height * sizeof(char));

    Status s = room_render(curr_room, &eng->charset, buffer, curr_room->width, curr_room->height);
    if(s != OK){
        free(buffer);
        return s;
    }

    int px = 0;
    int py = 0;
    s = player_get_position(eng->player, &px, &py);
    if(s != OK){
        free(buffer);
        return s;
    }

    buffer[py * curr_room->width + px] = eng->charset.player;

    char *out = malloc(curr_room->height * (curr_room->width + 1) + 1);


    int k = 0;
    for(int y = 0; y < curr_room->height; y++){
        for(int x = 0; x < curr_room->width; x++){
            out[k++] = buffer[y * curr_room->width + x];
        }
        out[k++] = '\n';
    }

    out[k] = '\0';

    free(buffer);

    *str_out = out;
    return OK;
}

Status game_engine_render_room(const GameEngine *eng, int room_id, char **str_out){
    if (!eng){ 
        return INVALID_ARGUMENT;
    }
    if (!str_out){
        return NULL_POINTER;
    }
    if(room_id < 0){
        return GE_NO_SUCH_ROOM;
    }

    Room key;
    key.id = room_id;

    Room *curr_room = (Room *)graph_get_payload(eng->graph, &key);
    if (!curr_room) {
        return GE_NO_SUCH_ROOM;
    }

    char *buffer = malloc((size_t)curr_room->width * (size_t)curr_room->height * sizeof(char));

    Status s = room_render(curr_room, &eng->charset, buffer, curr_room->width, curr_room->height);
    if(s != OK){
        free(buffer);
        return s;
    }

    char *out = malloc(curr_room->height * (curr_room->width + 1) + 1);

    int k = 0;
    for(int y = 0; y < curr_room->height; y++){
        for(int x = 0; x < curr_room->width; x++){
            out[k++] = buffer[y * curr_room->width + x];
        }
        out[k++] = '\n';
    }

    out[k] = '\0';

    free(buffer);

    *str_out = out;
    return OK;


}

Status game_engine_get_room_ids(const GameEngine *eng, int **ids_out, int *count_out){
    if(!eng){
        return INVALID_ARGUMENT;
    }
    if(!ids_out || !count_out){
        return NULL_POINTER;
    }

    const void * const *payloads = NULL;
    int count = 0;

    GraphStatus s = graph_get_all_payloads(eng->graph, &payloads, &count);
    if(s != GRAPH_STATUS_OK){
        return INTERNAL_ERROR;
    }

    int *all_ids = NULL;
    all_ids = malloc(count * sizeof(int));
    if (!all_ids) {
        return NO_MEMORY;
    }

    for(int i = 0; i < count; i++){
        Room *r = (Room *)payloads[i];
        all_ids[i] = r->id;
    }

    *ids_out = all_ids;
    *count_out = count;

    return OK;
}

Status game_engine_get_all_treasures(const GameEngine *eng, int *treasure_count){
    if(!eng){
        return INVALID_ARGUMENT;
    }
    if(!treasure_count){
        return NULL_POINTER;
    }

    *treasure_count = 0;

    const void * const *payloads = NULL;
    int count = 0;

    GraphStatus s = graph_get_all_payloads(eng->graph, &payloads, &count);
    if(s != GRAPH_STATUS_OK){
        return INTERNAL_ERROR;
    }

    for(int i = 0; i < count; i++){
        Room *r = (Room *)payloads[i];
        *treasure_count += r->treasure_count;
    }

    return OK;
}

Status game_engine_get_adjacency_matrix(GameEngine *eng, int **matrix, int *count){
    if(!eng){
        return INVALID_ARGUMENT;
    }
    if(!matrix){
        return NULL_POINTER;
    }
    if(!count){
        return NULL_POINTER;
    }

    const void * const *payloads = NULL;
    int n = 0;

    GraphStatus s = graph_get_all_payloads(eng->graph, &payloads, &n);
    if(s != GRAPH_STATUS_OK){
        return INTERNAL_ERROR;
    }

    int *mat = malloc((size_t)n * (size_t)n * sizeof(int));
    if (!mat){
        return INTERNAL_ERROR;
    } 

    for (int i = 0; i < n * n; i++) {
        mat[i] = 0;
    }

    for(int i = 0; i < n; i++){
        Room *room_i = (Room *)payloads[i];

        for(int j = 0; j < n; j++){
            Room *room_j = (Room *)payloads[j];
            
            if(graph_has_edge(eng->graph, room_i, room_j)){
                mat[i * n + j] = 1;
            }
        }
    }

    *matrix = mat;
    *count = n;
    return OK;
}

void game_engine_free_string(void *ptr){
    free(ptr);
}

