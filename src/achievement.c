#include "achievement.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

void InitAchievementManager(AchievementManager *manager) {
    // 初始化统计变量
    manager->totalPomodoros = 0;
    manager->cleanedTrashCount = 0;
    manager->generatedTrashCount = 0;
    manager->interruptionsCount = 0;
    
    // 初始化正面成就列表
    const char *positiveNames[ACH_COUNT] = {
        "初尝专注", "小有所成", "渐入佳境", "专注大师",
        "垃圾清理工", "环境卫士", "清洁专家",
        "持久专注", "专注马拉松", "七日坚持",
        "无懈可击", "自定义挑战", "清洁大师",
        "完美一天", "晨型人", "夜猫子"
    };
    
    const char *positiveDescriptions[ACH_COUNT] = {
        "完成你的第一个番茄钟", "完成5个番茄钟", "完成10个番茄钟", "完成20个番茄钟",
        "清理第一个垃圾", "清理5个垃圾", "清理10个垃圾",
        "完成一个45分钟的番茄钟", "连续完成3个番茄钟", "连续7天每天完成至少一个番茄钟",
        "完成一个番茄钟没有任何中断", "完成一个自定义时长的番茄钟", "清理所有类型的垃圾",
        "一天内完成5个番茄钟", "在早上6-8点完成一个番茄钟", "在晚上10-12点完成一个番茄钟"
    };
    
    for (int i = 0; i < ACH_COUNT; i++) {
        manager->achievements[i].id = i;
        strncpy(manager->achievements[i].name, positiveNames[i], sizeof(manager->achievements[i].name));
        strncpy(manager->achievements[i].description, positiveDescriptions[i], sizeof(manager->achievements[i].description));
        manager->achievements[i].unlocked = false;
        manager->achievements[i].unlockTime = 0;
    }
    
    // 初始化负面成就列表
    const char *negativeNames[NEG_COUNT] = {
        "首次分心", "频频分心", "分心成瘾",
        "垃圾制造者", "环境破坏者", "垃圾大王",
        "中断连胜"
    };
    
    const char *negativeDescriptions[NEG_COUNT] = {
        "中断专注1次", "中断专注5次", "中断专注10次",
        "产生1个垃圾", "产生5个垃圾", "产生10个垃圾",
        "中断连续专注天数"
    };
    
    for (int i = 0; i < NEG_COUNT; i++) {
        manager->negativeAchievements[i].id = i;
        strncpy(manager->negativeAchievements[i].name, negativeNames[i], sizeof(manager->negativeAchievements[i].name));
        strncpy(manager->negativeAchievements[i].description, negativeDescriptions[i], sizeof(manager->negativeAchievements[i].description));
        manager->negativeAchievements[i].unlocked = false;
        manager->negativeAchievements[i].unlockTime = 0;
    }
    
    // 初始化统计信息
    manager->totalPomodoros = 0;
    manager->cleanedTrashCount = 0;
    manager->generatedTrashCount = 0;
    manager->interruptionsCount = 0;
    manager->longSessionCount = 0;
    manager->currentStreak = 0;
    manager->lastPomodoroDate = 0;
    manager->streakDays = 0;
    manager->consecutivePomodoros = 0;
}

void UnlockAchievement(AchievementManager *manager, AchievementID id) {
    if (id < 0 || id >= ACH_COUNT) return;
    
    if (!manager->achievements[id].unlocked) {
        manager->achievements[id].unlocked = true;
        manager->achievements[id].unlockTime = time(NULL);
        // 这里可以添加成就解锁时的特殊效果
    }
}

void UnlockNegativeAchievement(AchievementManager *manager, NegativeAchievementID id) {
    if (id < 0 || id >= NEG_COUNT) return;
    
    if (!manager->negativeAchievements[id].unlocked) {
        manager->negativeAchievements[id].unlocked = true;
        manager->negativeAchievements[id].unlockTime = time(NULL);
        // 这里可以添加负面成就解锁时的特殊效果
    }
}

