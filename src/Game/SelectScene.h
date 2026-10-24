#pragma once
#include <System/Scene.h>

namespace PoittersPoint {

class PoittersPoint_SelectScene : public Scene::Base
{
public:
    BP_CLASS_DECL(PoittersPoint_SelectScene, u8"SelectScene");

    bool Init() override;
    void Update() override;
    void LateDraw() override;

private:
    int m_selected = 0;
};

}    // namespace PoittersPoint
