#include "../include/trash.h"
#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

Trash trashes[MAX_TRASH];
int trashCount = 0;

// 物理常量 - 增加重力效果
const float GRAVITY = 9.8f * 8.0f;   // 重力效果
const float FLOOR_FRICTION = 0.8f;   // 摩擦力
const float BOUNCE_FACTOR = 0.85f;    // 弹跳系数
const float WINDOW_INFLUENCE = 1.2f; // 窗口移动影响

// 窗口晃动变量
float windowShakeX = 0.0f;
float windowShakeY = 0.0f;
float windowShakeIntensity = 0.0f;
float windowShakeDecay = 0.95f;

// 全局变量存储窗口移动加速度
Vector2 windowAcceleration = {0};
float windowVelocityX = 0;
float windowVelocityY = 0;
Vector2 lastWindowPos = {0};

static float elapsedTime = 0.0f;
static float physicsTimeStep = 1.0f / 60.0f; // 60Hz物理更新

// 检查是否清理了所有类型的垃圾
bool IsAllTrashTypeCleaned(void) {
    // 假设有4种垃圾类型
    #define MAX_TRASH_TYPES 4
    
    // 跟踪每种类型是否被清理过
    bool typeCleaned[MAX_TRASH_TYPES] = {false};
    
    // 检查是否有任何未清理的垃圾
    bool allCleaned = true;
    
    // 遍历所有垃圾
    for (int i = 0; i < trashCount; i++) {
        if (trashes[i].active) {
            // 如果垃圾未被清理，则不是所有垃圾都被清理
            if (!trashes[i].cleaning) {
                allCleaned = false;
            }
            
            // 标记该类型已被遇到
            if (trashes[i].trashType < MAX_TRASH_TYPES) {
                typeCleaned[trashes[i].trashType] = true;
            }
        }
    }
    
    // 检查是否所有类型都存在过
    for (int i = 0; i < MAX_TRASH_TYPES; i++) {
        if (!typeCleaned[i]) {
            allCleaned = false;
            break;
        }
    }
    
    return allCleaned;
}

void UpdateTrash(void) {
    // 累计时间
    static float elapsedTime = 0.0f;
    const float physicsTimeStep = 1.0f / 60.0f; // 60Hz物理更新
    elapsedTime += GetFrameTime();
    
    // 确保固定时间步长更新
    while (elapsedTime >= physicsTimeStep) {
        // 应用加速度（包括重力）
        for (int i = 0; i < trashCount; i++) {
            if (trashes[i].active && !trashes[i].cleaning) {
                // 应用重力加速度 - 增加效果
                trashes[i].velocity.y += GRAVITY * physicsTimeStep;
                
                // 应用窗口加速度影响 - 增加影响
                trashes[i].velocity.x += windowAcceleration.x * WINDOW_INFLUENCE * physicsTimeStep;
                trashes[i].velocity.y += windowAcceleration.y * WINDOW_INFLUENCE * physicsTimeStep;
                
                // 减少速度衰减（让垃圾保持更快的速度）
                trashes[i].velocity.x *= 0.99f;  // 从0.98f改为0.99f
                trashes[i].velocity.y *= 0.99f;  // 从0.98f改为0.99f
                
                // 更新位置
                trashes[i].position.x += trashes[i].velocity.x * physicsTimeStep;
                trashes[i].position.y += trashes[i].velocity.y * physicsTimeStep;
            }
        }
        
        // 边界碰撞检测
        float screenWidth = (float)GetScreenWidth();
        float screenHeight = (float)GetScreenHeight();
        
        for (int i = 0; i < trashCount; i++) {
            if (!trashes[i].active || trashes[i].cleaning) continue;
            
            // 左右边界 - 增加反弹力
            if (trashes[i].position.x < trashes[i].radius) {
                trashes[i].position.x = trashes[i].radius;
                trashes[i].velocity.x = fabsf(trashes[i].velocity.x) * trashes[i].bounceFactor * 1.1f; // 增加10%反弹力
            } else if (trashes[i].position.x > screenWidth - trashes[i].radius) {
                trashes[i].position.x = screenWidth - trashes[i].radius;
                trashes[i].velocity.x = -fabsf(trashes[i].velocity.x) * trashes[i].bounceFactor * 1.1f; // 增加10%反弹力
            }
            
            // 上下边界 - 增加反弹力
            if (trashes[i].position.y < trashes[i].radius) {
                trashes[i].position.y = trashes[i].radius;
                trashes[i].velocity.y = fabsf(trashes[i].velocity.y) * trashes[i].bounceFactor * 1.1f; // 增加10%反弹力
            } else if (trashes[i].position.y > screenHeight - trashes[i].radius) {
                trashes[i].position.y = screenHeight - trashes[i].radius;
                trashes[i].velocity.y = -fabsf(trashes[i].velocity.y) * trashes[i].bounceFactor * 1.1f; // 增加10%反弹力
                
                // 减少地面摩擦力
                trashes[i].velocity.x *= (1.0f - trashes[i].friction * physicsTimeStep);
            }
        }
        
        // 垃圾之间的碰撞检测和解决
        for (int i = 0; i < trashCount; i++) {
            if (!trashes[i].active || trashes[i].cleaning) continue;
            for (int j = i + 1; j < trashCount; j++) {
                if (!trashes[j].active || trashes[j].cleaning) continue;
                
                // 检测碰撞
                CollisionInfo info = GetCollisionInfo(trashes[i], trashes[j]);
                if (info.collided) {
                    ResolveCollision(&trashes[i], &trashes[j], info);
                }
            }
        }
        
        elapsedTime -= physicsTimeStep;
    }
}

