// Name: Shehani Kaveenya Mukadange
// Student ID: G21364691
// Module: CO1301 – Games Concepts
// Title: Killer Quebes!
//
// Description:
// A TL-Engine based 3D marble-shooting game where the player
// aims and fires a marble at moving blocks to destroy them.
// The game uses a state machine (READY, FIRING, CONTACT, OVER)
// to control gameplay flow. Includes barriers, collisions,
// and animated blocks with wobbling movement.

// G21364691.cpp: A program using the TL-Engine
#include <TL-Engine.h>
#include <cmath>
using namespace tle;

// Constants
const float kSpeed = 0.01f;
const int kNumBlocks = 10;
const float kBlockWidth = 10.0f;
const float kGapSize = 3.0f;
const float kBlockZ = 120.0f;
const float kBlockY = 0.0f;
const float kMarbleRadius = 2.0f;
const float kBlockRadius = 5.0f;
const float kBarrierX = 60.0f;
const int kNumBarriers = 8;
const float kBarrierSpacing = 25.0f;
const float kMaxAimAngle = 60.0f;
const float kAimRotationSpeed = 2.0f * kSpeed;
const float kMarbleSpeed = 60.0f;
const int kNumRows = 2;
const float kBlockMoveSpeed = 3.0f;
const float kWobbleSpeed = 1.5f;
const float kWobbleAmplitude = 1.0f;

// Enums for game states
enum GameState { READY, FIRING, CONTACT, OVER };
enum BlockState { NORMAL, HIT_ONCE, DEAD };

// Game data structure storing all game objects and variables
struct GameData
{
    GameState currentState;
    BlockState blockStates[kNumRows][kNumBlocks];
    IModel* blocks[kNumRows][kNumBlocks];
    float blockInitialY[kNumRows][kNumBlocks];
    IModel* leftBarriers[kNumBarriers];
    IModel* rightBarriers[kNumBarriers];
    IModel* marble;
    IModel* arrow;
    IModel* dummyModel;
    IFont* font;
    float marbleVelocityX;
    float marbleVelocityZ;
    float dummyRotation;
    float blockMovement;
    float wobbleTime;
};

// Function declarations
bool SphereToBoxCollision(IModel* sphere, IModel* box, float sphereRadius);
void ResetMarble(GameData& game);
int CountHitBlocks(const GameData& game);
void InitializeGame(I3DEngine* engine, GameData& game);
void HandleReadyState(I3DEngine* engine, GameData& game, float frameTime);
void HandleFiringState(I3DEngine* engine, GameData& game, float frameTime);
void HandleContactState(GameData& game);
void HandleOverState(const GameData& game);
void UpdateAiming(I3DEngine* engine, GameData& game);
void UpdateBlockPositions(GameData& game, float frameTime);
bool CheckBlockCollisions(GameData& game);
bool CheckBarrierCollisions(GameData& game);

// Function: SphereToBoxCollision
// Purpose: Detects collision between a sphere (marble) and a box (block/barrier)
// Returns: true if overlap detected

bool SphereToBoxCollision(IModel* sphere, IModel* box, float sphereRadius)
{
    float sphereX = sphere->GetX();
    float sphereZ = sphere->GetZ();
    float boxX = box->GetX();
    float boxZ = box->GetZ();

    float boxHalfWidth = kBlockWidth / 2.0f;
    float boxHalfDepth = kBlockWidth / 2.0f;

    float closestX = sphereX;
    if (sphereX < boxX - boxHalfWidth) closestX = boxX - boxHalfWidth;
    else if (sphereX > boxX + boxHalfWidth) closestX = boxX + boxHalfWidth;

    float closestZ = sphereZ;
    if (sphereZ < boxZ - boxHalfDepth) closestZ = boxZ - boxHalfDepth;
    else if (sphereZ > boxZ + boxHalfDepth) closestZ = boxZ + boxHalfDepth;

    float dx = sphereX - closestX;
    float dz = sphereZ - closestZ;
    float distanceSq = dx * dx + dz * dz;

    return distanceSq < (sphereRadius * sphereRadius);
}

