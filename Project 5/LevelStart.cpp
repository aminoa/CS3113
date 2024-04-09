#include "LevelStart.h"
#include "Utility.h"

#define LEVEL_WIDTH 14
#define LEVEL_HEIGHT 8

unsigned int LEVELSTART_DATA[] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

const char FONT_FILEPATH[] = "assets/font1.png";

LevelStart::~LevelStart()
{
	Mix_FreeMusic(m_state.bgm);
}

void LevelStart::initialise()
{
	m_state.next_scene_id = -1;
    GLuint map_texture_id = Utility::load_texture("assets/black.png");
    m_state.map = new Map(LEVEL_WIDTH, LEVEL_HEIGHT, LEVELSTART_DATA, map_texture_id, 1.0f, 4, 1);

     // Can't remove this code atm
    m_state.player = new Entity();
    m_state.player->set_entity_type(PLAYER);
    m_state.player->set_position(glm::vec3(5.0f, 0.0f, 0.0f));
    m_state.player->set_movement(glm::vec3(0.0f));
    m_state.player->m_speed = 2.5f;
    m_state.player->set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));

    // Walking
    m_state.player->m_walking[m_state.player->LEFT] = new int[4] { 1, 5, 9, 13 };
    m_state.player->m_walking[m_state.player->RIGHT] = new int[4] { 3, 7, 11, 15 };
    m_state.player->m_walking[m_state.player->UP] = new int[4] { 2, 6, 10, 14 };
    m_state.player->m_walking[m_state.player->DOWN] = new int[4] { 0, 4, 8, 12 };

    m_state.player->m_animation_indices = m_state.player->m_walking[m_state.player->RIGHT];  // start George looking left
    m_state.player->m_animation_frames = 4;
    m_state.player->m_animation_index = 0;
    m_state.player->m_animation_time = 0.0f;
    m_state.player->m_animation_cols = 4;
    m_state.player->m_animation_rows = 4;
    m_state.player->set_height(0.8f);
    m_state.player->set_width(0.8f);

    // Jumping
    m_state.player->m_jumping_power = 5.0f;

    // Text
    m_state.text = new Entity();
    m_state.text->m_texture_id = Utility::load_texture(FONT_FILEPATH);
    m_state.text->set_position(glm::vec3(0.0f, 1.0f, 0.0f));

    // BGM and SFX
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
	m_state.bgm = Mix_LoadMUS("assets/title_screen.mp3");
	Mix_PlayMusic(m_state.bgm, -1);

}

void LevelStart::update(float delta_time)
{
	// check if enter is pressed
	const Uint8* key_state = SDL_GetKeyboardState(NULL);
	if (key_state[SDL_SCANCODE_RETURN])
	{
		m_state.next_scene_id = 1;
	}
}

void LevelStart::render(ShaderProgram* program)
{
    Utility::draw_text(program, m_state.text->m_texture_id, "Press Enter", 0.5f, 0.1f, glm::vec3(2.0f, -1.0f, 0.0f));
    Utility::draw_text(program, m_state.text->m_texture_id, "to Start...", 0.5f, 0.1f, glm::vec3(2.0f, -2.0f, 0.0f));
	m_state.map->render(program);
}
