#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "raylib.h"
#include "raymath.h"
#include "../include/trash.h"
#include "../include/data.h"
#include "../include/achievement.h"

// 初始屏幕尺寸
#define INIT_WIDTH 800
#define INIT_HEIGHT 600
#define STUDY_IMAGE_COUNT 8

typedef struct {
    float intensity;
    float decay;
    float duration;
    float timer;
} WindowShake;

// 界面状态
typedef enum {
    MAIN_SCREEN,        // 主界面
    TIMER_SCREEN,       // 计时界面
    INTERRUPTION_ALERT, // 中断提示
    CLEAN_FAILED_ALERT, // 清理失败提示
    ACHIEVEMENT_SCREEN,  // 成就界面
    STATISTICS_SCREEN   // 数据统计界面
} ScreenState;

// 番茄钟预设
typedef struct {
    char name[20];
    int minutes;
} PomodoroPreset;

// 持久化应用状态
typedef struct {
    int windowWidth;
    int windowHeight;
    int windowX;
    int windowY;
    int selectedPreset;
    char customMinutes[10];
} PersistentAppState;

// 应用状态
typedef struct {
    // 界面状态
    ScreenState currentScreen;
    ScreenState previousScreen;

    int windowWidth;
    int windowHeight;
    int windowX;
    int windowY;
    
    // 资源
    Font textFont;
    Font titleFont;
    Texture2D achieveIcon;
    Texture2D studyImages[STUDY_IMAGE_COUNT];
    
    // 成就系统
    AchievementManager achievementManager;
    const char *achievementFile;
    float positiveScrollOffset;
    float negativeScrollOffset;
    
    // 番茄钟状态
    PomodoroPreset presets[3];
    int selectedPreset;
    char customMinutes[10];
    bool editingCustom;
    int pomodoroDuration;
    int timeLeft;
    bool timerActive;
    bool windowFocused;
    bool interruptionOccurred;
    double lastUpdateTime;
    
    // 垃圾系统
    int currentTrashIndex;
    int cleanupDuration;
    
    // 学习图片
    int currentStudyImage;

    WindowShake windowShake;

    // 新增皮肤主题变量
    bool isDarkTheme;
    Texture2D themeIcon;  // 皮肤主题图标
    Texture2D sunTexture;  // 太阳图标
    Texture2D moonTexture; // 月亮图标
    // 成就图标（根据不同主题）
    Texture2D achieveIconLight;
    Texture2D achieveIconDark;

    // 数据统计
    Statistics statistics;
    const char *statisticsFile;
    Texture2D dateIconLight;  // 亮色主题统计图标
    Texture2D dateIconDark;   // 暗色主题统计图标
    Texture2D themeIconDark;
    Texture2D themeIconLight;
} AppState;

// 函数声明
static const char* GetResourcePath(const char* relativePath);
static int* GenerateCJKCodepoints(int *codepointCount);
void DrawAchievements(AppState *state, float screenWidth, float screenHeight);
void DrawMainScreen(AppState *state, float screenWidth, float screenHeight);
void DrawTimerScreen(AppState *state, float screenWidth, float screenHeight);
void DrawInterruptionAlert(AppState *state, float screenWidth, float screenHeight);
void DrawCleanFailedAlert(AppState *state, float screenWidth, float screenHeight);
void UpdateTimerLogic(AppState *state);
void HandleMainScreenInput(AppState *state, float screenWidth, float screenHeight);
bool LoadResources(AppState *state);
void UnloadResources(AppState *state);
void TriggerWindowShake(AppState *state, float intensity, float duration);
int* GenerateEssentialCodepoints(int* count);

    
#if defined(_WIN32)
    #define PATH_SEPARATOR '\\'
#else
    #define PATH_SEPARATOR '/'
#endif

static const char* GetResourcePath(const char* relativePath) {
    static char fullPath[512] = {0};
    const char* basePath = GetApplicationDirectory();
    
    snprintf(fullPath, sizeof(fullPath), "%s%c%s", basePath, PATH_SEPARATOR, relativePath);
    return fullPath;
}

void TriggerWindowShake(AppState *state, float intensity, float duration) {
    if (intensity > state->windowShake.intensity) {
        state->windowShake.intensity = intensity;
        state->windowShake.duration = duration;
        state->windowShake.timer = 0;
    }
}

// 辅助函数：生成完整的 CJK 字符集
static int* GenerateCJKCodepoints(int *codepointCount) {
    int *codepoints = NULL;
    int count = 0;
    
    // 基本 Latin 字符（ASCII）
    for (int c = 0x0020; c <= 0x007E; c++) {
        codepoints = (int*)realloc(codepoints, (count + 1) * sizeof(int));
        codepoints[count++] = c;
    }
    
    // 拉丁补充
    for (int c = 0x00A0; c <= 0x00FF; c++) {
        codepoints = (int*)realloc(codepoints, (count + 1) * sizeof(int));
        codepoints[count++] = c;
    }
    
    // CJK 符号和标点
    for (int c = 0x3000; c <= 0x303F; c++) {
        codepoints = (int*)realloc(codepoints, (count + 1) * sizeof(int));
        codepoints[count++] = c;
    }
    
    // CJK 统一表意文字块
    for (int c = 0x4E00; c <= 0x9FFF; c++) {
        codepoints = (int*)realloc(codepoints, (count + 1) * sizeof(int));
        codepoints[count++] = c;
    }
    
    *codepointCount = count;
    return codepoints;
}

// 保存主题状态到文件
void SaveAppThemeState(bool isDarkTheme) {
    FILE *file = fopen("theme_state.dat", "wb");
    if (file) {
        fwrite(&isDarkTheme, sizeof(bool), 1, file);
        fclose(file);
    }
}

// 从文件加载主题状态
bool LoadAppThemeState() {
    FILE *file = fopen("theme_state.dat", "rb");
    bool isDarkTheme = false;
    if (file) {
        fread(&isDarkTheme, sizeof(bool), 1, file);
        fclose(file);
    }
    return isDarkTheme;
}

