#include "gamescreen.h"

#include <prism/numberpopuphandler.h>

#include "storyscreen.h"

static struct 
{
    CollisionListData* mShotList;
    CollisionListData* mGrabList;
    CollisionListData* mHumanList;
    CollisionListData* mSoulList;
    int mLevel = 0;
    int mGameTicks = 0;
    int mHasShownTutorial = 0; 
} gGameScreenData;

class GameScreen
{
    public:
    GameScreen() {
        instantiateActor(getPrismNumberPopupHandler());
        load();
        //activateCollisionHandlerDebugMode();
    }

    MugenSpriteFile mSprites;
    MugenAnimations mAnimations;
    MugenSounds mSounds;

    void loadFiles() {
        mSprites = loadMugenSpriteFileWithoutPalette(std::string("game/GAME") + std::to_string(gGameScreenData.mLevel) + ".sff");
        mAnimations = loadMugenAnimationFile(std::string("game/GAME") + std::to_string(gGameScreenData.mLevel) + ".air");
        mSounds = loadMugenSoundFile("game/GAME.snd");
    }

    double sfxVol = 0.2;

    void load() {
        loadFiles();
        loadCollisionLists();
        loadBG();
        loadCrosshair();
        loadShots();
        loadHumans();
        loadEnemies();
        loadWinning();
        loadLosing();
        loadTutorial();
        loadPauseMenu();
        loadSpeedrunTimer();
        loadLevelIntro();
    }

    void update() {
        updateCollisionLists();
        updateBG();
        if (!isShowingTutorial && !isShowingPauseMenu && !isLosing && !isWinning)
        {
            updateLevelIntro();
        }
        if (!isShowingTutorial && !isShowingPauseMenu && !isLosing && !isWinning && !isShowingLevelIntro)
        {
            updateCrosshair();
            updateShots();
            updateHumans();
            updateEnemies();
            updateSpeedrunTimer();
        }
        if (!isShowingTutorial && !isShowingPauseMenu && !isShowingLevelIntro)
        {
            updateWinning();
            updateLosing();
        }
        if (!isShowingTutorial && !isLosing && !isWinning && !isShowingLevelIntro)
        {
            updatePauseMenu();
        }
        updateTutorial();
    }

    // LEVEL INTRO
    std::vector<std::string> introStrings1 = { "LEVEL 1 - MC RONALD'S", "LEVEL 2 - MC RONALD'S + SMOG", "FINAL LEVEL - HELL" };
    std::vector<std::string> introStrings2 = { "IF ONLY BURGERS WERE REAL", "PUT THIS ON THE BOXES", "THE GREATEST WEAPON OF ALL" };
    int levelIntroTextID1;
    int levelIntroTextID2;
    bool wasLevelIntroShown = false;
    bool isShowingLevelIntro = false;
    int levelIntroTicks = 0;
    void loadLevelIntro() {
        levelIntroTextID1 = addMugenTextMugenStyle(introStrings1[gGameScreenData.mLevel].c_str(), Vector3D(160, 130, 70), Vector3DI(2, 0, 0));
        levelIntroTextID2 = addMugenTextMugenStyle(introStrings2[gGameScreenData.mLevel].c_str(), Vector3D(160, 140, 70), Vector3DI(2, 0, 0));
        setMugenTextVisibility(levelIntroTextID1, 0);
        setMugenTextVisibility(levelIntroTextID2, 0);
        setMugenTextScale(levelIntroTextID1, 1.0);
        setMugenTextScale(levelIntroTextID2, 1.0);
    }
    void updateLevelIntro() {
        if (wasLevelIntroShown) return;
        if (isShowingLevelIntro)
        {
            levelIntroTicks++;
            if (levelIntroTicks == 180 || hasPressedStartFlank())
            {
                setMugenTextVisibility(levelIntroTextID1, 0);
                setMugenTextVisibility(levelIntroTextID2, 0);
                setMugenTextVisibility(soulsLeftTextId, 1);
                setMugenTextVisibility(enemySoulsTakenTextId, 1);
                isShowingLevelIntro = false;
                wasLevelIntroShown = true;
                stopAllSoundEffects();
                if (isOnDreamcast())
                {
                    streamMusicFile("game/GAME_DC.ogg");
                }
                else
                {
                    streamMusicFile("game/GAME.ogg");
                }
            }
        }
        else
        {
            tryPlayMugenSoundAdvanced(&mSounds, 1, 5, 1.0);
            setMugenTextVisibility(levelIntroTextID1, 1);
            setMugenTextVisibility(levelIntroTextID2, 1);
            setMugenTextVisibility(soulsLeftTextId, 0);
            setMugenTextVisibility(enemySoulsTakenTextId, 0);
            isShowingLevelIntro = true;
        }
    }

