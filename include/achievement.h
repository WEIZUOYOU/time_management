#ifndef ACHIEVEMENT_H
#define ACHIEVEMENT_H

#include <stdbool.h>
#include <time.h>

// 正面成就ID
typedef enum {
    ACH_FIRST_POMODORO,          // 完成第一个番茄钟
    ACH_5_POMODOROS,             // 完成5个番茄钟
    ACH_10_POMODOROS,            // 完成10个番茄钟
    ACH_20_POMODOROS,            // 完成20个番茄钟
    ACH_CLEAN_1_TRASH,           // 清理1个垃圾
    ACH_CLEAN_5_TRASHES,         // 清理5个垃圾
    ACH_CLEAN_10_TRASHES,        // 清理10个垃圾
    ACH_LONG_SESSION,            // 完成一个45分钟的番茄钟
    ACH_MARATHON,                // 连续完成3个番茄钟（中间没有中断）
    ACH_7_DAY_STREAK,            // 连续7天每天至少完成一个番茄钟
    ACH_NO_INTERRUPTIONS,        // 完成一个番茄钟没有任何中断
    ACH_CUSTOM_SESSION,          // 完成一个自定义时长的番茄钟
    ACH_MASTER_CLEANER,          // 清理所有类型的垃圾
    ACH_PERFECT_DAY,             // 一天内完成5个番茄钟
    ACH_EARLY_BIRD,              // 在早上6-8点完成番茄钟
    ACH_NIGHT_OWL,               // 在晚上10-12点完成番茄钟
    ACH_COUNT                    // 成就总数
} AchievementID;

// 负面成就ID
typedef enum {
    NEG_1_INTERRUPTION,      // 中断1次
    NEG_5_INTERRUPTIONS,     // 中断5次
    NEG_10_INTERRUPTIONS,    // 中断10次
    NEG_1_TRASH,             // 产生1个垃圾
    NEG_5_TRASHES,           // 产生5个垃圾
    NEG_10_TRASHES,          // 产生10个垃圾
    NEG_BROKEN_STREAK,       // 中断连续天数
    NEG_COUNT                // 负面成就总数
} NegativeAchievementID;

// 成就结构体
typedef struct {
    int id;
    char name[50];
    char description[100];
    bool unlocked;
    time_t unlockTime; // 解锁时间
} Achievement;

// 成就管理器
typedef struct {
    // 正面成就
    Achievement achievements[ACH_COUNT];
    // 负面成就
    Achievement negativeAchievements[NEG_COUNT];
    
    // 统计信息
    int totalPomodoros;          // 总番茄钟计数
    int cleanedTrashCount;       // 清理的垃圾总数
    int generatedTrashCount;     // 产生的垃圾总数
    int interruptionsCount;      // 中断次数
    int longSessionCount;        // 45分钟番茄钟计数
    int currentStreak;           // 当前连续番茄钟计数
    time_t lastPomodoroDate;     // 上一个番茄钟完成的日期
    int streakDays;              // 当前连续天数
    int consecutivePomodoros;
    bool interruptionOccurred;  // 标记当前番茄钟是否被中断
} AchievementManager;

// 函数声明
void InitAchievementManager(AchievementManager *manager);
void UnlockAchievement(AchievementManager *manager, AchievementID id);
void UnlockNegativeAchievement(AchievementManager *manager, NegativeAchievementID id);
void CheckAchievements(AchievementManager *manager, bool pomodoroCompleted, bool trashCleaned, int duration);
void SaveAchievements(const AchievementManager *manager, const char *filename);
void LoadAchievements(AchievementManager *manager, const char *filename);
void ResetAchievements(AchievementManager *manager);

#endif // ACHIEVEMENT_H