// 极简风格主界面
void DrawMainScreen(AppState *state, float screenWidth, float screenHeight) {
    // 顶部按钮区域参数
    const float topMargin = 20.0f;          // 上边距
    const float buttonSize = 50.0f;         // 按钮大小
    const float buttonSpacing = 40.0f;      // 按钮间距
    const float groupRightMargin = 20.0f;   // 组右边距

    // 计算按钮组总宽度
    float groupWidth = 3 * buttonSize + 2 * buttonSpacing;
    float startX = screenWidth - groupWidth - groupRightMargin;

    // 设置背景色 - 根据主题变化
    if (state->isDarkTheme) {
        ClearBackground((Color){30, 30, 40, 255}); // 暗色背景
    } else {
        ClearBackground(RAYWHITE); // 亮色背景
    }
    
    // 定义主题颜色
    Color titleColor = state->isDarkTheme ? LIGHTGRAY : DARKGRAY;
    Color textColor = state->isDarkTheme ? LIGHTGRAY : DARKGRAY;
    Color borderColor = state->isDarkTheme ? LIGHTGRAY : DARKGRAY;
    Color highlightColor = state->isDarkTheme ? GOLD : SKYBLUE;
    Color grayColor = state->isDarkTheme ? GRAY : LIGHTGRAY;
    
    // 标题
    const char* title = "番茄钟";
    Vector2 titleSize = MeasureTextEx(state->titleFont, title, state->titleFont.baseSize, 1);
    DrawTextEx(state->titleFont, title, 
             (Vector2){screenWidth/2.0f - titleSize.x/2.0f, 80.0f}, 
             state->titleFont.baseSize, 1, titleColor);
    
    // === 统计按钮 ===
    Rectangle statisticsButton = {
        startX,
        topMargin,
        buttonSize,
        buttonSize
    };
    bool hoverStatistics = CheckCollisionPointRec(GetMousePosition(), statisticsButton);
    
    // 根据主题选择图标
    Texture2D dateIcon = state->isDarkTheme ? state->dateIconDark : state->dateIconLight;
    
    if (dateIcon.id != 0) {
        Color tint = hoverStatistics ? 
                   (state->isDarkTheme ? GOLD : SKYBLUE) : 
                   (state->isDarkTheme ? LIGHTGRAY : GRAY);
        
        Rectangle source = {0, 0, (float)dateIcon.width, (float)dateIcon.height};
        Rectangle dest = {statisticsButton.x, statisticsButton.y, 50.0f, 50.0f};
        DrawTexturePro(dateIcon, source, dest, (Vector2){0, 0}, 0.0f, tint);
    } else {
        // 简约圆形按钮（备用）
        DrawCircleLines(statisticsButton.x + buttonSize/2, statisticsButton.y + buttonSize/2, buttonSize/2 - 5, 
                      hoverStatistics ? 
                      (state->isDarkTheme ? GOLD : SKYBLUE) : 
                      (state->isDarkTheme ? LIGHTGRAY : GRAY));
        DrawText("统", statisticsButton.x + buttonSize/2 - 10, statisticsButton.y + buttonSize/2 - 10, 20, 
               state->isDarkTheme ? LIGHTGRAY : GRAY);
    }

    // === 主题切换按钮 ===
    Rectangle themeButton = {
        startX + buttonSize + buttonSpacing,
        topMargin,
        buttonSize,
        buttonSize
    };
    bool hoverTheme = CheckCollisionPointRec(GetMousePosition(), themeButton);
    
    // 使用明暗主题分离的图标
    Texture2D themeIcon = state->themeIcon;
    
    if (themeIcon.id != 0) {
        Color tint = hoverTheme ? 
                (state->isDarkTheme ? GOLD : SKYBLUE) : 
                (state->isDarkTheme ? LIGHTGRAY : GRAY);
        
        Rectangle source = {0, 0, (float)themeIcon.width, (float)themeIcon.height};
        Rectangle dest = {themeButton.x, themeButton.y, buttonSize, buttonSize};
        DrawTexturePro(themeIcon, source, dest, (Vector2){0, 0}, 0.0f, tint);
    } else {
        // 简约圆形按钮
        DrawCircleLines(themeButton.x + buttonSize/2, themeButton.y + buttonSize/2, buttonSize/2 - 5, 
                      hoverTheme ? highlightColor : grayColor);
        DrawText(state->isDarkTheme ? "月" : "日", themeButton.x + buttonSize/2 - 10, themeButton.y + buttonSize/2 - 10, 20, grayColor);
    }

    // === 成就按钮 ===
    Rectangle achievementButton = {
        startX + 2 * (buttonSize + buttonSpacing),
        topMargin,
        buttonSize,
        buttonSize
    };
    bool hoverAchievement = CheckCollisionPointRec(GetMousePosition(), achievementButton);
    
    // 根据主题选择成就图标
    Texture2D achieveIcon = state->isDarkTheme ? state->achieveIconDark : state->achieveIconLight;
    
    if (achieveIcon.id != 0) {
        Color tint = hoverAchievement ? 
                   (state->isDarkTheme ? GOLD : SKYBLUE) : 
                   (state->isDarkTheme ? LIGHTGRAY : GRAY);
        
        Rectangle source = {0, 0, (float)achieveIcon.width, (float)achieveIcon.height};
        Rectangle dest = {achievementButton.x, achievementButton.y, buttonSize, buttonSize};
        DrawTexturePro(achieveIcon, source, dest, (Vector2){0, 0}, 0.0f, tint);
    } else {
        // 简约圆形按钮（备用）
        DrawCircleLines(achievementButton.x + buttonSize/2, achievementButton.y + buttonSize/2, buttonSize/2 - 5, 
                      hoverAchievement ? 
                      (state->isDarkTheme ? GOLD : SKYBLUE) : 
                      (state->isDarkTheme ? LIGHTGRAY : GRAY));
        DrawText("成", achievementButton.x + buttonSize/2 - 10, achievementButton.y + buttonSize/2 - 10, 20, 
               state->isDarkTheme ? LIGHTGRAY : GRAY);
    }
    
    // 预设选项 - 简约线条设计
    const int presetCount = 3;
    const float presetSpacing = 80.0f;
    const float presetStartY = 180.0f;
    
    for (int i = 0; i < presetCount; i++) {
        float rectY = presetStartY + i * presetSpacing;
        Rectangle rect = {
            screenWidth/2.0f - 150.0f,
            rectY,
            300.0f,
            50.0f
        };
        
        bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
        Color border = i == state->selectedPreset ? highlightColor : borderColor;
        
        // 简约线条边框
        DrawRectangleLinesEx(rect, 1.5f, border);
        
        const char *text = state->presets[i].name;
        Vector2 textSize = MeasureTextEx(state->textFont, text, 30, 1);
        DrawTextEx(state->textFont, text, 
                 (Vector2){rect.x + rect.width/2.0f - textSize.x/2.0f, 
                          rect.y + rect.height/2.0f - textSize.y/2.0f},
                 30, 1, textColor);
    }

    DrawTrash();  // 绘制垃圾
    
    // 自定义输入框 - 简约设计（位置调整）
    const float inputY = presetStartY + presetCount * presetSpacing + 20.0f;
    
    if (state->selectedPreset == 2) {
        Rectangle inputRect = {screenWidth/2.0f - 150.0f, inputY, 300.0f, 50.0f};
        
        // 简约边框
        DrawRectangleLinesEx(inputRect, 1.5f, state->editingCustom ? highlightColor : borderColor);
        
        char displayText[50];
        if (state->editingCustom) {
            sprintf(displayText, "%s_", state->customMinutes);
        } else if (strlen(state->customMinutes) > 0) {
            sprintf(displayText, "%s分钟", state->customMinutes);
        } else {
            strcpy(displayText, "输入分钟数");
        }
        
        Vector2 textSize = MeasureTextEx(state->textFont, displayText, 30, 1);
        DrawTextEx(state->textFont, displayText, 
                 (Vector2){inputRect.x + inputRect.width/2.0f - textSize.x/2.0f, 
                          inputRect.y + inputRect.height/2.0f - textSize.y/2.0f},
                 30, 1, textColor);
        
        // 提示文本
        if (strlen(state->customMinutes) == 0 && !state->editingCustom) {
            const char* hint = "点击输入分钟数 30-120";
            Vector2 hintSize = MeasureTextEx(state->textFont, hint, 20, 1);
            DrawTextEx(state->textFont, hint, 
                     (Vector2){inputRect.x + inputRect.width/2.0f - hintSize.x/2.0f, 
                              inputRect.y + inputRect.height + 10.0f}, 
                     20, 1, grayColor);
        }
    }
}

// 极简风格计时界面
void DrawTimerScreen(AppState *state, float screenWidth, float screenHeight) {
    // 设置背景色
    if (state->isDarkTheme) {
        ClearBackground((Color){30, 30, 40, 255});
    } else {
        ClearBackground(RAYWHITE);
    }
    
    // 定义主题颜色
    Color textColor = state->isDarkTheme ? LIGHTGRAY : DARKGRAY;
    Color timerActiveColor = state->isDarkTheme ? GOLD : MAROON;
    Color timerInactiveColor = state->isDarkTheme ? LIGHTGRAY : DARKGRAY;
    Color separatorColor = state->isDarkTheme ? (Color){100, 100, 100, 255} : LIGHTGRAY;
    Color hintColor = state->isDarkTheme ? (Color){150, 150, 150, 255} : GRAY;

    // 显示时间
    int minutes = state->timeLeft / 60;
    int seconds = state->timeLeft % 60;
    char timeText[10];
    sprintf(timeText, "%02d:%02d", minutes, seconds);
    
    int fontSize = state->titleFont.baseSize;
    Color timerColor = state->timerActive ? timerActiveColor : timerInactiveColor;
    
    Vector2 timeSize = MeasureTextEx(state->titleFont, timeText, fontSize, 1);
    Vector2 position = {screenWidth/2.0f - timeSize.x/2.0f, 50.0f};
    
    DrawTextEx(state->titleFont, timeText, position, fontSize, 1, timerColor);

    // 简约分隔线
    DrawLine(0, 120, screenWidth, 120, separatorColor);
    
    // 显示当前任务
    char taskText[50];
    if (state->currentTrashIndex >= 0) {
        sprintf(taskText, "清理垃圾: %d分钟", state->cleanupDuration / 60);
    } else {
        sprintf(taskText, "专注工作: %d分钟", state->pomodoroDuration / 60);
    }
    
    Vector2 taskSize = MeasureTextEx(state->textFont, taskText, 28, 1);
    DrawTextEx(state->textFont, taskText, 
             (Vector2){screenWidth/2.0f - taskSize.x/2.0f, 140.0f}, 
             28, 1, textColor);
    
    // 简约警告文本
    const char *warningText = "注意: 切换窗口将中断计时并产生垃圾";
    Vector2 warningSize = MeasureTextEx(state->textFont, warningText, 20, 1);
    DrawTextEx(state->textFont, warningText, 
             (Vector2){screenWidth/2.0f - warningSize.x/2.0f, 180.0f}, 
             20, 1, hintColor);
    
    // 显示学习图片 - 移除边框，深色主题下添加背景色
    if (state->currentStudyImage >= 0 && state->currentStudyImage < STUDY_IMAGE_COUNT && 
        state->studyImages[state->currentStudyImage].id != 0) {
        
        Texture2D currentImage = state->studyImages[state->currentStudyImage];
        float imageScale = 0.35f;
        float imageWidth = currentImage.width * imageScale;
        float imageHeight = currentImage.height * imageScale;
        
        Vector2 imagePos = {
            screenWidth / 2 - imageWidth / 2,
            screenHeight / 2 - imageHeight / 2 + 100.0f
        };
        
        // 深色主题下添加偏白的灰色背景
        if (state->isDarkTheme) {
            // 使用偏白的灰色背景 (220, 220, 220)
            DrawRectangle(imagePos.x - 10, imagePos.y - 10, 
                         imageWidth + 20, imageHeight + 20, 
                         (Color){220, 220, 220, 255});
        }
        
        // 绘制图片
        DrawTextureEx(currentImage, imagePos, 0.0f, imageScale, WHITE);
    }

    const char *timerHint = "空格键: 开始/暂停  R键: 重置";
    Vector2 timerHintSize = MeasureTextEx(state->textFont, timerHint, 20, 1);
    DrawTextEx(state->textFont, timerHint, 
            (Vector2){screenWidth/2.0f - timerHintSize.x/2.0f, 
                    screenHeight - 40.0f}, 
            20, 1, hintColor);
}

