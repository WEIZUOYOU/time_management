#ifndef ACHIEVEMENT_H
#define ACHIEVEMENT_H

#include <stdbool.h>
#include <time.h>

// 正面成就ID
typedef enum {
    ACH_FIRST_POMODORO,        // 0: 初尝专注
    ACH_5_POMODOROS,            // 1: 小有所成
    ACH_10_POMODOROS,           // 2: 渐入佳境
    ACH_20_POMODOROS,           // 3: 专注大师
    ACH_CLEAN_1_TRASH,          // 4: 垃圾清理工
    ACH_CLEAN_5_TRASHES,        // 5: 环境卫士
    ACH_CLEAN_10_TRASHES,       // 6: 清洁专家
    ACH_LONG_SESSION,           // 7: 持久专注
    ACH_MARATHON,               // 8: 专注马拉松
    ACH_7_DAY_STREAK,           // 9: 七日坚持
    ACH_FLAWLESS,               // 10: 无懈可击
    ACH_CUSTOM_SESSION,         // 11: 自定义挑战
    ACH_CLEAN_MASTER,           // 12: 清洁大师 - 新增
    ACH_PERFECT_DAY,            // 13: 完美一天 - 新增
    ACH_EARLY_BIRD,             // 14: 晨型人
    ACH_NIGHT_OWL,              // 15: 夜猫子
    ACH_COUNT                   // 16: 成就总数
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
    bool interruptionOccurred;  // 标记当前番茄钟是否被中断
    int consecutivePomodoros;   // 连续完成的番茄钟数
    int dailyPomodoros;         // 今日完成的番茄钟数
    time_t lastPomodoroDay;     // 上次完成番茄钟的日期
} AchievementManager;

// 函数声明
void InitAchievementManager(AchievementManager *manager);
void UnlockAchievement(AchievementManager *manager, AchievementID id);
void UnlockNegativeAchievement(AchievementManager *manager, NegativeAchievementID id);
void CheckAchievements(AchievementManager *manager, bool pomodoroCompleted, bool trashCleaned, int duration);
void SaveAchievements(const AchievementManager *manager, const char *filename);
void LoadAchievements(AchievementManager *manager, const char *filename);
void ResetAchievements(AchievementManager *manager);
bool IsAllTrashTypeCleaned(void);

#endif // ACHIEVEMENT_H