    // STRUCTS
    enum class HumanState
    {
        WALKING,
        SOUL_FORMING,
        WAITING,
        ASCENDING,
        OVER
    };
    struct Human
    {
        int entityID;
        int collisionID;
        int soulCollisionID;
        HumanState state;
        Vector2D velocity;
        int ticksNow = 0;
        int ticksTarget = 0;
        bool toBeDeleted = false;
    };
    std::map<int, Human> mHumans;

    // COLLISION LISTS
    void loadCollisionLists() {
        gGameScreenData.mHumanList = addCollisionListToHandler();
        gGameScreenData.mSoulList = addCollisionListToHandler();
        gGameScreenData.mShotList = addCollisionListToHandler();
        gGameScreenData.mGrabList = addCollisionListToHandler();
        addCollisionHandlerCheck(gGameScreenData.mShotList, gGameScreenData.mHumanList);
        addCollisionHandlerCheck(gGameScreenData.mGrabList, gGameScreenData.mSoulList);
    }
    void updateCollisionLists() { /*EMPTY*/ }

    // BG
    int mBGEntity;
    void loadBG() {
        mBGEntity = addBlitzEntity(Vector3D(0, 0, 1));
        addBlitzMugenAnimationComponent(mBGEntity, &mSprites, &mAnimations, 1);
    }
    void updateBG() { /*EMPTY*/ }

    // CROSSHAIR
    int crosshairEntity;
    void loadCrosshair() {
        crosshairEntity = addBlitzEntity(Vector3D(160, 120, 50));
        if (!isOnDreamcast())
        {
            setBlitzEntityPositionX(crosshairEntity, -100);
        }
        addBlitzMugenAnimationComponent(crosshairEntity, &mSprites, &mAnimations, 70);
        addBlitzCollisionAttackMugen(crosshairEntity, gGameScreenData.mGrabList);
    }
    void updateCrosshair() {
        if (isWinning || isLosing) return;
        updateCrosshairAngle();
        updateCrosshairMovement();
        updateCrosshairShooting();
        updateCrosshairGrabbing();
        checkEnemiesToBeShot();
    }
    void updateCrosshairAngle() {
        if (getBlitzMugenAnimationAnimationNumber(crosshairEntity) == 70)
        {
            addBlitzEntityRotationZ(crosshairEntity, 0.01);
        }
        else
        {
            setBlitzEntityRotationZ(crosshairEntity, 0.0);
        }
    }
    void updateCrosshairMovement() {
        auto* pos = getBlitzEntityPositionReference(crosshairEntity);
        if (isOnDreamcast())
        {
            double speed = 6;
            if (hasPressedLeft())
            {
                pos->x -= speed;
            }
            if (hasPressedRight())
            {
                pos->x += speed;
            }
            if (hasPressedUp())
            {
                pos->y -= speed;
            }
            if (hasPressedDown())
            {
                pos->y += speed;
            }
            *pos = clampPositionToGeoRectangle(*pos, GeoRectangle2D(0, 0, 320, 240));
        }
        else
        {
            auto mousePos = getMousePointerPosition();
            pos->x = mousePos.x;
            pos->y = mousePos.y;
        }
    }
    void updateCrosshairShooting() {
        if (hasPressedAFlank() || hasPressedMouseLeftFlank())
        {
            changeBlitzMugenAnimation(crosshairEntity, 70);
            addShot(getBlitzEntityPosition(crosshairEntity).xy().xyz(40));
        }
    }