// 极简风格成就界面
void DrawAchievements(AppState *state, float screenWidth, float screenHeight) {
    // 设置背景色
    if (state->isDarkTheme) {
        ClearBackground((Color){30, 30, 40, 255});
    } else {
        ClearBackground(RAYWHITE);
    }
    
    // 定义主题颜色
    Color titleColor = state->isDarkTheme ? GOLD : BLACK;
    Color textColor = state->isDarkTheme ? LIGHTGRAY : DARKGRAY;
    Color statsColor = state->isDarkTheme ? LIGHTGRAY : DARKGRAY;
    Color positivePanelBg = state->isDarkTheme ? (Color){40, 50, 40, 255} : (Color){245, 255, 245, 255};
    Color positivePanelBorder = state->isDarkTheme ? (Color){60, 80, 60, 255} : (Color){220, 240, 220, 255};
    Color negativePanelBg = state->isDarkTheme ? (Color){50, 40, 40, 255} : (Color){255, 245, 245, 255};
    Color negativePanelBorder = state->isDarkTheme ? (Color){80, 60, 60, 255} : (Color){240, 220, 220, 255};
    Color unlockedNameColor = state->isDarkTheme ? GOLD : DARKBLUE;
    Color unlockedDescColor = state->isDarkTheme ? LIGHTGRAY : (Color){80, 80, 80, 255};
    Color lockedNameColor = state->isDarkTheme ? (Color){150, 150, 150, 255} : (Color){100, 100, 100, 255};
    Color lockedDescColor = state->isDarkTheme ? (Color){120, 120, 120, 255} : (Color){180, 180, 180, 255};

    AchievementManager *manager = &state->achievementManager;
    Font *textFont = &state->textFont;
    Font *titleFont = &state->titleFont;
    
    // ================ 头部 ================
    const float headerHeight = 150.0f;
    
    // 标题
    const char* title = "成就";
    Vector2 titleSize = MeasureTextEx(*titleFont, title, 60, 2);
    DrawTextEx(*titleFont, title, 
             (Vector2){screenWidth/2.0f - titleSize.x/2.0f, 30.0f}, 
             60, 2, titleColor);
    
    // 统计信息
    char statsText[256];
    sprintf(statsText, "总番茄钟: %d | 清理垃圾: %d | 产生垃圾: %d | 中断次数: %d | 连续天数: %d", 
            manager->totalPomodoros, manager->cleanedTrashCount, manager->generatedTrashCount,
            manager->interruptionsCount, manager->streakDays);
    Vector2 statsSize = MeasureTextEx(*textFont, statsText, 20, 1);
    DrawTextEx(*textFont, statsText, 
             (Vector2){screenWidth/2.0f - statsSize.x/2.0f, 100.0f}, 
             20, 1, statsColor);
    
    // ================ 身体 ================
    const float bodyY = headerHeight + 10.0f;
    const float bodyHeight = screenHeight - headerHeight - 80.0f;
    const float panelWidth = screenWidth / 2.0f - 20.0f;
    const float panelHeight = bodyHeight - 20.0f;
    const float spacing = 60.0f;
    
    // 左侧面板 - 正面成就
    Rectangle leftPanel = {10.0f, bodyY, panelWidth, panelHeight};
    DrawRectangleRec(leftPanel, positivePanelBg);
    DrawRectangleLinesEx(leftPanel, 1, positivePanelBorder);
    
    // 右侧面板 - 负面成就
    Rectangle rightPanel = {screenWidth / 2.0f + 10.0f, bodyY, panelWidth, panelHeight};
    DrawRectangleRec(rightPanel, negativePanelBg);
    DrawRectangleLinesEx(rightPanel, 1, negativePanelBorder);
    
    // 面板标题
    const char* positiveTitle = "成长徽章";
    Vector2 positiveTitleSize = MeasureTextEx(*textFont, positiveTitle, 30, 1);
    DrawTextEx(*textFont, positiveTitle, 
             (Vector2){leftPanel.x + leftPanel.width/2 - positiveTitleSize.x/2, 
                      leftPanel.y - 30.0f}, 
             30, 1, state->isDarkTheme ? GOLD : DARKGREEN);
    
    const char* negativeTitle = "改进空间";
    Vector2 negativeTitleSize = MeasureTextEx(*textFont, negativeTitle, 30, 1);
    DrawTextEx(*textFont, negativeTitle, 
             (Vector2){rightPanel.x + rightPanel.width/2 - negativeTitleSize.x/2, 
                      rightPanel.y - 30.0f}, 
             30, 1, state->isDarkTheme ? (Color){220, 150, 150, 255} : MAROON);
    
    // 处理滚动
    Vector2 mousePos = GetMousePosition();
    float *positiveScroll = &state->positiveScrollOffset;
    float *negativeScroll = &state->negativeScrollOffset;
    
    // 左侧面板滚动
    if (CheckCollisionPointRec(mousePos, leftPanel)) {
        float wheel = GetMouseWheelMove();
        *positiveScroll -= wheel * 30.0f;
    }
    
    // 右侧面板滚动
    if (CheckCollisionPointRec(mousePos, rightPanel)) {
        float wheel = GetMouseWheelMove();
        *negativeScroll -= wheel * 30.0f;
    }
    
    // 限制滚动范围
    float maxPositiveScroll = (ACH_COUNT * spacing) - panelHeight;
    if (maxPositiveScroll < 0) maxPositiveScroll = 0;
    if (*positiveScroll < 0) *positiveScroll = 0;
    if (*positiveScroll > maxPositiveScroll) *positiveScroll = maxPositiveScroll;
    
    float maxNegativeScroll = (NEG_COUNT * spacing) - panelHeight;
    if (maxNegativeScroll < 0) maxNegativeScroll = 0;
    if (*negativeScroll < 0) *negativeScroll = 0;
    if (*negativeScroll > maxNegativeScroll) *negativeScroll = maxNegativeScroll;
    
    // 绘制左侧面板内容（正面成就）
    BeginScissorMode((int)leftPanel.x, (int)leftPanel.y, (int)leftPanel.width, (int)leftPanel.height);
    {
        float startY = leftPanel.y + 10.0f - *positiveScroll;
        
        for (int i = 0; i < ACH_COUNT; i++) {
            float yPos = startY + i * spacing;
            
            if (yPos + spacing > leftPanel.y && yPos < leftPanel.y + leftPanel.height) {
                Rectangle achievementRect = {
                    leftPanel.x + 10.0f,
                    yPos,
                    leftPanel.width - 20.0f,
                    50.0f
                };
                
                // 成就背景
                Color bgColor = manager->achievements[i].unlocked ? 
                    positivePanelBg : (state->isDarkTheme ? (Color){40, 40, 40, 255} : (Color){250, 250, 250, 255});
                DrawRectangleRec(achievementRect, bgColor);
                
                // 成就图标
                if (manager->achievements[i].unlocked) {
                    DrawCircle(achievementRect.x + 30, achievementRect.y + 25, 15, state->isDarkTheme ? GOLD : (Color){200, 170, 50, 255});
                } else {
                    DrawCircle(achievementRect.x + 30, achievementRect.y + 25, 15, state->isDarkTheme ? (Color){80, 80, 80, 255} : (Color){230, 230, 230, 255});
                }
                
                // 成就名称和描述
                Color nameColor = manager->achievements[i].unlocked ? unlockedNameColor : lockedNameColor;
                Color descColor = manager->achievements[i].unlocked ? unlockedDescColor : lockedDescColor;
                
                DrawTextEx(*textFont, manager->achievements[i].name, 
                         (Vector2){achievementRect.x + 60.0f, achievementRect.y + 10.0f}, 
                         22, 1, nameColor);
                
                DrawTextEx(*textFont, manager->achievements[i].description, 
                         (Vector2){achievementRect.x + 60.0f, achievementRect.y + 30.0f}, 
                         16, 1, descColor);
                
                // 解锁时间
                if (manager->achievements[i].unlocked) {
                    time_t unlockTime = manager->achievements[i].unlockTime;
                    time_t unlockTimeCopy = manager->achievements[i].unlockTime;
                    struct tm *timeinfo = localtime(&unlockTimeCopy);
                    char timeStr[50];
                    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M", timeinfo);
                    Vector2 timeSize = MeasureTextEx(*textFont, timeStr, 14, 1);
                    DrawTextEx(*textFont, timeStr, 
                             (Vector2){achievementRect.x + achievementRect.width - timeSize.x - 10.0f, 
                                      achievementRect.y + 15.0f}, 
                             14, 1, state->isDarkTheme ? (Color){150, 200, 150, 255} : (Color){100, 150, 100, 255});
                }
            }
        }
    }
    EndScissorMode();
    
    // 绘制右侧面板内容（负面成就）
    BeginScissorMode((int)rightPanel.x, (int)rightPanel.y, (int)rightPanel.width, (int)rightPanel.height);
    {
        float startY = rightPanel.y + 10.0f - *negativeScroll;
        
        for (int i = 0; i < NEG_COUNT; i++) {
            float yPos = startY + i * spacing;
            
            if (yPos + spacing > rightPanel.y && yPos < rightPanel.y + rightPanel.height) {
                Rectangle achievementRect = {
                    rightPanel.x + 10.0f,
                    yPos,
                    rightPanel.width - 20.0f,
                    50.0f
                };
                
                // 成就背景
                Color bgColor = manager->negativeAchievements[i].unlocked ? 
                    negativePanelBg : (state->isDarkTheme ? (Color){40, 40, 40, 255} : (Color){250, 250, 250, 255});
                DrawRectangleRec(achievementRect, bgColor);
                
                // 成就图标
                if (manager->negativeAchievements[i].unlocked) {
                    DrawCircle(achievementRect.x + 30, achievementRect.y + 25, 15, state->isDarkTheme ? (Color){150, 150, 150, 255} : (Color){180, 180, 180, 255});
                } else {
                    DrawCircle(achievementRect.x + 30, achievementRect.y + 25, 15, state->isDarkTheme ? (Color){80, 80, 80, 255} : (Color){230, 230, 230, 255});
                }
                
                // 成就名称和描述
                Color nameColor = manager->negativeAchievements[i].unlocked ? unlockedNameColor : lockedNameColor;
                Color descColor = manager->negativeAchievements[i].unlocked ? unlockedDescColor : lockedDescColor;
                
                DrawTextEx(*textFont, manager->negativeAchievements[i].name, 
                         (Vector2){achievementRect.x + 60.0f, achievementRect.y + 10.0f}, 
                         22, 1, nameColor);
                
                DrawTextEx(*textFont, manager->negativeAchievements[i].description, 
                         (Vector2){achievementRect.x + 60.0f, achievementRect.y + 30.0f}, 
                         16, 1, descColor);
                
                // 解锁时间
                if (manager->negativeAchievements[i].unlocked) {
                    struct tm *timeinfo = localtime(&manager->negativeAchievements[i].unlockTime);
                    char timeStr[50];
                    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M", timeinfo);
                    Vector2 timeSize = MeasureTextEx(*textFont, timeStr, 14, 1);
                    DrawTextEx(*textFont, timeStr, 
                             (Vector2){achievementRect.x + achievementRect.width - timeSize.x - 10.0f, 
                                      achievementRect.y + 15.0f}, 
                             14, 1, state->isDarkTheme ? (Color){200, 150, 150, 255} : (Color){150, 100, 100, 255});
                }
            }
        }
    }
    EndScissorMode();
    
    // 绘制滚动条 (修复滚动条高度计算)
    if (maxPositiveScroll > 0) {
        float contentHeight = ACH_COUNT * spacing;
        float visibleRatio = panelHeight / contentHeight;
        float scrollbarHeight = panelHeight * visibleRatio;
        float scrollbarPosition = (*positiveScroll / maxPositiveScroll) * (panelHeight - scrollbarHeight);
        
        Rectangle scrollbar = {
            leftPanel.x + leftPanel.width - 8.0f,
            leftPanel.y + scrollbarPosition,
            6.0f,
            scrollbarHeight
        };
        DrawRectangleRec(scrollbar, state->isDarkTheme ? (Color){100, 150, 100, 200} : (Color){180, 220, 180, 200});
    }
    
    if (maxNegativeScroll > 0) {
        float contentHeight = NEG_COUNT * spacing;
        float visibleRatio = panelHeight / contentHeight;
        float scrollbarHeight = panelHeight * visibleRatio;
        float scrollbarPosition = (*negativeScroll / maxNegativeScroll) * (panelHeight - scrollbarHeight);
        
        Rectangle scrollbar = {
            rightPanel.x + rightPanel.width - 8.0f,
            rightPanel.y + scrollbarPosition,
            6.0f,
            scrollbarHeight
        };
        DrawRectangleRec(scrollbar, state->isDarkTheme ? (Color){150, 100, 100, 200} : (Color){220, 180, 180, 200});
    }
    
    // ================ 尾部 ================
    // 返回按钮
    Rectangle backButton = {
        screenWidth / 2.0f - 75.0f,
        screenHeight - 70.0f,
        150.0f,
        50.0f
    };
    bool hoverBack = CheckCollisionPointRec(GetMousePosition(), backButton);
    DrawRectangleRec(backButton, hoverBack ? 
                   (state->isDarkTheme ? (Color){60, 60, 70, 255} : (Color){240, 240, 240, 255}) : 
                   (state->isDarkTheme ? (Color){40, 40, 50, 255} : RAYWHITE));
    DrawRectangleLinesEx(backButton, 1, state->isDarkTheme ? (Color){100, 100, 100, 255} : (Color){200, 200, 200, 255});
    
    const char* backText = "返回";
    Vector2 backTextSize = MeasureTextEx(*textFont, backText, 24, 1);
    DrawTextEx(*textFont, backText, 
             (Vector2){backButton.x + backButton.width/2 - backTextSize.x/2, 
                      backButton.y + backButton.height/2 - backTextSize.y/2},
             24, 1, textColor);
    
    if (hoverBack && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        state->currentScreen = MAIN_SCREEN;
    }
}

