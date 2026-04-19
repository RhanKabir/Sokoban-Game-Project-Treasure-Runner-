#include "world_loader.h"
#include <stdlib.h>
#include <string.h>
#include "datagen.h"
#include "graph.h"
#include "room.h"

static int compare_rooms(const void *a, const void *b)
{
    const Room *ra = (const Room *)a;
    const Room *rb = (const Room *)b;

    if (ra->id < rb->id) return -1;
    if (ra->id > rb->id) return 1;
    return 0;
}

static void destroy_room(void *p)
{
    room_destroy((Room *)p);
}

static char *copy_name(const char *n) {
    if (n == NULL) return NULL;
    char *copy = malloc(strlen(n) + 1);
    if (copy) strcpy(copy, n);
    return copy;
}

static Status build_room(const DG_Room *dgr, Room **room_out) {

        Room *r = room_create(dgr->id, NULL, dgr->width, dgr->height);
        if(!r){
            return NO_MEMORY;
        }

        if(dgr->floor_grid != NULL){
            int dimension = dgr->width * dgr->height;
            bool *grid = malloc(dimension * sizeof(bool));
            if(!grid){
                return NO_MEMORY;
            }

            for(int i = 0; i < dimension; i++){
                grid[i] = dgr->floor_grid[i];
            }

            room_set_floor_grid(r, grid);
        }

        if(dgr->portal_count > 0){
            Portal *p = malloc(dgr->portal_count * sizeof(Portal));
            if(!p){
                return NO_MEMORY;
            }
            for(int i = 0; i < dgr->portal_count; i++){
                    p[i].id = dgr->portals[i].id;
                    p[i].x = dgr->portals[i].x;
                    p[i].y = dgr->portals[i].y;
                    p[i].target_room_id = dgr->portals[i].neighbor_id;
                    p[i].name = NULL;
            }

            room_set_portals(r, p, dgr->portal_count);
        }

        if(dgr->treasure_count > 0){
            Treasure *t = malloc(dgr->treasure_count * sizeof(Treasure));
            if(!t){
                return NO_MEMORY;
            }
            for(int i = 0; i < dgr->treasure_count; i++){
                    t[i].id = dgr->treasures[i].global_id;
                    t[i].starting_room_id = dgr->id;
                    t[i].initial_x = dgr->treasures[i].x;
                    t[i].initial_y = dgr->treasures[i].y;
                    t[i].x = dgr->treasures[i].x;
                    t[i].y = dgr->treasures[i].y;
                    t[i].collected = false;

                    t[i].name = copy_name(dgr->treasures[i].name);
            }

            room_set_treasures(r, t, dgr->treasure_count);
        }

        if(dgr->pushable_count > 0){
            Pushable *pu = malloc(dgr->pushable_count * sizeof(Pushable));
            if(!pu){
                return NO_MEMORY;
            }
            for(int i = 0; i < dgr->pushable_count; i++){
                pu[i].id = dgr->pushables[i].id;
                char *n = dgr->pushables[i].name;
                pu[i].name = copy_name(n);
                if (n != NULL && !pu[i].name){ 
                    free(pu); 
                    return NO_MEMORY; 
                }
                pu[i].initial_x = dgr->pushables[i].x;
                pu[i].initial_y = dgr->pushables[i].y;
                pu[i].x = dgr->pushables[i].x;
                pu[i].y = dgr->pushables[i].y;
            }
            r->pushables = pu;
            r->pushable_count = dgr->pushable_count;

        }

    *room_out = r;
    return OK;

}

static void connect_portals(Graph *g, const void * const *payloads, int count) {
    for (int i = 0; i < count; i++) {
        Room *from = (Room *)payloads[i];
        for (int p = 0; p < from->portal_count; p++) {
            int neighbor_id = from->portals[p].target_room_id;
            if (neighbor_id < 0) continue;
            for (int j = 0; j < count; j++) {
                Room *to = (Room *)payloads[j];
                if (to->id == neighbor_id) { graph_connect(g, from, to); break; }
            }
        }
    }
}

Status loader_load_world(const char *config_file,
                         Graph **graph_out,
                         Room **first_room_out,
                         int  *num_rooms_out,
                         Charset *charset_out){

    if (!config_file || !graph_out || !first_room_out || !num_rooms_out || !charset_out) {
        return INVALID_ARGUMENT;
    }
    
    int dg_status = start_datagen(config_file);

    *graph_out = NULL;
    *first_room_out = NULL;
    *num_rooms_out = 0;

    if(dg_status == DG_ERR_CONFIG){
        return WL_ERR_CONFIG;
    }
    if(dg_status == DG_ERR_OOM){
        return NO_MEMORY;
    }
    if(dg_status == DG_ERR_INTERNAL){
        return WL_ERR_DATAGEN;
    }

    const DG_Charset *dgcs = dg_get_charset();

    charset_out->wall = dgcs->wall;
    charset_out->floor = dgcs->floor;
    charset_out->player = dgcs->player;
    charset_out->treasure = dgcs->treasure;
    charset_out->portal = dgcs->portal;
    charset_out->pushable = dgcs->pushable;

    Graph *g = NULL;
    GraphStatus gs = graph_create(compare_rooms, destroy_room, &g);
    if (gs != GRAPH_STATUS_OK || !g) {
        stop_datagen();
        return NO_MEMORY;
    }

    Room *first_room = NULL;
    int room_count = 0;

    while(has_more_rooms()){

        DG_Room dgr = get_next_room();
        Room *r = NULL;
        Status s = build_room(&dgr, &r);
        if (s != OK) { 
            stop_datagen(); 
            graph_destroy(g); 
            return s; 
        }
        graph_insert(g, r);

        if(room_count == 0){
            first_room = r;
        }

        room_count++;

    }

    const void * const *payloads = NULL;
    int payload_count = 0;

    if (graph_get_all_payloads(g, &payloads, &payload_count) == GRAPH_STATUS_OK) {
        connect_portals(g, payloads, payload_count);
    }

    stop_datagen();
    *graph_out = g;
    *first_room_out = first_room;
    *num_rooms_out = room_count;
    return OK;



}