    void updateCrosshairGrabbing() {
        if (hasPressedBFlank() || hasPressedMouseRightFlank())
        {
            changeBlitzMugenAnimation(crosshairEntity, 61);
        }
    }

    void checkEnemiesToBeShot()
    {
        auto enemies = getZOrderedEnemies();
        for (int i = 0; i < enemies.size(); i++)
        {
            if (checkEnemyForHit(enemies[i]))
            {
                break;
            }
        }
    }

    std::vector<Human*> getZOrderedEnemies()
    {
        std::vector<Human*> result;
        for (auto& it : mHumans)
        {
            result.push_back(&it.second);
        }
        std::sort(result.begin(), result.end(), [](Human* a, Human* b)
            {
                auto* posA = getBlitzEntityPositionReference(a->entityID);
                auto* posB = getBlitzEntityPositionReference(b->entityID);
                return posA->z > posB->z;
            });
        return result;
    }

    bool checkEnemyForHit(Human* s)
    {
        if (hasBlitzCollidedThisFrame(s->entityID, s->collisionID))
        {
            auto enemyPos = getBlitzEntityPositionReference(s->entityID);
            startHumanSoulForming(*s);
            addPrismNumberPopup(-1, enemyPos->xy().xyz(60) - Vector2D(0, 70 * (*getBlitzMugenAnimationBaseScaleReference(s->entityID))), 1, Vector3D(0, -1, 0), 2.0 * (*getBlitzMugenAnimationBaseScaleReference(s->entityID)), 1, 30);
            tryPlayMugenSoundAdvanced(&mSounds, 1, 0, sfxVol);
            return true;
        }
        if (hasBlitzCollidedThisFrame(s->entityID, s->soulCollisionID))
        {
            auto enemyPos = getBlitzEntityPositionReference(s->entityID);
            startHumanRemoving(*s);
            caughtSouls++;
            addPrismNumberPopup(1, enemyPos->xy().xyz(60) - Vector2D(0, 70 * (*getBlitzMugenAnimationBaseScaleReference(s->entityID))), 1, Vector3D(0, -1, 0), 2.0 * (*getBlitzMugenAnimationBaseScaleReference(s->entityID)), 2, 30);
            tryPlayMugenSoundAdvanced(&mSounds, 1, 3, sfxVol);
            return true;
        }
        return false;
    }

    // SHOTS
    struct Shot
    {
        int entityID;
        int collisionID;
        bool toBeDeleted = false;
    };
    std::map<int, Shot> mShots;
    void loadShots() {}
    void updateShots() {
        updateActiveShots();
    }
    void updateActiveShots() {
        auto it = mShots.begin();
        while (it != mShots.end())
        {
            updateSingleActiveShot(it->second);
            if (it->second.toBeDeleted)
            {
                unloadShot(it->second);
                it = mShots.erase(it);
            }
            else
            {
                it++;
            }
        }
    }
    void updateSingleActiveShot(Shot& s) {
        if (s.toBeDeleted) return;
        addBlitzEntityRotationZ(s.entityID, 0.1);
        auto currentScale = *getBlitzMugenAnimationBaseScaleReference(s.entityID);
        setBlitzMugenAnimationBaseDrawScale(s.entityID, currentScale * 0.95);
        if (currentScale < 0.5 || hasBlitzCollidedThisFrame(s.entityID, s.collisionID))
        {
            startShotRemoval(s);
        }
    }
    void startShotRemoval(Shot& s) {
        s.toBeDeleted = true;
        changeBlitzMugenAnimation(s.entityID, -1);
    }
    void addShot(const Vector3D& pos) {
        auto id = addBlitzEntity(pos);
        addBlitzMugenAnimationComponent(id, &mSprites, &mAnimations, 50);
        setBlitzMugenAnimationNoLoop(id);
        addBlitzEntityRotationZ(id, randfrom(0.0, 2.0 * M_PI));
        auto collisionId = addBlitzCollisionAttackMugen(id, gGameScreenData.mShotList);
        tryPlayMugenSoundAdvanced(&mSounds, 1, 2, sfxVol);
        mShots[id] = Shot{ id, collisionId };
    }
    void unloadShot(Shot& s) {
        removeBlitzEntity(s.entityID);
    }

