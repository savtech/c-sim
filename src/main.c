#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t i32;

typedef float f32;
typedef double f64;

#include "personalities.c"
#include "seed.c"

#define VA_ARGS(...) , ##__VA_ARGS__  // For variadic macros
#define entity_loop(index_name) for (int index_name = 0; index_name < entities_count; index_name++)
#define reverse_entity_loop(index_name) for (int index_name = entities_count - 1; index_name >= 0; index_name--)
#define mouse_primary_pressed(mouse_state) \
  (mouse_state.button == SDL_BUTTON_LEFT && mouse_state.state == SDL_PRESSED && mouse_state.prev_state == SDL_PRESSED)

#define INVALID_ENTITY (-100000000)
#define MAX_ENTITIES 1024
#define MAX_ENTITY_NAME_LENGTH 32
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define array_count(static_array) (sizeof(static_array) / sizeof((static_array)[0]))
/***
 * Taken from SDL.
 * NOTE: these f64-evaluate their arguments, so you should never have side effects in the parameters
 *      (e.g. SDL_min(x++, 10) is bad).
 */
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define clamp(x, a, b) (((x) < (a)) ? (a) : (((x) > (b)) ? (b) : (x)))
#define print(format, ...)            \
  printf(format "\n", ##__VA_ARGS__); \
  fflush(stdout)

typedef struct {
  i32 x;
  i32 y;
} IVec2;

typedef struct {
  f32 x;
  f32 y;
} Vec2;

typedef struct {
  IVec2 a;
  IVec2 b;
} IRect;

typedef struct {
  Vec2 a;
  Vec2 b;
} FRect;

// typedef struct Spring {
//   f32 target;
//   f32 position;
//   f32 velocity;
//   f32 acceleration;
//   f32 friction;
// } Spring;

typedef struct {
  //FRect viewport;
  Vec2 position;
  //FPoint target;
  //f32 target_zoom;
  f32 zoom;
  // Spring zoom_spring;
  // Spring pan_spring_x;
  // Spring pan_spring_y;
} Camera;

typedef struct {
  IRect box;
  bool active;
  // Spring spring_x;
  // Spring spring_y;
} Selection;

typedef struct {
  IVec2 position;
  u32 current;
  u32 previous;
  i32 wheel;
} MouseState;

typedef struct {
  SDL_Window* window;
  IVec2 dimensions;
} WindowData;

typedef struct {
  u32 count;
  TTF_Font* fonts[8];
} FontAtlas;

typedef struct {
  u32 count;
  SDL_Texture* textures[32];
  Vec2 dimensions[32];
} TextureAtlas;

typedef struct {
  SDL_Renderer* renderer;
  WindowData window_data;
  FontAtlas font_atlas;
  TextureAtlas texture_atlas;
  Camera camera;
  SDL_Color background_color;
  //f32 prev_speed;
  //f32 animated_time;
  //u32 frame_count;
  //f32 fps;
  Selection selection;
  //const u8 *keyboard_state;
  MouseState mouse_state;
} RenderContext;

typedef struct {
  Vec2 current_position;
  Vec2 previous_position;
} PositionComponent;

typedef struct {
  u32 texture_id;
  Vec2 dimensions;
} TextureComponent;

typedef struct {
  f32 current_velocity;
  f32 previous_velocity;
  Vec2 current_direction;
  Vec2 previous_direction;
} SpeedComponent;

typedef struct {
  bool selected;
} SelectableComponent;

typedef struct {
  u32 entity_count;
  char names[MAX_ENTITIES][MAX_ENTITY_NAME_LENGTH];
  i32 healths[MAX_ENTITIES];
  PositionComponent positions[MAX_ENTITIES];
  SpeedComponent speeds[MAX_ENTITIES];
  TextureComponent textures[MAX_ENTITIES];
  SelectableComponent selectables[MAX_ENTITIES];

  //bool selected[MAX_ENTITIES];
  //bool hovered[MAX_ENTITIES];
  //int personalities[MAX_ENTITIES][Personality_Count];
} GameContext;

typedef struct {
  f64 delta_time;
  f64 alpha; //This is the interpolation factor between the current and previous states. Used for rendering accurate physics steps
  f64 simulation_speed;
} PhysicsContext;

static RenderContext render_context;
static GameContext game_context;
static PhysicsContext physics_context;

int random_int_between(int min, int max) {
  return min + (rand() % (max - min));
}

i32 rect_width(IRect* rect) {
  return rect->b.x - rect->a.x;
}

i32 rect_height(IRect* rect) {
  return rect->b.y - rect->a.y;
}

f32 frect_width(FRect* rect) {
  return rect->b.x - rect->a.x;
}

f32 frect_height(FRect* rect) {
  return rect->b.y - rect->a.y;
}

Vec2 vec2_world_to_screen(Vec2 point) {
    Vec2 translated_point;
    translated_point.x = (point.x - render_context.camera.position.x) * render_context.camera.zoom + render_context.window_data.dimensions.x * 0.5f;
    translated_point.y = (point.y - render_context.camera.position.y) * render_context.camera.zoom + render_context.window_data.dimensions.y * 0.5f;
    return translated_point;
}

Vec2 vec2_screen_to_world(Vec2 point) {
    Vec2 translated_point;
    translated_point.x = (point.x - render_context.window_data.dimensions.x * 0.5f) / render_context.camera.zoom + render_context.camera.position.x;
    translated_point.y = (point.y - render_context.window_data.dimensions.y * 0.5f) / render_context.camera.zoom + render_context.camera.position.y;
    return translated_point;
}

FRect frect_world_to_screen(FRect rect) {
    FRect translated_frect;
    translated_frect.a = vec2_world_to_screen(rect.a);
    translated_frect.b = vec2_world_to_screen(rect.b);
    return translated_frect;
}

FRect frect_screen_to_world(FRect rect) {
    FRect translated_frect;
    translated_frect.a = vec2_screen_to_world(rect.a);
    translated_frect.b = vec2_screen_to_world(rect.b);
    return translated_frect;
}

// int Entity__get_personality_count(int entity_index) {
//   int result = 0;
//   for (int i = 0; i < Personality_Count; i++) {
//     if (game_states.current_state.personalities[i] > 0) {
//       result += 1;
//     }
//   }

//   return result;
// }

// bool Entity__has_personality(int entity_index, Personality personality) {
//   return game_states.current_state.personalities[entity_index][personality] > 0;
// }

void draw_texture(u32 texture_id, FRect* texture_rect) {
  SDL_FRect sdl_frect = {
    .x = texture_rect->a.x,
    .y = texture_rect->a.y,
    .w = frect_width(texture_rect),
    .h = frect_height(texture_rect)
  };
  int copy_result = SDL_RenderCopyF(render_context.renderer, render_context.texture_atlas.textures[texture_id], NULL, &sdl_frect);
  if (copy_result != 0) {
    printf("Failed to render copy: %s\n", SDL_GetError());
    return;
  }
}

// void draw_entity_name(int entity_id) {
//   TTF_Font *font = NULL;
//   SDL_Surface *text_surface = NULL;
//   f32 y = (game_states.current_state.rect[entity_id].y - render_context.camera.rect.y - (45.0f / render_context.camera.zoom)) * render_context.camera.zoom +
//             render_context.window_h / 2;

//   if (game_states.current_state.hovered[entity_id]) {
//     SDL_Color Yellow = {255, 255, 0};
//     font = render_context.fonts[0];
//     text_surface = TTF_RenderText_Blended(font, game_states.current_state.names[entity_id], Yellow);
//     y -= 10.0f;  // move the text up a little when using the bigger font
//   } else {
//     SDL_Color Black = {0, 0, 0};
//     font = render_context.fonts[1];
//     text_surface = TTF_RenderText_Blended(font, game_states.current_state.names[entity_id], Black);
//   }

//   assert(font);
//   assert(text_surface);

//   SDL_Texture *text_texture = SDL_CreateTextureFromSurface(render_context.renderer, text_surface);
//   if (!text_texture) {
//     fprintf(stderr, "could not create text texture: %s\n", SDL_GetError());
//   }

//   f32 diff = ((game_states.current_state.rect[entity_id].w * render_context.camera.zoom) - text_surface->w) / 2;
//   f32 x = (((game_states.current_state.rect[entity_id].x - render_context.camera.rect.x) * render_context.camera.zoom) + diff) + render_context.window_w / 2;

//   SDL_FRect text_rect = {.w = (f32)text_surface->w, .h = (f32)text_surface->h, .x = x, .y = y};

//   SDL_RenderCopyF(render_context.renderer, text_texture, NULL, &text_rect);
//   SDL_FreeSurface(text_surface);
//   SDL_DestroyTexture(text_texture);
// }

void draw_debug_text(int index, char *str, ...) {
  char text_buffer[128];
  va_list args;
  va_start(args, str);
  int chars_written = vsnprintf(text_buffer, sizeof(text_buffer), str, args);
  assert(chars_written > 0);
  va_end(args);

  TTF_Font *font = render_context.font_atlas.fonts[0];
  SDL_Color White = {255, 255, 255};
  SDL_Surface *text_surface = TTF_RenderText_Blended(font, text_buffer, White);
  if (!text_surface) {
    fprintf(stderr, "could not create text surface: %s\n", SDL_GetError());
  }

  SDL_FRect text_rect = {
      .w = (f32)text_surface->w,
      .h = (f32)text_surface->h,
      .x = 10.0f,
      .y = (32.0f * index),
  };

  SDL_Texture *text_texture = SDL_CreateTextureFromSurface(render_context.renderer, text_surface);
  if (!text_texture) {
    fprintf(stderr, "could not create text texture: %s\n", SDL_GetError());
  }

  SDL_RenderCopyF(render_context.renderer, text_texture, NULL, &text_rect);
  SDL_FreeSurface(text_surface);
  SDL_DestroyTexture(text_texture);
}

// f32 Spring__update(Spring *spring, f32 target) {
//   spring->target = target;
//   spring->velocity += (target - spring->position) * spring->acceleration;
//   spring->velocity *= spring->friction;
//   return spring->position += spring->velocity;
// }

// void draw_border(FRect around, f32 gap_width, f32 border_width) {
//   FRect borders[4];

//   //         1
//   //   |-----------|
//   //   |           |
//   // 0 |           | 2
//   //   |           |
//   //   |-----------|
//   //         3
//   for (int i = 0; i < 4; i++) {
//     borders[i] = around;

//     if (i % 2 == 0) {  // Left (0) and right (2)
//       borders[i].w = border_width;
//       borders[i].h += (gap_width + border_width) * 2;
//       borders[i].x += (i == 2 ? around.w + gap_width : -(gap_width + border_width));
//       borders[i].y -= gap_width + border_width;
//     } else {  // Top (1) and bottom (3)
//       borders[i].w += (gap_width + border_width) * 2;
//       borders[i].h = border_width;
//       borders[i].x -= gap_width + border_width;
//       borders[i].y += (i == 3 ? (around.h + gap_width) : -(gap_width + border_width));
//     }

//     SDL_SetRenderDrawColor(render_context.renderer, 255, 255, 255, 255);
//     SDL_FRect rect = world_to_screen(&borders[i]);
//     SDL_RenderFillRectF(render_context.renderer, &rect);
//   }
// }

// TTF_Font *Font__load(const char *font_file_path, int font_size) {
//   TTF_Font *font = TTF_OpenFont(font_file_path, font_size);
//   assert(font);

//   return font;
// }

void draw_grid() {
  f32 grid_size = 100.0f;
  //Maybe we should move this viewport into the actual camera struct and just update it during camera_zoom_update(). Not sure where else we may need it yet.
  Vec2 viewport_dimensions = {
    .x = render_context.window_data.dimensions.x / render_context.camera.zoom,
    .y = render_context.window_data.dimensions.y / render_context.camera.zoom
  };
  Vec2 start_position = {
    .x = (f32)(floor((render_context.camera.position.x - viewport_dimensions.x * 0.5f) / grid_size) * grid_size),
    .y = (f32)(floor((render_context.camera.position.y - viewport_dimensions.y * 0.5f) / grid_size) * grid_size)
  };
  Vec2 start_position_screen = vec2_world_to_screen(start_position);

  SDL_SetRenderDrawBlendMode(render_context.renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(render_context.renderer, 0, 0, 0, 25);

  for(f32 y = start_position.y; y < start_position.y + viewport_dimensions.y; y += grid_size) {
    Vec2 end_point_world = {
      .x = start_position.x + viewport_dimensions.x, 
      .y = y
    };
    Vec2 end_point_screen = vec2_world_to_screen(end_point_world);
    SDL_RenderDrawLineF(render_context.renderer, start_position_screen.x, end_point_screen.y, start_position_screen.x + render_context.window_data.dimensions.x, end_point_screen.y);
  }

  for(f32 x = start_position.x; x < start_position.x + viewport_dimensions.x; x += grid_size) {
    Vec2 end_point_world = {
      .x = x, 
      .y = start_position.y + viewport_dimensions.y
    };
    Vec2 end_point_screen = vec2_world_to_screen(end_point_world);
    SDL_RenderDrawLineF(render_context.renderer, end_point_screen.x, start_position_screen.y, end_point_screen.x, start_position_screen.y + render_context.window_data.dimensions.y);
  }

  SDL_SetRenderDrawBlendMode(render_context.renderer, SDL_BLENDMODE_NONE);
}

// void mouse_control_camera(MouseState *mouse_state) {
//   if (mouse_state->button == SDL_BUTTON_RIGHT && mouse_state->state == SDL_PRESSED) {
//     if (mouse_state->prev_position.x != mouse_state->position.x || mouse_state->prev_position.y != mouse_state->position.y) {
//       f32 delta_x = mouse_state->position.x - mouse_state->prev_position.x;
//       f32 delta_y = mouse_state->position.y - mouse_state->prev_position.y;
//       mouse_state->prev_position.x = mouse_state->position.x;
//       mouse_state->prev_position.y = mouse_state->position.y;

//       render_context.camera.target.x -= delta_x / render_context.camera.zoom;
//       render_context.camera.target.y -= delta_y / render_context.camera.zoom;
//     }
//   }
// }

// // Camera movement and selection rect movement
// void keyboard_control_camera() {
//   f32 camera_keyboard_movement_speed = 5.0f;
//   if (render_context.keyboard_state[SDL_GetScancodeFromKey(SDLK_w)]) {
//     render_context.camera.target.y -= camera_keyboard_movement_speed / render_context.camera.zoom;
//     render_context.selection.target.y += camera_keyboard_movement_speed;
//   }
//   if (render_context.keyboard_state[SDL_GetScancodeFromKey(SDLK_s)]) {
//     render_context.camera.target.y += camera_keyboard_movement_speed / render_context.camera.zoom;
//     render_context.selection.target.y -= camera_keyboard_movement_speed;
//   }
//   if (render_context.keyboard_state[SDL_GetScancodeFromKey(SDLK_a)]) {
//     render_context.camera.target.x -= camera_keyboard_movement_speed / render_context.camera.zoom;
//     render_context.selection.target.x += camera_keyboard_movement_speed;
//   }
//   if (render_context.keyboard_state[SDL_GetScancodeFromKey(SDLK_d)]) {
//     render_context.camera.target.x += camera_keyboard_movement_speed / render_context.camera.zoom;
//     render_context.selection.target.x -= camera_keyboard_movement_speed;
//   }
// }

// int get_entity_to_follow() {
//   int result = INVALID_ENTITY;
//   int selected_count = 0;
//   entity_loop(entity_i) {
//     if (game_states.current_state.selected[entity_i]) {
//       selected_count += 1;
//       result = entity_i;
//     }
//   }
//   return selected_count == 1 ? result : INVALID_ENTITY;
// }

// Set the camera to follow an entity, if only one entity is selected
// void camera_follow_entity() {
//   int to_follow = get_entity_to_follow();
//   if (to_follow != INVALID_ENTITY) {
//     render_context.camera.target.x = game_states.current_state.rect[to_follow].x + game_states.current_state.rect[to_follow].w / 2;
//     render_context.camera.target.y = game_states.current_state.rect[to_follow].y + game_states.current_state.rect[to_follow].h / 2;
//   }
// }

// bool entity_under_mouse(int entity_id, MouseState *mouse_state) {
//   SDL_FRect rect = world_to_screen(&game_states.current_state.rect[entity_id]);

//   return SDL_PointInFRect(
//       &(SDL_FPoint){
//           .x = mouse_state->position.x,
//           .y = mouse_state->position.y,
//       },
//       &rect
//   );
// }

void init() {
  srand(create_seed("ATHANO_LOVES_CHAT_OWO"));

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "could not initialize sdl2: %s\n", SDL_GetError());
    exit(1);
  }

  if (TTF_Init() == -1) {
    fprintf(stderr, "could not initialize ttf: %s\n", TTF_GetError());
    exit(1);
  }

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
}

