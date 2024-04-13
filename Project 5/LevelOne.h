#include "Scene.h"

class LevelOne : public Scene {
public:
    int ENEMY_COUNT = 1;

    ~LevelOne();
    
    void initialise() override;
    void update(float delta_time) override;
    void render(ShaderProgram *program) override;
};