    // HUMANS
    void loadHumans() {}
    void updateHumans() {
        updateHumanAdding();
        updateActiveHumans();
    }
    void updateHumanAdding() {
        if (isWinning || isLosing) return;
        if (randfromInteger(0, 1000) < 50)
        {
            addHuman();
        }
    }
    void addHuman() {
        bool isWalkingLeft = randfromInteger(0, 1000) < 500;
        auto entityId = addBlitzEntity(Vector3D(isWalkingLeft ? -20.0 : 340.0, randfrom(119, 200), 10));
        addBlitzMugenAnimationComponent(entityId, &mSprites, &mAnimations, 10);
        setBlitzMugenAnimationFaceDirection(entityId, isWalkingLeft);
        auto collisionId = addBlitzCollisionPassiveMugen(entityId, gGameScreenData.mHumanList);
        auto soulCollisionId = addBlitzCollisionAttackMugen(entityId, gGameScreenData.mSoulList);
        mHumans[entityId] = Human{ entityId, collisionId, soulCollisionId, HumanState::WALKING, Vector2D(isWalkingLeft ? 2.0 : -2.0, 0.0) };
    }
    void updateActiveHumans() {
        auto it = mHumans.begin();
        while (it != mHumans.end())
        {
            updateSingleActiveHuman(it->second);
            if (it->second.toBeDeleted)
            {
                unloadHuman(it->second);
                it = mHumans.erase(it);
            }
            else
            {
                it++;
            }
        }
    }
    void updateSingleActiveHuman(Human& s) {
        if (s.toBeDeleted) return;
        updateHumanState(s);
    }
    void updateHumanState(Human& s)
    {
        if (s.state == HumanState::WALKING)
        {
            updateHumanWalking(s);
            updateHumanZAndScale(s);
        }
        else if (s.state == HumanState::SOUL_FORMING)
        {
            updateHumanSoulForming(s);
        }
        else if (s.state == HumanState::WAITING)
        {
            updateHumanWaiting(s);
        }
        else if (s.state == HumanState::ASCENDING)
        {
            updateHumanAscending(s);
        }
    }

