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

#include "personalities.c"
#include "seed.c"

#define VA_ARGS(...) , ##__VA_ARGS__  // For variadic macros
#define entity_loop(index_name) for (int index_name = 0; index_name < entities_count; index_name++)
#define reverse_entity_loop(index_name) for (int index_name = entities_count - 1; index_name >= 0; index_name--)
#define mouse_primary_pressed(mouse_state) \
  (mouse_state.button == SDL_BUTTON_LEFT && mouse_state.state == SDL_PRESSED && mouse_state.prev_state == SDL_PRESSED)

#define INVALID_ENTITY (-100000000)
#define MAX_ENTITIES 1024
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define array_count(static_array) (sizeof(static_array) / sizeof((static_array)[0]))
/***
 * Taken from SDL.
 * NOTE: these double-evaluate their arguments, so you should never have side effects in the parameters
 *      (e.g. SDL_min(x++, 10) is bad).
 */
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define clamp(x, a, b) (((x) < (a)) ? (a) : (((x) > (b)) ? (b) : (x)))
#define print(format, ...)            \
  printf(format "\n", ##__VA_ARGS__); \
  fflush(stdout)

typedef struct {
  float x;
  float y;
} Vec2;

typedef struct FRect {
  float x;
  float y;
  float w;
  float h;
} FRect;

// typedef struct Spring {
//   float target;
//   float position;
//   float velocity;
//   float acceleration;
//   float friction;
// } Spring;

typedef struct {
  FRect rect;
  //FPoint target;
  //float target_zoom;
  float zoom;
  // Spring zoom_spring;
  // Spring pan_spring_x;
  // Spring pan_spring_y;
} Camera;

// typedef struct {
//   FPoint position;
//   FPoint target;
//   Spring spring_x;
//   Spring spring_y;
// } Selection;

// typedef struct {
//   int prev_state;
//   int state;
//   int button;
//   FPoint position;
//   FPoint prev_position;
//   int clicks;
// } MouseState;

typedef struct {
  SDL_Window* window;
  SDL_Rect dimensions;
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
  //float speed;
  //float prev_speed;
  //float animated_time;
  //u32 frame_count;
  //float fps;
  //Selection selection;
  //const u8 *keyboard_state;
  //MouseState mouse_state;
} RenderContext;

typedef struct {
  Vec2 current_position;
  Vec2 previous_position;
} PositionComponent;

typedef struct {
  PositionComponent sparse[MAX_ENTITIES];
  u32 packed[MAX_ENTITIES];
} PositionComponents;

typedef struct {
  u32 texture_id;
  Vec2 dimensions;
} TextureComponent;

typedef struct {
  float current_velocity;
  float previous_velocity;
  Vec2 current_direction;
  Vec2 previous_direction;
} SpeedComponent;

typedef struct {
  SpeedComponent sparse[MAX_ENTITIES];
  u32 packed[MAX_ENTITIES];
} SpeedComponents;

typedef struct {
  u32 entity_count;
  char *names[MAX_ENTITIES];
  int healths[MAX_ENTITIES];
  PositionComponent positions[MAX_ENTITIES];
  SpeedComponent speeds[MAX_ENTITIES];
  TextureComponent textures[MAX_ENTITIES];
  //bool selected[MAX_ENTITIES];
  //bool hovered[MAX_ENTITIES];
  //int personalities[MAX_ENTITIES][Personality_Count];
} GameContext;

typedef struct {
  double delta_time;
  double alpha; //This is the interpolation factor between the current and previous states. Used for rendering accurate physics steps
  double simulation_speed;
} PhysicsContext;

static RenderContext render_context;
static GameContext game_context;
static PhysicsContext physics_context;

int random_int_between(int min, int max) {
  return min + (rand() % (max - min));
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

// void Entity__create(char* name) {
//   u32 image_id = random_int_between(0, render_context.image_atlas.count);
//   float width = 100.0f;
//   float scale = width / render_context.image_atlas.images[image_id].w;
//   float height = (float)(render_context.image_atlas.images[image_id].h * scale);

//   game_states.current_state.health[entities_count] = 100;
//   game_states.current_state.speed[entities_count] = (float)random_int_between(40, 55);
//   game_states.current_state.names[entities_count] = name;
//   game_states.current_state.selected[entities_count] = false;
//   game_states.current_state.hovered[entities_count] = false;

//   game_states.current_state.rect[entities_count] = (FRect){
//       .h = height,
//       .w = width,
//       .x = (float)random_int_between(-1000, 1000),
//       .y = (float)random_int_between(-1000, 1000),
//   };

// // game_states.current_state.rect[entities_count] = (FRect){
// //   .h = height,
// //   .w = width,
// //   .x = 0.0f,
// //   .y = 0.0f
// // };

//   game_states.current_state.direction[entities_count] = (FPoint){
//       .x = ((float)(rand() % 200) - 100) / 100,
//       .y = ((float)(rand() % 200) - 100) / 100,
//   };

//   //game_states.current_state.direction[entities_count] = (FPoint) {.x = 0.0f, .y = 0.0f};

//   game_states.current_state.image[entities_count] = image_id;

//   int random_amount_of_personalities = random_int_between(5, 10);
//   for (int i = 0; i < random_amount_of_personalities; i++) {
//     int personality = random_int_between(0, Personality_Count);
//     game_states.current_state.personalities[entities_count][personality] = random_int_between(0, 100);
//   }

//   entities_count += 1;
// }

// SDL_FRect screen_to_world(FRect *screen_rect) {
//   SDL_FRect world_rect = {
//       .w = screen_rect->w / render_context.camera.zoom,
//       .h = screen_rect->h / render_context.camera.zoom,
//       .x = (screen_rect->x - render_context.window_data.dimensions.w / 2) / render_context.camera.zoom + render_context.camera.rect.x,
//       .y = (screen_rect->y - render_context.window_data.dimensions.h / 2) / render_context.camera.zoom + render_context.camera.rect.y,
//   };

//   return world_rect;
// }

void world_to_screen(FRect *world_rect) {
  world_rect->w *= render_context.camera.zoom;
  world_rect->h *= render_context.camera.zoom;
  world_rect->x = (world_rect->x - render_context.camera.rect.x) * render_context.camera.zoom + render_context.window_data.dimensions.w /2;
  world_rect->y = (world_rect->y - render_context.camera.rect.y) * render_context.camera.zoom + render_context.window_data.dimensions.h /2;
}

void draw_texture(u32 texture_id, FRect* rendering_rect) {
  SDL_FRect sdl_rect = {
    .w = rendering_rect->w,
    .h = rendering_rect->h,
    .x = rendering_rect->x,
    .y = rendering_rect->y
  };
  int copy_result = SDL_RenderCopyF(render_context.renderer, render_context.texture_atlas.textures[texture_id], NULL, &sdl_rect);
  if (copy_result != 0) {
    printf("Failed to render copy: %s\n", SDL_GetError());
    return;
  }
}

// void draw_entity_name(int entity_id) {
//   TTF_Font *font = NULL;
//   SDL_Surface *text_surface = NULL;
//   float y = (game_states.current_state.rect[entity_id].y - render_context.camera.rect.y - (45.0f / render_context.camera.zoom)) * render_context.camera.zoom +
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

//   float diff = ((game_states.current_state.rect[entity_id].w * render_context.camera.zoom) - text_surface->w) / 2;
//   float x = (((game_states.current_state.rect[entity_id].x - render_context.camera.rect.x) * render_context.camera.zoom) + diff) + render_context.window_w / 2;

//   SDL_FRect text_rect = {.w = (float)text_surface->w, .h = (float)text_surface->h, .x = x, .y = y};

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
      .w = (float)text_surface->w,
      .h = (float)text_surface->h,
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

// FRect get_selection_rect(MouseState *mouse_state) {
//   return (FRect){
//       .x = min(mouse_state->position.x, render_context.selection.position.x),
//       .y = min(mouse_state->position.y, render_context.selection.position.y),
//       .w = SDL_fabsf(mouse_state->position.x - render_context.selection.position.x),
//       .h = SDL_fabsf(mouse_state->position.y - render_context.selection.position.y),
//   };
// }

// void render_debug_info(MouseState *mouse_state) {
//   int index = 0;
//   draw_debug_text(index++, "fps: %.2f", render_context.fps);
//   draw_debug_text(index++, "mouse state: %d, button: %d, clicks: %d", mouse_state->state, mouse_state->button, mouse_state->clicks);
//   draw_debug_text(index++, "prev mouse state: %d", mouse_state->prev_state);
//   draw_debug_text(index++, "camera zoom: %.1f", render_context.camera.target_zoom);
//   draw_debug_text(index++, "game speed: %.1f", render_context.speed);
//   draw_debug_text(
//       index++, "camera: current x,y: %.2f,%.2f target x,y: %.2f,%.2f", render_context.camera.rect.x, render_context.camera.rect.y,
//       &render_context.camera.target.x, render_context.camera.target.y
//   );
//   FRect selection_rect = get_selection_rect(mouse_state);
//   draw_debug_text(
//       index++, "selection: current x,y: %.2f,%.2f target x,y: %.2f,%.2f", selection_rect.x, selection_rect.y,
//       render_context.selection.target.x, render_context.selection.target.y
//   );
// }

// void draw_selection_box(MouseState *mouse_state) {
//   FRect selection_rect = get_selection_rect(mouse_state);

//   if (selection_rect.w < 3.0f) {
//     return;
//   }

//   SDL_FRect selection_rect_f = {
//       .x = selection_rect.x,
//       .y = selection_rect.y,
//       .w = selection_rect.w,
//       .h = selection_rect.h,
//   };
//   SDL_SetRenderDrawColor(render_context.renderer, 255, 255, 255, 255);
//   int result = SDL_RenderDrawRectF(render_context.renderer, &selection_rect_f);
//   assert(result == 0);
// }

// float Spring__update(Spring *spring, float target) {
//   spring->target = target;
//   spring->velocity += (target - spring->position) * spring->acceleration;
//   spring->velocity *= spring->friction;
//   return spring->position += spring->velocity;
// }

// void draw_border(FRect around, float gap_width, float border_width) {
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

// void update_entity(int entity_id) {
//   game_states.current_state.rect[entity_id].x += game_states.current_state.direction[entity_id].x * (render_context.delta_time * render_context.speed);
//   game_states.current_state.rect[entity_id].y += game_states.current_state.direction[entity_id].y * (render_context.delta_time * render_context.speed);
// }

// void render_entity(int entity_id) {
//   SDL_FRect rendering_rect = world_to_screen(&game_states.current_state.rect[entity_id]);

//   draw_texture(game_states.current_state.image[entity_id], &rendering_rect);

//   if (game_states.current_state.selected[entity_id]) {
//     draw_border(game_states.current_state.rect[entity_id], 5.0f / render_context.camera.zoom, 4.0f / render_context.camera.zoom);
//   }
// }

// Image Image__load(const char *texture_file_path) {
//   SDL_Surface *surface = IMG_Load(texture_file_path);
//   assert(surface);

//   SDL_Texture *texture = SDL_CreateTextureFromSurface(render_context.renderer, surface);
//   assert(texture);

//   Image image = (Image){
//       .h = surface->h,
//       .w = surface->w,
//       .texture = texture,
//   };

//   return image;
// }

// TTF_Font *Font__load(const char *font_file_path, int font_size) {
//   TTF_Font *font = TTF_OpenFont(font_file_path, font_size);
//   assert(font);

//   return font;
// }

void draw_grid() {
  // Draw it blended
  SDL_SetRenderDrawBlendMode(render_context.renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(render_context.renderer, 0, 0, 0, 25);
  float grid_size = 100.0f;
  float window_w = (float)render_context.window_data.dimensions.w;
  float window_h = (float)render_context.window_data.dimensions.h;
  FRect grid_to_screen = {
    .w = grid_size,
    .h = grid_size
  };

  world_to_screen(&grid_to_screen);

  float x_start = grid_to_screen.x - floorf(grid_to_screen.x / grid_to_screen.w) * grid_to_screen.w;
  for (float x = x_start; x < window_w; x += grid_to_screen.w) {
    SDL_RenderDrawLineF(render_context.renderer, x, 0, x, window_h);
  }

  float y_start = grid_to_screen.y - floorf(grid_to_screen.y / grid_to_screen.h) * grid_to_screen.h;
  for (float y = y_start; y < window_h; y += grid_to_screen.h) {
    SDL_RenderDrawLineF(render_context.renderer, 0, y, window_w, y);
  }

  // Reset the blend mode
  SDL_SetRenderDrawBlendMode(render_context.renderer, SDL_BLENDMODE_NONE);
}

// void mouse_control_camera(MouseState *mouse_state) {
//   if (mouse_state->button == SDL_BUTTON_RIGHT && mouse_state->state == SDL_PRESSED) {
//     if (mouse_state->prev_position.x != mouse_state->position.x || mouse_state->prev_position.y != mouse_state->position.y) {
//       float delta_x = mouse_state->position.x - mouse_state->prev_position.x;
//       float delta_y = mouse_state->position.y - mouse_state->prev_position.y;
//       mouse_state->prev_position.x = mouse_state->position.x;
//       mouse_state->prev_position.y = mouse_state->position.y;

//       render_context.camera.target.x -= delta_x / render_context.camera.zoom;
//       render_context.camera.target.y -= delta_y / render_context.camera.zoom;
//     }
//   }
// }

// // Camera movement and selection rect movement
// void keyboard_control_camera() {
//   float camera_keyboard_movement_speed = 5.0f;
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

// // Set selected on any entity within the selection_rect
// void select_entities_within_selection_rect(MouseState *mouse_state) {
//   entity_loop(entity_i) {
//     SDL_FRect rect = world_to_screen(&game_states.current_state.rect[entity_i]);
//     SDL_FPoint point_top_left = {
//         .x = rect.x,
//         .y = rect.y,
//     };
//     SDL_FPoint point_bottom_right = {
//         .x = rect.x + rect.w,
//         .y = rect.y + rect.h,
//     };

//     // If the selection rect is bigger than 3 pixels, select the entity if it's within the selection rect
//     FRect selection_rect = get_selection_rect(mouse_state);
//     SDL_FRect sdl_frect = {
//         .x = selection_rect.x,
//         .y = selection_rect.y,
//         .w = selection_rect.w,
//         .h = selection_rect.h,
//     };
//     if (selection_rect.w > 3.0f) {
//       if (SDL_PointInFRect(&point_top_left, &sdl_frect) && SDL_PointInFRect(&point_bottom_right, &sdl_frect)) {
//         game_states.current_state.selected[entity_i] = true;
//       } else {
//         if (!render_context.keyboard_state[SDL_GetScancodeFromKey(SDLK_LSHIFT)]) {
//           game_states.current_state.selected[entity_i] = false;
//         }
//       }
//     }
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

// void aupdate(double a, double b) {
//     // frame_count++;
//     // if (SDL_GetTicks64() - start_ticks >= 1000) {
//     //   render_context.fps = (float)frame_count;
//     //   frame_count = 0;
//     //   start_ticks = SDL_GetTicks64();
//     // }

//     // current_time = SDL_GetTicks64();
//     //render_context.delta_time = (float)(current_time - last_update_time) / 1000;
//     //last_update_time = current_time;
//     //render_context.animated_time = fmodf(render_context.animated_time + render_context.delta_time * 0.5f, 1);
//     render_context.camera.zoom = Spring__update(&render_context.camera.zoom_spring, render_context.camera.target_zoom);
//     SDL_GetWindowSizeInPixels(render_context.window_data.window, &render_context.window_data.dimensions.w, &render_context.window_data.dimensions.h);
//     render_context.keyboard_state = SDL_GetKeyboardState(NULL);

//     SDL_Event event;
//     while (SDL_PollEvent(&event)) {
//       if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
//         render_context.mouse_state.prev_state = render_context.mouse_state.state;
//         render_context.mouse_state.state = event.button.state;
//         render_context.mouse_state.button = event.button.button;
//         render_context.mouse_state.clicks = event.button.clicks;
//         if (render_context.mouse_state.prev_state != SDL_PRESSED) {
//           // Set selection target to the current mouse position
//           render_context.selection.target.x = render_context.mouse_state.position.x;
//           render_context.selection.target.y = render_context.mouse_state.position.y;
//           // Reset selection spring so it doesn't spring between the old selection and the new one
//           render_context.selection.spring_x.position = render_context.selection.target.x;
//           render_context.selection.spring_y.position = render_context.selection.target.y;
//         }
//       }
//       if (event.type == SDL_MOUSEMOTION) {
//         render_context.mouse_state.prev_state = render_context.mouse_state.state;
//         render_context.mouse_state.prev_position.x = render_context.mouse_state.position.x;
//         render_context.mouse_state.prev_position.y = render_context.mouse_state.position.y;
//         render_context.mouse_state.position.x = (float)event.motion.x;
//         render_context.mouse_state.position.y = (float)event.motion.y;
//       }
//       if (event.type == SDL_MOUSEWHEEL) {
//         if (event.wheel.y > 0) {
//           // zoom in
//           render_context.camera.target_zoom = min(render_context.camera.target_zoom + 0.1f, 2.0f);
//         } else if (event.wheel.y < 0) {
//           // zoom out
//           render_context.camera.target_zoom = max(render_context.camera.target_zoom - 0.1f, 0.1f);
//         }
//       }
//       if (event.type == SDL_KEYDOWN) {
//         switch (event.key.keysym.sym) {
//           case SDLK_ESCAPE:
//             bool was_something_selected = false;

//             reverse_entity_loop(entity_i) {
//               if (game_states.current_state.selected[entity_i]) {
//                 was_something_selected = true;
//                 game_states.current_state.selected[entity_i] = false;
//               }
//             }
//             if (!was_something_selected) {
//               //game_is_still_running = 0;
//             }
//             // Maybe the following process:
//             // 1. If anything is selected, then deselect it and break.
//             // 2. If nothing was deselected, then open the pause menu.
//             // 3. If in the pause menu, then close the pause menu.
//             break;

//           case SDLK_UP:
//             render_context.speed += 100.0f;
//             break;

//           case SDLK_DOWN:
//             render_context.speed = max(render_context.speed - 100.0f, 0);
//             break;

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
//       } else if (event.type == SDL_QUIT) {
//         //game_is_still_running = 0;
//       }

//       // Two loops needed so we can have a case where multiple entities can be hovered over, but only one can be selected
//       reverse_entity_loop(entity_i) {
//         game_states.current_state.hovered[entity_i] = entity_under_mouse(entity_i, &render_context.mouse_state);
//       }

//       reverse_entity_loop(entity_i) {
//         if (entity_under_mouse(entity_i, &render_context.mouse_state)) {
//           if (render_context.mouse_state.button == SDL_BUTTON_LEFT && render_context.mouse_state.state == SDL_PRESSED && render_context.mouse_state.prev_state == SDL_RELEASED) {
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
  position->current_position.x += speed->current_direction.x * speed->current_velocity * (float)(physics_context.delta_time * physics_context.simulation_speed);
  position->current_position.y += speed->current_direction.y * speed->current_velocity * (float)(physics_context.delta_time * physics_context.simulation_speed);
}

void system_draw_entities(PositionComponent* position, TextureComponent* texture) {
  FRect rendering_rect = {
    .w = texture->dimensions.x,
    .h = texture->dimensions.y,
    .x = position->current_position.x * (float)physics_context.alpha + position->previous_position.x * (float)(1.0 - physics_context.alpha),
    .y = position->current_position.y * (float)physics_context.alpha + position->previous_position.y * (float)(1.0 - physics_context.alpha)
  };
  world_to_screen(&rendering_rect);
  draw_texture(texture->texture_id, &rendering_rect);
}

void update() {
  SDL_GetWindowSizeInPixels(render_context.window_data.window, &render_context.window_data.dimensions.w, &render_context.window_data.dimensions.h);

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

void render() {
  //render_state = current_state * alpha + previous_state * ( 1.0 - alpha );

  render_clear();
  draw_grid();

  for(u32 entity_id = 0; entity_id < game_context.entity_count; ++entity_id) {
    system_draw_entities(&game_context.positions[entity_id], &game_context.textures[entity_id]);
  }

  draw_debug_text(0, "Simulation: [Speed: %.2f]", physics_context.simulation_speed);
  draw_debug_text(1, "Camera: [Zoom: %.2f]", render_context.camera.zoom);

  SDL_RenderPresent(render_context.renderer);
}

void load_fonts() {
  //Eventually you could consider dynamically loading files from the assets folder. Implementing this is, however,
  //unfortunately platform dependent so it would also require some type of platform abstraction to be viable.
  char font_paths[1][128] = {
    "assets/OpenSans-Regular.ttf"
  };

  for(u32 i = 0; i < array_count(font_paths); i++) {
    static const u32 DEFAULT_FONT_SIZE = 16;
    render_context.font_atlas.fonts[i] = TTF_OpenFont(font_paths[i], DEFAULT_FONT_SIZE);
    assert(render_context.font_atlas.fonts[i]);
    ++render_context.font_atlas.count;
  }
}

void load_images() {
  //Eventually you could consider dynamically loading files from the assets folder. Implementing this is, however,
  //unfortunately platform dependent so it would also require some type of platform abstraction to be viable.
  char texture_paths[4][128] = {
    "assets/stone.bmp",
    "assets/fish.bmp",
    "assets/lamb.bmp",
    "assets/lamb2.bmp",
  };

  for(u32 i = 0; i < array_count(texture_paths); i++) {
    SDL_Surface* surface = IMG_Load(texture_paths[i]);
    render_context.texture_atlas.dimensions[i].x = (float)surface->w;
    render_context.texture_atlas.dimensions[i].y = (float)surface->h;
    render_context.texture_atlas.textures[i] = SDL_CreateTextureFromSurface(render_context.renderer, surface);
    assert(render_context.texture_atlas.textures[i]);
    ++render_context.texture_atlas.count;
    SDL_FreeSurface(surface);
  }
}

void create_entities() {
  char entity_names[43][32] = {
    "pushqrdx", "Athano", "AshenHobs", "adrian_learns", "RVerite", "Orshy", "ruggs888", "Xent12", "nuke_bird", "Kasper_573", "SturdyPose",
    "coffee_lava", "goudacheeseburgers", "ikiwixz", "NixAurvandil", "smilingbig", "tk_dev", "realSuperku", "Hoby2000", "CuteMath", "forodor",
    "Azenris", "collector_of_stuff", "EvanMMO", "thechaosbean", "Lutf1sk", "BauBas9883", "physbuzz", "rizoma0x00", "Tkap1", "GavinsAwfulStream",
    "Resist_0", "b1k4sh", "nhancodes", "qcircuit1", "fruloo", "programmer_jeff", "BluePinStudio", "Pierito95RsNg", "jumpylionnn", "Aruseus",
    "lastmiles", "soulfoam"
  };

  for(u32 name_index = 0; name_index < array_count(entity_names); name_index++) {
    game_context.names[game_context.entity_count] = entity_names[name_index];
    game_context.healths[game_context.entity_count] = 100;
    game_context.positions[game_context.entity_count] = (PositionComponent){
      .current_position = {
        .x = (float)random_int_between(-1000, 1000),
        .y = (float)random_int_between(-1000, 1000)
      }
    };
    game_context.speeds[game_context.entity_count] = (SpeedComponent){
      .current_direction = {
        .x = ((float)(rand() % 200) - 100) / 100,
        .y = ((float)(rand() % 200) - 100) / 100
      },
      .current_velocity = (float)random_int_between(40, 55)
    };
    u32 texture_id = random_int_between(0, render_context.texture_atlas.count);
    float texture_width = 100.0f;
    game_context.textures[game_context.entity_count] = (TextureComponent){
      .texture_id = texture_id,
      .dimensions = {
        .x = texture_width
      }
    };
    float scale = texture_width / render_context.texture_atlas.dimensions[texture_id].x;
    game_context.textures[game_context.entity_count].dimensions.y = (float)(render_context.texture_atlas.dimensions[texture_id].y * scale);
    ++game_context.entity_count;
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
      .zoom = 0.4f
    },
    .background_color = {35, 127, 178, 255}
  };

  SDL_GetWindowSizeInPixels(render_context.window_data.window, &render_context.window_data.dimensions.w, &render_context.window_data.dimensions.h);
  load_fonts();
  load_images();

  game_context = (GameContext) {
    .entity_count = 0
  };
 
  create_entities();

  physics_context = (PhysicsContext) {
    .delta_time = 0.01,
    .simulation_speed = 1.0
  };

  int game_is_still_running = 1;
  double accumulator = 0.0;
  double currentTime = SDL_GetTicks64() / 1000.0;

  while (game_is_still_running) {
      double newTime = SDL_GetTicks64() / 1000.0;
      double frameTime = newTime - currentTime;

      //Here we keep the frame_time within a reasonable bound. If a frame_time exceeds 250ms, we "give up" and drop simulation frames
      //This is necessary as if our frame_time were to become too large, we would effectively lock ourselves in an update cycle 
      //and the simulation would fall completely out of sync with the physics being rendered
      double max_frame_time_threshold = 0.25;
      if (frameTime > max_frame_time_threshold) {
          frameTime = max_frame_time_threshold;
      }
    
      currentTime = newTime;
      accumulator += frameTime;
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
        case SDL_MOUSEWHEEL: {
          float zoom = render_context.camera.zoom;
          if(event.wheel.y > 0) {
            zoom += 0.1f;
            zoom = min(zoom, 2.0f);
          } else if(event.wheel.y < 0) {
            zoom -= 0.1f;
            zoom = max(zoom, 0.1f);
          }
          render_context.camera.zoom = zoom;
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

  SDL_DestroyRenderer(render_context.renderer);
  SDL_DestroyWindow(render_context.window_data.window);
  SDL_Quit();

  return 0;
}