void UpdateWindowAcceleration(Vector2 currentPos) {
    Vector2 delta = {
        currentPos.x - lastWindowPos.x,
        currentPos.y - lastWindowPos.y
    };
    
    // 减少衰减因子，让窗口移动影响更大
    windowVelocityX = windowVelocityX * 0.3f + delta.x * 0.7f;  // 从0.5f改为0.3f/0.7f
    windowVelocityY = windowVelocityY * 0.3f + delta.y * 0.7f;  // 从0.5f改为0.3f/0.7f
    
    // 增加加速度影响 (从3.0f提高到5.0f)
    windowAcceleration.x = Clamp(windowVelocityX * 5.0f, -800.0f, 800.0f); // 从500提高到800
    windowAcceleration.y = Clamp(windowVelocityY * 5.0f, -800.0f, 800.0f); // 从500提高到800
    
    lastWindowPos = currentPos;
}

void InitTrashSystem(void) {
    for (int i = 0; i < MAX_TRASH; i++) {
        trashes[i].active = false;
        trashes[i].cleaning = false;
        trashes[i].cleanProgress = 0.0f;
        trashes[i].pomodoroDuration = 0;
        trashes[i].velocity = (Vector2){0.0f, 0.0f};
        trashes[i].acceleration = (Vector2){0.0f, GRAVITY};
        trashes[i].bounceFactor = BOUNCE_FACTOR;
        trashes[i].friction = FLOOR_FRICTION;
    }
    trashCount = 0;
}

void GenerateTrash(int duration) {
    if (trashCount >= MAX_TRASH) return;
    
    // 根据时长决定垃圾类型
    int trashType = duration / 15;  // 简单分类
    if (trashType > 3) trashType = 3;
    
    // 创建更快的初始速度 (从150-300提高到200-500)
    float initialSpeed = GetRandomValue(200, 500) / 100.0f; 
    float angle = DEG2RAD * GetRandomValue(0, 360); // 随机角度
    
    trashes[trashCount] = (Trash){
        .position = {
            (float)GetRandomValue(50, (int)GetScreenWidth() - 50),
            (float)GetRandomValue((int)GetScreenHeight() * 0.3f, (int)GetScreenHeight() * 0.7f)
        },
        .velocity = {
            cosf(angle) * initialSpeed, // X方向速度
            sinf(angle) * initialSpeed  // Y方向速度
        },
        .scale = GetRandomValue(5, 10) / 10.0f,
        .active = true,
        .cleaning = false,
        .cleanProgress = 0.0f,
        .trashType = trashType,
        .pomodoroDuration = duration,
        .radius = 30.0f * GetRandomValue(5, 10) / 10.0f,
        .bounceFactor = BOUNCE_FACTOR * (0.8f + (float)GetRandomValue(0, 40) / 100.0f),
        .friction = FLOOR_FRICTION * (0.9f + (float)GetRandomValue(0, 20) / 100.0f)
    };
    
    trashCount++;
}

void CleanTrash(int index) {
    if (index >= 0 && index < trashCount) {
        trashes[index].cleaning = true;
        trashes[index].cleanProgress = 0.0f;  // 重置进度
    }
}