// void log_entity_personalities(int entity_id) {
//   for (int personality_i = 0; personality_i < Personality_Count; personality_i++) {
//     if (Entity__has_personality(entity_id, personality_i)) {
//       print(
//           "Entity %s has personality %s with value %d", game_state.names[entity_id], Personality__Strings[personality_i],
//           game_states.current_state.personalities[entity_id][personality_i]
//       );
//     }
//   }
// }

// void aupdate(f64 a, f64 b) {
//     // frame_count++;
//     // if (SDL_GetTicks64() - start_ticks >= 1000) {
//     //   render_context.fps = (f32)frame_count;
//     //   frame_count = 0;
//     //   start_ticks = SDL_GetTicks64();
//     // }

//     // current_time = SDL_GetTicks64();
//     //render_context.delta_time = (f32)(current_time - last_update_time) / 1000;
//     //last_update_time = current_time;
//     //render_context.animated_time = fmodf(render_context.animated_time + render_context.delta_time * 0.5f, 1);
//     render_context.camera.zoom = Spring__update(&render_context.camera.zoom_spring, render_context.camera.target_zoom);
//     SDL_GetWindowSizeInPixels(render_context.window_data.window, &render_context.window_data.dimensions.w, &render_context.window_data.dimensions.h);
//     render_context.keyboard_state = SDL_GetKeyboardState(NULL);

