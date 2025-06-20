#ifndef TRASH_H
#define TRASH_H

#include "raylib.h"

#define MAX_TRASH 20

typedef struct {
    Vector2 position;
    Vector2 velocity;
    Vector2 acceleration;
    float scale;
    float radius;       // 添加半径属性
    bool active;
    bool cleaning;
    float cleanProgress;
    int trashType;
    int pomodoroDuration;
    float bounceFactor;
    float friction;
} Trash;

// 碰撞信息结构体
typedef struct CollisionInfo {
    bool collided;
    Vector2 normal; // 碰撞法线（从a指向b）
    float depth;    // 穿透深度
} CollisionInfo;

// 函数声明
bool CheckCollisionTrash(Trash a, Trash b);
CollisionInfo GetCollisionInfo(Trash a, Trash b);
void ResolveCollision(Trash *a, Trash *b, CollisionInfo info);
void InitTrashSystem(void);
void GenerateTrash(int duration);
void UpdateTrash();
void DrawTrash(void);
void CleanTrash(int index);
void ResetTrashSystem(void);
void SaveTrashSystem(const char* filename);  // 新增：保存垃圾系统状态
void LoadTrashSystem(const char* filename);  // 新增：加载垃圾系统状态
void UpdateWindowAcceleration(Vector2 currentPos);
bool IsAllTrashTypeCleaned(void);

extern Trash trashes[MAX_TRASH];
extern int trashCount;

#endif // TRASH_H