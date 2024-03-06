/**
* Author: Aneesh Maganti
* Assignment: Lunar Lander
* Date due: 2024-09-03, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define NUMBER_OF_ENEMIES 3
#define FIXED_TIMESTEP 0.0166666f
#define ACC_OF_GRAVITY -1.0f
#define PLATFORM_COUNT 3
#define SPIKE_PLATFORM_COUNT 9
#ifdef _WINDOWS
#define GAME_WIN 1
#define GAME_LOSE 2

#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"
#include <string>
#include "helper.h"

// ————— STRUCTS AND ENUMS —————//
struct GameState
{
    Entity* player;
    Entity* platforms; //win platform
    Entity* spikePlatforms;
    Entity* resultText;
    Entity* fuelText;
};

// ————— CONSTANTS ————— //
const int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

const float BG_RED = 0.0f,
            BG_BLUE = 0.0f,
            BG_GREEN = 0.0f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND  = 1000.0;
const char  SPRITESHEET_FILEPATH[]  = "assets/george_0.png",
            PLATFORM_FILEPATH[]     = "assets/platformPack_tile027.png",
			SPIKE_FILEPATH[]        = "assets/spikePlatform.png",
			FONT_FILEPATH[]         = "assets/font1.png";

const int NUMBER_OF_TEXTURES = 1;  // to be generated, that is
const GLint LEVEL_OF_DETAIL  = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER   = 0;  // this value MUST be zero

// ————— VARIABLES ————— //
GameState g_game_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_time_accumulator = 0.0f;

// ———— GENERAL FUNCTIONS ———— //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}

const int FONTBANK_SIZE = 16;

void draw_text(ShaderProgram* g_program, GLuint font_texture_id, std::string text, float screen_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for each character
    // Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their position
        //    relative to the whole sentence)
        int spritesheet_index = (int)text[i];  // ascii value of character
        float offset = (screen_size + spacing) * i;

        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float)(spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float)(spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
            });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
            });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);

    g_program->set_model_matrix(model_matrix);
    glUseProgram(g_program->get_program_id());

    glVertexAttribPointer(g_program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(g_program->get_position_attribute());
    glVertexAttribPointer(g_program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(g_program->get_tex_coordinate_attribute());

    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(g_program->get_position_attribute());
    glDisableVertexAttribArray(g_program->get_tex_coordinate_attribute());
}


void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Project 3",
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
    
    // ----- TEXT  ----- //
    g_game_state.resultText = new Entity();
    g_game_state.resultText->m_texture_id = load_texture(FONT_FILEPATH);
	g_game_state.resultText->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
    
    g_game_state.fuelText = new Entity();
    g_game_state.fuelText->m_texture_id = load_texture(FONT_FILEPATH);
    g_game_state.fuelText->set_position(glm::vec3(-4.5f, 3.5f, 0.0f));

    // ————— PLAYER ————— //
    // Existing
    g_game_state.player = new Entity();
    g_game_state.player->set_position(glm::vec3(0.0f, 4.0f, 0.0f));
    g_game_state.player->set_movement(glm::vec3(0.0f));
    g_game_state.player->set_acceleration(glm::vec3(0.0f, ACC_OF_GRAVITY * 0.1, 0.0f));
    g_game_state.player->set_speed(1.0f);
    g_game_state.player->m_texture_id = load_texture(SPRITESHEET_FILEPATH);
    g_game_state.player->m_object_type = 0;

    // Walking
    g_game_state.player->m_walking[g_game_state.player->LEFT]   = new int[4] { 1, 5, 9,  13 };
    g_game_state.player->m_walking[g_game_state.player->RIGHT]  = new int[4] { 3, 7, 11, 15 };
    g_game_state.player->m_walking[g_game_state.player->UP]     = new int[4] { 2, 6, 10, 14 };
    g_game_state.player->m_walking[g_game_state.player->DOWN]   = new int[4] { 0, 4, 8,  12 };

    g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->RIGHT];  // start George looking right
    g_game_state.player->m_animation_frames  = 4;
    g_game_state.player->m_animation_index   = 0;
    g_game_state.player->m_animation_time    = 0.0f;
    g_game_state.player->m_animation_cols    = 4;
    g_game_state.player->m_animation_rows    = 4;
    g_game_state.player->set_height(0.9f);
    g_game_state.player->set_width(0.9f);

    // ————— PLATFORM ————— //
    g_game_state.platforms = new Entity[PLATFORM_COUNT];

    for (int i = 0; i < PLATFORM_COUNT; i++)
    {
        g_game_state.platforms[i].m_texture_id = load_texture(PLATFORM_FILEPATH);
        g_game_state.platforms[i].set_position(glm::vec3(i - 1.0f, -4.0f, 0.0f));
        g_game_state.platforms[i].update(0.0f, NULL, 0);
        g_game_state.platforms[i].m_object_type = 1; 
    }

    g_game_state.spikePlatforms = new Entity[SPIKE_PLATFORM_COUNT];
    // Need to set the spike platforms
    for (int i = 0; i < 3; i++) // Three sets of spike platforms
    {
        g_game_state.spikePlatforms[i].m_texture_id = load_texture(SPIKE_FILEPATH);
        g_game_state.spikePlatforms[i].set_position(glm::vec3(i - 1.0f, 1.0f, 0.0f));
        g_game_state.spikePlatforms[i].update(0.0f, NULL, 0);    
        g_game_state.spikePlatforms[i].m_object_type = 2;
    }

    for (int i = 3; i < 6; i++) // Three sets of spike platforms
    {
        g_game_state.spikePlatforms[i].m_texture_id = load_texture(SPIKE_FILEPATH);
        g_game_state.spikePlatforms[i].set_position(glm::vec3(i - 1.0f, -2.0f, 0.0f));
        g_game_state.spikePlatforms[i].update(0.0f, NULL, 0);    
        g_game_state.spikePlatforms[i].m_object_type = 2;
    }

    for (int i = 6; i < 9; i++) // Three sets of spike platforms
    {
        g_game_state.spikePlatforms[i].m_texture_id = load_texture(SPIKE_FILEPATH);
        g_game_state.spikePlatforms[i].set_position(glm::vec3(i - 10.0f, -2.0f, 0.0f));
        g_game_state.spikePlatforms[i].update(0.0f, NULL, 0);    
        g_game_state.spikePlatforms[i].m_object_type = 2;
    }

    // ————— GENERAL ————— //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    // VERY IMPORTANT: If nothing is pressed, we don't want to go anywhere
    g_game_state.player->set_movement(glm::vec3(0.0f));

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

            default:
                break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT] and g_game_state.player->fuel > 0)
    {
        g_game_state.player->move_left();
        g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->LEFT];
        g_game_state.player->fuel -= 1;
    }
    else if (key_state[SDL_SCANCODE_RIGHT] and g_game_state.player->fuel > 0)
    {
        g_game_state.player->move_right();
        g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->RIGHT];
        g_game_state.player->fuel -= 1;
    }

    // This makes sure that the player can't move faster diagonally
    if (glm::length(g_game_state.player->get_movement()) > 1.0f)
    {
        g_game_state.player->set_movement(glm::normalize(g_game_state.player->get_movement()));
    }
}

void update()
{
    // ————— DELTA TIME ————— //
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - g_previous_ticks; // the delta time is the difference from the last frame
    g_previous_ticks = ticks;

    // ————— FIXED TIMESTEP ————— //
    // STEP 1: Keep track of how much time has passed since last step
    delta_time += g_time_accumulator;

    // STEP 2: Accumulate the ammount of time passed while we're under our fixed timestep
    if (delta_time < FIXED_TIMESTEP)
    {
        g_time_accumulator = delta_time;
        return;
    }

    // STEP 3: Once we exceed our fixed timestep, apply that elapsed time into the objects' update function invocation
    while (delta_time >= FIXED_TIMESTEP)
    {
        // Notice that we're using FIXED_TIMESTEP as our delta time
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.platforms, PLATFORM_COUNT);
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.spikePlatforms, SPIKE_PLATFORM_COUNT);
        delta_time -= FIXED_TIMESTEP;
    }
    
    // check if the player has game win or game lose set 
    if (g_game_state.player->m_game_end == GAME_WIN)
    {
        draw_text(&g_shader_program, g_game_state.resultText->m_texture_id, "You Win!", 0.5f, 0.1f, glm::vec3(0.0f, 0.0f, 0.0f));
		g_game_is_running = false;
	}
    else if (g_game_state.player->m_game_end == GAME_LOSE)
    {
        draw_text(&g_shader_program, g_game_state.resultText->m_texture_id, "You Lose!", 0.5f, 0.1f, glm::vec3(0.0f, 0.0f, 0.0f));
		g_game_is_running = false;
	}

    g_time_accumulator = delta_time;
}

void render()
{
    // ————— GENERAL ————— //
    glClear(GL_COLOR_BUFFER_BIT);

    // ————— PLAYER ————— //
    g_game_state.player->render(&g_shader_program);

    // ————— PLATFORM ————— //
    for (int i = 0; i < PLATFORM_COUNT; i++) g_game_state.platforms[i].render(&g_shader_program);

    // Spike platform
    for (int i = 0; i < SPIKE_PLATFORM_COUNT; i++) g_game_state.spikePlatforms[i].render(&g_shader_program);


    if (g_game_state.player->m_game_end == 1)
    {
        draw_text(&g_shader_program, g_game_state.resultText->m_texture_id, "You Win!", 0.5f, 0.1f, glm::vec3(0.0f, 0.0f, 0.0f));
    }
    else if (g_game_state.player->m_game_end == 2)
    {
        draw_text(&g_shader_program, g_game_state.resultText->m_texture_id, "You Lose!", 0.5f, 0.1f, glm::vec3(0.0f, 0.0f, 0.0f));
    }

    // fuel text
    draw_text(&g_shader_program, g_game_state.fuelText->m_texture_id, "Fuel: " + std::to_string(g_game_state.player->fuel), 0.5f, 0.1f, glm::vec3(-1.0f, 3.0f, 0.0f));

    // ————— GENERAL ————— //
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

// ————— DRIVER GAME LOOP ————— /
int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }
    while(true){}

    //shutdown();
    //return 0;
}
