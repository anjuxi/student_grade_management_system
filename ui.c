/*
 * ui.c —— 所有控制台输出、输入工具、柱状图可视化。
 *
 * 这层只负责"怎么显示"和"怎么读输入"。
 * 业务逻辑不在这里，在 student.c / commands.c。
 */

#include "student.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Ctrl+Z 标记。读输入时置 1。 */
static int g_input_eof = 0;


/* 一条横线。很多地方当视觉分隔用。 */
void ui_line(void) {
    printf("--------------------------------------------------\n");
}


/* 带上下分隔线的标题。 */
void ui_title(const char *text) {
    printf("\n");
    ui_line();
    printf("  %s\n", text);
    ui_line();
}


/* 动态表头：几科就打几列成绩。subject_count 决定列数。 */
void ui_table_head(int subject_count, const char subjects[][16]) {
    int j;
    printf("%-6s %-10s", "学号", "姓名");
    for (j = 0; j < subject_count; j++) {
        printf(" %6s", subjects[j]);
    }
    printf(" %7s %7s %4s\n", "总分", "平均", "名次");
    ui_line();
}


/* 一行数据。 */
void ui_table_row(const Student *s, int subject_count) {
    int j;
    printf("%-6d %-10s", s->id, s->name);
    for (j = 0; j < subject_count; j++) {
        printf(" %6.1f", s->score[j]);
    }
    printf(" %7.1f %7.1f %4d\n", s->sum, s->avg, s->rank);
}


/* 画排名前 top_n 的总分柱状图。
 * 每一行一个学生，█ 的数量等于他的总分 / max_sum * bar_width。 */
void ui_bar_chart(const GradeSystem *sys, int top_n) {
    int i, n;
    float max_sum = 0.0f;
    int bar_width = 40;

    n = top_n < sys->count ? top_n : sys->count;
    if (n <= 0) return;

    /* 先找最高分当 100%。 */
    for (i = 0; i < n; i++) {
        if (sys->students[i].sum > max_sum) {
            max_sum = sys->students[i].sum;
        }
    }
    if (max_sum <= 0.0f) {
        max_sum = 1.0f;                                             /* 防除零。 */
    }

    printf("\n[柱状图] 总分排名前 %d 名\n", n);
    ui_line();
    for (i = 0; i < n; i++) {
        const Student *s = &sys->students[i];
        int bar_len = (int)(s->sum / max_sum * bar_width);
        int j;

        printf("%2d. %-10s (总分 %5.1f): ", i + 1, s->name, s->sum);
        for (j = 0; j < bar_len; j++) {
            printf("#");                                            /* 方块字符，终端都能显示。 */
        }
        printf("\n");
    }
    ui_line();
}


/* 主菜单。当前人数和科目数动态显示。 */
int ui_menu(const GradeSystem *sys) {
    int choice;

    printf("\n");
    printf("==========================================================\n");
    printf("           学生成绩管理系统   （共 %d 人，%d 科）\n",
           sys->count, sys->subject_count);
    printf("==========================================================\n");
    printf("  1.  录入学生          2.  显示全部\n");
    printf("  3.  按学号查询        4.  按姓名模糊查询\n");
    printf("  5.  修改信息          6.  删除学生\n");
    printf("  7.  排序              8.  统计分析\n");
    printf("  9.  保存到文件       10.  从文件读取\n");
    printf(" 11.  导出 CSV          12.  排名柱状图\n");
    printf(" 13.  管理科目数         0.  退出\n");
    printf("==========================================================\n");

    choice = ui_input_int("请选择: ");
    return choice;
}


/* 输入工具们。
 *
 * 所有输入都走 ui_input_line 打底，它负责：
 *   - fgets 读一行
 *   - 去尾换行
 *   - 吃掉超长输入剩下的字符（不然会冲掉下一次输入）
 *   - Ctrl+Z/D 时置 g_input_eof = 1
 */

/* 清空 stdin 缓冲里剩余的字符。输入太长或 Ctrl+Z 时用。 */
static void flush_rest(void) {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
        /* 循环，直到换行或 EOF。 */
    }
}


/* 读一行字符串。prompt 可为 NULL。
 * 返回 1 成功，0 读到 EOF。 */
int ui_input_line(char *buf, int size, const char *prompt) {
    if (prompt != NULL) {
        printf("%s", prompt);
    }
    if (fgets(buf, size, stdin) == NULL) {
        g_input_eof = 1;
        return 0;
    }

    int len = (int)strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else {
        flush_rest();                                               /* 超长输入吃掉。 */
    }
    g_input_eof = 0;
    return 1;
}


/* 读整数，不合法让用户重输，直到对为止。
 * EOF 返回 -1。 */
int ui_input_int(const char *prompt) {
    char buf[64];
    char *end;
    long v;

    while (1) {
        if (!ui_input_line(buf, sizeof(buf), prompt)) {
            return -1;
        }
        v = strtol(buf, &end, 10);
        if (end != buf && *end == '\0') {
            return (int)v;
        }
        printf("输入的不是整数，重新输入!\n");
    }
}


/* 内部：把字符串转成 0~100 的分数。自动 clamp。
 * 成功返回 1。 */
static int str_to_score(const char *str, float *out) {
    char *end;
    double v;

    v = strtod(str, &end);
    if (end == str || *end != '\0') {
        return 0;
    }
    if (v < 0.0) v = 0.0;
    if (v > 100.0) v = 100.0;
    *out = (float)v;
    return 1;
}


/* 读一门课的分数。自动 clamp 到 0~100。 */
float ui_input_score(const char *prompt) {
    char buf[32];
    float v;

    while (1) {
        if (!ui_input_line(buf, sizeof(buf), prompt)) {
            return 0.0f;
        }
        if (str_to_score(buf, &v)) {
            return v;
        }
        printf("分数要在 0 到 100 之间!\n");
    }
}


/* 读姓名，不能为空。 */
void ui_input_name(char *name) {
    while (1) {
        if (!ui_input_line(name, V_NAME_LEN, "姓名: ")) {
            name[0] = '\0';
            return;
        }
        if (name[0] != '\0') {
            return;
        }
        printf("姓名不能空着!\n");
    }
}


/* 对外暴露 EOF 状态，main 循环用。 */
int ui_has_eof(void) { return g_input_eof; }


/* 暂停，按回车继续。
 * 每个命令执行完都调这个，给用户时间看清输出。 */
void ui_pause(void) {
    int c;
    printf("\n按回车键继续...");
    /* 把 stdin 里遗留的 \n 读掉——ui_input_line 已经处理过，
     * 但以防万一还是清一下。 */
    do {
        c = getchar();
    } while (c != '\n' && c != EOF);
}
