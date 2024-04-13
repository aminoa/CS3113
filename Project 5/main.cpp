/**
* Author: Aneesh Maganti
* Assignment: Platformer
* Date due: 2024-04-13, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define LEVEL1_WIDTH 14
#define LEVEL1_HEIGHT 8
#define LEVEL1_LEFT_EDGE 5.0f

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL_mixer.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"
#include "Map.h"
#include "Utility.h"
#include "Scene.h"
#include "Effects.h"

// Levels
#include "LevelStart.h"
#include "LevelOne.h"
#include "LevelTwo.h"
#include "LevelThree.h"

const char FONT_FILEPATH[] = "assets/font1.png";


// ––––– CONSTANTS ––––– //
const int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

const float BG_RED     = 0.0f,
            BG_BLUE    = 0.0f,
            BG_GREEN   = 1.0f,
            BG_OPACITY = 0.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;

// ––––– GLOBAL VARIABLES ––––– //
Scene  *g_current_scene;
LevelOne *g_levelOne;
LevelTwo *g_levelTwo;
LevelThree* g_levelThree;
LevelStart* g_levelStart;
int g_lives = 3;

Effects *g_effects;
Scene   *g_levels[4];

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

bool g_is_colliding_bottom = false;

//text
Entity* text_texture;
Entity* g_state_text;

// ––––– GENERAL FUNCTIONS ––––– //
void switch_to_scene(Scene *scene)
{    
    g_current_scene = scene;
    g_current_scene->initialise(); // DON'T FORGET THIS STEP!
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("Project 5",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    
    glUseProgram(g_shader_program.get_program_id());
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    g_levelStart = new LevelStart();
    g_levelOne = new LevelOne();
    g_levelTwo = new LevelTwo();
    g_levelThree = new LevelThree();
    
    g_levels[0] = g_levelStart;
    g_levels[1] = g_levelOne;
    g_levels[2] = g_levelTwo;
    g_levels[3] = g_levelThree;
    
    // Start at Level One
    switch_to_scene(g_levels[0]);
    
    g_effects = new Effects(g_projection_matrix, g_view_matrix);
    // Special effect added
    g_effects->start(FADEIN, 0.25f);

    //text
	g_state_text = new Entity();
	g_state_text->m_texture_id = Utility::load_texture(FONT_FILEPATH);
	g_state_text->set_position(glm::vec3(0.0f, 1.0f, 0.0f));
}

void process_input()
{
    // VERY IMPORTANT: If nothing is pressed, we don't want to go anywhere
    g_current_scene->m_state.player->set_movement(glm::vec3(0.0f));
    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_game_is_running = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        g_game_is_running = false;
                        break;
                        
                    case SDLK_SPACE:
                        // Jump
                        if (g_current_scene->m_state.player->m_collided_bottom)
                        {
                            g_current_scene->m_state.player->m_is_jumping = true;
                            Mix_PlayChannel(-1, g_current_scene->m_state.jump_sfx, 0);
                        }
                        break;

                    default:
                        break;
                }

            default:
                break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT])
    {
        g_current_scene->m_state.player->m_movement.x = -1.0f;
        g_current_scene->m_state.player->m_animation_indices = g_current_scene->m_state.player->m_walking[g_current_scene->m_state.player->LEFT];
    }
    else if (key_state[SDL_SCANCODE_RIGHT])
    {
        g_current_scene->m_state.player->m_movement.x = 1.0f;
        g_current_scene->m_state.player->m_animation_indices = g_current_scene->m_state.player->m_walking[g_current_scene->m_state.player->RIGHT];
    }

    if (glm::length(g_current_scene->m_state.player->m_movement) > 1.0f)
    {
        g_current_scene->m_state.player->m_movement = glm::normalize(g_current_scene->m_state.player->m_movement);
    }
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;

    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }

    if (g_lives == 0 || g_current_scene->level_number == -1) { return; }

    while (delta_time >= FIXED_TIMESTEP) {
        g_current_scene->update(FIXED_TIMESTEP);
        g_effects->update(FIXED_TIMESTEP);

        //if (g_is_colliding_bottom == false && g_current_scene->m_state.player->m_collided_bottom) g_effects->start(SHAKE, 1.0f);
        //check if the player is colliding with an enemy

        for (int i = 0; i < g_current_scene->m_number_of_enemies; i++)
        {
            if (g_current_scene->m_state.player->check_collision(&g_current_scene->m_state.enemies[i]))
            {
                //also need to decrease lives by 1; if lives == 0, then game over
                g_current_scene->m_state.player->set_position(g_current_scene->m_player_start_position);
                g_lives -= 1;
            }
        }

        g_is_colliding_bottom = g_current_scene->m_state.player->m_collided_bottom;
        delta_time -= FIXED_TIMESTEP;
    }

    g_accumulator = delta_time;

    // Prevent the camera from showing anything outside of the "edge" of the level
    g_view_matrix = glm::mat4(1.0f);

    if (g_current_scene->m_state.player->get_position().x > LEVEL1_LEFT_EDGE) {
        g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-g_current_scene->m_state.player->get_position().x, 3.75, 0));
    }
    else {
        g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-5, 3.75, 0));
    }

    if (g_current_scene == g_levelOne && g_current_scene->m_state.player->get_position().y < -10.0f) switch_to_scene(g_levelTwo);
    g_view_matrix = glm::translate(g_view_matrix, g_effects->m_view_offset);
}

void render()
{

    g_shader_program.set_view_matrix(g_view_matrix);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(g_shader_program.get_program_id());
    g_current_scene->render(&g_shader_program);
    g_effects->render();

    if (g_lives == 0)
    {
        Utility::draw_text(&g_shader_program, g_state_text->m_texture_id, "Rip in Peace...", 0.5f, -0.25f, glm::vec3(2.0f, -1.0f, 0.0f));
    }
    else if (g_current_scene->level_number == -1)
    {
		Utility::draw_text(&g_shader_program, g_state_text->m_texture_id, "You Win!", 0.5f, -0.25f, glm::vec3(14.0f, -1.0f, 0.0f));
	}
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{    
    SDL_Quit();
    
    delete g_levelOne;
    delete g_levelTwo;
    delete g_levelThree;
    delete g_levelStart;
    delete g_effects;
}

// ––––– DRIVER GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();
    
    while (g_game_is_running)
    {
        process_input();
        update();
        
        if (g_current_scene->m_state.next_scene_id >= 0) switch_to_scene(g_levels[g_current_scene->m_state.next_scene_id]);
        
        render();
    }
    
    shutdown();
    return 0;
}
