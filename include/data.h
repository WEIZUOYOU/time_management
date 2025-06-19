#ifndef DATA_H
#define DATA_H

#include "raylib.h"

// 统计数据类型
typedef struct {
    int totalPomodoros;
    int cleanedTrash;
    int generatedTrash;
    int interruptions;
    int streakDays;
    int longSessions;
} Statistics;

// 统计界面函数
void InitStatistics(Statistics *stats);
void DrawStatisticsScreen(Statistics *stats, Font font, bool isDarkTheme, float screenWidth, float screenHeight);
void SaveStatistics(const Statistics *stats, const char *filename);
void LoadStatistics(Statistics *stats, const char *filename);

#endif // DATA_H