//     SDL_Event event;
//     while (SDL_PollEvent(&event)) {
//       if (event.type == SDL_KEYDOWN) {
//         switch (event.key.keysym.sym) {
//           case SDLK_SPACE:
//             if (render_context.prev_speed > 0) {
//               render_context.speed = render_context.prev_speed;
//               render_context.prev_speed = 0;
//             } else {
//               render_context.prev_speed = render_context.speed;
//               render_context.speed = 0;
//             }
//           default:
//             break;
//         }
//       }
//       // Two loops needed so we can have a case where multiple entities can be hovered over, but only one can be selected
//       reverse_entity_loop(entity_i) {
//         game_states.current_state.hovered[entity_i] = entity_under_mouse(entity_i, &render_context.mouse_state);
//       }

//       reverse_entity_loop(entity_i) {
//         if (entity_under_mouse(entity_i, &render_context.mouse_state)) {
//           if (
            //  render_context.mouse_state.button == SDL_BUTTON_LEFT && 
            //  render_context.mouse_state.state == SDL_PRESSED && 
            //  render_context.mouse_state.prev_state == SDL_RELEASED) {
//             game_states.current_state.selected[entity_i] = !game_states.current_state.selected[entity_i];
//             log_entity_personalities(entity_i);
//             break;
//           }
//         }
//       }
//     }