// 极简风格中断提示界面
void DrawInterruptionAlert(AppState *state, float screenWidth, float screenHeight) {
    // 半透明背景 - 根据主题调整
    Color bgColor = state->isDarkTheme ? 
                   Fade((Color){20, 20, 30, 255}, 0.8f) : 
                   Fade((Color){30, 30, 30, 255}, 0.7f);
    DrawRectangle(0, 0, (int)screenWidth, (int)screenHeight, bgColor);
    
    // 简约提示框
    Rectangle alertRect = {
        screenWidth/2.0f - 200.0f,
        screenHeight/2.0f - 100.0f,
        400.0f,
        200.0f
    };
    
    // 绘制圆角矩形背景
    DrawRectangleRounded(alertRect, 0.05f, 8, state->isDarkTheme ? (Color){50, 50, 60, 255} : RAYWHITE);
    
    // 边框
    DrawRectangleLinesEx(alertRect, 2.0f, state->isDarkTheme ? GOLD : DARKGRAY);
    
    // 标题
    const char* title = "专注被打断!";
    Vector2 titleSize = MeasureTextEx(state->titleFont, title, 36, 1);
    DrawTextEx(state->titleFont, title, 
             (Vector2){alertRect.x + alertRect.width/2.0f - titleSize.x/2.0f, 
                      alertRect.y + 30.0f}, 
             36, 1, state->isDarkTheme ? GOLD : MAROON);
    
    // 消息
    const char* message = "已产生垃圾!请返回主界面清理";
    Vector2 msgSize = MeasureTextEx(state->textFont, message, 24, 1);
    DrawTextEx(state->textFont, message, 
             (Vector2){alertRect.x + alertRect.width/2.0f - msgSize.x/2.0f, 
                      alertRect.y + 90.0f}, 
             24, 1, state->isDarkTheme ? LIGHTGRAY : DARKGRAY);
    
    // 简约确定按钮
    Rectangle okButton = {
        alertRect.x + alertRect.width/2.0f - 50.0f,
        alertRect.y + 140.0f,
        100.0f,
        40.0f
    };
    bool hover = CheckCollisionPointRec(GetMousePosition(), okButton);
    
    // 简约线条按钮
    DrawRectangleLinesEx(okButton, 1.5f, hover ? (state->isDarkTheme ? GOLD : SKYBLUE) : GRAY);
    
    const char* okText = "确定";
    Vector2 okTextSize = MeasureTextEx(state->textFont, okText, 24, 1);
    DrawTextEx(state->textFont, okText, 
             (Vector2){okButton.x + okButton.width/2.0f - okTextSize.x/2.0f, 
                      okButton.y + okButton.height/2.0f - okTextSize.y/2.0f}, 
             24, 1, state->isDarkTheme ? LIGHTGRAY : DARKGRAY);
}

