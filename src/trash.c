#include "../include/trash.h"
#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

Trash trashes[MAX_TRASH];
int trashCount = 0;

// 物理常量
const float GRAVITY = 9.8f * 0.2f;  // 重力效果
const float FLOOR_FRICTION = 3;  // 摩擦力
const float BOUNCE_FACTOR = 0.3f;    // 弹跳
const float WINDOW_INFLUENCE = 0.05f; // 窗口移动影响

// 窗口晃动变量
float windowShakeX = 0.0f;
float windowShakeY = 0.0f;
float windowShakeIntensity = 0.0f;
float windowShakeDecay = 0.95f;


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
    
    trashes[trashCount] = (Trash){
        .position = {
            (float)GetRandomValue(50, (int)GetScreenWidth() - 50),
            (float)GetRandomValue((int)GetScreenHeight() * 0.3f, (int)GetScreenHeight() * 0.7f)
        },
        .velocity = {
            (float)GetRandomValue(-30, 30) / 100.0f,
            (float)GetRandomValue(50, 100) / 100.0f
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

void UpdateTrash(Vector2 windowAcceleration) {
    float screenWidth = (float)GetScreenWidth();
    float screenHeight = (float)GetScreenHeight();
    const float VELOCITY_DECAY = 0.98f;  // 添加速度衰减系数

    for (int i = 0; i < trashCount; i++) {
        if (!trashes[i].active) continue;
        
        // 使用垃圾的半径作为尺寸参考
        float radius = trashes[i].radius;
        
        // 每帧应用速度衰减
        trashes[i].velocity.x *= VELOCITY_DECAY;
        trashes[i].velocity.y *= VELOCITY_DECAY;
        
        // 减少窗口移动影响
        trashes[i].velocity.x += windowAcceleration.x * WINDOW_INFLUENCE;
        trashes[i].velocity.y += windowAcceleration.y * WINDOW_INFLUENCE;
        
        // 增强重力效果
        trashes[i].velocity.y += GRAVITY * GetFrameTime();
        
        // 限制最大速度
        trashes[i].velocity.x = Clamp(trashes[i].velocity.x, -5.0f, 5.0f);
        trashes[i].velocity.y = Clamp(trashes[i].velocity.y, -5.0f, 5.0f);
        
        // 更新位置
        trashes[i].position.x += trashes[i].velocity.x;
        trashes[i].position.y += trashes[i].velocity.y;
        
        // 边界碰撞检测
        const float BOUNDARY_MARGIN = 30.0f;

        // 底部碰撞处理 - 增加摩擦力
        if (trashes[i].position.y > screenHeight - radius) {
            trashes[i].position.y = screenHeight - radius;
            trashes[i].velocity.y *= -0.3f; // 减少反弹
            
            // 显著增加X方向摩擦力 (从0.9f改为0.3f)
            trashes[i].velocity.x *= 0.3f;
            
            // 当速度很小时直接停止
            if (fabs(trashes[i].velocity.x) < 0.1f) {
                trashes[i].velocity.x = 0;
            }
        }

        // ===== 新增：垃圾间碰撞检测 =====
        for (int i = 0; i < trashCount; i++) {
            if (!trashes[i].active) continue;
            
            for (int j = i + 1; j < trashCount; j++) {
                if (!trashes[j].active) continue;
                
                // 检测碰撞
                CollisionInfo info = GetCollisionInfo(trashes[i], trashes[j]);
                if (info.collided) {
                    // 解决碰撞
                    ResolveCollision(&trashes[i], &trashes[j], info);
                }
            }
        }
        
        // 左右边界
        if (trashes[i].position.x < radius + BOUNDARY_MARGIN) {
            trashes[i].position.x = radius + BOUNDARY_MARGIN;
            trashes[i].velocity.x *= -0.7f; // 反弹
        } else if (trashes[i].position.x > screenWidth - radius - BOUNDARY_MARGIN) {
            trashes[i].position.x = screenWidth - radius - BOUNDARY_MARGIN;
            trashes[i].velocity.x *= -0.7f; // 反弹
        }
        
        // 上下边界
        if (trashes[i].position.y < radius) {
            trashes[i].position.y = radius;
            trashes[i].velocity.y *= -0.7f; // 反弹
        }
        
        // 清理进度
        if (trashes[i].cleaning) {
            trashes[i].cleanProgress += GetFrameTime();
            if (trashes[i].cleanProgress >= 5.0f) {
                trashes[i].active = false;
            }
        }
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