void CheckAchievements(AchievementManager *manager, bool pomodoroCompleted, bool trashCleaned, int duration) {
    // 只有在完成番茄钟时才更新统计
    if (pomodoroCompleted) {
        manager->totalPomodoros++;
        
        // 检查成就解锁条件
        if (manager->totalPomodoros == 1) {
            UnlockAchievement(manager, ACH_FIRST_POMODORO);
        }
    }
    time_t now = time(NULL);
    struct tm *nowTm = localtime(&now);
    time_t today = mktime(nowTm); // 今天的0点时间
    
    // 连续天数统计
    if (pomodoroCompleted) {
        if (manager->lastPomodoroDate == 0) {
            manager->lastPomodoroDate = today;
            manager->currentStreak = 1;
        } else {
            // 计算天数差时使用本地时间
            double diff = difftime(today, manager->lastPomodoroDate) / (60 * 60 * 24);

            if (diff == 1) {
                manager->currentStreak++;
            } else if (diff > 1) {
                if (manager->currentStreak >= 3) {
                    UnlockNegativeAchievement(manager, NEG_BROKEN_STREAK);
                }
                manager->currentStreak = 1;
            }
        }
        manager->lastPomodoroDate = today;
        manager->streakDays = manager->currentStreak;  // 更新连续天数
    }
    
    if (pomodoroCompleted) {
        manager->totalPomodoros++;
        
        // 检查长时间番茄钟
        if (duration == 45 * 60) {
            manager->longSessionCount++;
        }
        
        // 检查时间相关成就
        if (nowTm->tm_hour >= 6 && nowTm->tm_hour < 8) {
            UnlockAchievement(manager, ACH_EARLY_BIRD);
        } else if (nowTm->tm_hour >= 22 || nowTm->tm_hour < 1) {
            UnlockAchievement(manager, ACH_NIGHT_OWL);
        }
    }
    
    // ================ 检查正面成就解锁条件 ================
    if (manager->totalPomodoros >= 1 && !manager->achievements[ACH_FIRST_POMODORO].unlocked) {
        UnlockAchievement(manager, ACH_FIRST_POMODORO);
    }
    if (manager->totalPomodoros >= 5 && !manager->achievements[ACH_5_POMODOROS].unlocked) {
        UnlockAchievement(manager, ACH_5_POMODOROS);
    }
    if (manager->totalPomodoros >= 10 && !manager->achievements[ACH_10_POMODOROS].unlocked) {
        UnlockAchievement(manager, ACH_10_POMODOROS);
    }
    if (manager->totalPomodoros >= 20 && !manager->achievements[ACH_20_POMODOROS].unlocked) {
        UnlockAchievement(manager, ACH_20_POMODOROS);
    }
    
    if (manager->cleanedTrashCount >= 1 && !manager->achievements[ACH_CLEAN_1_TRASH].unlocked) {
        UnlockAchievement(manager, ACH_CLEAN_1_TRASH);
    }
    if (manager->cleanedTrashCount >= 5 && !manager->achievements[ACH_CLEAN_5_TRASHES].unlocked) {
        UnlockAchievement(manager, ACH_CLEAN_5_TRASHES);
    }
    if (manager->cleanedTrashCount >= 10 && !manager->achievements[ACH_CLEAN_10_TRASHES].unlocked) {
        UnlockAchievement(manager, ACH_CLEAN_10_TRASHES);
    }
    
    if (manager->longSessionCount >= 1 && !manager->achievements[ACH_LONG_SESSION].unlocked) {
        UnlockAchievement(manager, ACH_LONG_SESSION);
    }
    
    if (manager->currentStreak >= 3 && !manager->achievements[ACH_MARATHON].unlocked) {
        UnlockAchievement(manager, ACH_MARATHON);
    }
    
    if (manager->streakDays >= 7 && !manager->achievements[ACH_7_DAY_STREAK].unlocked) {
        UnlockAchievement(manager, ACH_7_DAY_STREAK);
    }
    
    // ================ 检查负面成就解锁条件 ================
    if (manager->interruptionsCount >= 1 && !manager->negativeAchievements[NEG_1_INTERRUPTION].unlocked) {
        UnlockNegativeAchievement(manager, NEG_1_INTERRUPTION);
    }
    if (manager->interruptionsCount >= 5 && !manager->negativeAchievements[NEG_5_INTERRUPTIONS].unlocked) {
        UnlockNegativeAchievement(manager, NEG_5_INTERRUPTIONS);
    }
    if (manager->interruptionsCount >= 10 && !manager->negativeAchievements[NEG_10_INTERRUPTIONS].unlocked) {
        UnlockNegativeAchievement(manager, NEG_10_INTERRUPTIONS);
    }
    
    if (manager->generatedTrashCount >= 1 && !manager->negativeAchievements[NEG_1_TRASH].unlocked) {
        UnlockNegativeAchievement(manager, NEG_1_TRASH);
    }
    if (manager->generatedTrashCount >= 5 && !manager->negativeAchievements[NEG_5_TRASHES].unlocked) {
        UnlockNegativeAchievement(manager, NEG_5_TRASHES);
    }
    if (manager->generatedTrashCount >= 10 && !manager->negativeAchievements[NEG_10_TRASHES].unlocked) {
        UnlockNegativeAchievement(manager, NEG_10_TRASHES);
    }

    // 添加自定义成就检查
    if (duration != 25 && duration != 45 && duration > 0 && 
        !manager->achievements[ACH_CUSTOM_SESSION].unlocked) {
        UnlockAchievement(manager, ACH_CUSTOM_SESSION);
    }
}

void SaveAchievements(const AchievementManager *manager, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (file) {
        fwrite(manager, sizeof(AchievementManager), 1, file);
        fclose(file);
    }
}

void LoadAchievements(AchievementManager *manager, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file) {
        fread(manager, sizeof(AchievementManager), 1, file);
        // 兼容性检查 - 如果加载的数据结构不匹配，重新初始化
        if (manager->achievements[0].id != 0 || manager->negativeAchievements[0].id != 0) {
            // 检查版本兼容性，这里可以添加版本检查逻辑
            InitAchievementManager(manager);
        }
        fclose(file);
    } else {
        InitAchievementManager(manager);
    }
}

void ResetAchievements(AchievementManager *manager) {
    InitAchievementManager(manager);
}