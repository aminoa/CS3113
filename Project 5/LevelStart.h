#pragma once
#include "Scene.h"

class LevelStart : public Scene
{
public:
	int ENEMY_COUNT = 1;

	~LevelStart();
	
	void initialise() override;
	void update(float delta_time) override;
	void render(ShaderProgram *program) override;
};