//     mouse_control_camera(&render_context.mouse_state);

//     keyboard_control_camera(render_context);

//     if (mouse_primary_pressed(render_context.mouse_state)) {
//       select_entities_within_selection_rect(&render_context.mouse_state);
//     } else {
//       camera_follow_entity(render_context);
//     }

//     // Spring the selection box
//     render_context.selection.position.x = Spring__update(&render_context.selection.spring_x, render_context.selection.target.x);
//     render_context.selection.position.y = Spring__update(&render_context.selection.spring_y, render_context.selection.target.y);

//     // Spring the camera position
//     render_context.camera.rect.x = Spring__update(&render_context.camera.pan_spring_x, render_context.camera.target.x);
//     render_context.camera.rect.y = Spring__update(&render_context.camera.pan_spring_y, render_context.camera.target.y);

//     //clear_screen(render_context);

//     draw_grid(render_context);

//     // entity_loop(entity_i) {
//     //   update_entity(entity_i);
//     //   render_entity(entity_i);
//     // }

//     // if (render_context.camera.zoom > 0.5f) {
//     //   entity_loop(entity_i) {
//     //     draw_entity_name(entity_i);
//     //   }
//     // }

//     if (mouse_primary_pressed(render_context.mouse_state)) {
//       // Draw the selection box
//       draw_selection_box(&render_context.mouse_state);
//     }

