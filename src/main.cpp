#include <thread>
#include "main.hpp"
#include "beatsaber-hook/shared/utils/typedefs.h"
#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils-methods.hpp"
#include "beatsaber-hook/shared/utils/typedefs.h"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "GlobalNamespace/OVRInput.hpp"
#include "GlobalNamespace/OVRInput_Button.hpp"
#include "GlobalNamespace/OVRInput_Axis2D.hpp"
#include "GlobalNamespace/OVRInput_RawAxis2D.hpp"
#include "UnityEngine/Rigidbody.hpp"
#include "UnityEngine/Camera.hpp"
#include "UnityEngine/Collider.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/ForceMode.hpp"

#include "GorillaLocomotion/Player.hpp"

using namespace UnityEngine;
using namespace UnityEngine::XR;

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getConfig() {
    static Configuration config(modInfo);
    return config;
}

// Returns a logger, useful for printing debug messages
Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
}

bool allowFlyMode = false;

bool flyModeEnable = false;

bool speedOn = false;


float maximumSpeed = 5;

UnityEngine::Vector3 inputVector;

UnityEngine::Vector3 cameraEularAngles;

#include "GlobalNamespace/GorillaTagManager.hpp"
MAKE_HOOK_OFFSETLESS(GorillaTagManager_Update, void, GlobalNamespace::GorillaTagManager* self) {
    using namespace GlobalNamespace;
    using namespace GorillaLocomotion;

    Player* playerInstance = Player::get_Instance();
    if(playerInstance == nullptr) return;

    Rigidbody* playerPhysics = playerInstance->playerRigidBody;
    if(playerPhysics == nullptr) return;

    GameObject* playerGameObject = playerPhysics->get_gameObject();
    if(playerGameObject == nullptr) return;

    Transform* turnParent = playerGameObject->get_transform()->GetChild(0);

    Transform* mainCamera = turnParent->GetChild(0);

    bool rightInput = false;
    bool leftInput = false;
    rightInput = OVRInput::GetDown(OVRInput::Button::Two, OVRInput::Controller::RTouch);
    leftInput = OVRInput::GetDown(OVRInput::Button::Two, OVRInput::Controller::LTouch);

    if(leftInput || rightInput) {
        if(flyModeEnable) {
            flyModeEnable = false;
            playerPhysics->set_velocity(Vector3::get_zero());
        }
        else if(!flyModeEnable) {
            flyModeEnable = true;
            playerPhysics->set_velocity(Vector3::get_zero());
        }
    }
    if(allowFlyMode) 
    {  
        if(flyModeEnable) {
            playerPhysics->set_useGravity(false);
            Vector2 inputDir = OVRInput::GetResolvedAxis2D(OVRInput::Axis2D::_get_PrimaryThumbstick(), OVRInput::RawAxis2D::_get_RThumbstick(), OVRInput::Controller::LTouch);
           
            Vector3 velocityForward = mainCamera->get_forward() * (inputDir.y / 10);
            Vector3 velocitySideways = mainCamera->get_right() * (inputDir.x / 10);

            Vector3 newVelocityDir = velocityForward + velocitySideways;

            playerPhysics->set_velocity(playerPhysics->get_velocity() + newVelocityDir);


            
            float speed = UnityEngine::Vector3::Magnitude(playerPhysics->get_velocity());
            
            if(speed > maximumSpeed) {
                float brakeSpeed = speed - maximumSpeed;

                Vector3 normalisedVelocity = playerPhysics->get_velocity().get_normalized();
                Vector3 brakeVelocity = normalisedVelocity * brakeSpeed;

                playerPhysics->AddForce(-brakeVelocity);
            }
        }
    }
    else {
        playerPhysics->set_useGravity(true);
    }


    GorillaTagManager_Update(self);
}

#include "GlobalNamespace/PhotonNetworkController.hpp"

MAKE_HOOK_OFFSETLESS(PhotonNetworkController_OnJoinedRoom, void, Il2CppObject* self)
{
    PhotonNetworkController_OnJoinedRoom(self);

    Il2CppObject* currentRoom = CRASH_UNLESS(il2cpp_utils::RunMethod("Photon.Pun", "PhotonNetwork", "get_CurrentRoom"));

    if (currentRoom)
    {
        // get wether or not this is a private room
        allowFlyMode = !CRASH_UNLESS(il2cpp_utils::RunMethod<bool>(currentRoom, "get_IsVisible"));
    }
    else allowFlyMode = true;

    // ? construction to switch what is logged, logs work like printf in C with the % placeholders
    getLogger().info("Room Joined! %s", allowFlyMode ? "Room Was Private" : "Room Was not private");
}
#include "Photon/Realtime/DisconnectCause.hpp"
MAKE_HOOK_OFFSETLESS(PhotonNetworkController_OnLeavedRoom, void, Il2CppObject* self, Photon::Realtime::DisconnectCause cause)
{
    PhotonNetworkController_OnLeavedRoom(self, cause);
    using namespace GorillaLocomotion;

    Player* playerInstance = Player::get_Instance();
    if(playerInstance == nullptr) return;

    Rigidbody* playerPhysics = playerInstance->playerRigidBody;
    if(playerPhysics == nullptr) return;
    
    playerPhysics->set_useGravity(true);
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
	
    getConfig().Load();
    getLogger().info("Completed setup!");
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();

    INSTALL_HOOK_OFFSETLESS(getLogger(), GorillaTagManager_Update, il2cpp_utils::FindMethodUnsafe("", "GorillaTagManager", "Update", 0));
    INSTALL_HOOK_OFFSETLESS(getLogger(), PhotonNetworkController_OnJoinedRoom, il2cpp_utils::FindMethodUnsafe("", "PhotonNetworkController", "OnJoinedRoom", 0));
    INSTALL_HOOK_OFFSETLESS(getLogger(), PhotonNetworkController_OnLeavedRoom, il2cpp_utils::FindMethodUnsafe("", "PhotonNetworkController", "OnDisconnected", 1));
    getLogger().info("Installing hooks...");
    // Install our hooks (none defined yet)
    getLogger().info("Installed all hooks!");
}