    void updateHumanWalking(Human& s) {
        auto* pos = getBlitzEntityPositionReference(s.entityID);
        pos->x += s.velocity.x;
        if (pos->x <= -30 || pos->x >= 350)
        {
            startHumanRemoving(s);
        }
    }
    void updateHumanZAndScale(Human& s) {
        double miniScale = 0.5;
        double fullScale = 1.0;
        double startY = 119;
        double endY = 240;
        auto* pos = getBlitzEntityPositionReference(s.entityID);

        auto t = (pos->y - startY) / (endY - startY);
        auto scale = miniScale + (fullScale - miniScale) * t;
        pos->z = 10 + t * 10;
        setBlitzMugenAnimationBaseDrawScale(s.entityID, scale);
    }
    void startHumanSoulForming(Human& s) {
        s.ticksNow = 0;
        s.ticksTarget = 120;
        tryPlayMugenSoundAdvanced(&mSounds, 1, 1, sfxVol);
        changeBlitzMugenAnimation(s.entityID, 80);
        s.state = HumanState::SOUL_FORMING;
    }
    void updateHumanSoulForming(Human& s) {
        s.ticksNow++;
        if (s.ticksNow >= s.ticksTarget)
        {
            startHumanWaiting(s);
        }
    }
    void startHumanWaiting(Human& s) {
        s.ticksNow = 0;
        s.ticksTarget = 240;
        changeBlitzMugenAnimation(s.entityID, 20);
        s.state = HumanState::WAITING;
    }
    void updateHumanWaiting(Human& s) {
        s.ticksNow++;
        if (s.ticksNow >= s.ticksTarget)
        {
            startHumanAscending(s);
        }
    }
    void startHumanAscending(Human& s) {
        s.ticksNow = 0;
        s.ticksTarget = 120;
        changeBlitzMugenAnimation(s.entityID, 30);
        s.state = HumanState::ASCENDING;
    }
    void updateHumanAscending(Human& s) {
        s.ticksNow++;
        auto* pos = getBlitzEntityPositionReference(s.entityID);
        pos->y -= 1.0;
        auto t = double(s.ticksTarget - s.ticksNow) / s.ticksTarget;
        setBlitzMugenAnimationTransparency(s.entityID, t);
        if (s.ticksNow >= s.ticksTarget)
        {
            startHumanRemoving(s);
        }
    }
    void startHumanRemoving(Human& h)
    {
        changeBlitzMugenAnimation(h.entityID, -1);
        h.toBeDeleted = true;
        h.state = HumanState::OVER;
    }
    void unloadHuman(Human& h) {
        removeBlitzEntity(h.entityID);
    }

    // ENEMY
    enum class EnemyState
    {
        MOVING,
        WAITING,
        GRABBING,
        OVER,
    };
    struct Enemy
    {
        int entityID;
        int earEntityID;
        EnemyState state;
        int ticksNow = 0;
        int ticksTarget = 0;
        Vector2D target = Vector2D(0, 0);
        bool toBeDeleted = false;
    };
    std::map<int, Enemy> mEnemies;
    void loadEnemies() {}
    void updateEnemies() {
        updateEnemyAdding();
        updateActiveEnemies();
    }
    void updateEnemyAdding() {
        if (isWinning || isLosing) return;
        if (mEnemies.size() < gGameScreenData.mLevel + 1)
        {
            addEnemy();
        }
    }
    void addEnemy() {
        bool isWalkingLeft = randfromInteger(0, 1000) < 500;
        bool isWalkingTop = randfromInteger(0, 1000) < 500;
        auto entityId = addBlitzEntity(Vector3D(isWalkingLeft ? 0.0 : 320.0, isWalkingTop ? 0.0 : 240.0, 10));
        addBlitzMugenAnimationComponent(entityId, &mSprites, &mAnimations, 130);
        auto earEntityId = addBlitzEntity(getBlitzEntityPosition(entityId));
        addBlitzMugenAnimationComponent(earEntityId, &mSprites, &mAnimations, 140);
        auto color = 6;
        if (mEnemies.size() == 0) color = 6;
        else if (mEnemies.size() == 1) color = 5;
        else color = 4;
        Vector3D c;
        getRGBFromColor(getMugenTextColorFromMugenTextColorIndex(color), &c.x, &c.y, &c.z);
        setBlitzMugenAnimationColor(entityId, c.x, c.y, c.z);
        setBlitzMugenAnimationColor(earEntityId, c.x, c.y, c.z);
        mEnemies[entityId] = Enemy{ entityId, earEntityId, EnemyState::MOVING };
    }
    void updateActiveEnemies() {
        auto it = mEnemies.begin();
        while (it != mEnemies.end())
        {
            updateSingleActiveEnemy(it->second);
            if (it->second.toBeDeleted)
            {
                unloadEnemy(it->second);
                it = mEnemies.erase(it);
            }
            else
            {
                it++;
            }
        }
    }
    void updateSingleActiveEnemy(Enemy& s) {
        if (s.toBeDeleted) return;
        updateEnemyState(s);
        if (getBlitzMugenAnimationAnimationNumber(s.entityID) == 130)
        {
            addBlitzEntityRotationZ(s.entityID, 0.01);
        }
        else
        {
            setBlitzEntityRotationZ(s.entityID, 0.0);
        }
        setBlitzEntityPosition(s.earEntityID, getBlitzEntityPosition(s.entityID));
    }
    void updateEnemyState(Enemy& s)
    {
        if (s.state == EnemyState::MOVING)
        {
            updateEnemyMoving(s);
        }
        else if (s.state == EnemyState::WAITING)
        {
            updateEnemyWaiting(s);
        }
        else if (s.state == EnemyState::GRABBING)
        {
            updateEnemyGrabbing(s);
        }
    }

