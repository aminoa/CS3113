#include "Scene.h"

class LevelTwo : public Scene {
public:
    int ENEMY_COUNT = 1;
 
    ~LevelTwo();
    
    void initialise() override;
    void update(float delta_time) override;
    void render(ShaderProgram *program) override;
};