// 极简风格清理失败提示界面
void DrawCleanFailedAlert(AppState *state, float screenWidth, float screenHeight) {
    // 半透明遮罩
    Color bgColor = state->isDarkTheme ? 
                   Fade((Color){20, 20, 30, 255}, 0.8f) : 
                   Fade((Color){40, 40, 40, 255}, 0.7f);
    DrawRectangle(0, 0, (int)screenWidth, (int)screenHeight, bgColor);
    
    // 简约提示框
    Rectangle alertRect = {
        screenWidth/2.0f - 200.0f,
        screenHeight/2.0f - 100.0f,
        400.0f,
        200.0f
    };
    DrawRectangleRec(alertRect, state->isDarkTheme ? (Color){50, 50, 60, 255} : RAYWHITE);
    DrawRectangleLinesEx(alertRect, 1, state->isDarkTheme ? (Color){200, 150, 150, 255} : (Color){200, 100, 100, 255});
    
    // 标题
    const char* title = "清理失败!";
    Vector2 titleSize = MeasureTextEx(state->titleFont, title, 36, 1);
    DrawTextEx(state->titleFont, title, 
             (Vector2){alertRect.x + alertRect.width/2.0f - titleSize.x/2.0f, 
                      alertRect.y + 30.0f}, 
             36, 1, state->isDarkTheme ? (Color){220, 150, 150, 255} : (Color){200, 100, 100, 255});
    
    // 消息
    const char* message = "请完成整个番茄钟来清理垃圾";
    Vector2 msgSize = MeasureTextEx(state->textFont, message, 22, 1);
    DrawTextEx(state->textFont, message, 
             (Vector2){alertRect.x + alertRect.width/2.0f - msgSize.x/2.0f, 
                      alertRect.y + 90.0f}, 
             22, 1, state->isDarkTheme ? LIGHTGRAY : DARKGRAY);
    
    // 简约确定按钮
    Rectangle okButton = {
        alertRect.x + alertRect.width/2.0f - 50.0f,
        alertRect.y + 140.0f,
        100.0f,
        40.0f
    };
    bool hover = CheckCollisionPointRec(GetMousePosition(), okButton);
    
    // 悬停效果
    if (hover) {
        DrawRectangleRec(okButton, state->isDarkTheme ? Fade((Color){80, 80, 90, 255}, 0.5f) : Fade(LIGHTGRAY, 0.3f));
    }
    
    DrawRectangleLinesEx(okButton, 1, state->isDarkTheme ? LIGHTGRAY : DARKGRAY);
    const char* okText = "确定";
    Vector2 okTextSize = MeasureTextEx(state->textFont, okText, 22, 1);
    DrawTextEx(state->textFont, okText, 
             (Vector2){okButton.x + okButton.width/2.0f - okTextSize.x/2.0f, 
                      okButton.y + okButton.height/2.0f - okTextSize.y/2.0f}, 
             22, 1, state->isDarkTheme ? LIGHTGRAY : DARKGRAY);
}

// 计时逻辑更新
void UpdateTimerLogic(AppState *state) {
    bool trashCleaned = false;
    
    // 只处理计时结束的情况
    if (state->timeLeft <= 0) {
        state->timerActive = false;
        
        if (state->currentTrashIndex >= 0) {
            CleanTrash(state->currentTrashIndex);
            state->achievementManager.cleanedTrashCount++;
            state->currentTrashIndex = -1;
            trashCleaned = true;
        } else {
            state->achievementManager.totalPomodoros++;
            state->interruptionOccurred = false;
            CheckAchievements(&state->achievementManager, true, false, state->pomodoroDuration);
            
            if (state->pomodoroDuration == 45*60) {
                state->achievementManager.longSessionCount++;
            }
        }
        
        state->currentScreen = MAIN_SCREEN;
    }
        
    // 添加成就检查
    CheckAchievements(&state->achievementManager, false, true, state->pomodoroDuration);
}

// 主界面输入处理
void HandleMainScreenInput(AppState *state, float screenWidth, float screenHeight) {
    // 使用相同的坐标计算作为绘制函数
    const float topMargin = 20.0f;          // 上边距
    const float buttonSize = 50.0f;         // 按钮大小
    const float buttonSpacing = 40.0f;      // 按钮间距
    const float groupRightMargin = 20.0f;   // 组右边距

    // 计算按钮组总宽度
    float groupWidth = 3 * buttonSize + 2 * buttonSpacing;
    float startX = screenWidth - groupWidth - groupRightMargin;

    // 事件处理标志
    bool eventHandled = false;

    // === 主题按钮 ===
    Rectangle themeButton = {
        startX + buttonSize + buttonSpacing,
        topMargin,
        buttonSize,
        buttonSize
    };
    
    if (!eventHandled && CheckCollisionPointRec(GetMousePosition(), themeButton) && 
        IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) 
    {
        // 切换主题
        state->isDarkTheme = !state->isDarkTheme;
        state->themeIcon = state->isDarkTheme ? state->moonTexture : state->sunTexture;
        SaveAppThemeState(state->isDarkTheme);
        eventHandled = true;
    }

    // === 成就按钮 ===
    Rectangle achievementButton = {
        startX + 2 * (buttonSize + buttonSpacing),
        topMargin,
        buttonSize,
        buttonSize
    };
    
    if (!eventHandled && CheckCollisionPointRec(GetMousePosition(), achievementButton) && 
        IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) 
    {
        state->positiveScrollOffset = 0.0f;
        state->negativeScrollOffset = 0.0f;
        state->currentScreen = ACHIEVEMENT_SCREEN;
        eventHandled = true;
    }

    // === 统计按钮 ===
    Rectangle statisticsButton = {
        startX,
        topMargin,
        buttonSize,
        buttonSize
    };
    
    if (!eventHandled && CheckCollisionPointRec(GetMousePosition(), statisticsButton) && 
        IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) 
    {
        // 更新统计数据
        state->statistics.totalPomodoros = state->achievementManager.totalPomodoros;
        state->statistics.cleanedTrash = state->achievementManager.cleanedTrashCount;
        state->statistics.generatedTrash = state->achievementManager.generatedTrashCount;
        state->statistics.interruptions = state->achievementManager.interruptionsCount;
        state->statistics.streakDays = state->achievementManager.streakDays;
        state->statistics.longSessions = state->achievementManager.longSessionCount;
        
        state->currentScreen = STATISTICS_SCREEN;
        eventHandled = true;
    }
    
    // 预设选项点击 - 使用与绘制相同的坐标
    const float presetStartY = 180.0f;
    const float presetSpacing = 80.0f;
    
    for (int i = 0; i < 3; i++) {
        float rectY = presetStartY + i * presetSpacing;
        Rectangle rect = {
            screenWidth/2.0f - 150.0f,
            rectY,
            300.0f,
            50.0f
        };
        
        if (!eventHandled && CheckCollisionPointRec(GetMousePosition(), rect) && 
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            state->selectedPreset = i;
            eventHandled = true;
            
            if (i == 2) { // 自定义选项
                state->editingCustom = true;
                state->customMinutes[0] = '\0';
            } else {
                state->editingCustom = false;
                state->pomodoroDuration = state->presets[i].minutes * 60; // 分钟转秒
                state->timeLeft = state->pomodoroDuration;
                state->currentScreen = TIMER_SCREEN;
                state->timerActive = true;
                state->lastUpdateTime = GetTime();
                state->currentStudyImage = GetRandomValue(0, STUDY_IMAGE_COUNT - 1);
                
                if (state->pomodoroDuration == 45*60) {
                    state->achievementManager.longSessionCount++;
                }
            }
        }
    }
    
    // 自定义输入处理
    if (!eventHandled && state->selectedPreset == 2) {
        float inputY = presetStartY + 3 * presetSpacing + 20.0f;
        Rectangle inputRect = {screenWidth/2.0f - 150.0f, inputY, 300.0f, 50.0f};
        
        // 点击输入框
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            state->editingCustom = CheckCollisionPointRec(GetMousePosition(), inputRect);
            if (state->editingCustom) {
                eventHandled = true;
            }
        }
        
        if (state->editingCustom) {
            // 处理输入
            int key = GetCharPressed();
            while (key > 0) {
                if ((key >= '0' && key <= '9') && strlen(state->customMinutes) < 3) {
                    char str[2] = {(char)key, '\0'};
                    strcat(state->customMinutes, str);
                }
                key = GetCharPressed();
            }
            
            if (IsKeyPressed(KEY_BACKSPACE) && strlen(state->customMinutes) > 0) {
                state->customMinutes[strlen(state->customMinutes)-1] = '\0';
            }
            
            // 实时验证输入范围
            if (IsKeyPressed(KEY_ENTER)) {
                state->editingCustom = false;
                eventHandled = true;
                
                if (strlen(state->customMinutes) > 0) {
                    int minutes = atoi(state->customMinutes);
                    if (minutes >= 30 && minutes <= 120) { 
                        state->pomodoroDuration = minutes * 60; // 分钟转秒
                        state->timeLeft = state->pomodoroDuration;
                        state->currentScreen = TIMER_SCREEN;
                        state->timerActive = true;
                        state->lastUpdateTime = GetTime();
                        state->currentStudyImage = GetRandomValue(0, STUDY_IMAGE_COUNT - 1);
                    } else {
                        // 显示错误提示
                        const char* error = "请输入30-120之间的数字";
                        Vector2 errorSize = MeasureTextEx(state->textFont, error, 20, 1);
                        DrawTextEx(state->textFont, error, 
                                (Vector2){inputRect.x + inputRect.width/2.0f - errorSize.x/2.0f, 
                                        inputRect.y + 60.0f}, 
                                20, 1, RED);
                    }
                }
            }
        }
    }

    // 垃圾点击处理 - 只在没有其他事件处理时进行
    if (!eventHandled && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        for (int i = 0; i < trashCount; i++) {
            if (trashes[i].active && !trashes[i].cleaning) {
                // 计算鼠标到垃圾中心的距离
                float distance = Vector2Distance(mousePos, trashes[i].position);
                
                // 如果距离小于垃圾半径，则命中
                if (distance <= trashes[i].radius) {
                    // 分钟转秒
                    state->cleanupDuration = trashes[i].pomodoroDuration * 60;
                    state->currentTrashIndex = i;
                    state->pomodoroDuration = state->cleanupDuration;
                    state->timeLeft = state->pomodoroDuration;
                    state->currentScreen = TIMER_SCREEN;
                    state->timerActive = true;
                    state->lastUpdateTime = GetTime();
                    state->currentStudyImage = GetRandomValue(0, STUDY_IMAGE_COUNT - 1);
                    eventHandled = true;
                    break;
                }
            }
        }
    }
}