// Function: ResetMarble
// Purpose: Reset the marble to its original position 
void ResetMarble(GameData& game)
{
    game.marble->SetPosition(0.0f, 2.0f, 0.0f);
    game.marble->ResetOrientation();
    game.marble->RotateY(game.dummyRotation);
    game.marbleVelocityX = 0.0f;
    game.marbleVelocityZ = 0.0f;
}

// Function: CountHitBlocks
// Purpose: Returns number of destroyed (DEAD) blocks
int CountHitBlocks(const GameData& game)
{
    int count = 0;
    for (int row = 0; row < kNumRows; row++)
    {
        for (int i = 0; i < kNumBlocks; i++)
        {
            if (game.blockStates[row][i] == DEAD)
            {
                count++;
            }
        }
    }
    return count;
}
// Function: InitializeGame
// Purpose: Load all meshes, create scene objects, and initialize variables
void InitializeGame(I3DEngine* engine, GameData& game)
{
    // Set initial state and zero movement
    game.currentState = READY;
    game.marbleVelocityX = 0.0f;
    game.marbleVelocityZ = 0.0f;
    game.dummyRotation = 0.0f;
    game.blockMovement = 0.0f;
    game.wobbleTime = 0.0f;

    // Load font for text rendering
    game.font = engine->LoadFont("JetBrains Mono", 20);

    // Load meshes for all models
    IMesh* floorMesh = engine->LoadMesh("Floor.x");
    IMesh* skyboxMesh = engine->LoadMesh("Skybox_Hell.x");
    IMesh* blockMesh = engine->LoadMesh("Cube.x");
    IMesh* marbleMesh = engine->LoadMesh("Sphere.x");
    IMesh* arrowMesh = engine->LoadMesh("Arrow.x");
    IMesh* barrierMesh = engine->LoadMesh("Barrier.x");
    IMesh* dummyMesh = engine->LoadMesh("Dummy.x");

    // Create floor and sky
    IModel* floor = floorMesh->CreateModel(0.0f, -100.0f, 0.0f);
    floor->SetSkin("rough_mud.jpg");
    IModel* skybox = skyboxMesh->CreateModel(0.0f, -1000.0f, 0.0f);

    // Create player dummy and marble
    game.dummyModel = dummyMesh->CreateModel(0.0f, 0.0f, 0.0f);
    game.marble = marbleMesh->CreateModel(0.0f, 2.0f, 0.0f);

    // Attach arrow to dummy to show aim direction
    game.arrow = arrowMesh->CreateModel(0.0f, 0.0f, -30.0f);
    game.arrow->AttachToParent(game.dummyModel);
    game.arrow->Scale(1.5f);

    // Position blocks evenly across the play area
    float totalWidth = (kNumBlocks * kBlockWidth) + ((kNumBlocks - 1) * kGapSize);
    float startX = -(totalWidth / 2.0f) + (kBlockWidth / 2.0f);

    for (int row = 0; row < kNumRows; row++)
    {
        float zPos = kBlockZ + (row * 15.0f);
        for (int i = 0; i < kNumBlocks; i++)
        {
            float xPos = startX + (i * (kBlockWidth + kGapSize));
            game.blocks[row][i] = blockMesh->CreateModel(xPos, kBlockY, zPos);
            game.blockInitialY[row][i] = kBlockY;
            game.blockStates[row][i] = NORMAL;
        }
    }

    // Create barrier walls on left and right
    for (int i = 0; i < kNumBarriers; i++)
    {
        float zPos = i * kBarrierSpacing;

        game.leftBarriers[i] = barrierMesh->CreateModel(-kBarrierX, 0.0f, zPos);
        game.rightBarriers[i] = barrierMesh->CreateModel(kBarrierX, 0.0f, zPos);

        // Use wasp stripe skin for sections furthest from camera (4-7)
        if (i >= 4)
        {
            game.leftBarriers[i]->SetSkin("barrier2.jpg");
            game.rightBarriers[i]->SetSkin("barrier2.jpg");
        }
    }
}

