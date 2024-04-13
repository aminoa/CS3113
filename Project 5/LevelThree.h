#include "Scene.h"

class LevelThree : public Scene {
public:
    int ENEMY_COUNT = 1;

    ~LevelThree();

    void initialise() override;
    void update(float delta_time) override;
    void render(ShaderProgram *program) override;
};