void DrawTrash(void) {
    Font defaultFont = GetFontDefault();
    
    for (int i = 0; i < trashCount; i++) {
        if (trashes[i].active) {
            float baseSize = 60.0f * trashes[i].scale;
            Rectangle rect = {
                trashes[i].position.x - baseSize/2 + windowShakeX,
                trashes[i].position.y - baseSize/2 + windowShakeY,
                baseSize,
                baseSize
            };
            
            // 根据关联时长显示不同颜色
            Color trashColor;
            if (trashes[i].pomodoroDuration == 25) {
                trashColor = BROWN;
            } else if (trashes[i].pomodoroDuration == 45) {
                trashColor = DARKBROWN;
            } else {
                trashColor = (Color){100, 100, 100, 255};
            }
            
            DrawRectangleRec(rect, trashColor);
            
            // 显示关联时长
            char durationText[10];
            sprintf(durationText, "%d", trashes[i].pomodoroDuration);
            
            float fontSize = baseSize * 0.4f;
            if (fontSize < 15) fontSize = 15;
            if (fontSize > 30) fontSize = 30;
            
            Vector2 textSize = MeasureTextEx(defaultFont, durationText, fontSize, 1);
            DrawTextEx(defaultFont, durationText, 
                     (Vector2){rect.x + rect.width/2.0f - textSize.x/2.0f,
                              rect.y + rect.height/2.0f - textSize.y/2.0f},
                     fontSize, 1, WHITE);
            
            // 清理进度条
            if (trashes[i].cleaning) {
                float progress = trashes[i].cleanProgress / 5.0f;
                DrawRectangle(rect.x, rect.y - 20.0f, rect.width, 10.0f, LIGHTGRAY);
                DrawRectangle(rect.x, rect.y - 20.0f, rect.width * progress, 10.0f, GREEN);
            }
        }
    }
}

void ResetTrashSystem(void) {
    trashCount = 0;
    for (int i = 0; i < MAX_TRASH; i++) {
        trashes[i].active = false;
    }
}

void SaveTrashSystem(const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        TraceLog(LOG_WARNING, "无法打开垃圾状态文件: %s", filename);
        return;
    }
    
    fwrite(&trashCount, sizeof(int), 1, file);
    for (int i = 0; i < trashCount; i++) {
        fwrite(&trashes[i], sizeof(Trash), 1, file);
    }
    
    fclose(file);
    TraceLog(LOG_INFO, "垃圾状态已保存到: %s", filename);
}

void LoadTrashSystem(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        TraceLog(LOG_WARNING, "未找到垃圾状态文件: %s", filename);
        return;
    }
    
    InitTrashSystem();
    
    fread(&trashCount, sizeof(int), 1, file);
    for (int i = 0; i < trashCount; i++) {
        fread(&trashes[i], sizeof(Trash), 1, file);
    }
    
    fclose(file);
    TraceLog(LOG_INFO, "垃圾状态已从 %s 加载", filename);
}

// 检查两个垃圾是否碰撞
bool CheckCollisionTrash(Trash a, Trash b) {
    float distance = Vector2Distance(a.position, b.position);
    float minDistance = (a.radius + b.radius);
    return distance < minDistance;
}

// 获取碰撞详细信息
CollisionInfo GetCollisionInfo(Trash a, Trash b) {
    CollisionInfo info = {0};
    
    Vector2 delta = (Vector2){b.position.x - a.position.x, b.position.y - a.position.y};
    float distance = Vector2Length(delta);
    float minDistance = a.radius + b.radius;
    
    if (distance < minDistance) {
        info.collided = true;
        info.depth = minDistance - distance;
        
        if (distance > 0) {
            info.normal = (Vector2){delta.x / distance, delta.y / distance};
        } else {
            // 如果位置完全相同，使用默认法线
            info.normal = (Vector2){1, 0};
        }
    }
    
    return info;
}

// 解决碰撞
void ResolveCollision(Trash *a, Trash *b, CollisionInfo info) {
    if (!info.collided) return;
    
    // 计算相对速度
    Vector2 relativeVelocity = (Vector2){
        b->velocity.x - a->velocity.x,
        b->velocity.y - a->velocity.y
    };
    
    // 计算碰撞速度
    float velocityAlongNormal = Vector2DotProduct(relativeVelocity, info.normal);
    
    // 如果物体正在分离，不处理
    if (velocityAlongNormal > 0) return;
    
    // 计算冲量大小
    float restitution = 0.8f; // 弹性系数
    float impulseScalar = -(1.0f + restitution) * velocityAlongNormal;
    
    // 应用冲量
    float invMassA = 1.0f / a->radius; // 质量与半径成反比
    float invMassB = 1.0f / b->radius;
    impulseScalar /= (invMassA + invMassB);
    
    Vector2 impulse = (Vector2){
        impulseScalar * info.normal.x,
        impulseScalar * info.normal.y
    };
    
    // 应用冲量到速度
    a->velocity.x -= impulse.x * invMassA;
    a->velocity.y -= impulse.y * invMassA;
    b->velocity.x += impulse.x * invMassB;
    b->velocity.y += impulse.y * invMassB;
    
    // 位置修正 - 防止物体重叠
    float percent = 0.8f; // 穿透修正百分比
    float slop = 0.01f;   // 允许穿透量
    float correction = fmaxf(info.depth - slop, 0.0f) / (invMassA + invMassB) * percent;
    
    Vector2 correctionVec = (Vector2){
        correction * info.normal.x,
        correction * info.normal.y
    };
    
    a->position.x -= correctionVec.x * invMassA;
    a->position.y -= correctionVec.y * invMassA;
    b->position.x += correctionVec.x * invMassB;
    b->position.y += correctionVec.y * invMassB;
}