//---------------------------------------------------------------------------
//!	@file	TutorialX_Player.cpp
//! @brief	TutorialX_Player
//---------------------------------------------------------------------------

#include "TutorialX_Player.h"
#include "TutorialX_Bullet.h"
#include "TutorialX.h"
#include "Component/ComponentStateIdleWalk.h"
#include "Game/Component/ComponentStateThrow.h"
#include "Component/ComponentCameraController.h"
#include "Component/ComponentPlayerState.h"

namespace TutorialX {
//! @brief 初期化
//! @return 初期化終了
bool Player::Init()
{
    Super::Init();

    SetName("Player");

    //auto player = Scene::Object::Create<Object>("Player");
    AddComponent<ComponentModel>("data/Game/Models/Player/Player.mv1")
        ->SetAnimation({
            { "idle",  "data/Game/Models/Player/Anims/test.mv1", 0, 1.0f}, // Idle
            { "walk",  "data/Game/Models/Player/Anims/test.mv1", 1, 1.0f}, // Jump
            {"throw", "data/Game/Models/Player/Anims/Throw.mv1", 1, 1.0f}  // Throw
    });

    SetTranslate({0, 5, 0});

    AddComponent<ComponentPlayerState>();

    AddComponent<ComponentCameraController>();

    auto col = AddComponent<ComponentCollisionCapsule>()    //
                   ->SetRadius(3.0f)
                   ->SetHeight(13.0f);

    col->UseGravity();
    col->SetCollisionGroup(ComponentCollision::CollisionGroup::PLAYER);

    return true;
}

void Player::Update()
{
    //if(Input::IsKeyDown(KEY_INPUT_SPACE))
    //{
    //	RemoveComponent<ComponentStateIdleWalk>();
    //	AddComponent<ComponentStateThrow>();
    //}

#if 0
		if(Input::IsKeyDown(KEY_INPUT_SPACE))
		{
			const float3 up_len = {0.0f, 5.0f, 0.0f};
			float3		 vec	= {0.0f, 1.0f, 0.0f};
			if(auto model = GetComponent<ComponentModel>())
			{
				constexpr float front_len = 5.0f;

				vec = -model->GetWorldVectorAxisZ() * front_len;
			}

			auto bullet = Scene::Object::Create<Bullet>();
			bullet->SetDirection(vec);
			bullet->SetTranslate(GetTranslate() + up_len + vec);
		}
#endif
}

}    // namespace TutorialX