// 资源加载函数
bool LoadResources(AppState *state) {
    // 提前加载一张随机学习图片用于加载界面
    int randomIndex = GetRandomValue(0, STUDY_IMAGE_COUNT-1);
    char loadingImgPath[256];
    sprintf(loadingImgPath, "assets/img/study%d.png", randomIndex + 1);
    
    Texture2D loadingTexture = {0};
    if (FileExists(loadingImgPath)) {
        loadingTexture = LoadTexture(loadingImgPath);
    }
    
    // 绘制加载界面
    BeginDrawing();
    ClearBackground(RAYWHITE);
    
    if (loadingTexture.id != 0) {
        // 计算缩放比例，使图片占据屏幕高度的40%
        float scale = (GetScreenHeight() * 0.4f) / loadingTexture.height;
        float width = loadingTexture.width * scale;
        float height = loadingTexture.height * scale;
        
        // 居中显示
        DrawTextureEx(loadingTexture, 
                     (Vector2){GetScreenWidth()/2 - width/2, 
                              GetScreenHeight()/2 - height/2}, 
                     0.0f, scale, WHITE);
        
        // 在图片下方显示加载文本
        const char* loadingText = "The resource is loading...";
        int fontSize = 30;
        Vector2 textSize = MeasureTextEx(GetFontDefault(), loadingText, fontSize, 1);
        DrawText(loadingText, 
                GetScreenWidth()/2 - textSize.x/2, 
                GetScreenHeight()/2 + height/2 + 20, 
                fontSize, DARKGRAY);
    } else {
        // 没有图片时显示文本
        DrawText("The resource is loading...", GetScreenWidth()/2 - 100, GetScreenHeight()/2, 30, DARKGRAY);
    }
    
    EndDrawing();

    // 生成CJK字符集
    int codepointCount = 0;
    int *codepoints = GenerateCJKCodepoints(&codepointCount);
    
    const int baseFontSize = 32;
    const char *regularFontPath = "assets/fonts/SourceHanSansCN-Regular.otf";
    const char *boldFontPath = "assets/fonts/SourceHanSansCN-Bold.otf";
    
    const int titleFontSize = 60;  // 增大标题字体尺寸

    // 确保资源目录存在
    if (!DirectoryExists("assets/fonts")) {
        TraceLog(LOG_WARNING, "字体目录不存在");
    }
    
    // 加载常规字体
    if (FileExists(regularFontPath)) {
        state->textFont = LoadFontEx(regularFontPath, baseFontSize, codepoints, codepointCount);
        if (state->textFont.texture.id == 0) {
            TraceLog(LOG_WARNING, "常规字体加载失败: %s", regularFontPath);
            state->textFont = GetFontDefault();
        } else {
            SetTextureFilter(state->textFont.texture, TEXTURE_FILTER_BILINEAR);
        }
    } else {
        TraceLog(LOG_WARNING, "常规字体文件未找到: %s", regularFontPath);
        state->textFont = GetFontDefault();
    }
    
    // 加载粗体字体
    if (FileExists(boldFontPath)) {
        state->titleFont = LoadFontEx(boldFontPath, titleFontSize, codepoints, codepointCount);
        if (state->titleFont.texture.id == 0) {
            TraceLog(LOG_WARNING, "粗体字体加载失败: %s", boldFontPath);
            state->titleFont = state->textFont;
        } else {
            SetTextureFilter(state->titleFont.texture, TEXTURE_FILTER_BILINEAR);
        }
    } else {
        TraceLog(LOG_WARNING, "粗体字体文件未找到: %s", boldFontPath);
        state->titleFont = state->textFont;
    }

    // 加载成就图标 - 亮色主题
    const char *achieveIconLightPath = "assets/img/achieve1.png";
    if (FileExists(achieveIconLightPath)) {
        state->achieveIconLight = LoadTexture(achieveIconLightPath);
        SetTextureFilter(state->achieveIconLight, TEXTURE_FILTER_BILINEAR);
    } else {
        TraceLog(LOG_WARNING, "亮色成就图标未找到: %s", achieveIconLightPath);
    }
    
    // 加载成就图标 - 暗色主题
    const char *achieveIconDarkPath = "assets/img/achieve2.png";
    if (FileExists(achieveIconDarkPath)) {
        state->achieveIconDark = LoadTexture(achieveIconDarkPath);
        SetTextureFilter(state->achieveIconDark, TEXTURE_FILTER_BILINEAR);
    } else {
        TraceLog(LOG_WARNING, "暗色成就图标未找到: %s", achieveIconDarkPath);
    }

    // 加载统计图标 - 亮色主题
    const char *dateIconLightPath = "assets/img/date2.png";
    if (FileExists(dateIconLightPath)) {
        state->dateIconLight = LoadTexture(dateIconLightPath);
        SetTextureFilter(state->dateIconLight, TEXTURE_FILTER_BILINEAR);
    } else {
        TraceLog(LOG_WARNING, "亮色统计图标未找到: %s", dateIconLightPath);
    }
    
    // 加载统计图标 - 暗色主题
    const char *dateIconDarkPath = "assets/img/date1.png";
    if (FileExists(dateIconDarkPath)) {
        state->dateIconDark = LoadTexture(dateIconDarkPath);
        SetTextureFilter(state->dateIconDark, TEXTURE_FILTER_BILINEAR);
    } else {
        TraceLog(LOG_WARNING, "暗色统计图标未找到: %s", dateIconDarkPath);
    }

    // 加载皮肤图标
    const char *sunIconPath = GetResourcePath("assets/img/sun.png");
    const char *moonIconPath = GetResourcePath("assets/img/moon.png");
    
    // 根据当前主题加载对应图标
    if (FileExists(sunIconPath) && FileExists(moonIconPath)) {
        state->sunTexture = LoadTexture(sunIconPath);
        state->moonTexture = LoadTexture(moonIconPath);
        state->themeIcon = state->isDarkTheme ? state->moonTexture : state->sunTexture;
    } else {
        TraceLog(LOG_WARNING, "皮肤图标文件未找到");
    }
    
    free(codepoints);
    
    // 加载成就图标
    const char *achieveIconPath = "assets/img/achieve.png";
    if (FileExists(achieveIconPath)) {
        state->achieveIcon = LoadTexture(achieveIconPath);
        if (state->achieveIcon.id == 0) {
            TraceLog(LOG_WARNING, "成就图标加载失败: %s", achieveIconPath);
        } else {
            SetTextureFilter(state->achieveIcon, TEXTURE_FILTER_BILINEAR);
        }
    } else {
        TraceLog(LOG_WARNING, "成就图标文件未找到: %s", achieveIconPath);
    }
    
    // 加载学习图片
    for (int i = 0; i < STUDY_IMAGE_COUNT; i++) {
        char imgPath[256];
        sprintf(imgPath, "assets/img/study%d.png", i + 1);
        
        if (FileExists(imgPath)) {
            state->studyImages[i] = LoadTexture(imgPath);
            if (state->studyImages[i].id == 0) {
                TraceLog(LOG_WARNING, "图片加载失败: %s", imgPath);
            } else {
                SetTextureFilter(state->studyImages[i], TEXTURE_FILTER_BILINEAR);
            }
        } else {
            TraceLog(LOG_WARNING, "图片文件未找到: %s", imgPath);
        }
    }

    // 卸载临时加载的纹理
    if (loadingTexture.id != 0) {
        UnloadTexture(loadingTexture);
    }

    return true;
}