//     render_debug_info(&render_context.mouse_state);

//     SDL_RenderPresent(render_context.renderer);
// }

void system_move_entities(PositionComponent* position, SpeedComponent* speed) {
  position->previous_position = position->current_position;
  speed->previous_direction = speed->current_direction;
  speed->previous_velocity = speed->current_velocity;
  position->current_position.x += speed->current_direction.x * speed->current_velocity * (f32)(physics_context.delta_time * physics_context.simulation_speed);
  position->current_position.y += speed->current_direction.y * speed->current_velocity * (f32)(physics_context.delta_time * physics_context.simulation_speed);
}

void system_draw_entities(PositionComponent* position, TextureComponent* texture) {
  FRect texture_rect = {
    .a = {
      .x = position->current_position.x * (f32)physics_context.alpha + position->previous_position.x * (f32)(1.0 - physics_context.alpha),
      .y = position->current_position.y * (f32)physics_context.alpha + position->previous_position.y * (f32)(1.0 - physics_context.alpha)
    }
  };
  texture_rect.b.x = texture_rect.a.x + texture->dimensions.x;
  texture_rect.b.y = texture_rect.a.y + texture->dimensions.y;
  FRect translated_rect = frect_world_to_screen(texture_rect);
  draw_texture(texture->texture_id, &translated_rect);
}

void system_draw_selected_boxes(PositionComponent* position, TextureComponent* texture, SelectableComponent* selectable) {
  if(selectable->selected) {
    f32 border_width = 3.0f * render_context.camera.zoom;
    Vec2 entity_position = {
      .x = position->current_position.x * (f32)physics_context.alpha + position->previous_position.x * (f32)(1.0 - physics_context.alpha),
      .y = position->current_position.y * (f32)physics_context.alpha + position->previous_position.y * (f32)(1.0 - physics_context.alpha)
    };
    FRect entity_rect = {
      .a = {
        .x = entity_position.x,
        .y = entity_position.y
      },
      .b = {
        .x = entity_position.x + texture->dimensions.x,
        .y = entity_position.y + texture->dimensions.y
      }
    };

    FRect translated_entity_rect = frect_world_to_screen(entity_rect);
    SDL_FRect sdl_select_borders[4];

    sdl_select_borders[0].x = translated_entity_rect.a.x;
    sdl_select_borders[0].y = translated_entity_rect.a.y;
    sdl_select_borders[0].w = frect_width(&translated_entity_rect);
    sdl_select_borders[0].h = border_width;

    sdl_select_borders[1].x = translated_entity_rect.b.x - border_width;
    sdl_select_borders[1].y = translated_entity_rect.a.y;
    sdl_select_borders[1].w = border_width;
    sdl_select_borders[1].h = frect_height(&translated_entity_rect);

    sdl_select_borders[2].x = translated_entity_rect.a.x;
    sdl_select_borders[2].y = translated_entity_rect.b.y - border_width;
    sdl_select_borders[2].w = frect_width(&translated_entity_rect);
    sdl_select_borders[2].h = border_width;

    sdl_select_borders[3].x = translated_entity_rect.a.x;
    sdl_select_borders[3].y = translated_entity_rect.a.y;
    sdl_select_borders[3].w = border_width;
    sdl_select_borders[3].h = frect_height(&translated_entity_rect);

    SDL_SetRenderDrawColor(render_context.renderer, 255, 0, 0, 255);
    SDL_RenderFillRectsF(render_context.renderer, sdl_select_borders, 4);
  }
}

void update() {
  SDL_GetWindowSizeInPixels(render_context.window_data.window, &render_context.window_data.dimensions.x, &render_context.window_data.dimensions.y);

  for(u32 entity_id = 0; entity_id < game_context.entity_count; entity_id++) {
    system_move_entities(&game_context.positions[entity_id], &game_context.speeds[entity_id]);
  }
}

void render_clear() {
  SDL_SetRenderDrawColor(
    render_context.renderer, 
    render_context.background_color.r, 
    render_context.background_color.g, 
    render_context.background_color.b, 
    render_context.background_color.a
  );
  SDL_RenderClear(render_context.renderer);
}