// Function: UpdateBlockPositions
// Purpose: Moves blocks forward and adds wobbling motion
void UpdateBlockPositions(GameData& game, float frameTime)
{
    float movement = kBlockMoveSpeed * frameTime;
    game.blockMovement += movement;
    game.wobbleTime += frameTime;

    for (int row = 0; row < kNumRows; row++)
    {
        for (int i = 0; i < kNumBlocks; i++)
        {
            if (game.blockStates[row][i] != DEAD)
            {
                // Move blocks forward slowly
                game.blocks[row][i]->MoveZ(-movement);

                // Add wobble effect with phase offset per row
                float phaseOffset = row * 3.14159f;
                float wobbleOffset = sin(game.wobbleTime * kWobbleSpeed + phaseOffset) * kWobbleAmplitude;
                game.blocks[row][i]->SetY(game.blockInitialY[row][i] + wobbleOffset);
            }
        }
    }
}

// Function: UpdateAiming
// Purpose: Handles left/right aiming with Z and X keys
void UpdateAiming(I3DEngine* engine, GameData& game)
{
    if (engine->KeyHeld(Key_Z))
    {
        // Rotate dummy left
        game.dummyRotation -= kAimRotationSpeed;
        if (game.dummyRotation < -kMaxAimAngle)
        {
            game.dummyRotation = -kMaxAimAngle;
        }
        game.dummyModel->RotateY(-kAimRotationSpeed);
    }
    else if (engine->KeyHeld(Key_X))
    {
        // Rotate dummy right
        game.dummyRotation += kAimRotationSpeed;
        if (game.dummyRotation > kMaxAimAngle)
        {
            game.dummyRotation = kMaxAimAngle;
        }
        game.dummyModel->RotateY(kAimRotationSpeed);
    }
}

// Function: CheckBlockCollisions
// Purpose: Check collisions with blocks
bool CheckBlockCollisions(GameData& game)
{
    for (int row = 0; row < kNumRows; row++)
    {
        for (int i = 0; i < kNumBlocks; i++)
        {
            if (game.blockStates[row][i] != DEAD)
            {
                // Detect collision between marble and block
                if (SphereToBoxCollision(game.marble, game.blocks[row][i], kMarbleRadius))
                {
                    float blockX = game.blocks[row][i]->GetX();
                    float blockZ = game.blocks[row][i]->GetZ();
                    float marbleX = game.marble->GetX();
                    float marbleZ = game.marble->GetZ();

                    float dx = marbleX - blockX;
                    float dz = marbleZ - blockZ;

                    // Reverse marble direction depending on impact side
                    if (abs(dx) > abs(dz))
                    {
                        game.marbleVelocityX = -game.marbleVelocityX;
                    }
                    else
                    {
                        game.marbleVelocityZ = -game.marbleVelocityZ;
                    }

                    // Handle block damage levels
                    switch (game.blockStates[row][i])
                    {
                    case NORMAL:
                        game.blocks[row][i]->SetSkin("tiles_red.jpg");
                        game.blockStates[row][i] = HIT_ONCE;
                        break;

                    case HIT_ONCE:
                        game.blocks[row][i]->SetPosition(9999.0f, 9999.0f, 9999.0f);
                        game.blockStates[row][i] = DEAD;
                        break;

                    default:
                        break;
                    }

                    return true;
                }
            }
        }
    }
    return false;
}

// Function: CheckBarrierCollisions
// Purpose: Bounce marble off left and right barriers
bool CheckBarrierCollisions(GameData& game)
{
    for (int i = 0; i < kNumBarriers; i++)
    {
        if (SphereToBoxCollision(game.marble, game.leftBarriers[i], kMarbleRadius))
        {
            game.marbleVelocityX = -game.marbleVelocityX;
            return true;
        }

        if (SphereToBoxCollision(game.marble, game.rightBarriers[i], kMarbleRadius))
        {
            game.marbleVelocityX = -game.marbleVelocityX;
            return true;
        }
    }
    return false;
}