// 仅生成必要字符集
int* GenerateEssentialCodepoints(int* count) {
    // 基本ASCII字符 (0-127)
    const int asciiCount = 128;
    
    // 常用汉字 (Unicode范围: 0x4E00-0x9FA5)
    const int minHan = 0x4E00;
    const int maxHan = 0x9FA5;
    const int hanCount = maxHan - minHan + 1;
    
    // 数字和标点符号
    const int extraCount = 50;
    
    *count = asciiCount + hanCount + extraCount;
    int* codepoints = (int*)malloc(*count * sizeof(int));
    
    // 填充ASCII
    for (int i = 0; i < asciiCount; i++) {
        codepoints[i] = i;
    }
    
    // 填充常用汉字
    for (int i = 0; i < hanCount; i++) {
        codepoints[asciiCount + i] = minHan + i;
    }
    
    // 填充额外字符
    const int extraChars[] = {0x3000, 0x3001, 0x3002, 0xFF01, 0xFF08, 0xFF09}; // 中文标点等
    for (int i = 0; i < extraCount; i++) {
        if (i < sizeof(extraChars)/sizeof(extraChars[0])) {
            codepoints[asciiCount + hanCount + i] = extraChars[i];
        } else {
            codepoints[asciiCount + hanCount + i] = 0;
        }
    }
    
    return codepoints;
}

// 卸载资源
void UnloadResources(AppState *state) {
    if (state->textFont.texture.id != 0 && state->textFont.texture.id != GetFontDefault().texture.id) {
        UnloadFont(state->textFont);
    }
    if (state->titleFont.texture.id != 0 && state->titleFont.texture.id != GetFontDefault().texture.id) {
        UnloadFont(state->titleFont);
    }
    if (state->achieveIcon.id != 0) {
        UnloadTexture(state->achieveIcon);
    }
    for (int i = 0; i < STUDY_IMAGE_COUNT; i++) {
        if (state->studyImages[i].id != 0) {
            UnloadTexture(state->studyImages[i]);
        }
    }
    // 卸载皮肤图标
    if (state->sunTexture.id != 0) {
        UnloadTexture(state->sunTexture);
    }
    if (state->moonTexture.id != 0) {
        UnloadTexture(state->moonTexture);
    }
    // 卸载成就图标
    if (state->achieveIconLight.id != 0) {
        UnloadTexture(state->achieveIconLight);
    }
    if (state->achieveIconDark.id != 0) {
        UnloadTexture(state->achieveIconDark);
    }
    // 卸载统计图标
    if (state->dateIconLight.id != 0) {
        UnloadTexture(state->dateIconLight);
    }
    if (state->dateIconDark.id != 0) {
        UnloadTexture(state->dateIconDark);
    }
}