    void startEnemyMoving(Enemy& s)
    {
        s.state = EnemyState::MOVING;
    }

    Human* selectEnemyTarget(Enemy& s)
    {
        Human* h = nullptr;
        auto enemies = getZOrderedEnemies();
        auto pos = getBlitzEntityPosition(s.entityID).xy();
        s.target = Vector2D(0, 0);
        double distSmallest = INFINITY;
        for (auto& enemy : enemies)
        {
            if (enemy->state == HumanState::WAITING)
            {
                auto enemyPos = getBlitzEntityPosition(enemy->entityID).xy();
                bool alreadyChosen = false;
                for (auto& e : mEnemies)
                {
                    if (e.second.entityID < s.entityID && e.second.target == enemyPos)
                    {
                        alreadyChosen = true;
                        break;
                    }
                }
                if (alreadyChosen) continue;
                auto dist = vecLength(pos - enemyPos);
                if (dist < distSmallest)
                {
                    distSmallest = dist;
                    s.target = enemyPos;
                    h = enemy;
                }
            }
        }
        return h;
    }

    void updateEnemyMoving(Enemy& s) {
        selectEnemyTarget(s);
        if (s.target == Vector2D(0, 0)) return;
        auto* pos = getBlitzEntityPositionReference(s.entityID);
        auto delta = s.target - pos->xy();
        auto dist = vecLength(delta);
        if (dist <= 5)
        {
            startEnemyWaiting(s);
        }
        else
        {
            double speed = 2.0;
            auto dir = vecNormalize(delta);
            *pos = *pos + (dir * speed);
        }
    }
    void startEnemyWaiting(Enemy& s) {
        s.ticksNow = 0;
        s.ticksTarget = 50;
        changeBlitzMugenAnimation(s.entityID, 60);
        s.state = EnemyState::WAITING;
    }
    void updateEnemyWaiting(Enemy& s) {
        selectEnemyTarget(s);
        auto* pos = getBlitzEntityPositionReference(s.entityID);
        auto delta = s.target - pos->xy();
        auto dist = vecLength(delta);
        if (s.target == Vector2D(0, 0) || dist > 5)
        {
            changeBlitzMugenAnimation(s.entityID, 130);
            startEnemyMoving(s);
        }
        else
        {
            s.ticksNow++;
            if (s.ticksNow >= s.ticksTarget)
            {
                startEnemyGrabbing(s);
            }
        }
    }
    void startEnemyGrabbing(Enemy& s) {
        changeBlitzMugenAnimation(s.entityID, 61);
        s.ticksNow = 0;
        s.ticksTarget = 20;
        s.state = EnemyState::GRABBING;
    }
    void updateEnemyGrabbing(Enemy& s) {
        selectEnemyTarget(s);
        auto* pos = getBlitzEntityPositionReference(s.entityID);
        auto delta = s.target - pos->xy();
        auto dist = vecLength(delta);
        if (s.target == Vector2D(0, 0) || dist > 5)
        {
            changeBlitzMugenAnimation(s.entityID, 130);
            startEnemyMoving(s);
        }
        else
        {
            if (s.ticksNow == 0)
            {
                auto h = selectEnemyTarget(s);
                auto enemyPos = getBlitzEntityPositionReference(h->entityID);
                addPrismNumberPopup(1, enemyPos->xy().xyz(60) - Vector2D(0, 70 * (*getBlitzMugenAnimationBaseScaleReference(h->entityID))), 1, Vector3D(0, -1, 0), 2.0 * (*getBlitzMugenAnimationBaseScaleReference(h->entityID)), 6, 30);
                tryPlayMugenSoundAdvanced(&mSounds, 1, 4, sfxVol);
                startHumanRemoving(*h);
                enemySoulsCaught++;
            }
            s.ticksNow++;
            if (s.ticksNow >= s.ticksTarget)
            {
                changeBlitzMugenAnimation(s.entityID, 130);
                startEnemyMoving(s);
            }
        }
    }
    void startEnemyRemoving(Enemy& h)
    {
        changeBlitzMugenAnimation(h.entityID, -1);
        h.toBeDeleted = true;
        h.state = EnemyState::OVER;
    }
    void unloadEnemy(Enemy& h) {
        removeBlitzEntity(h.entityID);
    }