FRect normalize_selection_rect(FRect* selection_rect) {
    //The values of selection.end will correspond with the following traits below if they lie within that quadrant.
    //For example, if selection.end.x > selection.start.x && selection.end.y > selection.start.y then the end point lies within quadrant 4
    //We must transmute the rectangle so that, regardless of which quadrant the selection.end resides in, 
    //we have a positive growth in both the x and y axes. Which will give us a valid rectangle, logically, to use with SDL functions.
    //As an aside, I've also chosen to check the quadrants in counter-clockwise fashion as it feels more pragmatic to make selections
    //in the 3rd and 4th quadrants. We may as well check the most common usages first to short circuit asap.


    FRect normalized_rect;

    //Quadrant 4: +X, +Y
    if((selection_rect->b.x > selection_rect->a.x) && (selection_rect->b.y > selection_rect->a.y)) {
      //printf("Quadrant 4\n");
      normalized_rect.a.x = selection_rect->a.x;
      normalized_rect.a.y = selection_rect->a.y;
      normalized_rect.b.x = selection_rect->b.x;
      normalized_rect.b.y = selection_rect->b.y;
    }
    //Quadrant 3: -X, +Y
    else if((selection_rect->b.x < selection_rect->a.x) && (selection_rect->b.y > selection_rect->a.y)) {
      //printf("Quadrant 3\n");
      normalized_rect.a.x = selection_rect->b.x;
      normalized_rect.a.y = selection_rect->a.y;
      normalized_rect.b.x = selection_rect->a.x;
      normalized_rect.b.y = selection_rect->b.y;
    }
    //Quadrant 2: -X, -Y
    else if((selection_rect->b.x < selection_rect->a.x) && (selection_rect->b.y < selection_rect->a.y)) {
      //printf("Quadrant 2\n");
      normalized_rect.a.x = selection_rect->b.x;
      normalized_rect.a.y = selection_rect->b.y;
      normalized_rect.b.x = selection_rect->a.x;
      normalized_rect.b.y = selection_rect->a.y;
    }
    //Quadrant 1: +X, -Y
    else if((selection_rect->b.x > selection_rect->a.x) && (selection_rect->b.y < selection_rect->a.y)) {
      //printf("Quadrant 1\n");
      normalized_rect.a.x = selection_rect->a.x;
      normalized_rect.a.y = selection_rect->b.y;
      normalized_rect.b.x = selection_rect->b.x;
      normalized_rect.b.y = selection_rect->a.y;
    } else {
      normalized_rect = *selection_rect;
    }

    return normalized_rect;
}

