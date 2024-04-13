#pragma once
#include "Scene.h"

class LevelStart : public Scene
{
public:
	int ENEMY_COUNT = 0;

	~LevelStart();
	
	void initialise() override;
	void update(float delta_time) override;
	void render(ShaderProgram *program) override;
};