    // WINNING
    int soulsToGrab = 25;
    int isWinning = 0;
    int caughtSouls = 0;
    MugenAnimationHandlerElement* winningEntity;
    int winningTicks = 0;
    int soulsLeftTextId;
    int enemySoulsTakenTextId;
    void loadWinning() {
        winningEntity = addMugenAnimation(getMugenAnimation(&mAnimations, 120), &mSprites, Vector3D(0, 0, 70));
        setMugenAnimationVisibility(winningEntity, 0);

        soulsLeftTextId = addMugenTextMugenStyle("", Vector3D(10, 10, 60), Vector3DI(2, 1, 1));
        enemySoulsTakenTextId = addMugenTextMugenStyle("", Vector3D(10, 20, 60), Vector3DI(2, 6, 1));
        updateEnemiesLeftUI();
    }

    void updateEnemiesLeftUI()
    {
        int soulsLeftNo = std::max(0, soulsToGrab - caughtSouls);
        std::string text = std::string("SOULS LEFT TO COLLECT: ") + std::to_string(soulsLeftNo) + " / " + std::to_string(soulsToGrab);
        changeMugenText(soulsLeftTextId, text.c_str());

        text = std::string("SOULS STOLEN BY BAD GUYS: ") + std::to_string(enemySoulsCaught) + " / " + std::to_string(enemySoulsCaughtThreshold);
        changeMugenText(enemySoulsTakenTextId, text.c_str());
    }

    void updateWinning() {
        if (isLosing) return;
        updateWinningStart();
        updateWinningOngoing();
    }
    void updateWinningStart() {
        if (isWinning) return;
        gGameScreenData.mGameTicks++;
        updateEnemiesLeftUI();

        if (caughtSouls >= soulsToGrab)
        {
            winningTicks = 0;
            setMugenAnimationVisibility(winningEntity, 1);
            stopStreamingMusicFile();
            tryPlayMugenSoundAdvanced(&mSounds, 100, 0, 1.0);
            removeAllThings();
            isWinning = true;
        }
    }
    void updateWinningOngoing() {
        if (!isWinning) return;

        if (hasPressedStartFlank())
        {
            
            if (gGameScreenData.mLevel == 2)
            {
                setCurrentStoryDefinitionFile("game/OUTRO.def", -1);
                setNewScreen(getStoryScreen());
            }
            else
            {
                gGameScreenData.mLevel++;
                setNewScreen(getGameScreen());
            }
        }
    }

