#include "data.h"
#include <stdio.h>

void InitStatistics(Statistics *stats) {
    stats->totalPomodoros = 0;
    stats->cleanedTrash = 0;
    stats->generatedTrash = 0;
    stats->interruptions = 0;
    stats->streakDays = 0;
    stats->longSessions = 0;
}

void DrawStatisticsScreen(Statistics *stats, Font font, bool isDarkTheme, float screenWidth, float screenHeight) {
    // 设置背景色
    if (isDarkTheme) {
        ClearBackground((Color){30, 30, 40, 255});
    } else {
        ClearBackground(RAYWHITE);
    }
    
    // 定义主题颜色
    Color titleColor = isDarkTheme ? GOLD : DARKBLUE;
    Color textColor = isDarkTheme ? LIGHTGRAY : DARKGRAY;
    Color panelBg = isDarkTheme ? (Color){40, 40, 50, 255} : (Color){240, 240, 240, 255};
    Color panelBorder = isDarkTheme ? (Color){60, 60, 70, 255} : (Color){200, 200, 200, 255};
    
    // 标题
    const char* title = "数据统计";
    Vector2 titleSize = MeasureTextEx(font, title, 60, 2);
    DrawTextEx(font, title, 
             (Vector2){screenWidth/2.0f - titleSize.x/2.0f, 40.0f}, 
             60, 2, titleColor);
    
    // 统计面板
    Rectangle panel = {
        screenWidth/2.0f - 300.0f,
        120.0f,
        600.0f,
        screenHeight - 240.0f
    };
    DrawRectangleRec(panel, panelBg);
    DrawRectangleLinesEx(panel, 2, panelBorder);
    
    // 统计数据
    int yPos = panel.y + 40;
    int lineHeight = 50;
    
    const char *statsLabels[] = {
        "总番茄钟数:",
        "清理垃圾数:",
        "产生垃圾数:",
        "中断次数:",
        "最长连续天数:",
        "长时间专注次数:"
    };
    
    char statsValues[6][20];
    sprintf(statsValues[0], "%d", stats->totalPomodoros);
    sprintf(statsValues[1], "%d", stats->cleanedTrash);
    sprintf(statsValues[2], "%d", stats->generatedTrash);
    sprintf(statsValues[3], "%d", stats->interruptions);
    sprintf(statsValues[4], "%d", stats->streakDays);
    sprintf(statsValues[5], "%d", stats->longSessions);
    
    for (int i = 0; i < 6; i++) {
        DrawTextEx(font, statsLabels[i], 
                 (Vector2){panel.x + 50, (float)yPos}, 
                 36, 1, textColor);
        DrawTextEx(font, statsValues[i], 
                 (Vector2){panel.x + panel.width - 150, (float)yPos}, 
                 36, 1, titleColor);
        yPos += lineHeight;
    }
    
    // === 图表区域 ===
    const int chartHeight = 200;
    const int chartWidth = 500;
    int chartX = panel.x + (panel.width - chartWidth) / 2;
    int chartY = yPos + 40;
    
    // 图表标题
    DrawTextEx(font, "番茄钟分布统计", 
             (Vector2){(float)chartX, (float)(chartY - 30)}, 
             28, 1, textColor);
    
    // 图表背景
    DrawRectangle(chartX, chartY, chartWidth, chartHeight, 
                isDarkTheme ? (Color){50, 50, 60, 255} : (Color){220, 220, 220, 255});
    
    // 计算最大值用于比例缩放
    int maxValue = stats->pomodoros25;
    if (stats->pomodoros45 > maxValue) maxValue = stats->pomodoros45;
    if (stats->pomodorosCustom > maxValue) maxValue = stats->pomodorosCustom;
    if (maxValue == 0) maxValue = 1; // 防止除以零

    // 简单条形图
    int barWidth = 80;
    int barSpacing = 20;
    int startX = chartX + (chartWidth - (3 * barWidth + 2 * barSpacing)) / 2;
    
    // 25分钟番茄钟
    int barHeight25 = (int)((float)stats->pomodoros25 / maxValue * chartHeight);
    DrawRectangle(startX, chartY + chartHeight - barHeight25, barWidth, barHeight25, 
                isDarkTheme ? GOLD : SKYBLUE);
    DrawTextEx(font, "25分钟", 
             (Vector2){(float)(startX + 10), (float)(chartY + chartHeight + 10)}, 
             20, 1, textColor);
    
    // 45分钟番茄钟
    int barHeight45 = (int)((float)stats->pomodoros45 / maxValue * chartHeight);
    DrawRectangle(startX + barWidth + barSpacing, chartY + chartHeight - barHeight45, barWidth, barHeight45, 
                isDarkTheme ? GOLD : SKYBLUE);
    DrawTextEx(font, "45分钟", 
             (Vector2){(float)(startX + barWidth + barSpacing + 10), (float)(chartY + chartHeight + 10)}, 
             20, 1, textColor);
    
    // 自定义番茄钟
    int barHeightCustom = (int)((float)stats->pomodorosCustom / maxValue * chartHeight);
    DrawRectangle(startX + 2 * (barWidth + barSpacing), chartY + chartHeight - barHeightCustom, barWidth, barHeightCustom, 
                isDarkTheme ? GOLD : SKYBLUE);
    DrawTextEx(font, "自定义", 
             (Vector2){(float)(startX + 2 * (barWidth + barSpacing) + 10), (float)(chartY + chartHeight + 10)}, 
             20, 1, textColor);
    
    // 返回按钮
    Rectangle backButton = {
        screenWidth / 2.0f - 75.0f,
        screenHeight - 70.0f,
        150.0f,
        50.0f
    };
    bool hoverBack = CheckCollisionPointRec(GetMousePosition(), backButton);
    DrawRectangleRec(backButton, hoverBack ? 
                   (isDarkTheme ? (Color){60, 60, 70, 255} : (Color){240, 240, 240, 255}) : 
                   (isDarkTheme ? (Color){40, 40, 50, 255} : RAYWHITE));
    DrawRectangleLinesEx(backButton, 1, isDarkTheme ? (Color){100, 100, 100, 255} : (Color){200, 200, 200, 255});
    
    const char* backText = "返回";
    Vector2 backTextSize = MeasureTextEx(font, backText, 24, 1);
    DrawTextEx(font, backText, 
             (Vector2){backButton.x + backButton.width/2 - backTextSize.x/2, 
                      backButton.y + backButton.height/2 - backTextSize.y/2},
             24, 1, textColor);
}

void SaveStatistics(const Statistics *stats, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (file) {
        fwrite(stats, sizeof(Statistics), 1, file);
        fclose(file);
    }
}

void LoadStatistics(Statistics *stats, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file) {
        fread(stats, sizeof(Statistics), 1, file);
        fclose(file);
    } else {
        InitStatistics(stats);
    }
}