int main(void) {
    // 状态文件路径
    const char* trashStateFile = "trash_state.dat";
    const char* appStateFile = "app_state.dat";
    
    // 尝试加载应用状态
    PersistentAppState persistentState = {0};
    AppState state = {0};
    
    FILE* appState = fopen(appStateFile, "rb");
    if (appState) {
        fread(&persistentState, sizeof(PersistentAppState), 1, appState);
        fclose(appState);
        TraceLog(LOG_INFO, "应用状态已加载");
        
        // 应用持久化状态
        state.windowWidth = persistentState.windowWidth > 0 ? persistentState.windowWidth : INIT_WIDTH;
        state.windowHeight = persistentState.windowHeight > 0 ? persistentState.windowHeight : INIT_HEIGHT;
        state.windowX = persistentState.windowX;
        state.windowY = persistentState.windowY;
        state.selectedPreset = persistentState.selectedPreset;
        strcpy(state.customMinutes, persistentState.customMinutes);
    } else {
        // 默认初始化
        state.windowWidth = INIT_WIDTH;
        state.windowHeight = INIT_HEIGHT;
        state.windowX = 100;
        state.windowY = 100;
        state.selectedPreset = 0;
        state.customMinutes[0] = '\0';
    }

    // ===== 关键修改：窗口初始化部分 =====
    
    // 1. 设置窗口标志（必须在InitWindow之前）
    SetConfigFlags(FLAG_WINDOW_MAXIMIZED | FLAG_WINDOW_RESIZABLE);
    
    // 2. 初始化窗口
    InitWindow(INIT_WIDTH, INIT_HEIGHT, "番茄钟");
    
    // 3. 确保窗口最大化
    if (!IsWindowState(FLAG_WINDOW_MAXIMIZED)) {
        MaximizeWindow();
        
        // 等待窗口最大化完成（最多等待1秒）
        double startTime = GetTime();
        while (!IsWindowState(FLAG_WINDOW_MAXIMIZED) && (GetTime() - startTime) < 1.0) {
            PollInputEvents(); // 处理事件队列
            WaitTime(0.01);    // 等待10ms
        }
    }
    
    // 4. 获取实际窗口尺寸
    state.windowWidth = GetScreenWidth();
    state.windowHeight = GetScreenHeight();
    state.windowX = GetWindowPosition().x;
    state.windowY = GetWindowPosition().y;

    // 加载主题状态
    state.isDarkTheme = LoadAppThemeState();

    // 初始化垃圾系统
    LoadTrashSystem(trashStateFile);
    if (trashCount == 0) {
        InitTrashSystem();
    }
 
    // 初始化AppState
    state.windowShake = (WindowShake){0};
    state.windowShake.decay = 0.9f;

    // 初始化统计数据
    state.statisticsFile = "statistics.dat";
    InitStatistics(&state.statistics);
    LoadStatistics(&state.statistics, state.statisticsFile);
    
    // 初始化番茄钟预设
    state.presets[0] = (PomodoroPreset){"25分钟", 25};
    state.presets[1] = (PomodoroPreset){"45分钟", 45};
    state.presets[2] = (PomodoroPreset){"自定义", 0};
    
    // 初始化成就系统
    state.achievementFile = "achievements.dat";
    state.achievementManager = (AchievementManager){0};
    LoadAchievements(&state.achievementManager, state.achievementFile);
    
    // 初始化计时器相关状态
    state.pomodoroDuration = state.presets[0].minutes * 60;
    state.timeLeft = state.pomodoroDuration;
    state.currentStudyImage = GetRandomValue(0, STUDY_IMAGE_COUNT - 1);
    state.currentScreen = MAIN_SCREEN;
    state.windowFocused = true;
    state.currentTrashIndex = -1;
    state.interruptionOccurred = false;
    
    SetRandomSeed((unsigned int)time(NULL));

    // 加载资源（确保在窗口初始化后）
    if (!LoadResources(&state)) {
        TraceLog(LOG_ERROR, "资源加载失败");
        CloseWindow();
        return 1;
    }

    SetTargetFPS(60);

    // 窗口聚焦监测
    bool wasFocused = true;
    double focusLostTime = 0;
    const double FOCUS_LOSS_TOLERANCE = 3.0; // 3秒容忍时间

    // 确保主题图标正确初始化
    state.themeIcon = state.isDarkTheme ? state.moonTexture : state.sunTexture;
    
    // === 关键修复：定义 lastWindowPos ===
    Vector2 lastWindowPos = GetWindowPosition();
        
    while (!WindowShouldClose()) {
        Vector2 currentWindowPos = GetWindowPosition();
        
        // 实时更新窗口加速度
        UpdateWindowAcceleration(currentWindowPos);
        
        // 窗口焦点处理
        bool isFocused = IsWindowFocused();
        if (state.currentScreen == TIMER_SCREEN && state.timerActive) {
            if (wasFocused && !isFocused) {
                // 立即处理中断逻辑
                state.interruptionOccurred = true;
                state.achievementManager.interruptionsCount++;
                GenerateTrash(state.pomodoroDuration / 60);
                state.currentScreen = INTERRUPTION_ALERT;

                // 设置中断标记
                state.achievementManager.interruptionOccurred = true;
                
                // 震动效果
                TriggerWindowShake(&state, 15.0f, 0.7f);
            }
        }
        wasFocused = isFocused;
        
        // 优化：减少垃圾更新频率
        static int trashUpdateCounter = 0;
        trashUpdateCounter++;
        if (trashUpdateCounter >= 2) { // 每2帧更新一次垃圾
            UpdateTrash();
            trashUpdateCounter = 0;
        }
        
        // 修复数据统计
        if (state.currentScreen == STATISTICS_SCREEN) {
            // 确保使用最新数据
            state.statistics.totalPomodoros = state.achievementManager.totalPomodoros;
            state.statistics.cleanedTrash = state.achievementManager.cleanedTrashCount;
            state.statistics.generatedTrash = state.achievementManager.generatedTrashCount;
            state.statistics.interruptions = state.achievementManager.interruptionsCount;
            state.statistics.streakDays = state.achievementManager.streakDays;
            state.statistics.longSessions = state.achievementManager.longSessionCount;
        }
        
        // 更新窗口尺寸
        state.windowWidth = GetScreenWidth();
        state.windowHeight = GetScreenHeight();
        state.windowX = GetWindowPosition().x;
        state.windowY = GetWindowPosition().y;
        
        float screenWidth = (float)state.windowWidth;
        float screenHeight = (float)state.windowHeight;

        if (state.currentScreen == MAIN_SCREEN) {
            Rectangle themeButton = {
                screenWidth - 130.0f,
                20.0f,
                50.0f,
                50.0f
            };
            
            if (CheckCollisionPointRec(GetMousePosition(), themeButton) && 
                IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                
                // 切换主题
                state.isDarkTheme = !state.isDarkTheme;
                
                // 更新图标
                state.themeIcon = state.isDarkTheme ? state.moonTexture : state.sunTexture;
                
                // 保存主题设置
                SaveAppThemeState(state.isDarkTheme);
            }
        }
        
        // 处理输入
        if (IsKeyPressed(KEY_SPACE) && state.currentScreen == TIMER_SCREEN) {
            state.timerActive = !state.timerActive;
            state.lastUpdateTime = GetTime();
            TraceLog(LOG_DEBUG, "计时器状态: %s", state.timerActive ? "运行" : "暂停");
        }
        
        if (IsKeyPressed(KEY_R) && state.currentScreen == TIMER_SCREEN) {
            TraceLog(LOG_DEBUG, "重置计时器");
            state.timerActive = false;
            state.timeLeft = state.pomodoroDuration;
            state.currentStudyImage = GetRandomValue(0, STUDY_IMAGE_COUNT - 1);
        }

        // 重置当前任务信息 - 这是唯一的时间更新逻辑
        if (state.currentScreen == TIMER_SCREEN && state.timerActive) {
            double currentTime = GetTime();
            double deltaTime = currentTime - state.lastUpdateTime;
            state.lastUpdateTime = currentTime;
            
            // 确保25:00显示完整一秒
            static double accumulatedTime = 0.0;
            accumulatedTime += deltaTime;
            
            // 每满1秒才减少时间
            if (accumulatedTime >= 1.0) {
                int secondsToDeduct = (int)accumulatedTime; // 取整数部分
                state.timeLeft -= secondsToDeduct; // 减少秒数
                accumulatedTime -= secondsToDeduct; // 保留小数部分
                
                // 防止时间变为负数
                if (state.timeLeft < 0) {
                    state.timeLeft = 0;
                }
            }
            
            // 检查计时结束
            if (state.timeLeft <= 0) {
                state.achievementManager.interruptionOccurred = false;

                // 检查成就并处理计时结束逻辑
                CheckAchievements(&state.achievementManager, 
                                state.currentTrashIndex < 0,  // pomodoroCompleted
                                state.currentTrashIndex >= 0,  // trashCleaned
                                state.pomodoroDuration);
                
                // 处理计时结束的逻辑
                UpdateTimerLogic(&state);
            }
        }
        
        // 处理主界面输入
        if (state.currentScreen == MAIN_SCREEN) {
            HandleMainScreenInput(&state, screenWidth, screenHeight);
        }
        
        // 开始绘制
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // 根据当前状态绘制不同界面
        switch (state.currentScreen) {
            case MAIN_SCREEN:
                TraceLog(LOG_DEBUG, "绘制主屏幕");
                DrawMainScreen(&state, screenWidth, screenHeight);
                break;
                
            case TIMER_SCREEN:
                TraceLog(LOG_DEBUG, "绘制计时器屏幕");
                DrawTimerScreen(&state, screenWidth, screenHeight);
                break;
                
            case INTERRUPTION_ALERT:
                TraceLog(LOG_DEBUG, "绘制中断警告");
                DrawInterruptionAlert(&state, screenWidth, screenHeight);
                // 处理确定按钮
                if (CheckCollisionPointRec(GetMousePosition(), (Rectangle){
                    screenWidth/2.0f - 50.0f,
                    screenHeight/2.0f + 40.0f,
                    100.0f,
                    40.0f
                }) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || 
                IsKeyPressed(KEY_ENTER)) {
                    state.currentScreen = MAIN_SCREEN;
                    TraceLog(LOG_DEBUG, "返回主屏幕");
                }
                break;
                
            case CLEAN_FAILED_ALERT:
                TraceLog(LOG_DEBUG, "绘制清理失败警告");
                DrawCleanFailedAlert(&state, screenWidth, screenHeight);
                // 处理确定按钮
                if (CheckCollisionPointRec(GetMousePosition(), (Rectangle){
                    screenWidth/2.0f - 50.0f,
                    screenHeight/2.0f + 40.0f,
                    100.0f,
                    40.0f
                }) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || 
                IsKeyPressed(KEY_ENTER)) {
                    if (state.currentTrashIndex >= 0) {
                        trashes[state.currentTrashIndex].cleaning = false;
                        trashes[state.currentTrashIndex].cleanProgress = 0.0f;
                    }
                    state.currentTrashIndex = -1;
                    state.currentScreen = MAIN_SCREEN;
                    TraceLog(LOG_DEBUG, "返回主屏幕");
                }
                break;
                
            case ACHIEVEMENT_SCREEN:
                TraceLog(LOG_DEBUG, "绘制成就屏幕");
                DrawAchievements(&state, screenWidth, screenHeight);
                // ESC返回主界面
                if (IsKeyPressed(KEY_ESCAPE)) {
                    state.currentScreen = MAIN_SCREEN;
                    TraceLog(LOG_DEBUG, "返回主屏幕");
                }
                break;

            case STATISTICS_SCREEN:
                DrawStatisticsScreen(&state.statistics, state.textFont, 
                                    state.isDarkTheme, screenWidth, screenHeight);
                // 处理返回按钮
                Rectangle backButton = {
                    screenWidth / 2.0f - 75.0f,
                    screenHeight - 70.0f,
                    150.0f,
                    50.0f
                };
                if ((CheckCollisionPointRec(GetMousePosition(), backButton) && 
                    IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || 
                    IsKeyPressed(KEY_ESCAPE)) {
                    state.currentScreen = MAIN_SCREEN;
                }
                break;
                
            default:
                // 添加默认情况处理
                DrawText("未知屏幕状态", screenWidth/2-100, screenHeight/2, 30, RED);
                break;
        }

        lastWindowPos = currentWindowPos;
        
        EndDrawing();
    }

    // 程序退出前保存状态
    SaveTrashSystem(trashStateFile);

    // 程序退出前保存统计数据
    SaveStatistics(&state.statistics, state.statisticsFile);
    
    // 保存持久化应用状态
    PersistentAppState persistentStateToSave = {
        .windowWidth = state.windowWidth,
        .windowHeight = state.windowHeight,
        .windowX = state.windowX,
        .windowY = state.windowY,
        .selectedPreset = state.selectedPreset
    };
    strcpy(persistentStateToSave.customMinutes, state.customMinutes);
    
    FILE* saveAppState = fopen(appStateFile, "wb");
    if (saveAppState) {
        fwrite(&persistentStateToSave, sizeof(PersistentAppState), 1, saveAppState);
        fclose(saveAppState);
        TraceLog(LOG_INFO, "应用状态已保存");
    }
    
    SaveAchievements(&state.achievementManager, state.achievementFile);

    // 清理资源
    UnloadResources(&state);
    
    CloseWindow();
    return 0;
}