void debug_draw_info() {
  SDL_SetRenderDrawBlendMode(render_context.renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(render_context.renderer, 30, 30, 30, 220);
  SDL_RenderFillRectF(render_context.renderer, &(SDL_FRect){
    .w = 300.0f,
    .h = 100.0f
  });
  SDL_SetRenderDrawBlendMode(render_context.renderer, SDL_BLENDMODE_NONE);

  draw_debug_text(0, "Simulation: [Speed: %.2f]", physics_context.simulation_speed);
  draw_debug_text(1, "Camera: [Zoom: %.2f]", render_context.camera.zoom);
  if(render_context.selection.active) {
    draw_debug_text(2, "[Selecting] (%d, %d) -> (%d, %d)", render_context.selection.box.a.x, render_context.selection.box.a.y, render_context.selection.box.b.x + render_context.selection.box.a.x, render_context.selection.box.b.y + render_context.selection.box.a.y);
  } else {
    draw_debug_text(2, "[Not Selecting]");
  }
}

void render() {
  //render_state = current_state * alpha + previous_state * ( 1.0 - alpha );

  render_clear();
  draw_grid();

  for(u32 entity_id = 0; entity_id < game_context.entity_count; ++entity_id) {
    system_draw_entities(&game_context.positions[entity_id], &game_context.textures[entity_id]);
    system_draw_selected_boxes(&game_context.positions[entity_id], &game_context.textures[entity_id], &game_context.selectables[entity_id]);
  }

  if(render_context.selection.active) {
    SDL_Rect selection = {
      .x = render_context.selection.box.a.x,
      .y = render_context.selection.box.a.y,
      .w = render_context.selection.box.b.x - render_context.selection.box.a.x,
      .h = render_context.selection.box.b.y - render_context.selection.box.a.y
    };

    SDL_SetRenderDrawColor(render_context.renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(render_context.renderer, &selection);
  }

  debug_draw_info();

  SDL_RenderPresent(render_context.renderer);
}

void load_fonts() {
  //Eventually you could consider dynamically loading files from the assets folder. Implementing this is, however,
  //unfortunately platform dependent so it would also require some type of platform abstraction to be viable.
  char font_paths[][128] = {
    "assets/OpenSans-Regular.ttf"
  };

  for(u32 i = 0; i < array_count(font_paths); i++) {
    static const u32 DEFAULT_FONT_SIZE = 16;
    render_context.font_atlas.fonts[i] = TTF_OpenFont(font_paths[i], DEFAULT_FONT_SIZE);
    assert(render_context.font_atlas.fonts[i]);
    ++render_context.font_atlas.count;
  }
}

void load_textures() {
  //Eventually you could consider dynamically loading files from the assets folder. Implementing this is, however,
  //unfortunately platform dependent so it would also require some type of platform abstraction to be viable.
  char texture_paths[][128] = {
    "assets/stone.bmp",
    "assets/fish.bmp",
    "assets/lamb.bmp",
    "assets/lamb2.bmp",
    "assets/theclaw.png"
  };

  for(u32 i = 0; i < array_count(texture_paths); i++) {
    SDL_Surface* surface = IMG_Load(texture_paths[i]);
    render_context.texture_atlas.dimensions[i].x = (f32)surface->w;
    render_context.texture_atlas.dimensions[i].y = (f32)surface->h;
    render_context.texture_atlas.textures[i] = SDL_CreateTextureFromSurface(render_context.renderer, surface);
    assert(render_context.texture_atlas.textures[i]);
    ++render_context.texture_atlas.count;
    SDL_FreeSurface(surface);
  }
}

void create_entities() {
  char entity_names[][MAX_ENTITY_NAME_LENGTH] = {
    "pushqrdx", "Athano", "AshenHobs", "adrian_learns", "RVerite", "Orshy", "ruggs888", "Xent12", "nuke_bird", "Kasper_573", "SturdyPose",
    "coffee_lava", "goudacheeseburgers", "ikiwixz", "NixAurvandil", "smilingbig", "tk_dev", "realSuperku", "Hoby2000", "CuteMath", "forodor",
    "Azenris", "collector_of_stuff", "EvanMMO", "thechaosbean", "Lutf1sk", "BauBas9883", "physbuzz", "rizoma0x00", "Tkap1", "GavinsAwfulStream",
    "Resist_0", "b1k4sh", "nhancodes", "qcircuit1", "fruloo", "programmer_jeff", "BluePinStudio", "Pierito95RsNg", "jumpylionnn", "Aruseus",
    "lastmiles", "soulfoam"
  };

  for(u32 name_index = 0; name_index < array_count(entity_names); name_index++) {
    strcpy_s(game_context.names[game_context.entity_count], sizeof(entity_names[name_index]), entity_names[name_index]);
    game_context.healths[game_context.entity_count] = 100;
    game_context.positions[game_context.entity_count] = (PositionComponent){
      .current_position = {
        .x = (f32)random_int_between(-1000, 1000),
        .y = (f32)random_int_between(-1000, 1000)
      }
    };
    game_context.speeds[game_context.entity_count] = (SpeedComponent){
      .current_direction = {
        .x = ((f32)(rand() % 200) - 100) / 100,
        .y = ((f32)(rand() % 200) - 100) / 100
      },
      .current_velocity = (f32)random_int_between(40, 55)
    };
    u32 texture_id = random_int_between(0, render_context.texture_atlas.count);
    f32 texture_width = 100.0f;
    game_context.textures[game_context.entity_count] = (TextureComponent){
      .texture_id = texture_id,
      .dimensions = {
        .x = texture_width
      }
    };
    f32 scale = texture_width / render_context.texture_atlas.dimensions[texture_id].x;
    game_context.textures[game_context.entity_count].dimensions.y = (f32)(render_context.texture_atlas.dimensions[texture_id].y * scale);
    game_context.selectables[game_context.entity_count].selected = false;
    ++game_context.entity_count;
  }
}

void camera_update_position() {
  if(render_context.mouse_state.current & SDL_BUTTON_MMASK) {
    render_context.camera.position = (Vec2){
      .x = (f32)render_context.mouse_state.position.x,
      .y = (f32)render_context.mouse_state.position.y
    };
  }
}

void camera_update_zoom() {
  //printf("Mouse wheel: %d\n", render_context.mouse_state.wheel);
    f32 zoom_scaling_factor = 0.1f;
    f32 zoom = render_context.camera.zoom;
    zoom += zoom_scaling_factor * render_context.mouse_state.wheel;
    if(render_context.mouse_state.wheel > 0) {
      zoom += 0.1f * render_context.mouse_state.wheel;
      zoom = min(zoom, 2.0f);
    } else if(render_context.mouse_state.wheel < 0) {
      zoom += 0.1f * render_context.mouse_state.wheel;
      zoom = max(zoom, 0.1f);
    }
    render_context.camera.zoom = zoom;
    render_context.mouse_state.wheel = 0;
}

void entities_clear_selected() {
  for(u32 entity_id = 0; entity_id < game_context.entity_count; ++entity_id) {
      game_context.selectables[entity_id].selected = false;
  }
}

void entities_select_from_rect(FRect* selection_rect) {
  entities_clear_selected();
  FRect translated_rect = frect_screen_to_world(*selection_rect);

  SDL_FRect sdl_selection_rect = {
    .x = translated_rect.a.x,
    .y = translated_rect.a.y,
    .w = frect_width(&translated_rect),
    .h = frect_height(&translated_rect)
  };

  u32 selected_entity_count = 0;
  for(u32 entity_id = 0; entity_id < game_context.entity_count; ++entity_id) {
    SDL_FRect sdl_entity_rect = {
      .x = game_context.positions[entity_id].current_position.x,
      .y = game_context.positions[entity_id].current_position.y,
      .w = game_context.textures[entity_id].dimensions.x,
      .h = game_context.textures[entity_id].dimensions.y
    };

    if(SDL_HasIntersectionF(&sdl_selection_rect, &sdl_entity_rect) == SDL_TRUE) {
      ++selected_entity_count;
      game_context.selectables[entity_id].selected = true;
    }
  }

  //printf("Selected entity count: %d\n", selected_entity_count);
}

void selection_update() {
  if(render_context.mouse_state.current & SDL_BUTTON_LMASK) {
    if(!(render_context.mouse_state.previous & SDL_BUTTON_LMASK)) {
      //printf("Selection start\n");
      entities_clear_selected();
      render_context.selection.active = true;
      render_context.selection.box.a =  render_context.selection.box.b = render_context.mouse_state.position;
    } else {
      //printf("Dragging selection\n");
      render_context.selection.box.b.x = render_context.mouse_state.position.x;
      render_context.selection.box.b.y = render_context.mouse_state.position.y;

      FRect selection_rect = {
        .a = {
          .x = (f32)render_context.selection.box.a.x,
          .y = (f32)render_context.selection.box.a.y
        },
        .b = {
          .x = (f32)render_context.selection.box.b.x,
          .y = (f32)render_context.selection.box.b.y
        }
      };

      FRect normalized_rect = normalize_selection_rect(&selection_rect);
      entities_select_from_rect(&normalized_rect);
    }
  }
  if((render_context.mouse_state.previous & SDL_BUTTON_LMASK) && !(render_context.mouse_state.current & SDL_BUTTON_LMASK)) {
    //printf("Selection complete\n");
    render_context.selection.active = false;
  }

  if(render_context.mouse_state.current & SDL_BUTTON_RMASK) {
    entities_clear_selected();
  }
}

int main(int argc, char *args[]) {
  init();

  SDL_Window *window = SDL_CreateWindow(
    "Cultivation Sim", 
    SDL_WINDOWPOS_UNDEFINED, 
    SDL_WINDOWPOS_UNDEFINED, 
    SCREEN_WIDTH, 
    SCREEN_HEIGHT, 
    SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE
  );
  if (!window) {
    fprintf(stderr, "could not create window: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    fprintf(stderr, "could not create renderer: %s\n", SDL_GetError());
    return 1;
  }

  render_context = (RenderContext) {
    .window_data = {
      .window = window
    },
    .renderer = renderer,
    .camera = {
      .zoom = 1.0f
    },
    .background_color = {35, 127, 178, 255}
  };

  SDL_GetWindowSizeInPixels(render_context.window_data.window, &render_context.window_data.dimensions.x, &render_context.window_data.dimensions.y);
  load_fonts();
  load_textures();

  game_context = (GameContext) {
    .entity_count = 0
  };
 
  create_entities();

  physics_context = (PhysicsContext) {
    .delta_time = 0.01,
    .simulation_speed = 1.0
  };

  int game_is_still_running = 1;
  f64 accumulator = 0.0;
  f64 current_time = SDL_GetTicks64() / 1000.0;

  while (game_is_still_running) {
      f64 new_time = SDL_GetTicks64() / 1000.0;
      f64 frame_time = new_time - current_time;

      //Here we keep the frame_time within a reasonable bound. If a frame_time exceeds 250ms, we "give up" and drop simulation frames
      //This is necessary as if our frame_time were to become too large, we would effectively lock ourselves in an update cycle 
      //and the simulation would fall completely out of sync with the physics being rendered
      f32  max_frame_time_threshold = 0.25;
      if (frame_time > max_frame_time_threshold) {
          frame_time = max_frame_time_threshold;
      }
    
      current_time = new_time;
      accumulator += frame_time;
      while (accumulator >= physics_context.delta_time) {
          update();
          accumulator -= physics_context.delta_time;
      }

      physics_context.alpha = fmin(accumulator / physics_context.delta_time, 1.0);

    render();

    SDL_Event event;
    while(SDL_PollEvent(&event)) {
      switch(event.type) {
        case SDL_KEYDOWN: {
          switch (event.key.keysym.sym) {
            case SDLK_ESCAPE: {
              game_is_still_running = 0;
            } break;
            case SDLK_UP: {
              physics_context.simulation_speed += 0.5;
              physics_context.simulation_speed = min(physics_context.simulation_speed, 10.0);
            } break;
            case SDLK_DOWN: {
              physics_context.simulation_speed -= 0.5;
              physics_context.simulation_speed = max(physics_context.simulation_speed, 0.0);
            } break;
            default: {}
          }
        } break;
        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
          render_context.mouse_state.previous = render_context.mouse_state.current;
          render_context.mouse_state.current = SDL_GetMouseState(&render_context.mouse_state.position.x, &render_context.mouse_state.position.y);
          camera_update_position();
          selection_update();
        } break;
        case SDL_MOUSEWHEEL: {
          render_context.mouse_state.wheel = event.wheel.y;
          camera_update_zoom();
        } break;
        case SDL_WINDOWEVENT: {
          switch(event.window.event) {
            case SDL_WINDOWEVENT_CLOSE: {
              game_is_still_running = 0;
            } break;
            default: {}
          }
        } break;
        default: {}
      }
    }
  }

  return 0;
}