    // LOSING
    int isLosing = 0;
    int enemySoulsCaught = 0;
    int enemySoulsCaughtThreshold = 10;
    MugenAnimationHandlerElement* losingEntity;
    void loadLosing() {
        enemySoulsCaughtThreshold *= (gGameScreenData.mLevel + 1);
        losingEntity = addMugenAnimation(getMugenAnimation(&mAnimations, 110), &mSprites, Vector3D(0, 0, 70));
        setMugenAnimationVisibility(losingEntity, 0);
    }
    void updateLosing() {
        if (isWinning) return;
        updateLosingStart();
        updateLosingOngoing();
    }
    void updateLosingStart() {
        if (isLosing) return;

        if (enemySoulsCaught >= enemySoulsCaughtThreshold)
        {
            winningTicks = 0;
            setMugenAnimationVisibility(losingEntity, 1);
            stopStreamingMusicFile();
            tryPlayMugenSoundAdvanced(&mSounds, 100, 1, 1.0);
            removeAllThings();
            isLosing = true;
        }
    }
    void updateLosingOngoing() {
        if (!isLosing) return;

        if (hasPressedStartFlank())
        {
            setNewScreen(getGameScreen());
        }
    }

    // TUTORIAL
    MugenAnimationHandlerElement* tutorialEntity;
    bool isShowingTutorial = false;
    void loadTutorial() {
        if (gGameScreenData.mHasShownTutorial) return;
        tutorialEntity = addMugenAnimation(getMugenAnimation(&mAnimations, 100), &mSprites, Vector3D(0, 0, 70));
        setMugenAnimationVisibility(tutorialEntity, 1);
        isShowingTutorial = 1;
    }
    void updateTutorial() {
        if (!isShowingTutorial) return;
        if (hasPressedStartFlank())
        {
            setMugenAnimationVisibility(tutorialEntity, 0);
            isShowingTutorial = false;
            gGameScreenData.mHasShownTutorial = 1;
        }
    }

    // PAUSEMENU
    MugenAnimationHandlerElement* pauseEntity;
    bool isShowingPauseMenu = false;
    void loadPauseMenu() {
        pauseEntity = addMugenAnimation(getMugenAnimation(&mAnimations, 90), &mSprites, Vector3D(0, 0, 70));
        setMugenAnimationVisibility(pauseEntity, 0);
    }
    void updatePauseMenu() {
        updatePauseMenuActivation();
    }

    void updatePauseMenuActivation() {
        if (hasPressedKeyboardKeyFlank(KEYBOARD_P_PRISM) || (isShowingPauseMenu && hasPressedStartFlank()))
        {
            if (!isShowingPauseMenu)
            {
                setMugenAnimationVisibility(pauseEntity, 1);
                pauseAllAnimations();
                isShowingPauseMenu = true;
            }
            else
            {
                setMugenAnimationVisibility(pauseEntity, 0);
                resumeAllAnimations();
                isShowingPauseMenu = false;
            }
        }
    }

    void pauseAllAnimations()
    {
        for (auto& h : mHumans)
        {
            pauseBlitzMugenAnimation(h.second.entityID);
        }
    }

    void resumeAllAnimations()
    {
        for (auto& h : mHumans)
        {
            unpauseBlitzMugenAnimation(h.second.entityID);
        }
    }

    // SPEEDRUN
    void loadSpeedrunTimer() {}
    void updateSpeedrunTimer() {}

    void removeAllThings() { 
        for (auto& h : mHumans)
        {
            setBlitzMugenAnimationVisibility(h.second.entityID, 0);
        }
        for (auto& e : mEnemies)
        {
            setBlitzMugenAnimationVisibility(e.second.entityID, 0);
            setBlitzMugenAnimationVisibility(e.second.earEntityID, 0);
        }
        setBlitzMugenAnimationVisibility(crosshairEntity, 0);
        setMugenTextVisibility(soulsLeftTextId, 0);
        setMugenTextVisibility(enemySoulsTakenTextId, 0);
    }
};

EXPORT_SCREEN_CLASS(GameScreen);

void resetGame()
{
    gGameScreenData.mLevel = 0;
    gGameScreenData.mGameTicks = 0;
}

std::string getSpeedRunString() {
    int totalSeconds = gGameScreenData.mGameTicks / 60;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    int milliseconds = (gGameScreenData.mGameTicks % 60) * 1000 / 60;
    return std::to_string(minutes) + "m " + std::to_string(seconds) + "s " + std::to_string(milliseconds) + "ms.";
}