// Function: HandleReadyState
// Purpose: Player can aim and fire marble
void HandleReadyState(I3DEngine* engine, GameData& game, float frameTime)
{
    // Display instructions
    game.font->Draw("Press SPACE to fire!\nUse Z/X to aim.", 200, 30, kWhite);

    UpdateAiming(engine, game);
    UpdateBlockPositions(game, frameTime);

    // If player presses space, fire marble
    if (engine->KeyHit(Key_Space))
    {
        game.marble->ResetOrientation();
        game.marble->RotateY(game.dummyRotation);

        // Calculate velocity based on angle
        float angleRad = game.dummyRotation * 3.14159f / 180.0f;
        game.marbleVelocityX = sin(angleRad) * kMarbleSpeed * kSpeed;
        game.marbleVelocityZ = cos(angleRad) * kMarbleSpeed * kSpeed;

        game.currentState = FIRING; // switch to firing state
    }
}

// Function: HandleFiringState
// Purpose: Marble moves, collisions detected
void HandleFiringState(I3DEngine* engine, GameData& game, float frameTime)
{
    // Display firing message
    game.font->Draw("Firing! Use Z/X to aim next shot.", 200, 30, kWhite);

    // Allow aiming while firing
    UpdateAiming(engine, game);
    UpdateBlockPositions(game, frameTime);

    // Move marble
    game.marble->MoveX(game.marbleVelocityX);
    game.marble->MoveZ(game.marbleVelocityZ);

    // Check if marble reached back of arena (behind blocks)
    if (game.marble->GetZ() > kBlockZ + 20.0f)
    {
        ResetMarble(game);
        game.currentState = READY;
        return;
    }

    // Check if marble returned to front of arena
    if (game.marble->GetZ() < 0.0f)
    {
        ResetMarble(game);
        game.currentState = READY;
        return;
    }

    // Check collisions
    bool collisionDetected = CheckBlockCollisions(game);

    if (!collisionDetected)
    {
        collisionDetected = CheckBarrierCollisions(game);
    }

    if (collisionDetected)
    {
        game.currentState = CONTACT;
    }
}

// Function: HandleContactState
// Purpose: After collision, update game status
void HandleContactState(GameData& game)
{
    int hitCount = CountHitBlocks(game);
    int totalBlocks = kNumBlocks * kNumRows;

    if (hitCount >= totalBlocks)
    {
        game.marble->SetSkin("Mars.jpg");
        game.currentState = OVER;
    }
    else
    {
        game.currentState = FIRING;
    }
}

// Function: HandleOverState
// Purpose: Displays end-game message
void HandleOverState(const GameData& game)
{
    game.font->Draw("GAME OVER! All blocks destroyed!", 200, 300, kGreen);
    game.font->Draw("Press ESC to quit.", 300, 350, kWhite);
}

// MAIN FUNCTION
void main()
{
    // Create engine
    I3DEngine* myEngine = New3DEngine(kTLX);
    myEngine->StartWindowed();
    myEngine->SetWindowCaption("Killer Quebes!");
    myEngine->AddMediaFolder("C:\\ProgramData\\TL-Engine\\Media");
    myEngine->AddMediaFolder("C:\\Users\\sheha\\Desktop\\Shehani_G21364691\\Media");

    // Setup camera
    ICamera* myCamera = myEngine->CreateCamera(kManual);
    myCamera->SetPosition(0.0f, 30.0f, -70.0f);
    myCamera->RotateX(15.0f);

    // Initialize game
    GameData game;
    InitializeGame(myEngine, game);

    // Main game loop
    while (myEngine->IsRunning())
    {
        myEngine->DrawScene();
        float frameTime = myEngine->Timer();

        // ESC key exits from any state
        if (myEngine->KeyHit(Key_Escape))
        {
            myEngine->Stop();
        }

        // State machine
        switch (game.currentState)
        {
        case READY:
            HandleReadyState(myEngine, game, frameTime);
            break;

        case FIRING:
            HandleFiringState(myEngine, game, frameTime);
            break;

        case CONTACT:
            HandleContactState(game);
            break;

        case OVER:
            HandleOverState(game);
            break;
        }
    }
    myEngine->Delete();
}